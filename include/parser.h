#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#define MAX_ARGS 128
#define MAX_COMMANDS 32

typedef struct {
    char *argv[MAX_ARGS];
    int argc;
    char *input_file;
    char *output_file;
    int append_mode;
    int background;
} Command;

typedef struct {
    Command commands[MAX_COMMANDS];
    int command_count;
} Pipeline;

Pipeline parse_pipeline(const char *input);
void free_pipeline(Pipeline *pipeline);

#endif // PARSER_H
