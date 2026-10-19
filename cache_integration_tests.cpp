
#include <iostream>
#include <cmath>
#include "src/cpu/CPU.h"

int passed = 0;
int failed = 0;

void check(bool condition, const char* description) {
    if (condition) {
        ++passed;
        std::cout << "[PASS] " << description << '\n';
    } else {
        ++failed;
        std::cout << "[FAIL] " << description << '\n';
    }
}

void testRepeatedLoads() {
    CPU cpu(1024, 16, 1);

    cpu.setMemory(0, 100);
    cpu.setMemory(4, 200);

    cpu.loadProgram({
        Instruction(OpCode::ADDI, 1, 0, 0, 0),
        Instruction(OpCode::LOAD, 2, 1, 0, 0),
        Instruction(OpCode::LOAD, 3, 1, 0, 0),
        Instruction(OpCode::LOAD, 4, 1, 0, 4)
    });

    cpu.run();

    check(cpu.getRegister(2) == 100, "First load returns correct value");
    check(cpu.getRegister(3) == 100, "Repeated load returns correct value");
    check(cpu.getRegister(4) == 200, "Adjacent word load returns correct value");

    check(cpu.getCacheAccesses() == 3, "Three loads produce three cache accesses");
    check(cpu.getCacheMisses() == 1, "First load misses in a cold cache");
    check(cpu.getCacheHits() == 2, "Repeated and adjacent loads hit");
    check(std::abs(cpu.getCacheHitRate() - 2.0 / 3.0) < 1e-9,
          "CPU cache hit rate is correct");
}

void testStoreAndLoad() {
    CPU cpu(1024, 16, 1);

    cpu.setMemory(0, 10);

    cpu.loadProgram({
        Instruction(OpCode::ADDI, 1, 0, 0, 0),
        Instruction(OpCode::ADDI, 2, 0, 0, 77),
        Instruction(OpCode::STORE, 0, 1, 2, 0),
        Instruction(OpCode::LOAD, 3, 1, 0, 0)
    });

    cpu.run();

    check(cpu.getMemory(0) == 77, "Store updates backing memory");
    check(cpu.getRegister(3) == 77, "Load reads the stored value");
    check(cpu.getCacheAccesses() == 2, "Store and load both access cache");
    check(cpu.getCacheMisses() == 1, "Store first access misses");
    check(cpu.getCacheHits() == 1, "Following load hits the same cache block");
}

void testCacheReset() {
    CPU cpu(1024, 16, 1);

    cpu.setMemory(0, 55);

    cpu.loadProgram({
        Instruction(OpCode::LOAD, 1, 0, 0, 0)
    });
    cpu.run();

    check(cpu.getCacheAccesses() == 1, "Initial program accesses cache");
    check(cpu.getCacheMisses() == 1, "Initial access is a miss");

    cpu.resetCache();

    check(cpu.getCacheAccesses() == 0, "Reset clears access statistics");
    check(cpu.getCacheHits() == 0, "Reset clears hit counter");
    check(cpu.getCacheMisses() == 0, "Reset clears miss counter");

    cpu.loadProgram({
        Instruction(OpCode::LOAD, 2, 0, 0, 0)
    });
    cpu.run();

    check(cpu.getCacheMisses() == 1, "Access after reset misses again");
    check(cpu.getRegister(2) == 55, "Reset does not erase backing memory");
}

void testConfigurableCache() {
    CPU cpu(32, 4, 2);

    cpu.setMemory(0, 11);
    cpu.setMemory(4, 22);

    cpu.loadProgram({
        Instruction(OpCode::LOAD, 1, 0, 0, 0),
        Instruction(OpCode::LOAD, 2, 0, 0, 4)
    });

    cpu.run();

    check(cpu.getRegister(1) == 11, "Configured cache preserves load behavior");
    check(cpu.getRegister(2) == 22, "Configured cache handles second load");
    check(cpu.getCacheAccesses() == 2, "Configured cache records both accesses");
}

int main() {
    std::cout << "Running CPU-cache integration tests...\n\n";

    testRepeatedLoads();
    testStoreAndLoad();
    testCacheReset();
    testConfigurableCache();

    std::cout << "\nPassed: " << passed
              << " Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
