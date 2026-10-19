
#ifndef CPU_H
#define CPU_H

#include <iostream>
#include <stdexcept>
#include <vector>

#include "Instruction.h"
#include "RegisterFile.h"
#include "ALU.h"
#include "Memory.h"
#include "Cache.h"

struct PipelineRegister {
    bool valid = false;
    Instruction instruction{OpCode::NOP};
    int pc = 0;
    int a = 0;
    int b = 0;
    int result = 0;
    int storeValue = 0;
};

class CPU {
private:
    RegisterFile registers;
    ALU alu;
    Memory dataMemory;

    // Default: 1 KiB cache, 16-byte blocks, direct mapped.
    Cache dataCache;

    std::vector<Instruction> program;

    int pc = 0;
    int cycle = 0;
    int stalls = 0;
    int branchesTaken = 0;
    int flushedInstructions = 0;
    int instructionsRetired = 0;

    PipelineRegister if_id, id_ex, ex_mem, mem_wb;

    bool isWriter(OpCode op) const {
        return op == OpCode::ADD ||
               op == OpCode::SUB ||
               op == OpCode::ADDI ||
               op == OpCode::LOAD;
    }

    bool writesRegister(const Instruction& instruction, int reg) const {
        return reg != 0 &&
               isWriter(instruction.opcode) &&
               instruction.rd == reg;
    }

    bool hasMoreInstructions() const {
        return pc >= 0 &&
               pc % 4 == 0 &&
               pc / 4 < static_cast<int>(program.size());
    }

    bool pipelineEmpty() const {
        return !if_id.valid &&
               !id_ex.valid &&
               !ex_mem.valid &&
               !mem_wb.valid;
    }

    bool usesRs1(OpCode op) const {
        return op == OpCode::ADD ||
               op == OpCode::SUB ||
               op == OpCode::ADDI ||
               op == OpCode::LOAD ||
               op == OpCode::STORE ||
               op == OpCode::BEQ;
    }

    bool usesRs2(OpCode op) const {
        return op == OpCode::ADD ||
               op == OpCode::SUB ||
               op == OpCode::STORE ||
               op == OpCode::BEQ;
    }

    bool hasLoadUseHazard(
        const PipelineRegister& waiting,
        const PipelineRegister& executing) const {

        if (!waiting.valid ||
            !executing.valid ||
            executing.instruction.opcode != OpCode::LOAD) {
            return false;
        }

        const Instruction& consumer = waiting.instruction;
        const Instruction& producer = executing.instruction;

        if (usesRs1(consumer.opcode) &&
            writesRegister(producer, consumer.rs1)) {
            return true;
        }

        if (usesRs2(consumer.opcode) &&
            writesRegister(producer, consumer.rs2)) {
            return true;
        }

        return false;
    }

    int forwardedRegisterValue(
        int reg,
        int originalValue,
        const PipelineRegister& previousExMem,
        const PipelineRegister& previousMemWb) const {

        if (reg == 0) {
            return 0;
        }

        if (previousExMem.valid &&
            writesRegister(previousExMem.instruction, reg) &&
            previousExMem.instruction.opcode != OpCode::LOAD) {
            return previousExMem.result;
        }

        if (previousMemWb.valid &&
            writesRegister(previousMemWb.instruction, reg)) {
            return previousMemWb.result;
        }

        return originalValue;
    }

    void validateBranchTarget(int target) const {
        if (target < 0 || target % 4 != 0) {
            throw std::runtime_error(
                "Invalid branch target: must be non-negative "
                "and 4-byte aligned."
            );
        }

        if (target > static_cast<int>(program.size()) * 4) {
            throw std::runtime_error(
                "Invalid branch target: outside program."
            );
        }
    }

public:
    // Defaults: 1024-byte capacity, 16-byte blocks, direct mapped.
    explicit CPU(
        int cacheCapacityBytes = 1024,
        int cacheBlockSizeBytes = 16,
        int cacheAssociativity = 1)
        : dataCache(
              cacheCapacityBytes,
              cacheBlockSizeBytes,
              cacheAssociativity) {}

