
#include <iostream>
#include <vector>

#include "src/cpu/CPU.h"

int main() {
    std::cout << "========================================\n";
    std::cout << "       RISC-V Pipeline Simulator\n";
    std::cout << "========================================\n\n";

    // Configure a 1 KiB direct-mapped cache with 16-byte blocks.
    CPU cpu(1024, 16, 1);

    // Initialize backing memory.
    cpu.setMemory(0, 100);
    cpu.setMemory(4, 200);
    cpu.setMemory(16, 300);

    std::vector<Instruction> program = {
        // x1 = 0: memory address
        Instruction(OpCode::ADDI, 1, 0, 0, 0),

        // x2 = memory[0] = 100
        Instruction(OpCode::LOAD, 2, 1, 0, 0),

        // x3 = memory[0] = 100 (same cache block: expected hit)
        Instruction(OpCode::LOAD, 3, 1, 0, 0),

        // x4 = memory[4] = 200 (same 16-byte block: expected hit)
        Instruction(OpCode::LOAD, 4, 1, 0, 4),

        // Store 100 to address 8.
        Instruction(OpCode::STORE, 0, 1, 2, 8),

        // Load address 8 (same cache block: expected hit).
        Instruction(OpCode::LOAD, 5, 1, 0, 8),

        // Branch test: x2 == x3, so skip the instruction writing 99.
        Instruction(OpCode::BEQ, 0, 2, 3, 8),

        Instruction(OpCode::ADDI, 6, 0, 0, 99),
        Instruction(OpCode::ADDI, 6, 0, 0, 42)
    };

    std::cout << "Running program...\n\n";

    cpu.loadProgram(program);
    cpu.run();

    std::cout << "\n========================================\n";
    std::cout << "             FINAL RESULTS\n";
    std::cout << "========================================\n";

    std::cout << "\nRegisters:\n";
    std::cout << "x1 = " << cpu.getRegister(1) << " (expected 0)\n";
    std::cout << "x2 = " << cpu.getRegister(2) << " (expected 100)\n";
    std::cout << "x3 = " << cpu.getRegister(3) << " (expected 100)\n";
    std::cout << "x4 = " << cpu.getRegister(4) << " (expected 200)\n";
    std::cout << "x5 = " << cpu.getRegister(5) << " (expected 100)\n";
    std::cout << "x6 = " << cpu.getRegister(6) << " (expected 42)\n";

    std::cout << "\nMemory:\n";
    std::cout << "memory[8] = " << cpu.getMemory(8)
              << " (expected 100)\n";

    cpu.printStatistics();

    std::cout << "\n========================================\n";
    std::cout << "Program execution finished.\n";
    std::cout << "========================================\n";

    return 0;
}












