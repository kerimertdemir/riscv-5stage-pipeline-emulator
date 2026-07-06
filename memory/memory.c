#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

void memory_init(Memory *mem) {
    for (int i = 0; i < MEM_SIZE; i++) {
        mem->data[i] = 0;
    }
}

uint32_t memory_read_instruction(Memory *mem, uint64_t address) {
    if (address + 3 >= MEM_SIZE) {
        printf("ERROR: Instruction read out of memory bounds!");
        exit(1);
    }

    uint32_t b0 = mem->data[address];
    uint32_t b1 = mem->data[address + 1];
    uint32_t b2 = mem->data[address + 2];
    uint32_t b3 = mem->data[address + 3];

    return (b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
}

int64_t memory_read64(Memory *mem, uint64_t address) {
    if (address + 7 >= MEM_SIZE) {
        printf("ERROR: Data read out of memory bounds!");
        exit(1);
    }

    int64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= ((int64_t)mem->data[address + i]) << (i * 8);
    }
    return value;
}

void memory_write64(Memory *mem, uint64_t address, int64_t value) {
    if (address + 7 >= MEM_SIZE) {
        printf("ERROR: Memory write out of bounds!");
        exit(1);
    }

    for (int i = 0; i < 8; i++) {
        mem->data[address + i] = (value >> (i * 8)) & 0xFF;
    }
}

int64_t memory_stage(I_type inst, int64_t alu_result, Memory *mem, CPU *my_cpu) {
    if (inst.opcode == 0x03) {
        return memory_read64(mem, alu_result);
    }
    if (inst.opcode == 0x23) {
        int64_t write_val = my_cpu->registers[inst.rd];
        memory_write64(mem, alu_result, write_val);
        return write_val;
    }
    return alu_result;
}
