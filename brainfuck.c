#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2 || (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        puts("This is a simple brainfuck interpreter!\n"
             "Usage:\n"
             "\tbrainfuck (-h | --help)\n"
             "\tbrainfuck <source file path>");
        return 0;
    }
    FILE* fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "The source file: \"%s\" could not be opened for reading.\n", argv[1]);
        return 1;
    }

    // a dynamically resizing array
    unsigned long data_length = 1;
    char* data_start = calloc(data_length, sizeof(*data_start));
    char* data_position = data_start;

    char c;
    while ((c = getc(fp)) != EOF) {
        switch (c) {
            case '>':
                if (++data_position - data_start == data_length) {
                    char* new_data = realloc(data_start, data_length * 2 * sizeof(*data_start));
                    if (!new_data) {
                        fprintf(stderr, "Memory failure.\n");
                        return 2;
                    }
                    data_position = new_data + (data_position - data_start);
                    data_start = new_data;
                    memset(data_position, 0, data_length);
                    data_length *= 2;
                }
            break;
            case '<':
                if (data_position - data_start == 0) {
                    char* new_data = realloc(data_start, data_length * 2);
                    if (!new_data) {
                        fprintf(stderr, "Memory failure.\n");
                        return 2;
                    }
                    memmove(new_data + data_length, new_data, data_length * sizeof(*data_start));
                    memset(new_data, 0, data_length * sizeof(*data_start));
                    data_position = new_data + data_length + (data_position - data_start);
                    data_start = new_data;
                    data_length *= 2;
                }
                --data_position;
            break;
            case '+':
                ++*data_position;
            break;
            case '-':
                --*data_position;
            break;
            case '.':
                putchar(*data_position);
            break;
            case ',':
                c = getchar();
                *data_position = c == EOF ? 0 : c;
            break;
            case '[':
                {
                    unsigned long forward_loop_level = 1;
                    if (!*data_position) {
                        while (1) {
                            c = getc(fp);
                            if (c == EOF) {
                                fprintf(stderr, "Missing ']'!\n");
                                return 3;
                            }
                            if (c == '[') {
                                forward_loop_level += 1;
                            } else if (c == ']') {
                                forward_loop_level -= 1;
                                if (forward_loop_level == 0) {
                                    break;
                                }
                            }
                        }
                    }
                }
            break;
            case ']':
                {
                    unsigned long backward_loop_level = 1;
                    if (*data_position) {
                        do {
                            if (fseek(fp, -2L, SEEK_CUR)) {
                                fprintf(stderr, "Missing '['!\n");
                                return 4;
                            }
                            c = getc(fp);
                            if (c == '[') {
                                backward_loop_level -= 1;
                                if (backward_loop_level == 0) {
                                    fseek(fp, -1L, SEEK_CUR); 
                                    break;
                                }
                            } else if (c == ']') {
                                backward_loop_level += 1;
                            }
                        } while (1);
                    }
                }
            break;
        }
    }
    // fclose(fp); // no need
    return 0;
}