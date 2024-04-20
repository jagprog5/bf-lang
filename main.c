#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void exit_errno(const char* message) {
    fputs("err ", stderr);
    perror(message);
    exit(1);
}

void exit_err_msg(const char* message) {
    fputs("err ", stderr);
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main(int argc, char** argv) {
    if (argc == 0) { // should never happen
        exit_err_msg("no args");
        return 1;
    }

    // incorrect number of arguments OR the one arg is -h or --help
    if (argc != 2 || (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        printf("Usage: \n"
             "\t%s (-h | --help)\n"
             "\t%s <source file path>\n", argv[0], argv[0]);
        return 0;
    }

    char* instructions;
    size_t instructions_capacity;
    size_t instructions_index = 0;

    {
        FILE* fp = fopen(argv[1], "r");
        if (!fp) {
            exit_errno("file read");
        }

        // how large is the file?
        if (fseek(fp, 0, SEEK_END)) {
            fclose(fp);
            exit_errno("fseek");
        }

        {
            long instructions_capacity_raw = ftell(fp);
            if (instructions_capacity_raw < 0) {
                fclose(fp);
                exit_errno("ftell");
            }

            instructions_capacity = instructions_capacity_raw;
            if (instructions_capacity != instructions_capacity_raw) { // guard against narrowing
                fclose(fp);
                exit_err_msg("file too large");
            }
        }

        if (fseek(fp, 0, SEEK_SET)) {
            fclose(fp);
            exit_errno("fseek");
        }

        instructions = malloc(instructions_capacity);
        if (!instructions) {
            fclose(fp);
            exit_err_msg("mem");
        }

        if (fread(instructions, 1, instructions_capacity, fp) != instructions_capacity) {
            fclose(fp);
            exit_err_msg("fread");
        }

        fclose(fp);
    }

    size_t memory_index = 0; // offset within memory
    size_t memory_capacity = 1;
    char* memory = calloc(memory_capacity, 1);
    if (!memory) {
        free(instructions);
        exit_err_msg("mem");
    }

    while (instructions_index < instructions_capacity) {
        switch (instructions[instructions_index++]) {
            case '>':
                ++memory_index;
                if (memory_index == memory_capacity) {
                    size_t old_memory_capacity = memory_capacity;
                    {
                        size_t new_memory_capacity = memory_capacity * 2;
                        if (new_memory_capacity < memory_capacity) {
                            free(instructions);
                            free(memory);
                            exit_err_msg("mem");
                        }
                        memory_capacity = new_memory_capacity;
                    }
                    memory = realloc(memory, memory_capacity);
                    if (!memory) {
                        free(instructions);
                        exit_err_msg("mem");
                    }

                    // set the expanded range to zero
                    memset(memory + old_memory_capacity, 0, old_memory_capacity);
                }
                break;
            case '<':
                if (memory_index == 0) {
                    // expand then shift memory
                    size_t old_memory_capacity = memory_capacity;
                    {
                        size_t new_memory_capacity = memory_capacity * 2;
                        if (new_memory_capacity < memory_capacity) {
                            free(instructions);
                            free(memory);
                            exit_err_msg("mem");
                        }
                        memory_capacity = new_memory_capacity;
                    }
                    memory = realloc(memory, memory_capacity);
                    if (!memory) {
                        free(memory);
                        exit_err_msg("mem");
                    }

                    memcpy(memory + old_memory_capacity, memory, old_memory_capacity);
                    memset(memory, 0, old_memory_capacity);
                    memory_index += old_memory_capacity;
                }
                --memory_index;
                break;
            case '+':
                ++memory[memory_index];
                break;
            case '-':
                --memory[memory_index];
                break;
            case '.':
                if (putchar(memory[memory_index]) == EOF) {
                    free(instructions);
                    free(memory);
                    exit_err_msg("output");
                }
                break;
            case ',':
                {
                    int ch = getchar();
                    if (ch == EOF) {
                        free(instructions);
                        free(memory);
                        exit_err_msg("input");
                    }
                    memory[memory_index] = ch;
                }
                break;
            case '[':
                if (!memory[memory_index]) {
                    // jump forward to corresponding ']'
                    unsigned long nesting_level = 0;
                    while (1) {
                        if (instructions_index >= instructions_capacity) {
                            free(instructions);
                            free(memory);
                            exit_err_msg("unmatched [");
                        }
                        switch (instructions[instructions_index++]) {
                            default:
                                break;
                            case '[':
                                nesting_level += 1;
                                break;
                            case ']':
                                if (nesting_level == 0) {
                                    goto done_jumping_forward;
                                }
                                nesting_level -= 1;
                            break;
                        }
                    }
                }
                done_jumping_forward:
                break;
            case ']':
                if (memory[memory_index]) {
                    --instructions_index; // reverse above increment. points at the ']'
                    // jump backwards to corresponding '['
                    unsigned long nesting_level = 0;
                    while (1) {
                        if (instructions_index == 0) {
                            free(instructions);
                            free(memory);
                            exit_err_msg("unmatched ]");
                        }
                        switch (instructions[--instructions_index]) {
                            default:
                                break;
                            case ']':
                                nesting_level += 1;
                                break;
                            case '[':
                                if (nesting_level == 0) {
                                    ++instructions_index;
                                    goto done_jumping_backward;
                                }
                                nesting_level -= 1;
                        }
                    }
                }
                done_jumping_backward:
                break;
            default:
                break; // ignore unknown instructions
        }
    }
}
