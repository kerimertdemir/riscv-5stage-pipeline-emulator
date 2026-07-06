#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include "decode.h"
#include "cpu.h"

#define MEM_SIZE 65536

typedef struct {
    uint8_t data[MEM_SIZE];
} Memory;

void memory_init(Memory *mem);
uint32_t memory_read_instruction(Memory *mem, uint64_t address);
int64_t memory_read64(Memory *mem, uint64_t address);
void memory_write64(Memory *mem, uint64_t address, int64_t value);
int64_t memory_stage(I_type inst, int64_t alu_result, Memory *mem, CPU *my_cpu);

#endif
