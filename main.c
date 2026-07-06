#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "memory.h"
#include "decode.h"
#include "execute.h"
#include "writeback.h"
#include "assembler.h"

#define MAX_LINES 200

const char* get_inst_name(uint32_t opcode, uint32_t funct3) {
    if (opcode == 0x03) {
        switch (funct3) {
            case 0x3: return "ld";
            case 0x2: return "lw";
            case 0x1: return "lh";
            case 0x0: return "lb";
            default:  return "ld";
        }
    }
    else if (opcode == 0x23) {
        switch (funct3) {
            case 0x0: return "sb";
            case 0x1: return "sh";
            case 0x2: return "sw";
            case 0x3: return "sd";
            default:  return "sd";
        }
    }
    else if (opcode == 0x63) {
        switch (funct3) {
            case 0x0: return "beq";
            case 0x1: return "bne";
            case 0x4: return "blt";
            case 0x5: return "bge";
            default:  return "beq";
        }
    }
    else if (opcode == 0x6F) {
        return "jal";
    }
    else if (opcode == 0x67) {
        return "jalr";
    }
    else if (opcode == 0x1B) {
        switch (funct3) {
            case 0x0: return "addiw";
            case 0x1: return "slliw";
            case 0x5: return "srliw";
            default:  return "addiw";
        }
    }
    else if (opcode == 0x33) {
        switch (funct3) {
            case 0x0: return "add";
            case 0x1: return "sll";
            case 0x4: return "xor";
            case 0x5: return "srl";
            case 0x6: return "or";
            case 0x7: return "and";
            default:  return "add";
        }
    }
    else {
        switch (funct3) {
            case 0x0: return "addi";
            case 0x7: return "andi";
            case 0x6: return "ori";
            case 0x4: return "xori";
            case 0x2: return "slti";
            case 0x3: return "sltiu";
            case 0x1: return "slli";
            case 0x5: return "srli";
            default:  return "addi";
        }
    }
}

