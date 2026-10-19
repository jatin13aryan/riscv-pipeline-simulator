
#ifndef ALU_H
#define ALU_H

#include "Instruction.h"
#include <stdexcept>

class ALU {
public:
    int execute(OpCode opcode, int a, int b) const {
        switch (opcode) {
            case OpCode::ADD:
            case OpCode::ADDI:
                return a + b;

            case OpCode::SUB:
                return a - b;

            default:
                throw std::invalid_argument(
                    "Unsupported ALU operation"
                );
        }
    }
};

#endif

