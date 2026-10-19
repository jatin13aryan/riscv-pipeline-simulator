
#include <cmath>
#include <iostream>
#include <string>
#include "src/cpu/CPU.h"

int passed = 0;
int failed = 0;

void check(bool condition, const std::string& description) {
    if (condition) {
        ++passed;
        std::cout << "[PASS] " << description << '\n';
    } else {
        ++failed;
        std::cout << "[FAIL] " << description << '\n';
    }
}

void testArithmeticAndForwarding() {
    CPU cpu;
    cpu.loadProgram({
        Instruction(OpCode::ADDI, 1, 0, 0, 10),
        Instruction(OpCode::ADDI, 2, 0, 0, 20),
        Instruction(OpCode::ADD, 3, 1, 2),
        Instruction(OpCode::SUB, 4, 2, 1)
    });
    cpu.run();

    check(cpu.getRegister(1) == 10, "ADDI writes x1");
    check(cpu.getRegister(2) == 20, "ADDI writes x2");
    check(cpu.getRegister(3) == 30, "ADD and forwarding");
    check(cpu.getRegister(4) == 10, "SUB and forwarding");
}

void testLoadUseHazard() {
    CPU cpu;
    cpu.loadProgram({
        Instruction(OpCode::ADDI, 1, 0, 0, 100),
        Instruction(OpCode::STORE, 0, 0, 1, 0),
        Instruction(OpCode::LOAD, 2, 0, 0, 0),
        Instruction(OpCode::ADD, 3, 2, 2)
    });
    cpu.run();

    check(cpu.getRegister(2) == 100, "LOAD reads stored value");
    check(cpu.getRegister(3) == 200, "Load-use forwarding gives correct result");
    check(cpu.getStalls() > 0, "Load-use hazard inserts a stall");
}

void testStoreForwarding() {
    CPU cpu;
    cpu.loadProgram({
        Instruction(OpCode::ADDI, 1, 0, 0, 77),
        Instruction(OpCode::STORE, 0, 0, 1, 0),
        Instruction(OpCode::LOAD, 2, 0, 0, 0)
    });
    cpu.run();

    check(cpu.getRegister(2) == 77, "Store receives forwarded value");
}

void testTakenBranch() {
    CPU cpu;
    cpu.loadProgram({
        Instruction(OpCode::ADDI, 1, 0, 0, 5),
        Instruction(OpCode::ADDI, 2, 0, 0, 5),
        Instruction(OpCode::BEQ, 0, 1, 2, 8),
        Instruction(OpCode::ADDI, 3, 0, 0, 99),
        Instruction(OpCode::ADDI, 3, 0, 0, 42)
    });
    cpu.run();

    check(cpu.getRegister(3) == 42, "Taken branch skips wrong-path instruction");
    check(cpu.getBranchesTaken() == 1, "Taken branch is counted");
    check(cpu.getFlushedInstructions() == 1, "Wrong-path instruction is flushed");
}

void testNotTakenBranch() {
    CPU cpu;
    cpu.loadProgram({
        Instruction(OpCode::ADDI, 1, 0, 0, 5),
        Instruction(OpCode::ADDI, 2, 0, 0, 8),
        Instruction(OpCode::BEQ, 0, 1, 2, 8),
        Instruction(OpCode::ADDI, 3, 0, 0, 99),
        Instruction(OpCode::ADDI, 4, 0, 0, 42)
    });
    cpu.run();

    check(cpu.getRegister(3) == 99, "Not-taken branch executes next instruction");
    check(cpu.getRegister(4) == 42, "Not-taken branch continues normally");
    check(cpu.getBranchesTaken() == 0, "Not-taken branch is not counted");
}

void testLoadThenBranch() {
    CPU cpu;
    cpu.loadProgram({
        Instruction(OpCode::ADDI, 2, 0, 0, 5),
        Instruction(OpCode::STORE, 0, 0, 2, 0),
        Instruction(OpCode::LOAD, 1, 0, 0, 0),
        Instruction(OpCode::BEQ, 0, 1, 2, 8),
        Instruction(OpCode::ADDI, 3, 0, 0, 99),
        Instruction(OpCode::ADDI, 3, 0, 0, 42)
    });
    cpu.run();

    check(cpu.getRegister(1) == 5, "Load before branch gets correct value");
    check(cpu.getRegister(3) == 42, "Branch compares correct forwarded values");
    check(cpu.getStalls() > 0, "Load-related hazard is handled");
}

void testLoadThenStore() {
    CPU cpu;
    cpu.loadProgram({
        Instruction(OpCode::ADDI, 1, 0, 0, 55),
        Instruction(OpCode::STORE, 0, 0, 1, 0),
        Instruction(OpCode::LOAD, 2, 0, 0, 0),
        Instruction(OpCode::STORE, 0, 0, 2, 4),
        Instruction(OpCode::LOAD, 3, 0, 0, 4)
    });
    cpu.run();

    check(cpu.getRegister(3) == 55, "Loaded value is correctly forwarded to a store");
}

void testZeroRegister() {
    CPU cpu;
    cpu.loadProgram({
        Instruction(OpCode::ADDI, 0, 0, 0, 123),
        Instruction(OpCode::ADDI, 1, 0, 0, 7)
    });
    cpu.run();

    check(cpu.getRegister(0) == 0, "x0 always remains zero");
    check(cpu.getRegister(1) == 7, "Other registers still work");
}

void testStatistics() {
    CPU cpu;
    cpu.loadProgram({
        Instruction(OpCode::ADDI, 1, 0, 0, 2),
        Instruction(OpCode::ADDI, 2, 0, 0, 3),
        Instruction(OpCode::ADD, 3, 1, 2)
    });
    cpu.run();

    check(cpu.getInstructionsRetired() == 3, "Retired instruction count is correct");
    check(cpu.getCycles() > 0, "Cycle count is positive");

    const double cpi = cpu.getCPI();
    const double ipc = cpu.getIPC();

    check(std::abs(cpi -
          static_cast<double>(cpu.getCycles()) / cpu.getInstructionsRetired()) < 1e-9,
          "CPI calculation is correct");

    check(std::abs(ipc -
          static_cast<double>(cpu.getInstructionsRetired()) / cpu.getCycles()) < 1e-9,
          "IPC calculation is correct");

    check(std::abs(cpi * ipc - 1.0) < 1e-9, "CPI times IPC equals one");
}

int main() {
    std::cout << "Running RISC-V pipeline tests...\n\n";

    testArithmeticAndForwarding();
    testLoadUseHazard();
    testStoreForwarding();
    testTakenBranch();
    testNotTakenBranch();
    testLoadThenBranch();
    testLoadThenStore();
    testZeroRegister();
    testStatistics();

    std::cout << "\nPassed: " << passed << " Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}






