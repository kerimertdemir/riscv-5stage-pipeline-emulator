#ifndef WRITEBACK_H
#define WRITEBACK_H

#include "cpu.h"
#include "decode.h"

void writeback_stage(I_type inst, int64_t final_result, CPU *my_cpu);

#endif
