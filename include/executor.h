#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

typedef Command command_t;
typedef Pipeline pipeline_t;

int execute_command(command_t *cmd);
int execute_pipeline(pipeline_t *pipeline);

#endif // EXECUTOR_H
