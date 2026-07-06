#include <stdio.h>
#include "decode.h"
#include <stdint.h>
#include "cpu.h"

I_type decode_i_type(uint32_t instruction){
    uint32_t opcode = instruction & 0x7F;

    // JAL DECODER (J-TYPE TO INTERNAL I-TYPE STRUCT)
    if (opcode == 0x6F) {
        I_type jal_inst;
        jal_inst.opcode = opcode;
        jal_inst.rd = (instruction >> 7) & 0x1F;
        jal_inst.funct3 = 0;
        jal_inst.rs1 = 0;

        int32_t imm20 = (instruction >> 31) & 0x1;
        int32_t imm19_12 = (instruction >> 12) & 0xFF;
        int32_t imm11 = (instruction >> 20) & 0x1;
        int32_t imm10_1 = (instruction >> 21) & 0x3FF;

        int32_t sign_extended_imm = (imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1);
        if (imm20) {
            sign_extended_imm |= 0xFFE00000;
        }
        jal_inst.imm = sign_extended_imm;
        return jal_inst;
    }

    // BRANCH DECODER (B-TYPE)
    if (opcode == 0x63) {
        I_type branch_inst;
        branch_inst.opcode = opcode;
        branch_inst.rs1 = (instruction >> 15) & 0x1F;
        branch_inst.funct3 = (instruction >> 12) & 0x07;

        int rs2_reg = (instruction >> 20) & 0x1F;
        branch_inst.rd = rs2_reg;

        int32_t imm12 = (instruction >> 31) & 0x1;
        int32_t imm11 = (instruction >> 7) & 0x1;
        int32_t imm10_5 = (instruction >> 25) & 0x3F;
        int32_t imm4_1 = (instruction >> 8) & 0xF;

        int32_t sign_extended_imm = (imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1 << 1);
        if (imm12) {
            sign_extended_imm |= 0xFFFFE000;
        }
        branch_inst.imm = sign_extended_imm;
        return branch_inst;
    }

    // GENERAL I-TYPE DECODER
    I_type res;
    res.opcode = instruction & 0x7F;
    res.rd = (instruction >> 7) & 0x1F;
    res.funct3 = (instruction >> 12) & 0x07;
    res.rs1 = (instruction >> 15) & 0x1F;
    res.imm = ((int32_t)instruction >> 20);
    return res;
}

R_type decode_r_type(uint32_t instruction) {
    R_type inst;
    inst.opcode = instruction & 0x7F;
    inst.rd     = (instruction >> 7) & 0x1F;
    inst.funct3 = (instruction >> 12) & 0x07;
    inst.rs1    = (instruction >> 15) & 0x1F;
    inst.rs2    = (instruction >> 20) & 0x1F;
    inst.funct7 = (instruction >> 25) & 0x7F;
    return inst;
}