int main() {
    CPU my_cpu;
    Memory my_mem;

    cpu_init(&my_cpu);
    memory_init(&my_mem);

    char user_input[50];
    char program_storage[MAX_LINES][50];
    const char* program_pointers[MAX_LINES];
    int total_lines = 0;
    uint64_t load_address = 0;

    printf("--- RISC-V 5-Stage Pipeline Emulator ---\n");
    printf("Enter instructions line by line. Type 'run' to execute.\n\n");

    printf("Instruction:\n");

    // CODE LOADER BUFFER INPUT
    while (total_lines < MAX_LINES) {
        if (fgets(user_input, sizeof(user_input), stdin) == NULL) break;

        user_input[strcspn(user_input, "\r\n")] = 0;

        if (strcmp(user_input, "run") == 0) break;
        if (strlen(user_input) == 0) continue;

        strcpy(program_storage[total_lines], user_input);
        program_pointers[total_lines] = program_storage[total_lines];
        total_lines++;
    }

    // TWO-PASS ASSEMBLER: PASS 1 (RESOLVE LABELS)
    assembler_pass1(program_pointers, total_lines);

    // TWO-PASS ASSEMBLER: PASS 2 (GENERATE MACHINE CODE)
    uint64_t assemble_pc = 0;
    for (int i = 0; i < total_lines; i++) {
        char check_label[50];
        if (sscanf(program_pointers[i], "%s", check_label) == 1) {
            if (check_label[strlen(check_label) - 1] == ':') {
                continue;
            }
        }

        uint32_t machine_code = assemble_line(program_pointers[i], assemble_pc);
        if (machine_code == 0) {
            printf("Error: Invalid assembly syntax at line %d.\n", i + 1);
            printf("\nPress Enter to exit...");
            fflush(stdout);
            getchar();
            return 1;
        }

        my_mem.data[load_address]     = machine_code & 0xFF;
        my_mem.data[load_address + 1] = (machine_code >> 8) & 0xFF;
        my_mem.data[load_address + 2] = (machine_code >> 16) & 0xFF;
        my_mem.data[load_address + 3] = (machine_code >> 24) & 0xFF;
        load_address += 4;
        assemble_pc += 4;
    }

    int clock_cycle = 1;
    int is_flushed = 0;
    int total_executed_instructions = 0;
    int total_stalled_cycles = 0;

    printf("\nStart running ...\n");

    while (my_cpu.pc < load_address || my_cpu.IF_ID.valid || my_cpu.ID_EX.valid || my_cpu.EX_MEM.valid || my_cpu.MEM_WB.valid) {

        IF_ID_Reg  next_IF_ID  = my_cpu.IF_ID;
        ID_EX_Reg  next_ID_EX  = my_cpu.ID_EX;
        EX_MEM_Reg next_EX_MEM = my_cpu.EX_MEM;
        MEM_WB_Reg next_MEM_WB = my_cpu.MEM_WB;

        is_flushed = 0;

        if (my_cpu.MEM_WB.valid) {
            // STAGE 5: WRITEBACK
            writeback_stage(my_cpu.MEM_WB.decoded_inst, my_cpu.MEM_WB.final_result, &my_cpu);
            total_executed_instructions++;

            uint32_t op = my_cpu.MEM_WB.decoded_inst.opcode;
            const char* name = get_inst_name(op, my_cpu.MEM_WB.decoded_inst.funct3);
            if (op == 0x33) {
                uint32_t f7 = (my_cpu.MEM_WB.decoded_inst.imm >> 5) & 0x7F;
                if (my_cpu.MEM_WB.decoded_inst.funct3 == 0x00 && f7 == 0x20) name = "sub";
                if (my_cpu.MEM_WB.decoded_inst.funct3 == 0x05 && f7 == 0x20) name = "sra";
                int rs2_reg = my_cpu.MEM_WB.decoded_inst.imm & 0x1F;
                printf("%d : writeback : [pc=%ld] %s x%d, x%d, x%d [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.MEM_WB.pc, name,
                       my_cpu.MEM_WB.decoded_inst.rd, my_cpu.MEM_WB.decoded_inst.rs1, rs2_reg,
                       name, my_cpu.MEM_WB.final_result, my_cpu.registers[my_cpu.MEM_WB.decoded_inst.rs1], my_cpu.registers[rs2_reg]);
            } else if (op == 0x63) {
                printf("%d : writeback : [pc=%ld] %s x%d, x%d, %ld [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.MEM_WB.pc, name,
                       my_cpu.MEM_WB.decoded_inst.rs1, my_cpu.MEM_WB.decoded_inst.rd, my_cpu.MEM_WB.decoded_inst.imm,
                       name, my_cpu.MEM_WB.final_result, my_cpu.registers[my_cpu.MEM_WB.decoded_inst.rs1], my_cpu.registers[my_cpu.MEM_WB.decoded_inst.rd]);
            } else if (op == 0x6F) {
                printf("%d : writeback : [pc=%ld] jal x%d, %ld [jal %ld]\n",
                       clock_cycle, my_cpu.MEM_WB.pc, my_cpu.MEM_WB.decoded_inst.rd, my_cpu.MEM_WB.decoded_inst.imm, my_cpu.MEM_WB.final_result);
            } else if (op == 0x67) {
                printf("%d : writeback : [pc=%ld] jalr x%d, x%d, %ld [jalr %ld]\n",
                       clock_cycle, my_cpu.MEM_WB.pc, my_cpu.MEM_WB.decoded_inst.rd, my_cpu.MEM_WB.decoded_inst.rs1, my_cpu.MEM_WB.decoded_inst.imm, my_cpu.MEM_WB.final_result);
            } else {
                printf("%d : writeback : [pc=%ld] %s x%d, x%d, %ld [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.MEM_WB.pc, name,
                       my_cpu.MEM_WB.decoded_inst.rd, my_cpu.MEM_WB.decoded_inst.rs1, my_cpu.MEM_WB.decoded_inst.imm,
                       name, my_cpu.MEM_WB.final_result, my_cpu.registers[my_cpu.MEM_WB.decoded_inst.rs1], my_cpu.registers[my_cpu.MEM_WB.decoded_inst.imm]);
            }

            next_MEM_WB.valid = 0;
        }

        if (my_cpu.EX_MEM.valid) {
            // STAGE 4: MEMORY ACCESS
            int64_t mem_out = memory_stage(my_cpu.EX_MEM.decoded_inst, my_cpu.EX_MEM.alu_result, &my_mem, &my_cpu);

            next_MEM_WB.decoded_inst = my_cpu.EX_MEM.decoded_inst;
            next_MEM_WB.final_result = mem_out;
            next_MEM_WB.pc = my_cpu.EX_MEM.pc;
            next_MEM_WB.valid = 1;

            uint32_t op = my_cpu.EX_MEM.decoded_inst.opcode;
            const char* name = get_inst_name(op, my_cpu.EX_MEM.decoded_inst.funct3);
            if (op == 0x33) {
                uint32_t f7 = (my_cpu.EX_MEM.decoded_inst.imm >> 5) & 0x7F;
                if (my_cpu.EX_MEM.decoded_inst.funct3 == 0x00 && f7 == 0x20) name = "sub";
                if (my_cpu.EX_MEM.decoded_inst.funct3 == 0x05 && f7 == 0x20) name = "sra";
                int rs2_reg = my_cpu.EX_MEM.decoded_inst.imm & 0x1F;
                printf("%d : memory    : [pc=%ld] %s x%d, x%d, x%d [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.EX_MEM.pc, name,
                       my_cpu.EX_MEM.decoded_inst.rd, my_cpu.EX_MEM.decoded_inst.rs1, rs2_reg,
                       name, mem_out, my_cpu.registers[my_cpu.EX_MEM.decoded_inst.rs1], my_cpu.registers[rs2_reg]);
            } else if (op == 0x63) {
                printf("%d : memory    : [pc=%ld] %s x%d, x%d, %ld [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.EX_MEM.pc, name,
                       my_cpu.EX_MEM.decoded_inst.rs1, my_cpu.EX_MEM.decoded_inst.rd, my_cpu.EX_MEM.decoded_inst.imm,
                       name, my_cpu.EX_MEM.alu_result, my_cpu.registers[my_cpu.EX_MEM.decoded_inst.rs1], my_cpu.registers[my_cpu.EX_MEM.decoded_inst.rd]);
            } else if (op == 0x6F) {
                printf("%d : memory    : [pc=%ld] jal x%d, %ld [jal %ld]\n",
                       clock_cycle, my_cpu.EX_MEM.pc, my_cpu.EX_MEM.decoded_inst.rd, my_cpu.EX_MEM.decoded_inst.imm, my_cpu.EX_MEM.alu_result);
            } else if (op == 0x67) {
                printf("%d : memory    : [pc=%ld] jalr x%d, x%d, %ld [jalr %ld]\n",
                       clock_cycle, my_cpu.EX_MEM.pc, my_cpu.EX_MEM.decoded_inst.rd, my_cpu.EX_MEM.decoded_inst.rs1, my_cpu.EX_MEM.decoded_inst.imm, my_cpu.EX_MEM.alu_result);
            } else {
                printf("%d : memory    : [pc=%ld] %s x%d, x%d, %ld [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.EX_MEM.pc, name,
                       my_cpu.EX_MEM.decoded_inst.rd, my_cpu.EX_MEM.decoded_inst.rs1, my_cpu.EX_MEM.decoded_inst.imm,
                       name, mem_out, my_cpu.registers[my_cpu.EX_MEM.decoded_inst.rs1], my_cpu.registers[my_cpu.EX_MEM.decoded_inst.imm]);
            }

            next_EX_MEM.valid = 0;
        }

        if (my_cpu.ID_EX.valid) {
            // STAGE 3: EXECUTE
            int rs1_reg = my_cpu.ID_EX.decoded_inst.rs1;
            int64_t forwarded_rs1 = my_cpu.registers[rs1_reg];

            uint32_t op = my_cpu.ID_EX.decoded_inst.opcode;
            int rs2_reg = 0;
            if (op == 0x33 || op == 0x63 || op == 0x23) {
                rs2_reg = (op == 0x63 || op == 0x23) ? my_cpu.ID_EX.decoded_inst.rd : (my_cpu.ID_EX.decoded_inst.imm & 0x1F);
            }
            int64_t forwarded_rs2 = my_cpu.registers[rs2_reg];

            // FORWARDING LOGIC FOR RS1 (INCLUDES JAL/JALR DEPENDENCY REDIRECTION)
            if (my_cpu.EX_MEM.valid && my_cpu.EX_MEM.decoded_inst.rd != 0 && my_cpu.EX_MEM.decoded_inst.rd == rs1_reg) {
                forwarded_rs1 = my_cpu.EX_MEM.alu_result;
            }
            else if (my_cpu.MEM_WB.valid && my_cpu.MEM_WB.decoded_inst.rd != 0 && my_cpu.MEM_WB.decoded_inst.rd == rs1_reg) {
                forwarded_rs1 = my_cpu.MEM_WB.final_result;
            }

            // FORWARDING LOGIC FOR RS2
            if (op == 0x33 || op == 0x63 || op == 0x23) {
                if (my_cpu.EX_MEM.valid && my_cpu.EX_MEM.decoded_inst.rd != 0 && my_cpu.EX_MEM.decoded_inst.rd == rs2_reg) {
                    forwarded_rs2 = my_cpu.EX_MEM.alu_result;
                }
                else if (my_cpu.MEM_WB.valid && my_cpu.MEM_WB.decoded_inst.rd != 0 && my_cpu.MEM_WB.decoded_inst.rd == rs2_reg) {
                    forwarded_rs2 = my_cpu.MEM_WB.final_result;
                }
            }

            int64_t backup_rs1 = my_cpu.registers[rs1_reg];
            int64_t backup_rs2 = my_cpu.registers[rs2_reg];

            my_cpu.registers[rs1_reg] = forwarded_rs1;
            if (op == 0x33 || op == 0x63 || op == 0x23) my_cpu.registers[rs2_reg] = forwarded_rs2;

            int64_t alu_out = execute_stage(my_cpu.ID_EX.decoded_inst, &my_cpu);

            my_cpu.registers[rs1_reg] = backup_rs1;
            my_cpu.registers[rs2_reg] = backup_rs2;

            next_EX_MEM.decoded_inst = my_cpu.ID_EX.decoded_inst;
            next_EX_MEM.alu_result = alu_out;
            next_EX_MEM.pc = my_cpu.ID_EX.pc;
            next_EX_MEM.valid = 1;

            // CONTROL HAZARD: FLUSH PIPELINE STAGES FOR BRANCH
            if (op == 0x63 && alu_out == 1) {
                my_cpu.pc = my_cpu.ID_EX.pc + my_cpu.ID_EX.decoded_inst.imm;
                next_IF_ID.valid = 0;
                next_ID_EX.valid = 0;
                is_flushed = 1;
                total_stalled_cycles += 2;
            }
            // CONTROL HAZARD: FLUSH PIPELINE STAGES FOR JAL
            else if (op == 0x6F) {
                my_cpu.pc = my_cpu.ID_EX.pc + my_cpu.ID_EX.decoded_inst.imm;
                next_IF_ID.valid = 0;
                next_ID_EX.valid = 0;
                is_flushed = 1;
                total_stalled_cycles += 2;
            }
            // CONTROL HAZARD: FLUSH PIPELINE STAGES FOR JALR
            else if (op == 0x67) {
                my_cpu.pc = forwarded_rs1 + my_cpu.ID_EX.decoded_inst.imm;
                next_IF_ID.valid = 0;
                next_ID_EX.valid = 0;
                is_flushed = 1;
                total_stalled_cycles += 2;
            }

            const char* name = get_inst_name(op, my_cpu.ID_EX.decoded_inst.funct3);
            if (op == 0x33) {
                uint32_t f7 = (my_cpu.ID_EX.decoded_inst.imm >> 5) & 0x7F;
                if (my_cpu.ID_EX.decoded_inst.funct3 == 0x00 && f7 == 0x20) name = "sub";
                if (my_cpu.ID_EX.decoded_inst.funct3 == 0x05 && f7 == 0x20) name = "sra";
                printf("%d : execute   : [pc=%ld] %s x%d, x%d, x%d [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.ID_EX.pc, name,
                       my_cpu.ID_EX.decoded_inst.rd, rs1_reg, rs2_reg,
                       name, alu_out, forwarded_rs1, forwarded_rs2);
            } else if (op == 0x63) {
                printf("%d : execute   : [pc=%ld] %s x%d, x%d, %ld [%s %ld, %ld, %ld] %s\n",
                       clock_cycle, my_cpu.ID_EX.pc, name,
                       rs1_reg, rs2_reg, my_cpu.ID_EX.decoded_inst.imm,
                       name, forwarded_rs1, forwarded_rs2, my_cpu.ID_EX.decoded_inst.imm,
                       (alu_out == 1) ? "-> TAKEN (FLUSH)" : "-> NOT TAKEN");
            } else if (op == 0x6F) {
                printf("%d : execute   : [pc=%ld] jal x%d, %ld [jal %ld]\n",
                       clock_cycle, my_cpu.ID_EX.pc, my_cpu.ID_EX.decoded_inst.rd, my_cpu.ID_EX.decoded_inst.imm, alu_out);
            } else if (op == 0x67) {
                printf("%d : execute   : [pc=%ld] jalr x%d, x%d, %ld [jalr %ld]\n",
                       clock_cycle, my_cpu.ID_EX.pc, my_cpu.ID_EX.decoded_inst.rd, rs1_reg, my_cpu.ID_EX.decoded_inst.imm, alu_out);
            } else {
                printf("%d : execute   : [pc=%ld] %s x%d, x%d, %ld [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.ID_EX.pc, name,
                       my_cpu.ID_EX.decoded_inst.rd, rs1_reg, my_cpu.ID_EX.decoded_inst.imm,
                       name, alu_out, forwarded_rs1, my_cpu.ID_EX.decoded_inst.imm);
            }

            if (!is_flushed) next_ID_EX.valid = 0;
        }

        if (my_cpu.IF_ID.valid && !is_flushed) {
            // STAGE 2: DECODE
            I_type decoded = decode_i_type(my_cpu.IF_ID.instruction);

            next_ID_EX.decoded_inst = decoded;
            next_ID_EX.pc = my_cpu.IF_ID.pc;
            next_ID_EX.valid = 1;

            uint32_t op = my_cpu.IF_ID.instruction & 0x7F;
            uint32_t f3 = (my_cpu.IF_ID.instruction >> 12) & 0x07;
            const char* name = get_inst_name(op, f3);

            if (op == 0x33) {
                uint32_t f7 = (my_cpu.IF_ID.instruction >> 25) & 0x7F;
                if (f3 == 0x00 && f7 == 0x20) name = "sub";
                if (f3 == 0x05 && f7 == 0x20) name = "sra";
                int rs2_reg = (my_cpu.IF_ID.instruction >> 20) & 0x1F;
                printf("%d : decode    : [pc=%ld] %s x%d, x%d, x%d [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.IF_ID.pc, name,
                       decoded.rd, decoded.rs1, rs2_reg,
                       name, my_cpu.registers[decoded.rd], my_cpu.registers[decoded.rs1], my_cpu.registers[rs2_reg]);
            } else if (op == 0x63) {
                printf("%d : decode    : [pc=%ld] %s x%d, x%d, %ld [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.IF_ID.pc, name,
                       decoded.rs1, decoded.rd, decoded.imm,
                       name, my_cpu.registers[decoded.rs1], my_cpu.registers[decoded.rd], decoded.imm);
            } else if (op == 0x6F) {
                printf("%d : decode    : [pc=%ld] jal x%d, %ld [jal 0]\n",
                       clock_cycle, my_cpu.IF_ID.pc, decoded.rd, decoded.imm);
            } else if (op == 0x67) {
                printf("%d : decode    : [pc=%ld] jalr x%d, x%d, %ld [jalr 0]\n",
                       clock_cycle, my_cpu.IF_ID.pc, decoded.rd, decoded.rs1, decoded.imm);
            } else {
                printf("%d : decode    : [pc=%ld] %s x%d, x%d, %ld [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.IF_ID.pc, name,
                       decoded.rd, decoded.rs1, decoded.imm,
                       name, my_cpu.registers[decoded.rd], my_cpu.registers[decoded.rs1], decoded.imm);
            }

            next_IF_ID.valid = 0;
        }

        if (my_cpu.pc < load_address && !is_flushed) {
            // STAGE 1: FETCH
            uint32_t b0 = my_mem.data[my_cpu.pc];
            uint32_t b1 = my_mem.data[my_cpu.pc + 1];
            uint32_t b2 = my_mem.data[my_cpu.pc + 2];
            uint32_t b3 = my_mem.data[my_cpu.pc + 3];
            uint32_t inst = (b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));

            next_IF_ID.instruction = inst;
            next_IF_ID.pc = my_cpu.pc;
            next_IF_ID.valid = 1;

            uint32_t op = inst & 0x7F;
            uint32_t f3 = (inst >> 12) & 0x07;
            const char* name = get_inst_name(op, f3);

            I_type f_decoded = decode_i_type(inst);

            if (op == 0x33) {
                uint32_t f7 = (inst >> 25) & 0x7F;
                if (f3 == 0x00 && f7 == 0x20) name = "sub";
                if (f3 == 0x05 && f7 == 0x20) name = "sra";
                int rs2_reg = (inst >> 20) & 0x1F;
                printf("%d : fetch     : [pc=%ld] %s x%d, x%d, x%d [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.pc, name,
                       f_decoded.rd, f_decoded.rs1, rs2_reg,
                       name, my_cpu.registers[f_decoded.rd], my_cpu.registers[f_decoded.rs1], my_cpu.registers[rs2_reg]);
            } else if (op == 0x63) {
                printf("%d : fetch     : [pc=%ld] %s x%d, x%d, %ld [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.pc, name,
                       f_decoded.rs1, f_decoded.rd, f_decoded.imm,
                       name, my_cpu.registers[f_decoded.rs1], my_cpu.registers[f_decoded.rd], f_decoded.imm);
            } else if (op == 0x6F) {
                printf("%d : fetch     : [pc=%ld] jal x%d, %ld [jal 0]\n",
                       clock_cycle, my_cpu.pc, f_decoded.rd, f_decoded.imm);
            } else if (op == 0x67) {
                printf("%d : fetch     : [pc=%ld] jalr x%d, x%d, %ld [jalr 0]\n",
                       clock_cycle, my_cpu.pc, f_decoded.rd, f_decoded.rs1, f_decoded.imm);
            } else {
                printf("%d : fetch     : [pc=%ld] %s x%d, x%d, %ld [%s %ld, %ld, %ld]\n",
                       clock_cycle, my_cpu.pc, name,
                       f_decoded.rd, f_decoded.rs1, f_decoded.imm,
                       name, my_cpu.registers[f_decoded.rd], my_cpu.registers[f_decoded.rs1], f_decoded.imm);
            }

            my_cpu.pc += 4;
        }

        my_cpu.IF_ID  = next_IF_ID;
        my_cpu.ID_EX  = next_ID_EX;
        my_cpu.EX_MEM = next_EX_MEM;
        my_cpu.MEM_WB = next_MEM_WB;

        clock_cycle++;
    }

    printf("Done.\n\n");

    // REPORT DETAILED KITE SIMULATOR FORMATTED STATISTICS
    printf("======== [Kite Pipeline Stats] =========\n");
    printf("Total number of clock cycles = %d\n", clock_cycle - 1);
    printf("Total number of stalled cycles = %d\n", total_stalled_cycles);
    printf("Total number of executed instructions = %d\n", total_executed_instructions);
    if (total_executed_instructions > 0) {
        printf("Cycles per instruction = %.3f\n", (float)(clock_cycle - 1) / total_executed_instructions);
    } else {
        printf("Cycles per instruction = 0.000\n");
    }
    printf("\nData cache stats:\n");
    printf("    Number of loads = 0\n");
    printf("    Number of stores = 0\n");
    printf("    Number of writebacks = 0\n");
    printf("    Miss rate = 0.000 (0/0)\n\n");

    printf("Register state:\n");
    for (int i = 0; i < REG_COUNT; i++) {
        printf("x%d = %ld\n", i, my_cpu.registers[i]);
    }
    printf("\nMemory state (only accessed addresses):\n\n");
    printf("======== [End of Pipeline Stats] =========\n");

    printf("\nSimulation finished. Press Enter to exit...");
    fflush(stdout);
    while (getchar() != '\n');
    getchar();

    return 0;
}
