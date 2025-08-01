# RISC-V Simulator

A simple, out-of-order RISC-V processor simulator implementing the Tomasulo algorithm for dynamic instruction scheduling.

## Features

### Architecture
- **Out-of-Order Execution**: Implements Tomasulo's algorithm with reservation stations
- **Dynamic Instruction Scheduling**: Instructions execute as soon as operands are available
- **Branch Prediction**: Integrated branch predictor to minimize pipeline stalls
- **Speculative Execution**: Execute instructions speculatively and rollback on misprediction
- **Precise Exception Handling**: Maintains program correctness during exceptions

### Core Components
- **Instruction Fetch Module (IFM)**: Fetches instructions with branch prediction
- **ReOrder Buffer (ROB)**: Maintains program order and handles commits
- **Reservation Station (RS)**: Manages ALU instruction dependencies
- **Load-Store Buffer (LSB)**: Handles memory operations with dependency resolution
- **Branch Analyzer (BA)**: Processes branch instructions and jump calculations
- **Common Data Bus (CDB)**: Broadcasts results to dependent instructions
- **Register File**: Manages register renaming and dependency tracking

### RISC-V ISA Support
- The project supports a major subset of the RISC-V Base Integer Instruction Set (RV32I):
- **Arithmetic Instructions**: ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU
- **Immediate Instructions**: ADDI, ANDI, ORI, XORI, SLLI, SRLI, SRAI, SLTI, SLTIU
- **Load/Store Instructions**: LB, LBU, LH, LHU, LW, SB, SH, SW
- **Branch Instructions**: BEQ, BNE, BLT, BGE, BLTU, BGEU
- **Jump Instructions**: JAL, JALR
- **Upper Immediate Instructions**: LUI, AUIPC

## Building and Running

### Prerequisites
- CMake 3.28 or higher
- C++20 compatible compiler
- Linux/Unix environment (tested on Linux)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/RayZh-hs/RISC-V-Sim.git
cd RISC-V-Sim

# Create build directory
mkdir -p build
cd build

# Configure and build
cmake ..
make

# The executable will be created as 'code' in the project root
cd ..
time ./code < testcases/array_test1.data
```

### Build Configurations

The project supports different build configurations through CMake definitions:

- **Optimized Build** (default): `-O3` optimization with logging and checks disabled
- **Debug Build**: Enable all logging and runtime checks
- **Unit Tests**: Individual test executables for each component

### Running Tests

```bash
# Run a specific test case
./code < testcases/array_test1.data

# Expected output: program return value (typically 0-255)
# The simulator outputs the value of register x10 (a0) as program result
```

## Project Structure

```
RISC-V-Sim/
├── src/
│   ├── main.cpp             # Main entry point
│   ├── include/             # Header files
│   │   ├── simulator.hpp    # Main simulator class
│   │   ├── rob.hpp          # ReOrder Buffer
│   │   ├── rs.hpp           # Reservation Station
│   │   ├── lsb.hpp          # Load-Store Buffer
│   │   ├── ifm.hpp          # Instruction Fetch Module
│   │   ├── decoder.hpp      # Instruction decoder
│   │   ├── alu.hpp          # Arithmetic Logic Unit
│   │   ├── cdb.hpp          # Common Data Bus
│   │   └── utility/         # Utility classes
│   └── impl/                # Implementation files
├── testcases/               # Test programs and data
├── unit_tests/              # Unit test files
├── docs/                    # Documentation and specifications
├── dumps/                   # Register dump outputs
└── CMakeLists.txt           # Build configuration
```

Official testcases, documentations and program dumps are not provided in the online repository. The online version only contains the source code and unit tests.

## References

- RISC-V Instruction Set Manual
- Computer Architecture: A Quantitative Approach (Hennessy & Patterson)
- Tomasulo's Algorithm for Dynamic Instruction Scheduling
- Course materials: Tomasulo2025.pdf, CAAQA5.pdf

---

**Note**: This simulator is designed for educational purposes and may not implement all aspects of a production RISC-V processor. It focuses on demonstrating out-of-order execution principles and the Tomasulo algorithm.
