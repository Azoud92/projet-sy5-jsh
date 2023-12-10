#ifndef REDIRECTIONS_H
#define REDIRECTIONS_H

#define APPEND 1
#define OVERWRITE 2
#define WITHOUT_OVERWRITE 3

int save_flows();
int restore_flows();
int redirect_file_to_cmd(const char* file);
int redirect_cmd_to_file(const char* file, int mode, int output);
char *extract_command(const char *cmd);
int handle_redirections(char *cmd);

#endif