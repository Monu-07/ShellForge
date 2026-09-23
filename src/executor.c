#define _POSIX_C_SOURCE 200809L
#include "executor.h"
#include "builtin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

static void reap_zombies(void) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

static int setup_redirection(const command_t *cmd) {
    if (cmd->input_file != NULL) {
        int fd_in = open(cmd->input_file, O_RDONLY);
        if (fd_in < 0) {
            perror(cmd->input_file);
            return -1;
        }
        if (dup2(fd_in, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd_in);
            return -1;
        }
        close(fd_in);
    }

    if (cmd->output_file != NULL) {
        int flags = O_WRONLY | O_CREAT | (cmd->append_mode ? O_APPEND : O_TRUNC);
        int fd_out = open(cmd->output_file, flags, 0644);
        if (fd_out < 0) {
            perror(cmd->output_file);
            return -1;
        }
        if (dup2(fd_out, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd_out);
            return -1;
        }
        close(fd_out);
    }

    return 0;
}

static int execute_single_builtin(command_t *cmd) {
    int saved_stdin = -1;
    int saved_stdout = -1;

    if (cmd->input_file != NULL) {
        saved_stdin = dup(STDIN_FILENO);
        if (saved_stdin < 0) {
            perror("dup");
            return -1;
        }
        int fd_in = open(cmd->input_file, O_RDONLY);
        if (fd_in < 0) {
            perror(cmd->input_file);
            close(saved_stdin);
            return -1;
        }
        if (dup2(fd_in, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd_in);
            close(saved_stdin);
            return -1;
        }
        close(fd_in);
    }

    if (cmd->output_file != NULL) {
        saved_stdout = dup(STDOUT_FILENO);
        if (saved_stdout < 0) {
            perror("dup");
            if (saved_stdin != -1) {
                dup2(saved_stdin, STDIN_FILENO);
                close(saved_stdin);
            }
            return -1;
        }
        int flags = O_WRONLY | O_CREAT | (cmd->append_mode ? O_APPEND : O_TRUNC);
        int fd_out = open(cmd->output_file, flags, 0644);
        if (fd_out < 0) {
            perror(cmd->output_file);
            close(saved_stdout);
            if (saved_stdin != -1) {
                dup2(saved_stdin, STDIN_FILENO);
                close(saved_stdin);
            }
            return -1;
        }
        if (dup2(fd_out, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd_out);
            close(saved_stdout);
            if (saved_stdin != -1) {
                dup2(saved_stdin, STDIN_FILENO);
                close(saved_stdin);
            }
            return -1;
        }
        close(fd_out);
    }

    int result = execute_builtin(cmd);

    if (saved_stdin != -1) {
        if (dup2(saved_stdin, STDIN_FILENO) < 0) {
            perror("dup2 restore stdin");
        }
        close(saved_stdin);
    }

    if (saved_stdout != -1) {
        if (dup2(saved_stdout, STDOUT_FILENO) < 0) {
            perror("dup2 restore stdout");
        }
        close(saved_stdout);
    }

    return result;
}

static int execute_single_command(command_t *cmd) {
    if (cmd == NULL || cmd->argc == 0) {
        return 0;
    }

    // Built-in commands execute directly in the parent process
    if (is_builtin(cmd)) {
        return execute_single_builtin(cmd);
    }

    // External command: fork child, execvp, parent waitpid
    fflush(NULL);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
        // Child process
        if (setup_redirection(cmd) < 0) {
            exit(1);
        }
        execvp(cmd->argv[0], cmd->argv);
        perror(cmd->argv[0]);
        exit(127);
    } else {
        // Parent process: if background, print PID and return immediately
        if (cmd->background) {
            printf("[1] %d\n", pid);
            return 0;
        }
        int status = 0;
        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid");
            return -1;
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            return 128 + WTERMSIG(status);
        }
        return 0;
    }
}

int execute_command(command_t *cmd) {
    reap_zombies();
    return execute_single_command(cmd);
}

int execute_pipeline(pipeline_t *pipeline) {
    reap_zombies();

    if (pipeline == NULL || pipeline->command_count == 0) {
        return 0;
    }

    int N = pipeline->command_count;

    // Single command execution
    if (N == 1) {
        return execute_single_command(&pipeline->commands[0]);
    }

    // Multi-command pipeline: N >= 2 commands connected by pipes
    int pipes[MAX_COMMANDS][2];
    for (int i = 0; i < N - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return -1;
        }
    }

    pid_t pids[MAX_COMMANDS];

    fflush(NULL);
    for (int i = 0; i < N; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            for (int j = 0; j < N - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            for (int j = 0; j < i; j++) {
                waitpid(pids[j], NULL, 0);
            }
            return -1;
        }

        if (pids[i] == 0) {
            // Child process for command i
            command_t *cmd = &pipeline->commands[i];

            // Connect stdin from previous pipe if not the first command
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(1);
                }
            }

            // Connect stdout to next pipe if not the last command
            if (i < N - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(1);
                }
            }

            // Command-specific redirection overrides pipeline stdin/stdout if present
            if (setup_redirection(cmd) < 0) {
                exit(1);
            }

            // Close all pipe file descriptors in child
            for (int j = 0; j < N - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute command: builtin or external
            if (is_builtin(cmd)) {
                int ret = execute_builtin(cmd);
                exit(ret == 0 ? 0 : 1);
            } else {
                execvp(cmd->argv[0], cmd->argv);
                perror(cmd->argv[0]);
                exit(127);
            }
        }
    }

    // Parent process: close all pipe file descriptors immediately
    for (int i = 0; i < N - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // If pipeline is in the background, print PID and return immediately without waiting
    int is_bg = pipeline->commands[N - 1].background;
    if (is_bg) {
        printf("[1] %d\n", pids[N - 1]);
        return 0;
    }

    // Wait for all child processes and capture exit status of the last command
    int last_status = 0;
    for (int i = 0; i < N; i++) {
        int status = 0;
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
        }
        if (i == N - 1) {
            last_status = status;
        }
    }

    if (WIFEXITED(last_status)) {
        return WEXITSTATUS(last_status);
    } else if (WIFSIGNALED(last_status)) {
        return 128 + WTERMSIG(last_status);
    }

    return 0;
}