    void setRegister(int index, int value) {
        registers.write(index, value);
    }

    int getRegister(int index) const {
        return registers.read(index);
    }

    // These methods access backing memory directly.
    // They do not count as simulated CPU cache accesses.
    void setMemory(int address, int value) {
        dataMemory.storeWord(address, value);
    }

    int getMemory(int address) const {
        return dataMemory.loadWord(address);
    }

    int getPC() const {
        return pc;
    }

    int getCycles() const {
        return cycle;
    }

    int getStalls() const {
        return stalls;
    }

    int getBranchesTaken() const {
        return branchesTaken;
    }

    int getFlushedInstructions() const {
        return flushedInstructions;
    }

    int getInstructionsRetired() const {
        return instructionsRetired;
    }

    std::uint64_t getCacheHits() const {
        return dataCache.getHits();
    }

    std::uint64_t getCacheMisses() const {
        return dataCache.getMisses();
    }

    std::uint64_t getCacheAccesses() const {
        return dataCache.getAccesses();
    }

    double getCacheHitRate() const {
        return dataCache.getHitRate();
    }

    double getCacheMissRate() const {
        return dataCache.getMissRate();
    }

    // Clears cache contents and cache statistics.
    void resetCache() {
        dataCache.clear();
    }

    double getCPI() const {
        if (instructionsRetired == 0) {
            return 0.0;
        }

        return static_cast<double>(cycle) /
               instructionsRetired;
    }

    double getIPC() const {
        if (cycle == 0) {
            return 0.0;
        }

        return static_cast<double>(instructionsRetired) /
               cycle;
    }

    void loadProgram(const std::vector<Instruction>& instructions) {
        program = instructions;

        pc = 0;
        cycle = 0;
        stalls = 0;
        branchesTaken = 0;
        flushedInstructions = 0;
        instructionsRetired = 0;

        if_id = {};
        id_ex = {};
        ex_mem = {};
        mem_wb = {};

        // Cache state intentionally persists across programs.
        // Call resetCache() explicitly when a cold cache is needed.
    }

