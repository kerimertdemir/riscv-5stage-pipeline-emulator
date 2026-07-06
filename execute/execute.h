#ifndef EXECUTE_H
#define EXECUTE_H

#include "decode.h"
#include "cpu.h"

int64_t execute_stage(I_type inst, CPU *my_cpu);

#endif
