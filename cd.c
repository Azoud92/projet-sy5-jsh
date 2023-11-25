#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cd.h"

char *prevDir = NULL;

int cd(char *path) {
    char *home = getenv("HOME");
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return 1;  
    }

    if (path == NULL) {
        path = home;
    } else if (strcmp(path, "-") == 0) {
        if (prevDir) {
            path = prevDir;
        } else {
            fprintf(stderr, "cd: OLDPWD not set\n");
            return 1;
        }
    }

    if (chdir(path) != 0) {
        perror("cd");
        return 1;
    }

    free(prevDir);
    prevDir = strdup(cwd);

    return 0;
}