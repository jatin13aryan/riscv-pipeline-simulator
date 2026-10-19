

\# RISC-V Pipeline Simulator



A C++17 educational simulator for exploring a simplified five-stage RISC-V-style processor pipeline and data-cache behavior.



\## Features



\- Instruction execution for ADD, SUB, ADDI, LOAD, STORE, BEQ and NOP

\- Register file with x0 hardwired to zero

\- ALU and word-addressed data memory

\- Five pipeline stages: IF, ID, EX, MEM and WB

\- Data forwarding for supported dependencies

\- Load-use hazard detection and stalls

\- Branch handling and wrong-path instruction flushing

\- Configurable data cache capacity, block size and associativity

\- LRU cache-line replacement

\- Cache hit/miss counters and hit/miss rates

\- Pipeline cycle count, retired instructions, CPI and IPC

\- Standalone and integration test suites



\## Requirements



\- A C++17-compatible compiler

\- GCC or MinGW-w64 on Windows



\## Build and run



Compile the simulator:



```bash

g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o simulator.exe

```



Run it:



```bash

./simulator.exe

```



\## Run tests



Pipeline tests:



```bash

g++ -std=c++17 -Wall -Wextra -Wpedantic tests.cpp -o tests.exe

./tests.exe

```



Standalone cache tests:



```bash

g++ -std=c++17 -Wall -Wextra -Wpedantic cache\_tests.cpp -o cache\_tests.exe

./cache\_tests.exe

```



CPU-cache integration tests:



```bash

g++ -std=c++17 -Wall -Wextra -Wpedantic cache\_integration\_tests.cpp -o cache\_integration\_tests.exe

./cache\_integration\_tests.exe

```



\## Cache configuration



The CPU constructor accepts:



```cpp

CPU cpu(1024, 16, 1);

```



These values represent cache capacity in bytes, block size in bytes, and associativity. The example creates a 1 KiB direct-mapped cache with 16-byte blocks.



\## Current limitations



\- This is an educational, simplified pipeline model, not a complete RISC-V implementation.

\- Cache accesses update statistics but do not yet add cache-miss latency or stall the pipeline.

\- Backing memory is accessed directly by the simulator; cache contents are used for hit/miss accounting rather than data storage.

\- The supported instruction set and branch behavior are intentionally limited.



\## License



Add a license before distributing the project publicly.





