#include "assembler.h"
#include <string.h>
#include <stdio.h>

LabelSymbol symbol_table[100];
int symbol_count = 0;

// HELPER: TRANSLATE RISC-V ABI ALIAS NAMES TO HARDWARE X-REGISTERS Safely
void replace_abi_registers(char *str) {
    const char *abi_names[] = {
        "zero", "ra", "sp", "gp", "tp",
        "t6", "t5", "t4", "t3",
        "s11", "s10", "s9", "s8", "s7", "s6", "s5", "s4", "s3", "s2", "s1", "s0", "fp",
        "a7", "a6", "a5", "a4", "a3", "a2", "a1", "a0",
        "t2", "t1", "t0"
    };

    const char *x_names[] = {
        "x0", "x1", "x2", "x3", "x4",
        "x31", "x30", "x29", "x28",
        "x27", "x26", "x25", "x24", "x23", "x22", "x21", "x20", "x19", "x18", "x9", "x8", "x8",
        "x17", "x16", "x15", "x14", "x13", "x12", "x11", "x10",
        "x7", "x6", "x5"
    };

    char buffer[200];
    for (int i = 0; i < 33; i++) {
        char *pos;
        while ((pos = strstr(str, abi_names[i])) != NULL) {
            int len = strlen(abi_names[i]);

            // Validate word boundaries to protect custom labels (e.g., "my_func_ra:")
            int valid_before = (pos == str || *(pos - 1) == ' ' || *(pos - 1) == ',' || *(pos - 1) == '(' || *(pos - 1) == ':');
            int valid_after = (*(pos + len) == '\0' || *(pos + len) == ' ' || *(pos + len) == ',' || *(pos + len) == ')' || *(pos + len) == '\r' || *(pos + len) == '\n');

            if (valid_before && valid_after) {
                strncpy(buffer, str, pos - str);
                buffer[pos - str] = '\0';
                strcat(buffer, x_names[i]);
                strcat(buffer, pos + len);
                strcpy(str, buffer);
            } else {
                // Not an explicit alias match, break to prevent infinite tracking loops
                break;
            }
        }
    }
}

// PASS 1: RESOLVE LABELS AND ADDRESSES
void assembler_pass1(const char *program[], int line_count) {
    uint64_t current_pc = 0;
    symbol_count = 0;

    for (int i = 0; i < line_count; i++) {
        char first_token[50];
        if (sscanf(program[i], "%s", first_token) != 1) continue;

        // CHECK IF IT IS A LABEL (ENDS WITH COLON)
        if (first_token[strlen(first_token) - 1] == ':') {
            first_token[strlen(first_token) - 1] = '\0'; // STRIP COLON
            strcpy(symbol_table[symbol_count].name, first_token);
            symbol_table[symbol_count].address = current_pc;
            symbol_count++;
        } else if (strlen(program[i]) > 0) {
            current_pc += 4; // BUMP PC FOR VALID INSTRUCTION
        }
    }
}

