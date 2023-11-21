#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *prev_dir = NULL;

int cd(char *path) {
    char *home = getenv("HOME");
    char cwd[1024];

    if (path == NULL) {
        path = home;
    } else if (strcmp(path, "-") == 0) {
        if (prev_dir) {
            path = prev_dir;
        } else {
            fprintf(stderr, "cd: OLDPWD not set\n");
            return -1;
        }
    }

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        free(prev_dir);
        prev_dir = strdup(cwd);
    }

    if (chdir(path) != 0) {
        perror("cd");
        return -1;
    }

    return 0;
}