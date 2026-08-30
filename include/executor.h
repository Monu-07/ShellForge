#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

typedef Command command_t;

int execute_command(command_t *cmd);

#endif // EXECUTOR_H
