#ifndef REDIRECTIONS_H
#define REDIRECTIONS_H

#define APPEND 1
#define OVERWRITE 2
#define WITHOUT_OVERWRITE 3

int redirect_file_to_cmd(const char* file);
int redirect_cmd_to_file(const char* file, int mode, int output);

#endif