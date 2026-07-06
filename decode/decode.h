#ifndef DECODE_H
#define DECODE_H

#include <stdint.h>

typedef struct{
    uint8_t opcode;
    uint8_t rd;
    uint8_t funct3;
    uint8_t rs1;
    int64_t imm;
}I_type;

typedef struct {
    uint32_t opcode;
    uint32_t rd;
    uint32_t funct3;
    uint32_t rs1;
    uint32_t rs2;
    uint32_t funct7;
} R_type;

I_type decode_i_type(uint32_t instruction);

R_type decode_r_type(uint32_t instruction);

#endif // DECODE
