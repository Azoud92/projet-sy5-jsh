#include <stdbool.h>
#ifndef EXTERNAL_COMMANDS_H
#define EXTERNAL_COMMANDS_H

int execute_external_command(char *cmd, char *cmdLine, bool isPipeBg);

#endif