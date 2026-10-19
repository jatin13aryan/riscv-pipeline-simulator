
#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>

enum class OpCode {
    ADD,
    SUB,
    ADDI,
    LOAD,
    STORE,
    BEQ,
    NOP
};

class Instruction {
public:
    OpCode opcode;
    int rd;
    int rs1;
    int rs2;
    int immediate;

    Instruction(
        OpCode opcode,
        int rd = 0,
        int rs1 = 0,
        int rs2 = 0,
        int immediate = 0
    )
        : opcode(opcode),
          rd(rd),
          rs1(rs1),
          rs2(rs2),
          immediate(immediate) {}

    std::string toString() const {
        switch (opcode) {
            case OpCode::ADD:   return "ADD";
            case OpCode::SUB:   return "SUB";
            case OpCode::ADDI:  return "ADDI";
            case OpCode::LOAD:  return "LOAD";
            case OpCode::STORE: return "STORE";
            case OpCode::BEQ:   return "BEQ";
            case OpCode::NOP:   return "NOP";
        }
        return "UNKNOWN";
    }
};

#endif

