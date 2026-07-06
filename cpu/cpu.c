#include "cpu.h"
#include "memory.h"

void cpu_init(CPU *my_cpu) {
    for (int i = 0; i < REG_COUNT; i++) {
        my_cpu->registers[i] = 0;
    }
    my_cpu->pc = 0;

    my_cpu->IF_ID.valid = 0;
    my_cpu->ID_EX.valid = 0;
    my_cpu->EX_MEM.valid = 0;
    my_cpu->MEM_WB.valid = 0;
}

uint32_t cpu_fetch(CPU *my_cpu, void *my_mem) {
    Memory *mem = (Memory *)my_mem;
    uint32_t instruction = memory_read_instruction(mem, my_cpu->pc);
    my_cpu->pc += 4;
    return instruction;
}
