# RISC-V Pipeline & Data Cache Simulator

A C++17 educational computer architecture simulator that models a simplified five-stage RISC-V-style processor pipeline, data forwarding, hazard detection, branch handling, and configurable data-cache behavior.

The project explores how instruction-level dependencies and memory-access patterns affect processor execution. It includes a standalone cache simulator, CPU-cache integration, pipeline performance statistics, and automated test programs.

**Repository:** [jatin13aryan/riscv-pipeline-simulator](https://github.com/jatin13aryan/riscv-pipeline-simulator)

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Five-Stage Pipeline](#five-stage-pipeline)
- [Hazard Detection and Forwarding](#hazard-detection-and-forwarding)
- [Branch Handling](#branch-handling)
- [Data Cache Architecture](#data-cache-architecture)
- [Performance Metrics](#performance-metrics)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [Build and Run](#build-and-run)
- [Running the Tests](#running-the-tests)
- [Example Configuration](#example-configuration)
- [Design Decisions and Limitations](#design-decisions-and-limitations)
- [Future Improvements](#future-improvements)

---

## Overview

Modern processors improve performance by overlapping the execution of multiple instructions, resolving data dependencies, and exploiting locality in memory accesses.

This project implements a simplified processor pipeline and a configurable data-cache model to demonstrate these concepts in C++.

The simulator supports a small instruction set, a register file, an arithmetic logic unit (ALU), word-addressed data memory using byte addresses, pipeline registers, forwarding paths, load-use hazard detection, branch flushing, and cache statistics.

The goal is to make fundamental computer architecture concepts observable through executable code rather than treating them as purely theoretical concepts.

### Technology Stack

| Component | Technology |
|---|---|
| Programming language | C++17 |
| Compiler | GCC / MinGW-w64 |
| Processor model | Simplified five-stage pipeline |
| Cache model | Configurable set-associative cache |
| Replacement policy | Least Recently Used (LRU) |
| Testing | Standalone C++ test executables |
| Version control | Git and GitHub |

## Features

### Processor and instruction execution

- Five-stage pipeline: IF, ID, EX, MEM, and WB.
- Register file containing 32 integer registers, with `x0` hardwired to zero.
- ALU operations for arithmetic instructions.
- Data memory supporting word loads and stores.
- Program counter and instruction sequencing.
- Branch-equality instruction with program-counter redirection.

### Pipeline behavior

- Operand forwarding from pipeline stages.
- Load-use hazard detection.
- Pipeline stalls and bubble insertion for detected load-use dependencies.
- Branch redirection and wrong-path instruction flushing.
- Cycle counting and instruction-retirement tracking.

### Data cache

- Configurable cache capacity and block size.
- Direct-mapped, N-way set-associative, and fully associative configurations.
- LRU replacement when a set is full.
- Cache hit and miss counters.
- Hit-rate and miss-rate calculations.
- Explicit cache reset functionality.
- Integration with CPU load and store operations.

### Performance reporting

- Total simulated cycles.
- Instructions retired.
- Load-use stalls.
- Taken branches.
- Flushed instructions.
- Cycles per instruction (CPI).
- Instructions per cycle (IPC).
- Data-cache accesses, hits, misses, and hit/miss rates.

---

## Architecture

The project is organized into processor components, pipeline control, cache modeling, and test programs.

### High-level architecture

```text
                    RISC-V-Style Simulator
                             |
                  +----------+----------+
                  |                     |
             CPU Pipeline           Test Programs
                  |                     |
     +------------+------------+   +----+----------------+
     |            |            |   |    |                |
   Fetch         Decode       Execute  Pipeline Tests  Cache Tests
    (IF)          (ID)         (EX)   Integration Tests
     |            |            |
     +------------+------------+
                  |
             Memory Stage
                (MEM)
                  |
          +-------+--------+
          |                |
      Data Cache        Data Memory
      Hit/Miss           Backing Store
       Tracking
          |
      Write Back
         (WB)
          |
      Register File
```

The diagram represents the logical organization. The cache currently records accesses while the backing memory supplies load values and receives store updates directly. Cache misses do not yet introduce additional pipeline cycles.

### Core components

| Component | Responsibility |
|---|---|
| `Instruction.h` | Defines supported opcodes and instruction representation. |
| `RegisterFile.h` | Implements register storage and protects `x0` from writes. |
| `ALU.h` | Executes supported arithmetic operations. |
| `Memory.h` | Implements data-memory reads and writes with address validation. |
| `Cache.h` | Models cache lookup, set indexing, tags, LRU replacement, and statistics. |
| `CPU.h` | Coordinates pipeline execution, hazards, forwarding, branches, memory operations, and statistics. |
| `main.cpp` | Runs an example program and prints execution results. |
| `tests.cpp` | Tests core pipeline behavior. |
| `cache_tests.cpp` | Tests the standalone cache model. |
| `cache_integration_tests.cpp` | Tests cache behavior through CPU execution. |

---

## Five-Stage Pipeline

The processor uses a simplified five-stage pipeline inspired by the classic RISC pipeline organization.

### 1. Instruction Fetch (IF)

The processor uses the program counter to fetch an instruction from the loaded program.

- Reads the instruction at the current PC.
- Advances the PC by four bytes during normal sequential execution.
- Holds the PC when a detected load-use hazard requires a stall.
- Redirects instruction fetching after a taken branch.

### 2. Instruction Decode (ID)

The instruction is decoded into its represented operation, and its source operands are read from the register file.

- Reads source registers.
- Prepares operand values for execution.
- Passes instruction information through the ID/EX pipeline register.
- Holds the waiting instruction when the pipeline detects a load-use dependency.

### 3. Execute (EX)

The execute stage performs arithmetic, computes memory addresses, and evaluates branch conditions.

Examples:

- `ADD`: adds two register operands.
- `SUB`: subtracts one register operand from another.
- `ADDI`: adds an immediate value to a register operand.
- `LOAD` and `STORE`: calculate effective addresses.
- `BEQ`: compares two operands and determines whether to redirect the PC.

The stage uses available forwarding paths to obtain newer operand values when supported dependencies occur.

### 4. Memory Access (MEM)

The memory stage handles data-memory instructions.

- `LOAD` reads a word from backing memory.
- `STORE` writes a word to backing memory.
- Both operations access the data-cache model and update its hit/miss statistics.
- Non-memory instructions continue toward write-back.

**Current simplification:** the cache does not contain or supply the actual data values. The backing memory remains authoritative, and cache hits and misses do not change pipeline timing.

### 5. Write Back (WB)

The write-back stage commits results to the register file.

- Arithmetic results are written to their destination registers.
- Loaded values are written to their destination registers.
- `x0` remains zero.
- Instructions reaching write-back are counted as retired.

### Pipeline registers

The implementation uses pipeline-register structures to carry instruction state between stages:

```text
IF/ID  ->  ID/EX  ->  EX/MEM  ->  MEM/WB
```

Each pipeline register tracks whether it contains a valid instruction and carries relevant information such as the instruction, PC, operands, execution result, and store value.

The pipeline advances using snapshots of the previous cycle's state, allowing stage results to be committed together.

---

## Hazard Detection and Forwarding

Instruction dependencies can prevent an instruction from safely using the value currently available in the register file. The simulator implements forwarding and load-use stall detection to handle supported cases.

### Data forwarding

Consider:

```asm
ADDI x1, x0, 10
ADDI x2, x0, 20
ADD  x3, x1, x2
```

The `ADD` depends on values produced by preceding instructions. Forwarding allows the execute stage to use available results without waiting for every result to be read back from the register file.

The forwarding logic checks:

1. A matching non-load result in EX/MEM.
2. A matching result in MEM/WB.
3. The original register-file value if no forwarding match exists.

The implementation gives EX/MEM priority over MEM/WB and does not forward a load result from EX/MEM before that value has been retrieved from memory.

### Load-use hazards

A load-use hazard occurs when an instruction needs a value that a preceding load has not yet made available.

Example:

```asm
LOAD x1, 0(x0)
ADD  x2, x1, x1
```

The simulator detects supported dependencies involving the destination register of a load and the source registers used by the waiting instruction.

When a hazard is detected:

- The PC is held.
- The IF/ID instruction is held.
- A bubble is inserted into ID/EX.
- The stall counter is incremented.

This implementation models a simplified load-use hazard. It is not a complete implementation of every possible RISC-V dependency or pipeline-control scenario.

---

## Branch Handling

The simulator implements branch-equality (`BEQ`) instructions.

Example:

```asm
ADDI x1, x0, 5
ADDI x2, x0, 5
BEQ  x0, x1, x2, 8
ADDI x3, x0, 99
ADDI x3, x0, 42
```

The `BEQ` representation used by the simulator compares its two source registers. When the branch condition is true, execution redirects to the branch target, calculated relative to the branch instruction's PC.

The implementation:

- Evaluates the branch condition in EX.
- Validates the branch target.
- Redirects the PC when the condition is true.
- Discards the wrong-path instruction held in IF/ID.
- Increments the taken-branch and flushed-instruction counters.

For this simplified pipeline, the branch model flushes one waiting IF/ID instruction. It should not be interpreted as a complete model of all real RISC-V control-flow or branch-prediction behavior.

---

## Data Cache Architecture

The cache model is implemented separately in `Cache.h` and can be instantiated with different capacities, block sizes, and associativities.

### Cache configuration

The CPU constructor accepts:

```cpp
CPU cpu(1024, 16, 1);
```

The parameters are:

1. Cache capacity in bytes: `1024`.
2. Cache block size in bytes: `16`.
3. Associativity: `1`, meaning direct mapped.

The cache validates its configuration and calculates the number of cache lines and sets.

For a cache with capacity \(C\), block size \(B\), and associativity \(A\):

\[
\text{Cache lines} = \frac{C}{B}
\]

\[
\text{Number of sets} = \frac{C/B}{A}
\]

The configuration must have positive values, capacity divisible by block size, and an associativity that divides the total number of cache lines.

### Supported cache organizations

| Organization | Associativity | Behavior |
|---|---:|---|
| Direct mapped | `1` | Each memory block maps to exactly one cache line. |
| N-way set associative | `N` | Each memory block maps to one set containing N possible lines. |
| Fully associative | Total number of lines | A memory block may occupy any cache line. |

Example configurations:

```cpp
// 1 KiB cache, 16-byte blocks, direct mapped
CPU directMapped(1024, 16, 1);

// 1 KiB cache, 16-byte blocks, 2-way set associative
CPU twoWay(1024, 16, 2);

// 1 KiB cache, 16-byte blocks, fully associative
CPU fullyAssociative(1024, 16, 64);
```

The fully associative example has 64 cache lines because 1024 / 16 = 64.

### Address mapping

For a byte address, the cache calculates the memory block address:

\[
\text{Block address} =
\left\lfloor \frac{\text{Byte address}}{\text{Block size}} \right\rfloor
\]

It then calculates:

\[
\text{Set index} = \text{Block address} \bmod \text{Number of sets}
\]

\[
\text{Tag} =
\left\lfloor
\frac{\text{Block address}}{\text{Number of sets}}
\right\rfloor
\]

A cache hit occurs when a valid line in the selected set contains the matching tag. Otherwise, the access is a miss.

### LRU replacement

When an access misses and the selected set has no invalid lines, the cache evicts the least recently used line.

The implementation tracks the most recent access to each valid line and selects the line with the oldest recorded access when replacement is necessary.

### Cache statistics

The cache tracks:

- Total accesses.
- Hits.
- Misses.
- Hit rate.
- Miss rate.

The rates are calculated as:

\[
\text{Hit rate} = \frac{\text{Hits}}{\text{Total accesses}}
\]

\[
\text{Miss rate} = \frac{\text{Misses}}{\text{Total accesses}}
\]

When no accesses have occurred, both rates return zero.

### Cache integration

During CPU execution, each `LOAD` and `STORE` instruction invokes the cache's access method in MEM.

The backing memory still performs the actual read or write. Consequently, the current model can measure locality and replacement behavior, but it does not yet model cache data storage, dirty lines, write-back traffic, or miss penalties.

Calls to the CPU's direct `setMemory()` and `getMemory()` methods bypass cache accounting; only simulated CPU load/store instructions contribute to the CPU's cache statistics.

---

## Performance Metrics

The simulator reports pipeline and cache statistics after execution.

### Cycles per instruction (CPI)

CPI measures the average number of simulated cycles per retired instruction:

\[
\text{CPI} =
\frac{\text{Total cycles}}{\text{Instructions retired}}
\]

Lower CPI generally indicates better instruction throughput for the same instruction workload, although comparisons must use equivalent workloads and timing assumptions.

### Instructions per cycle (IPC)

IPC measures the average number of retired instructions per cycle:

\[
\text{IPC} =
\frac{\text{Instructions retired}}{\text{Total cycles}}
\]

For nonzero cycle and retirement counts, CPI and IPC are reciprocals under these definitions.

### Pipeline statistics

| Metric | Meaning |
|---|---|
| Cycles | Total simulated pipeline cycles. |
| Instructions retired | Valid instructions reaching WB. |
| Load-use stalls | Cycles in which the modeled load-use hazard stalls the front of the pipeline. |
| Branches taken | Number of taken branches. |
| Instructions flushed | Number of wrong-path IF/ID instructions discarded. |
| CPI | Average cycles per retired instruction. |
| IPC | Average retired instructions per cycle. |

### Cache statistics

| Metric | Meaning |
|---|---|
| Cache accesses | Number of simulated CPU loads and stores. |
| Cache hits | Accesses whose memory block is already represented in the selected cache set. |
| Cache misses | Accesses whose memory block is not present in the selected set. |
| Hit rate | Fraction of accesses that hit. |
| Miss rate | Fraction of accesses that miss. |

**Important:** cache hit/miss results currently do not affect the cycle count or CPI. These metrics describe cache behavior independently of memory-access latency.

---

## Project Structure

```text
riscv-pipeline-simulator/
├── main.cpp
├── tests.cpp
├── cache_tests.cpp
├── cache_integration_tests.cpp
├── README.md
├── .gitignore
└── src/
    └── cpu/
        ├── CPU.h
        ├── Instruction.h
        ├── RegisterFile.h
        ├── ALU.h
        ├── Memory.h
        └── Cache.h
```

### File responsibilities

- `main.cpp`: example program and final execution report.
- `tests.cpp`: pipeline, forwarding, hazards, branches, and statistics tests.
- `cache_tests.cpp`: standalone cache mapping, LRU, and statistics tests.
- `cache_integration_tests.cpp`: checks cache behavior through CPU execution.
- `CPU.h`: processor pipeline and CPU-cache integration.
- `Cache.h`: cache organization, lookup, replacement, and statistics.
- `Instruction.h`: supported instruction representation.
- `RegisterFile.h`: register storage.
- `ALU.h`: arithmetic execution.
- `Memory.h`: backing data memory.

---

## Requirements

- Windows, Linux, or another environment with a compatible C++17 toolchain.
- GCC or MinGW-w64 is recommended for the provided commands.
- A terminal or command prompt.
- Git is optional for running the simulator but useful for obtaining and modifying the repository.

Check the compiler version:

```bash
g++ --version
```

---

## Build and Run

Clone the repository:

```bash
git clone https://github.com/jatin13aryan/riscv-pipeline-simulator.git
cd riscv-pipeline-simulator
```

Compile the simulator:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o simulator.exe
```

Run it on Windows Command Prompt:

```cmd
simulator.exe
```

On Linux, the output executable can instead be named `simulator`:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o simulator
./simulator
```

The example program initializes backing memory, performs loads and a store, exercises branch handling, and prints register values, memory contents, pipeline metrics, and cache statistics.

---

## Running the Tests

Each test suite is a separate executable.

### 1. Pipeline tests

Windows:

```cmd
g++ -std=c++17 -Wall -Wextra -Wpedantic tests.cpp -o tests.exe
tests.exe
```

Linux:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic tests.cpp -o tests
./tests
```

These tests cover supported arithmetic instructions, forwarding, load-use stalls, load/store behavior, branch handling, register-zero behavior, and pipeline statistics.

### 2. Standalone cache tests

Windows:

```cmd
g++ -std=c++17 -Wall -Wextra -Wpedantic cache_tests.cpp -o cache_tests.exe
cache_tests.exe
```

Linux:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic cache_tests.cpp -o cache_tests
./cache_tests
```

These tests cover cache hits and misses, address mapping, associativity, LRU replacement, rates, and cache reset.

### 3. CPU-cache integration tests

Windows:

```cmd
g++ -std=c++17 -Wall -Wextra -Wpedantic cache_integration_tests.cpp -o cache_integration_tests.exe
cache_integration_tests.exe
```

Linux:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic cache_integration_tests.cpp -o cache_integration_tests
./cache_integration_tests
```

These tests exercise the cache through CPU memory instructions and check the resulting statistics.

A passing test suite should finish with `Failed: 0`. Run all three suites after modifying pipeline or cache behavior.

---

## Design Decisions and Limitations

This project prioritizes clarity and observability over cycle-accurate reproduction of a commercial processor.

Current limitations include:

- **Limited instruction set:** only the implemented operations are supported; this is not a complete RISC-V ISA implementation.
- **Simplified pipeline control:** hazard detection and branch flushing cover the modeled cases rather than every possible processor dependency.
- **Simplified branch behavior:** there is no branch prediction or speculative execution.
- **Statistics-only cache integration:** the cache tracks tags and accesses, but backing memory supplies load data and receives stores directly.
- **No cache miss penalty:** cache misses do not stall the pipeline or add memory latency to simulated cycles.
- **No complete memory hierarchy:** there are no separate instruction/data caches, multilevel caches, TLBs, or virtual memory.
- **No cache coherence:** the model does not simulate multiple cores or coherent shared caches.
- **No hardware performance claims:** reported metrics are measurements of this simulator's model, not benchmark results from a physical RISC-V processor.

These constraints make the implementation easier to study and provide a foundation for future extensions.

---

## Future Improvements

Potential extensions include:

- Model cache-hit and cache-miss latency and stall the pipeline during memory waits.
- Add cache data storage and explicit write-through or write-back policies.
- Add dirty bits, write allocation, and no-write-allocate behavior.
- Support separate instruction and data caches.
- Add instruction fetch and branch prediction experiments.
- Improve pipeline control and dependency testing.
- Add cache configuration comparisons and memory-access traces.
- Implement a larger subset of RISC-V instructions.
- Add CMake-based builds and automated continuous integration.
- Compare CPI and IPC across controlled workloads and cache configurations.

---

## License

No license is currently specified. Add a `LICENSE` file before distributing the project under an open-source license.

---

## Author

**Aryan Jateen**

GitHub: [@jatin13aryan](https://github.com/jatin13aryan)

Project: [RISC-V Pipeline Simulator](https://github.com/jatin13aryan/riscv-pipeline-simulator)

---

*Built as an educational computer architecture project to explore processor pipelines, dependency handling, and data-cache behavior in C++.*





