#include "parser.h"
#include "expand.h"
#include <stdlib.h>
#include <string.h>

Pipeline parse_pipeline(const char *input) {
    Pipeline pipeline;
    pipeline.command_count = 0;
    
    // Initialize first command
    Command *cmd = &pipeline.commands[0];
    cmd->argc = 0;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_mode = 0;
    cmd->background = 0;
    pipeline.command_count = 1;

    Lexer lexer;
    lexer_init(&lexer, input);
    
    Token token;
    
    while (1) {
        token = lexer_next_token(&lexer);
        if (token.type == TOKEN_END || token.type == TOKEN_ERROR) {
            free_token(&token);
            break;
        }
        
        if (token.type == TOKEN_PIPE) {
            // Finish current command, start next
            if (pipeline.command_count >= MAX_COMMANDS) {
                free_token(&token);
                break;
            }
            cmd = &pipeline.commands[pipeline.command_count++];
            cmd->argc = 0;
            cmd->input_file = NULL;
            cmd->output_file = NULL;
            cmd->append_mode = 0;
            cmd->background = 0;
        } 
        else if (token.type == TOKEN_REDIRECT_IN) {
            Token file_token = lexer_next_token(&lexer);
            if (file_token.type == TOKEN_WORD) {
                cmd->input_file = expand_variables(file_token.value);
            }
            free_token(&file_token);
        } 
        else if (token.type == TOKEN_REDIRECT_OUT) {
            Token file_token = lexer_next_token(&lexer);
            if (file_token.type == TOKEN_WORD) {
                cmd->output_file = expand_variables(file_token.value);
                cmd->append_mode = 0;
            }
            free_token(&file_token);
        } 
        else if (token.type == TOKEN_REDIRECT_APPEND) {
            Token file_token = lexer_next_token(&lexer);
            if (file_token.type == TOKEN_WORD) {
                cmd->output_file = expand_variables(file_token.value);
                cmd->append_mode = 1;
            }
            free_token(&file_token);
        } 
        else if (token.type == TOKEN_AMPERSAND) {
            cmd->background = 1;
        } 
        else if (token.type == TOKEN_WORD) {
            if (cmd->argc < MAX_ARGS - 1) {
                cmd->argv[cmd->argc++] = expand_variables(token.value);
            }
        }
        
        free_token(&token);
    }
    
    // Set NULL terminator for argv
    for (int i = 0; i < pipeline.command_count; i++) {
        pipeline.commands[i].argv[pipeline.commands[i].argc] = NULL;
    }
    
    // Propagate background flag from the last command to all commands in pipeline
    if (pipeline.command_count > 0) {
        int bg = pipeline.commands[pipeline.command_count - 1].background;
        for (int i = 0; i < pipeline.command_count; i++) {
            pipeline.commands[i].background = bg;
        }
    }
    
    return pipeline;
}

void free_pipeline(Pipeline *pipeline) {
    for (int i = 0; i < pipeline->command_count; i++) {
        Command *cmd = &pipeline->commands[i];
        for (int j = 0; j < cmd->argc; j++) {
            free(cmd->argv[j]);
            cmd->argv[j] = NULL;
        }
        if (cmd->input_file) {
            free(cmd->input_file);
            cmd->input_file = NULL;
        }
        if (cmd->output_file) {
            free(cmd->output_file);
            cmd->output_file = NULL;
        }
    }
    pipeline->command_count = 0;
}
