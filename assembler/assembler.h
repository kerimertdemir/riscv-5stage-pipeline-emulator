#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>

// LABEL SYMBOL TABLE STRUCTURE
typedef struct {
    char name[20];
    uint64_t address;
} LabelSymbol;

extern LabelSymbol symbol_table[100];
extern int symbol_count;

void assembler_pass1(const char *program[], int line_count);
uint32_t assemble_line(const char *assembly_string, uint64_t current_pc);

#endif
