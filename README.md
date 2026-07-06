# RISC-V 5-Stage Pipeline Emulator

A cycle-accurate, high-performance 5-stage pipeline RISC-V architectural emulator written in clean C. The project features a custom two-pass integrated assembler with ABI register translation, structural data forwarding paths, and precise pipeline hazard management designed to match standard industrial metrics (e.g., Kite simulator configurations).

---

## 🚀 Key Architectural Features

### 1. 5-Stage Pipeline Infrastructure
The core processor simulation models a synchronous clock-cycle execution flow split into five distinct pipeline stages with intermediate latch boundaries:
- **Instruction Fetch (IF):** Fetches the 32-bit instruction from memory based on the Program Counter (PC).
- **Instruction Decode (ID):** Decodes the opcode, funct3, and funct7 fields. Translates RISC-V ABI register aliases to raw logical addresses and extracts signs/immediates.
- **Execute (EX):** Handles ALU operations, branch evaluation, and effective address calculations.
- **Memory Access (MEM):** Interacts with data memory for load/store (`lw`, `sw`) instructions.
- **Writeback (WB):** Commits computed results or loaded memory content back to the register file.

### 2. Hazard Management & Interlocking
- **Data Forwarding Paths:** Implements dedicated bypassing logic from both the **EX/MEM** and **MEM/WB** pipeline registers down to the **ID/EX** execution inputs. This completely eliminates data-dependency stalls for consecutive R-type and I-type instructions.
- **Control Hazard Flushing:** Implements a branch/jump misprediction/resolution mechanism. When a conditional branch is taken or an unconditional jump (`jal`, `jalr`) is executed, the pipeline performs a **2-cycle bubble flush penalty** to clear speculative instructions in the IF and ID stages, matching realistic hardware layouts.

### 3. Integrated Two-Pass Assembler
Features a highly flexible assembly parser that simplifies test execution:
- **First Pass:** Scans source code to map symbol labels to absolute memory instruction offsets.
- **Second Pass:** Translates instructions into machine state representations and calculates PC-relative offsets for branches and jumps.
- **Full ABI Translation:** Fully supports standard RISC-V Application Binary Interface (ABI) register mnemonics (`zero`, `ra`, `sp`, `gp`, `tp`, `t0-t6`, `a0-a7`, `s0-s11`) instead of forcing raw numeric x-register indices.

---

## 💾 Pre-Compiled Windows Binary (Quick Start)

If you are on Windows and don't want to set up a C toolchain, you can download the standalone executable directly:
1. Navigate to the **Releases** tab on the right side of this repository.
2. Download `emulator.exe` (compiled via MinGW cross-compiler).
3. Simply double-click the `emulator.exe` to launch the standalone console application, or run it via **PowerShell** / **Command Prompt**:
   ```powershell
   ./emulator.exe