    void run() {
        while (hasMoreInstructions() || !pipelineEmpty()) {
            ++cycle;

            // Snapshot the state at the beginning of the cycle.
            const PipelineRegister oldIfId = if_id;
            const PipelineRegister oldIdEx = id_ex;
            const PipelineRegister oldExMem = ex_mem;
            const PipelineRegister oldMemWb = mem_wb;

            // WB: every valid instruction reaching WB retires once.
            if (oldMemWb.valid) {
                ++instructionsRetired;

                if (isWriter(oldMemWb.instruction.opcode)) {
                    registers.write(
                        oldMemWb.instruction.rd,
                        oldMemWb.result
                    );
                }
            }

            PipelineRegister nextMemWb = {};
            PipelineRegister nextExMem = {};
            PipelineRegister nextIdEx = {};
            PipelineRegister nextIfId = {};

            // MEM stage.
            if (oldExMem.valid) {
                nextMemWb = oldExMem;

                const Instruction& instruction =
                    oldExMem.instruction;

                if (instruction.opcode == OpCode::LOAD) {
                    // Record a cache access, then read backing memory.
                    dataCache.access(oldExMem.result);
                    nextMemWb.result =
                        dataMemory.loadWord(oldExMem.result);

                } else if (instruction.opcode == OpCode::STORE) {
                    // Record a cache access, then update backing memory.
                    dataCache.access(oldExMem.result);
                    dataMemory.storeWord(
                        oldExMem.result,
                        oldExMem.storeValue
                    );
                }
            }

            // EX stage.
            bool takenBranch = false;
            int branchTarget = 0;

            if (oldIdEx.valid) {
                nextExMem = oldIdEx;

                const Instruction& instruction =
                    oldIdEx.instruction;

                const int a = forwardedRegisterValue(
                    instruction.rs1,
                    oldIdEx.a,
                    oldExMem,
                    oldMemWb
                );

                const int b = forwardedRegisterValue(
                    instruction.rs2,
                    oldIdEx.b,
                    oldExMem,
                    oldMemWb
                );

                nextExMem.storeValue = b;

                switch (instruction.opcode) {
                    case OpCode::ADD:
                    case OpCode::SUB:
                        nextExMem.result =
                            alu.execute(instruction.opcode, a, b);
                        break;

                    case OpCode::ADDI:
                        nextExMem.result =
                            alu.execute(
                                OpCode::ADDI,
                                a,
                                instruction.immediate
                            );
                        break;

                    case OpCode::LOAD:
                    case OpCode::STORE:
                        nextExMem.result =
                            a + instruction.immediate;
                        break;

                    case OpCode::BEQ:
                        if (a == b) {
                            takenBranch = true;
                            branchTarget =
                                oldIdEx.pc + instruction.immediate;

                            validateBranchTarget(branchTarget);
                        }
                        break;

                    case OpCode::NOP:
                        break;

                    default:
                        throw std::invalid_argument(
                            "Instruction not implemented."
                        );
                }
            }

            const bool stall =
                hasLoadUseHazard(oldIfId, oldIdEx);

            if (takenBranch) {
                ++branchesTaken;

                if (oldIfId.valid) {
                    ++flushedInstructions;
                }

                // Redirect fetch and discard the wrong-path instruction.
                pc = branchTarget;

            } else if (stall) {
                ++stalls;

                // Hold IF/ID and PC; ID/EX receives a bubble.
                nextIfId = oldIfId;

            } else {
                // ID: read operands.
                if (oldIfId.valid) {
                    nextIdEx.valid = true;
                    nextIdEx.instruction = oldIfId.instruction;
                    nextIdEx.pc = oldIfId.pc;

                    nextIdEx.a =
                        registers.read(oldIfId.instruction.rs1);
                    nextIdEx.b =
                        registers.read(oldIfId.instruction.rs2);
                    nextIdEx.storeValue = nextIdEx.b;
                }

                // IF: fetch an instruction when one remains.
                if (hasMoreInstructions()) {
                    nextIfId.valid = true;
                    nextIfId.instruction = program[pc / 4];
                    nextIfId.pc = pc;
                    pc += 4;
                }
            }

            // Commit the next pipeline state simultaneously.
            if_id = nextIfId;
            id_ex = nextIdEx;
            ex_mem = nextExMem;
            mem_wb = nextMemWb;

            std::cout << "Cycle " << cycle << " | PC=" << pc;

            if (takenBranch) {
                std::cout << " | BRANCH TAKEN: FLUSH";
            } else if (stall) {
                std::cout << " | LOAD-USE STALL";
            }

            std::cout << '\n';
        }
    }

    void printStatistics() const {
        std::cout << "\n=== Pipeline Statistics ===\n";
        std::cout << "Cycles: " << cycle << '\n';
        std::cout << "Instructions retired: "
                  << instructionsRetired << '\n';
        std::cout << "Load-use stalls: " << stalls << '\n';
        std::cout << "Branches taken: " << branchesTaken << '\n';
        std::cout << "Instructions flushed: "
                  << flushedInstructions << '\n';
        std::cout << "CPI: " << getCPI() << '\n';
        std::cout << "IPC: " << getIPC() << '\n';

        std::cout << "\n=== Data Cache Statistics ===\n";
        std::cout << "Cache accesses: " << getCacheAccesses() << '\n';
        std::cout << "Cache hits: " << getCacheHits() << '\n';
        std::cout << "Cache misses: " << getCacheMisses() << '\n';
        std::cout << "Cache hit rate: "
                  << getCacheHitRate() * 100.0 << "%\n";
        std::cout << "Cache miss rate: "
                  << getCacheMissRate() * 100.0 << "%\n";
    }
};

#endif













