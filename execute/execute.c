#include "execute.h"

int64_t execute_stage(I_type inst, CPU *my_cpu) {
    if (inst.opcode == 0x13) {
        switch (inst.funct3) {
            case 0x0:
                return my_cpu->registers[inst.rs1] + inst.imm; // ADDI
            case 0x7:
                return my_cpu->registers[inst.rs1] & inst.imm; // ANDI
            case 0x6:
                return my_cpu->registers[inst.rs1] | inst.imm; // ORI
            case 0x4:
                return my_cpu->registers[inst.rs1] ^ inst.imm; // XORI
            case 0x2:
                return ((int64_t)my_cpu->registers[inst.rs1] < (int64_t)inst.imm) ? 1 : 0; // SLTI
            case 0x3:
                return ((uint64_t)my_cpu->registers[inst.rs1] < (uint64_t)inst.imm) ? 1 : 0; // SLTIU
            case 0x1:
                return my_cpu->registers[inst.rs1] << (inst.imm & 0x3F); // SLLI
            case 0x5: {
                uint32_t is_arithmetic = (inst.imm >> 10) & 0x1;
                if (is_arithmetic) {
                    return (int64_t)my_cpu->registers[inst.rs1] >> (inst.imm & 0x3F); // SRAI
                } else {
                    return my_cpu->registers[inst.rs1] >> (inst.imm & 0x3F); // SRLI
                }
            }
        }
    }
    if (inst.opcode == 0x03) {
        return my_cpu->registers[inst.rs1] + inst.imm; // LOAD
    }
    if (inst.opcode == 0x23) {
        return my_cpu->registers[inst.rs1] + inst.imm; // STORE
    }
    if (inst.opcode == 0x6F) {
        return my_cpu->ID_EX.pc + 4; // JAL
    }
    if (inst.opcode == 0x67) {
        return my_cpu->ID_EX.pc + 4; // JALR
    }
    if (inst.opcode == 0x63) {
        int64_t src1 = my_cpu->registers[inst.rs1];
        int64_t src2 = my_cpu->registers[inst.rd];
        switch (inst.funct3) {
            case 0x0: return (src1 == src2) ? 1 : 0; // BEQ
            case 0x1: return (src1 != src2) ? 1 : 0; // BNE
            case 0x4: return (src1 < src2) ? 1 : 0;  // BLT
            case 0x5: return (src1 >= src2) ? 1 : 0; // BGE
            default:  return 0;
        }
    }
    if (inst.opcode == 0x1B) {
        int32_t src1_32 = (int32_t)my_cpu->registers[inst.rs1];

        switch (inst.funct3) {
            case 0x0:
                return (int64_t)(src1_32 + (int32_t)inst.imm); // ADDIW

            case 0x1:
                return (int64_t)(src1_32 << (inst.imm & 0x1F)); // SLLIW

            case 0x5: {
                uint32_t is_arithmetic = (inst.imm >> 10) & 0x1;
                if (is_arithmetic) {
                    return (int64_t)(src1_32 >> (inst.imm & 0x1F)); // SRAIW
                } else {
                    uint32_t src1_u32 = (uint32_t)my_cpu->registers[inst.rs1];
                    return (int64_t)(int32_t)(src1_u32 >> (inst.imm & 0x1F)); // SRLIW
                }
            }
        }
    }
    if (inst.opcode == 0x33) {
        int rs2_reg = inst.imm & 0x1F;
        uint32_t funct7 = (inst.imm >> 5) & 0x7F;
        int64_t src1 = my_cpu->registers[inst.rs1];
        int64_t src2 = my_cpu->registers[rs2_reg];

        switch (inst.funct3) {
            case 0x0:
                if (funct7 == 0x20) return src1 - src2; // SUB
                return src1 + src2;                     // ADD

            case 0x1: return src1 << (src2 & 0x3F);     // SLL
            case 0x4: return src1 ^ src2;               // XOR

            case 0x5:
                if (funct7 == 0x20) return src1 >> (src2 & 0x3F);  // SRA
                return (int64_t)((uint64_t)src1 >> (src2 & 0x3F)); // SRL

            case 0x6: return src1 | src2;               // OR
            case 0x7: return src1 & src2;               // AND
        }
    }
    return 0;
}
