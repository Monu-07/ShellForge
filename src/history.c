#define _POSIX_C_SOURCE 200809L
#include "history.h"
#include "expand.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/history.h>

#ifdef _MSC_VER
#define strdup _strdup
#endif

void print_history(void) {
    printf("------ Command History ------\n");
    HIST_ENTRY **list = history_list();
    if (list) {
        for (int i = 0; list[i] != NULL; i++) {
            printf("%2d  %s\n", i + 1, list[i]->line);
        }
    }
    printf("-----------------------------\n");
}

void lexer(const char *input, token_list_t *list) {
    Lexer lex;
    lexer_init(&lex, input);
    list->count = 0;
    while (list->count < MAX_TOKENS) {
        Token t = lexer_next_token(&lex);
        list->tokens[list->count++] = t;
        if (t.type == TOKEN_END) {
            break;
        }
    }
}

int parser(const token_list_t *token_list, pipeline_t *pipeline) {
    pipeline->command_count = 0;
    if (token_list->count == 0) return 0;
    
    Command *cmd = &pipeline->commands[0];
    cmd->argc = 0;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_mode = 0;
    cmd->background = 0;
    pipeline->command_count = 1;
    
    for (int i = 0; i < token_list->count; i++) {
        Token token = token_list->tokens[i];
        if (token.type == TOKEN_END || token.type == TOKEN_ERROR) {
            break;
        }
        
        if (token.type == TOKEN_PIPE) {
            if (pipeline->command_count >= MAX_COMMANDS) {
                break;
            }
            cmd = &pipeline->commands[pipeline->command_count++];
            cmd->argc = 0;
            cmd->input_file = NULL;
            cmd->output_file = NULL;
            cmd->append_mode = 0;
            cmd->background = 0;
        } 
        else if (token.type == TOKEN_REDIRECT_IN) {
            i++;
            if (i < token_list->count && token_list->tokens[i].type == TOKEN_WORD) {
                cmd->input_file = strdup(token_list->tokens[i].value);
            }
        } 
        else if (token.type == TOKEN_REDIRECT_OUT) {
            i++;
            if (i < token_list->count && token_list->tokens[i].type == TOKEN_WORD) {
                cmd->output_file = strdup(token_list->tokens[i].value);
                cmd->append_mode = 0;
            }
        } 
        else if (token.type == TOKEN_REDIRECT_APPEND) {
            i++;
            if (i < token_list->count && token_list->tokens[i].type == TOKEN_WORD) {
                cmd->output_file = strdup(token_list->tokens[i].value);
                cmd->append_mode = 1;
            }
        } 
        else if (token.type == TOKEN_AMPERSAND) {
            cmd->background = 1;
        } 
        else if (token.type == TOKEN_WORD) {
            if (cmd->argc < MAX_ARGS - 1) {
                cmd->argv[cmd->argc++] = strdup(token.value);
            }
        }
    }
    
    for (int i = 0; i < pipeline->command_count; i++) {
        pipeline->commands[i].argv[pipeline->commands[i].argc] = NULL;
    }
    
    if (pipeline->command_count > 0) {
        int bg = pipeline->commands[pipeline->command_count - 1].background;
        for (int i = 0; i < pipeline->command_count; i++) {
            pipeline->commands[i].background = bg;
        }
    }
    
    return 1;
}

void expand_variables(pipeline_t *pipeline) {
    for (int i = 0; i < pipeline->command_count; i++) {
        Command *cmd = &pipeline->commands[i];
        for (int j = 0; j < cmd->argc; j++) {
            char *expanded = expand_word(cmd->argv[j]);
            free(cmd->argv[j]);
            cmd->argv[j] = expanded;
        }
        if (cmd->input_file) {
            char *expanded = expand_word(cmd->input_file);
            free(cmd->input_file);
            cmd->input_file = expanded;
        }
        if (cmd->output_file) {
            char *expanded = expand_word(cmd->output_file);
            free(cmd->output_file);
            cmd->output_file = expanded;
        }
    }
}

void token_print(const token_list_t *list) {
    printf("\n---------------- TOKENS ----------------\n");
    for (int i = 0; i < list->count; i++) {
        printf(" %d : %-12s %s\n", i, token_type_to_string(list->tokens[i].type), list->tokens[i].value ? list->tokens[i].value : "");
    }
    printf("----------------------------------------\n");
}

void pipeline_print(const pipeline_t *pipeline) {
    printf("\n========== PIPELINE ==========\n\n");
    for (int c_idx = 0; c_idx < pipeline->command_count; c_idx++) {
        const Command *cmd = &pipeline->commands[c_idx];
        printf("Command %d\n", c_idx + 1);
        printf("------------------------------\n");
        printf("Arguments\n");
        for (int a_idx = 0; a_idx < cmd->argc; a_idx++) {
            printf("argv[%d] = %s\n", a_idx, cmd->argv[a_idx]);
        }
        printf("%-12s : %s\n", "Input", cmd->input_file ? cmd->input_file : "None");
        printf("%-12s : %s\n", "Output", cmd->output_file ? cmd->output_file : "None");
        printf("%-12s : %s\n", "Append", cmd->append_mode ? "Yes" : "No");
        printf("%-12s : %s\n", "Background", cmd->background ? "Yes" : "No");
        if (c_idx < pipeline->command_count - 1) {
            printf("\n");
        }
    }
    printf("==============================\n");
}