uint32_t assemble_line(const char *assembly_string, uint64_t current_pc) {
    char inst_name[10];
    int rd = 0, rs1 = 0, rs2 = 0, imm = 0;
    uint32_t machine_code = 0;

    char cleaned_string[100];
    strcpy(cleaned_string, assembly_string);

    // SKIP IF IT IS JUST A LABEL DEF
    if (cleaned_string[strlen(cleaned_string) - 1] == ':') return 0;

    // INTERCEPT AND TRANSLATE RISC-V ABI NAMES TO X-REGISTERS BEFORE PARSING
    replace_abi_registers(cleaned_string);

    if (sscanf(cleaned_string, "%s", inst_name) != 1) return 0;

    // PROCESS JAL INSTRUCTION (J-TYPE)
    if (strcmp(inst_name, "jal") == 0) {
        char label_name[30];
        if (sscanf(cleaned_string, "%s x%d, %s", inst_name, &rd, label_name) == 3) {
            uint32_t safe_rd = rd & 0x1F;
            int32_t offset = 0;
            int found = 0;

            // LOOKUP LABEL IN SYMBOL TABLE
            for (int i = 0; i < symbol_count; i++) {
                if (strcmp(symbol_table[i].name, label_name) == 0) {
                    offset = (int32_t)symbol_table[i].address - (int32_t)current_pc;
                    found = 1;
                    break;
                }
            }

            if (!found) {
                sscanf(label_name, "%d", &imm);
                offset = imm;
            }

            uint32_t safe_imm = (uint32_t)offset & 0x1FFFFF;
            uint32_t opcode = 0x6F;

            // REAL RISC-V J-TYPE IMMEDIATE BIT JUGGLING
            uint32_t imm20 = (safe_imm >> 20) & 0x1;
            uint32_t imm10_1 = (safe_imm >> 1) & 0x3FF;
            uint32_t imm11 = (safe_imm >> 11) & 0x1;
            uint32_t imm19_12 = (safe_imm >> 12) & 0xFF;

            return (imm20 << 31) | (imm10_1 << 21) | (imm11 << 20) | (imm19_12 << 12) | (safe_rd << 7) | opcode;
        }
    }

    // PROCESS JALR INSTRUCTION (I-TYPE)
    if (strcmp(inst_name, "jalr") == 0) {
        if (sscanf(cleaned_string, "jalr x%d , %d ( x%d )", &rd, &imm, &rs1) == 3 ||
            sscanf(cleaned_string, "jalr x%d,%d(x%d)", &rd, &imm, &rs1) == 3 ||
            sscanf(cleaned_string, "jalr x%d, %d(x%d)", &rd, &imm, &rs1) == 3) {

            uint32_t safe_rd = rd & 0x1F;
            uint32_t safe_rs1 = rs1 & 0x1F;
            uint32_t safe_imm = (uint32_t)imm & 0xFFF;
            uint32_t funct3 = 0x0;
            uint32_t opcode = 0x67;

            return (safe_imm << 20) | (safe_rs1 << 15) | (funct3 << 12) | (safe_rd << 7) | opcode;
        }
    }

    // PROCESS BRANCH INSTRUCTIONS WITH LABELS
    if (strcmp(inst_name, "beq") == 0 || strcmp(inst_name, "bne") == 0 ||
        strcmp(inst_name, "blt") == 0 || strcmp(inst_name, "bge") == 0) {

        char label_name[30];
        if (sscanf(cleaned_string, "%s x%d, x%d, %s", inst_name, &rd, &rs1, label_name) == 4) {
            uint32_t safe_rs1 = rd & 0x1F;
            uint32_t safe_rs2 = rs1 & 0x1F;
            int32_t offset = 0;
            int found = 0;

            // LOOKUP LABEL IN SYMBOL TABLE
            for (int i = 0; i < symbol_count; i++) {
                if (strcmp(symbol_table[i].name, label_name) == 0) {
                    offset = (int32_t)symbol_table[i].address - (int32_t)current_pc;
                    found = 1;
                    break;
                }
            }

            // FALLBACK TO DIRECT IMM IF NOT FOUND IN TABLE
            if (!found) {
                sscanf(label_name, "%d", &imm);
                offset = imm;
            }

            uint32_t safe_imm = (uint32_t)offset & 0x1FFF;
            uint32_t opcode = 0x63;
            uint32_t funct3 = 0;

            if (strcmp(inst_name, "beq") == 0) funct3 = 0x0;
            if (strcmp(inst_name, "bne") == 0) funct3 = 0x1;
            if (strcmp(inst_name, "blt") == 0) funct3 = 0x4;
            if (strcmp(inst_name, "bge") == 0) funct3 = 0x5;

            uint32_t imm12 = (safe_imm >> 12) & 0x1;
            uint32_t imm11 = (safe_imm >> 11) & 0x1;
            uint32_t imm10_5 = (safe_imm >> 5) & 0x3F;
            uint32_t imm4_1 = (safe_imm >> 1) & 0xF;

            return (imm12 << 31) | (imm10_5 << 25) | (safe_rs2 << 20) | (safe_rs1 << 15) | (funct3 << 12) | (imm4_1 << 8) | (imm11 << 7) | opcode;
        }
    }

    if (strcmp(inst_name, "add") == 0 || strcmp(inst_name, "sub") == 0 ||
        strcmp(inst_name, "sll") == 0 || strcmp(inst_name, "xor") == 0 ||
        strcmp(inst_name, "srl") == 0 || strcmp(inst_name, "sra") == 0 ||
        strcmp(inst_name, "or")  == 0 || strcmp(inst_name, "and") == 0) {

        if (sscanf(cleaned_string, "%s x%d, x%d, x%d", inst_name, &rd, &rs1, &rs2) == 4) {
            uint32_t safe_rd  = rd & 0x1F;
            uint32_t safe_rs1 = rs1 & 0x1F;
            uint32_t safe_rs2 = rs2 & 0x1F;
            uint32_t opcode   = 0x33;

            if (strcmp(inst_name, "add") == 0) {
                uint32_t funct3 = 0x00;
                uint32_t funct7 = 0x00;
                return (funct7 << 25) | (safe_rs2 << 20) | (safe_rs1 << 15) | (funct3 << 12) | (safe_rd << 7) | opcode;
            }
            if (strcmp(inst_name, "sub") == 0) {
                uint32_t funct3 = 0x00;
                uint32_t funct7 = 0x20;
                return (funct7 << 25) | (safe_rs2 << 20) | (safe_rs1 << 15) | (funct3 << 12) | (safe_rd << 7) | opcode;
            }
            if (strcmp(inst_name, "sll") == 0) {
                uint32_t funct3 = 0x01;
                uint32_t funct7 = 0x00;
                return (funct7 << 25) | (safe_rs2 << 20) | (safe_rs1 << 15) | (funct3 << 12) | (safe_rd << 7) | opcode;
            }
            if (strcmp(inst_name, "xor") == 0) {
                uint32_t funct3 = 0x04;
                uint32_t funct7 = 0x00;
                return (funct7 << 25) | (safe_rs2 << 20) | (safe_rs1 << 15) | (funct3 << 12) | (safe_rd << 7) | opcode;
            }
            if (strcmp(inst_name, "srl") == 0) {
                uint32_t funct3 = 0x05;
                uint32_t funct7 = 0x00;
                return (funct7 << 25) | (safe_rs2 << 20) | (safe_rs1 << 15) | (funct3 << 12) | (safe_rd << 7) | opcode;
            }
            if (strcmp(inst_name, "sra") == 0) {
                uint32_t funct3 = 0x05;
                uint32_t funct7 = 0x20;
                return (funct7 << 25) | (safe_rs2 << 20) | (safe_rs1 << 15) | (funct3 << 12) | (safe_rd << 7) | opcode;
            }
            if (strcmp(inst_name, "or") == 0) {
                uint32_t funct3 = 0x06;
                uint32_t funct7 = 0x00;
                return (funct7 << 25) | (safe_rs2 << 20) | (safe_rs1 << 15) | (funct3 << 12) | (safe_rd << 7) | opcode;
            }
            if (strcmp(inst_name, "and") == 0) {
                uint32_t funct3 = 0x07;
                uint32_t funct7 = 0x00;
                return (funct7 << 25) | (safe_rs2 << 20) | (safe_rs1 << 15) | (funct3 << 12) | (safe_rd << 7) | opcode;
            }
        }
    }

    if (strcmp(inst_name, "ld") == 0 || strcmp(inst_name, "lw") == 0 ||
        strcmp(inst_name, "lh") == 0 || strcmp(inst_name, "lb") == 0 ||
        strcmp(inst_name, "sb") == 0 || strcmp(inst_name, "sh") == 0 ||
        strcmp(inst_name, "sw") == 0 || strcmp(inst_name, "sd") == 0) {

        if (sscanf(cleaned_string, "%s x%d, %d(x%d)", inst_name, &rd, &imm, &rs1) == 4) {
            uint32_t safe_imm = (uint32_t)imm & 0xFFF;

            if (strcmp(inst_name, "ld") == 0) {
                uint32_t opcode = 0x03;
                uint32_t funct3 = 0x03;
                return (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            }
            if (strcmp(inst_name, "lw") == 0) {
                uint32_t opcode = 0x03;
                uint32_t funct3 = 0x02;
                return (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            }
            if (strcmp(inst_name, "lh") == 0) {
                uint32_t opcode = 0x03;
                uint32_t funct3 = 0x01;
                return (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            }
            if (strcmp(inst_name, "lb") == 0) {
                uint32_t opcode = 0x03;
                uint32_t funct3 = 0x00;
                return (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            }
            if (strcmp(inst_name, "sb") == 0) {
                uint32_t opcode = 0x23;
                uint32_t funct3 = 0x00;
                return (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            }
            if (strcmp(inst_name, "sh") == 0) {
                uint32_t opcode = 0x23;
                uint32_t funct3 = 0x01;
                return (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            }
            if (strcmp(inst_name, "sw") == 0) {
                uint32_t opcode = 0x23;
                uint32_t funct3 = 0x02;
                return (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            }
            if (strcmp(inst_name, "sd") == 0) {
                uint32_t opcode = 0x23;
                uint32_t funct3 = 0x03;
                return (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            }
        }
    }

    if (sscanf(cleaned_string, "%s x%d, x%d, %d", inst_name, &rd, &rs1, &imm) == 4) {
        uint32_t safe_imm = (uint32_t)imm & 0xFFF;

        if (strcmp(inst_name, "addi") == 0) {
            uint32_t opcode = 0x13;
            uint32_t funct3 = 0x00;
            machine_code = (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            return machine_code;
        }
        if (strcmp(inst_name, "andi") == 0) {
            uint32_t opcode = 0x13;
            uint32_t funct3 = 0x07;
            machine_code = (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            return machine_code;
        }
        if (strcmp(inst_name, "ori") == 0) {
            uint32_t opcode = 0x13;
            uint32_t funct3 = 0x06;
            machine_code = (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            return machine_code;
        }
        if (strcmp(inst_name, "xori") == 0) {
            uint32_t opcode = 0x13;
            uint32_t funct3 = 0x04;
            machine_code = (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            return machine_code;
        }
        if (strcmp(inst_name, "slti") == 0) {
            uint32_t opcode = 0x13;
            uint32_t funct3 = 0x02;
            machine_code = (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            return machine_code;
        }
        if (strcmp(inst_name, "sltiu") == 0) {
            uint32_t opcode = 0x13;
            uint32_t funct3 = 0x03;
            machine_code = (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            return machine_code;
        }
        if (strcmp(inst_name, "slli") == 0) {
            uint32_t opcode = 0x13;
            uint32_t funct3 = 0x01;
            uint32_t shamt = safe_imm & 0x3F;
            machine_code = (shamt << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            return machine_code;
        }
        if (strcmp(inst_name, "srli") == 0) {
            uint32_t opcode = 0x13;
            uint32_t funct3 = 0x05;
            uint32_t shamt = safe_imm & 0x3F;
            machine_code = (shamt << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            return machine_code;
        }
        if (strcmp(inst_name, "srai") == 0) {
            uint32_t opcode = 0x13;
            uint32_t funct3 = 0x05;
            uint32_t shamt = (safe_imm & 0x3F) | 0x400;
            machine_code = (shamt << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            return machine_code;
        }
        if (strcmp(inst_name, "addiw") == 0) {
            uint32_t opcode = 0x1B;
            uint32_t funct3 = 0x00;
            machine_code = (safe_imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            return machine_code;
        }
        if (strcmp(inst_name, "slliw") == 0) {
            uint32_t opcode = 0x1B;
            uint32_t funct3 = 0x01;
            uint32_t shamt = safe_imm & 0x1F;
            machine_code = (shamt << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            return machine_code;
        }
        if (strcmp(inst_name, "srliw") == 0 || strcmp(inst_name, "sraiw") == 0) {
            uint32_t opcode = 0x1B;
            uint32_t funct3 = 0x05;
            uint32_t shamt = safe_imm & 0x1F;
            uint32_t high_bits = (strcmp(inst_name, "sraiw") == 0) ? 0x400 : 0x000;
            machine_code = ((high_bits | shamt) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
            return machine_code;
        }
    }

    return 0;
}
