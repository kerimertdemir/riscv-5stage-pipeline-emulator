#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "decode.h"

#define REG_COUNT 32

typedef struct {
    uint32_t instruction;
    uint64_t pc; // Tracked PC
    int valid;
} IF_ID_Reg;

typedef struct {
    I_type decoded_inst;
    uint64_t pc; // Tracked PC
    int valid;
} ID_EX_Reg;

typedef struct {
    I_type decoded_inst;
    int64_t alu_result;
    uint64_t pc; // Tracked PC
    int valid;
} EX_MEM_Reg;

typedef struct {
    I_type decoded_inst;
    int64_t final_result;
    uint64_t pc; // Tracked PC
    int valid;
} MEM_WB_Reg;

typedef struct {
    uint64_t registers[REG_COUNT];
    uint64_t pc;

    IF_ID_Reg   IF_ID;
    ID_EX_Reg   ID_EX;
    EX_MEM_Reg  EX_MEM;
    MEM_WB_Reg  MEM_WB;
} CPU;

void cpu_init(CPU *my_cpu);

#endif
