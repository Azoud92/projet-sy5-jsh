#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "commands.h"

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
        if (is_redirection(token) == 1 || strcmp(token, "|") == 0) {
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

int handle_redirections_for_pipelines(char *cmd, int first, int last) {
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

            int mode = -1;
            int output = -1;

            // On détermine le mode et le flux de la redirection
            if (strcmp(token, "<") == 0 && first == 1) { // entrée
                if (redirect_file_to_cmd(file) != 0) {
                    return 1;
                }
            }
            else if (strcmp(token, ">") == 0 && last == 1) {
                mode = WITHOUT_OVERWRITE;
                output = STDOUT_FILENO;
            }
            else if (strcmp(token, ">|") == 0 && last == 1) {
                mode = OVERWRITE;
                output = STDOUT_FILENO;
            }
            else if (strcmp(token, ">>") == 0 && last == 1) {
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

            if (mode == -1 || output == -1) {
                return 0;
            }

            if (redirect_cmd_to_file(file, mode, output) != 0) {
                return 1;
            }
        }
        token = strtok_r(NULL, " ", &saveptr);
    }
    return 0;
}

int handle_pipelines(char *cmd) {
    char *saveptr;

    // Permet de stocker la commande courante (sans les pipes)
    char *currentCmd = malloc(strlen(cmd) + 1);
    if (currentCmd == NULL) {
        perror("malloc");
        return 1;
    }
    currentCmd[0] = '\0';

    // Tube anonyme et enregistrement de la dernière entrée (entrée standard par défaut)
    int pipefd[2];
    int lastIn = STDIN_FILENO;

    // Pour savoir si on est sur la première commande, la dernière commande, ou aucune des deux
    int first = 1;
    int last = 0;

    // Le pid du fils courant
    pid_t pid;

    // On stocke le pid de chaque fils (commande) pour pouvoir les attendre à la fin
    pid_t *pids = malloc(sizeof(pid_t));
    if (pids == NULL) {
        perror("malloc");
        free(currentCmd);
        return 1;
    }
    int nb_childs = 0;

    char *token = strtok_r(cmd, " ", &saveptr);
    while (token != NULL) {
        //save_flows();
        if (strcmp("|", token) != 0) { // On est pas sur un pipeline donc on ajoute le token à la commande courante
            strcat(currentCmd, token);
            strcat(currentCmd, " ");
        }

        char *next_token = strtok_r(NULL, " ", &saveptr);
        last = (next_token == NULL);

        if (strcmp("|", token) == 0 || next_token == NULL) { // On est sur un pipeline ou sur la dernière commande
            if (next_token != NULL) {
                if (pipe(pipefd) < 0) {
                    perror("pipe");
                    free(currentCmd);
                    return 1;
                }
            }

            currentCmd[strlen(currentCmd) - 1] = '\0';

            // On gère les redirections pour la commande courante
            char *cmdCpy = strdup(currentCmd);            
            if (handle_redirections_for_pipelines(cmdCpy, first, last) != 0) {
                free(currentCmd);
                free(cmdCpy);
                return 1;
            }

            switch(pid = fork()) {
                case -1:
                    perror("fork");
                    free(currentCmd);
                    return 1;            
                case 0:
                    if (lastIn != STDIN_FILENO) {
                        dup2(lastIn, STDIN_FILENO);
                        close(lastIn);
                    }
                    if (next_token != NULL) {
                        dup2(pipefd[1], STDOUT_FILENO);
                        close(pipefd[1]);
                    }
                    close(pipefd[0]);

                    char *cmdClean = extract_command(currentCmd);
                    execute_command(cmdClean);
                    free(cmdClean);
                    exit(0);
                default:
                    if (lastIn != STDIN_FILENO) {
                        close(lastIn);
                    }
                    if (next_token != NULL) {
                        lastIn = pipefd[0];
                        close(pipefd[1]);
                    }
                    currentCmd[0] = '\0';
            }
            pids = realloc(pids, (nb_childs + 1) * sizeof(pid_t));
            pids[nb_childs++] = pid;
            free(cmdCpy);
            first = 0;
        }
        //restore_flows();
        token = next_token;
    }

    // On attend que le dernier fils se termine
    waitpid(pid, NULL, 0);

    free(pids);
    free(currentCmd);
    return 0;
}