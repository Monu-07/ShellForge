#ifndef HISTORY_H
#define HISTORY_H

#include "token.h"
#include "parser.h"

#define MAX_TOKENS 1024

typedef struct {
    Token tokens[MAX_TOKENS];
    int count;
} token_list_t;

typedef Pipeline pipeline_t;
typedef Command command_t;

void print_history(void);
void lexer(const char *input, token_list_t *list);
int parser(const token_list_t *tokens, pipeline_t *pipeline);
void expand_variables(pipeline_t *pipeline);
void token_print(const token_list_t *list);
void pipeline_print(const pipeline_t *pipeline);

#endif // HISTORY_H
