#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define APPEND 1
#define OVERWRITE 2
#define WITHOUT_OVERWRITE 3

int redirect_file_to_cmd(const char* file) {
    int fd = open(file, O_RDONLY);
    if (fd == -1) {
        perror("open");
        close(fd);
        return 1;
    }
    if (dup2(fd, STDIN_FILENO) == -1) {
        perror("dup2");
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}

int redirect_cmd_to_file(const char* file, int mode, int output) {
    int fd;
    switch (mode) {
        case APPEND:
            fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            break;
        case OVERWRITE:
            fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            break;
        case WITHOUT_OVERWRITE:
            fd = open(file, O_WRONLY | O_CREAT | O_EXCL, 0644);
            break;
        default:
            return 1;
    }
    if (fd == -1) {
        perror("open");
        close(fd);
        return 1;
    }
    if (output != STDOUT_FILENO && output != STDERR_FILENO) {
        fprintf(stderr, "Erreur : descripteur de sortie invalide.\n");
        close(fd);
        return 1;
    }
    if (dup2(fd, output) == -1) {
        perror("dup2");
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}