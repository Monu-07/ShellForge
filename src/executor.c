#define _POSIX_C_SOURCE 200809L
#include "executor.h"
#include "builtin.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int execute_command(command_t *cmd) {
    if (cmd == NULL || cmd->argc == 0) {
        return 0;
    }
    
    // Check if it is a built-in command
    if (is_builtin(cmd)) {
        return execute_builtin(cmd);
    }
    
    // Execute external command using fork and execvp
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
        // Child process
        execvp(cmd->argv[0], cmd->argv);
        perror(cmd->argv[0]);
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
    
    return 0;
}
