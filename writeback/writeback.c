#include "writeback.h"

void writeback_stage(I_type inst, int64_t final_result, CPU *my_cpu) {
    if (inst.rd != 0) {
        my_cpu->registers[inst.rd] = final_result;
    }
}
