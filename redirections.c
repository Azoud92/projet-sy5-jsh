#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define APPEND 1
#define OVERWRITE 2
#define WITHOUT_OVERWRITE 3

int stdin_copy;
int stdout_copy;
int stderr_copy;

// On sauvegarde les différents flux d'entrée / sortie pour ne pas les perdre après une redirection
int save_flows() {
    stdin_copy = dup(STDIN_FILENO);
    stdout_copy = dup(STDOUT_FILENO);
    stderr_copy = dup(STDERR_FILENO);
    if (stdin_copy == -1 || stdout_copy == -1 || stderr_copy == -1) {
        perror("dup");
        return 1;
    }
    return 0;
}

// On restaure les flux d'entrée / sortie
int restore_flows() {
    if (dup2(stdin_copy, STDIN_FILENO) == -1) {
        perror("dup2");
        return 1;
    }
    if (dup2(stdout_copy, STDOUT_FILENO) == -1) {
        perror("dup2");
        return 1;
    }
    if (dup2(stderr_copy, STDERR_FILENO) == -1) {
        perror("dup2");
        return 1;
    }
    return 0;
}

// Redirection d'entrée
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

// Redirection de sortie
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

int is_redirection (const char *token) {
    if (strcmp(token, "<") == 0 || strcmp(token, ">") == 0 || strcmp(token, ">|") == 0 ||
        strcmp(token, ">>") == 0 || strcmp(token, "2>") == 0 || strcmp(token, "2>|") == 0 ||
        strcmp(token, "2>>") == 0) {
        return 1;
    }
    return 0;
}

// On extrait la commande et ses arguments de la ligne de commande
char *extract_command(const char *cmd) {
    char *saveptr;
    char *tmp = strdup(cmd);
    char *token = strtok_r(tmp, " ", &saveptr);
    int command_len = 0;

    // Calculate the length of the final command
    while (token != NULL) {
        if (is_redirection(token) == 1) {
            break;
        }
        command_len += strlen(token) + 1; // +1 for the space
        token = strtok_r(NULL, " ", &saveptr);
    }

    // Allocate memory for the command
    char *command = malloc(command_len + 1); // +1 for the null terminator

    if (!command) {
        free(tmp);
        perror("malloc");
        return NULL;
    }
    command[0] = '\0';

    // Reset strtok_r
    free(tmp);
    tmp = strdup(cmd);
    token = strtok_r(tmp, " ", &saveptr);

    // Build the command
    while (token != NULL) {
        if (is_redirection(token) == 1) {
            break;
        }
        strcat(command, token);
        strcat(command, " ");
        token = strtok_r(NULL, " ", &saveptr);
    }

    free(tmp);
    return command;
}

int handle_redirections(char *cmd) {
    char *saveptr;
    char *token = strtok_r(cmd, " ", &saveptr);

    while (token != NULL) {
        // On est sur une redirection
        if (is_redirection(token) == 1) {
            
            char *file = strtok_r(NULL, " ", &saveptr);
            if (file == NULL) {
                fprintf(stderr, "Erreur : aucun fichier spécifié pour la redirection.\n");
                return 1;
            }

            int mode;
            int output;

            // On détermine le mode et le flux de la redirection
            if (strcmp(token, "<") == 0) { // entrée
                if (redirect_file_to_cmd(file) != 0) {
                    return 1;
                }
            }
            else { // sortie
                if (strcmp(token, ">") == 0) {
                    mode = WITHOUT_OVERWRITE;
                    output = STDOUT_FILENO;
                }
                else if (strcmp(token, ">|") == 0) {
                    mode = OVERWRITE;
                    output = STDOUT_FILENO;
                }
                else if (strcmp(token, ">>") == 0) {
                    mode = APPEND;
                    output = STDOUT_FILENO;
                }
                else if (strcmp(token, "2>") == 0) {
                    mode = WITHOUT_OVERWRITE;
                    output = STDERR_FILENO;
                }
                else if (strcmp(token, "2>|") == 0) {
                    mode = OVERWRITE;
                    output = STDERR_FILENO;
                }
                else if (strcmp(token, "2>>") == 0) {
                    mode = APPEND;
                    output = STDERR_FILENO;
                }

                if (redirect_cmd_to_file(file, mode, output) != 0) {
                    return 1;
                }
            }            
        }
        token = strtok_r(NULL, " ", &saveptr);
    }
    return 0;
}