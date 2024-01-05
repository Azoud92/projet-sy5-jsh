#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "jobs.h"
#include "commands.h"

#define APPEND 1
#define OVERWRITE 2
#define WITHOUT_OVERWRITE 3

#define MAX_SUBST 1024
#define MAX_SUBST_FD 1024

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

int contains_pipeline (const char *cmd) {
    if (strstr(cmd, " | ") != NULL) {
        return 1;
    }
    return 0;
}

int contains_subst (const char *cmd) {
    if (strstr(cmd, " <( ") != NULL) {
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

    // Longueur finale de la commande
    while (token != NULL) {
        if (is_redirection(token) == 1 || strcmp(token, "|") == 0) {
            break;
        }
        command_len += strlen(token) + 1; // +1 pour l'espace
        token = strtok_r(NULL, " ", &saveptr);
    }
   
    char *command = malloc(command_len + 1); // +1 pour le \0

    if (!command) {
        free(tmp);
        perror("malloc");
        return NULL;
    }
    command[0] = '\0';

    // On réinitialise strtok_r
    free(tmp);
    tmp = strdup(cmd);
    token = strtok_r(tmp, " ", &saveptr);

    // On construit la commande
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

void set_mode_and_output_for_stderr(const char *token, int *mode, int *output) {
    if (strcmp(token, "2>") == 0) {
        *mode = WITHOUT_OVERWRITE;
        *output = STDERR_FILENO;
    }
    else if (strcmp(token, "2>|") == 0) {
        *mode = OVERWRITE;
        *output = STDERR_FILENO;
    }
    else if (strcmp(token, "2>>") == 0) {
        *mode = APPEND;
        *output = STDERR_FILENO;
    }
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
                else {
                    set_mode_and_output_for_stderr(token, &mode, &output);
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

            int mode;
            int output;

            // On détermine le mode et le flux de la redirection
            if (strcmp(token, "<") == 0 && first == 1) { // entrée
                if (redirect_file_to_cmd(file) != 0) {
                    return 1;
                }
            }
            else { // sortie
                if (strcmp(token, ">") == 0 && last == 1) {
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
                else {
                    set_mode_and_output_for_stderr(token, &mode, &output);
                }

                if (mode == -1 || output == -1) {
                    continue;
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

int handle_pipelines(char *cmd, bool isBg) {
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
        save_flows();
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
                case 0: // on exécute la commande
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
                    execute_command(cmdClean, isBg);
                    free(cmdClean);
                    exit(0);
                default: // on passe à la commande suivante
                    if (lastIn != STDIN_FILENO) {
                        close(lastIn);
                    }
                    if (next_token != NULL) {
                        lastIn = pipefd[0];
                        close(pipefd[1]);
                    }
                    else {
                        close(pipefd[0]);
                    }
                    currentCmd[0] = '\0';
            }
            // On ajoute le pid du fils courant au tableau des pids
            pids = realloc(pids, (nb_childs + 1) * sizeof(pid_t));
            pids[nb_childs++] = pid;
            free(cmdCpy);
            first = 0;
        }
        restore_flows();
        token = next_token;
    }

    // On attend que le dernier fils se termine
    if (!isBg) {
        for (int i = 0; i < nb_childs; ++i) {
            waitpid(pids[i], NULL, 0);
        }
    }

    free(pids);
    free(currentCmd);
    return 0;
}

int proc_subst (const char *currentCmd) {
    int pipefd[2];

    if (pipe(pipefd) < 0) {
        perror("pipe");
        return -1;
    }

    switch (fork()) {
        case -1:
            perror("fork");
            return -1;
        case 0:
            close(pipefd[0]);
            if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
                perror("dup2");
                return -1;
            }
            close(pipefd[1]);

            char *cmdCopy = strdup(currentCmd);
            if (cmdCopy == NULL) {
                perror("strdup");
                return -1;
            }

            if (strstr(cmdCopy, " | ") != NULL) { // On est sur un pipeline
                handle_pipelines(cmdCopy, false);
            }
            else { // On est sur une commande classique
                save_flows();
                char *cmdClean = extract_command(currentCmd);
                handle_redirections(cmdCopy); // On gère les redirections éventuelles avant l'exécution
                execute_command(cmdClean, false);
                restore_flows();
            }
            free(cmdCopy);
            exit(0);
        default:
            close(pipefd[1]);
            wait(NULL);
            return pipefd[0]; // On retourne le descripteur de fichier du tube anonyme pour pouvoir le lire et le fermer ensuite
    }
    return -1;
}

int handle_proc_subst(char *cmdLine) {
    char *saveptr;
    char *token = strtok_r(cmdLine, " ", &saveptr);

    char *commandStack[MAX_SUBST];
    int stack_index = -1;
    size_t capacity = 1024;

    char *currentCmd = malloc(capacity);
    currentCmd[0] = '\0';

    int fds[MAX_SUBST_FD];
    for (int i = 0; i < 10; ++i) {
        fds[i] = -1;
    }
    int subst_handled = 0;

    while (token != NULL) {
        if (strcmp(token, "<(") == 0) { // On aborde une substitution            
            commandStack[++stack_index] = currentCmd; // On empile la commande en cours
            currentCmd = malloc(capacity); // On alloue une nouvelle commande
            currentCmd[0] = '\0';
        } else if (strcmp(token, ")") == 0) { // On termine une substitution : on exécute traite la commande en cours et on dépile

            int fd = proc_subst(currentCmd);
            free(currentCmd);
            
            if (fd < 0) {
                for (int i = 0; i <= stack_index; i++) {
                    free(commandStack[i]);
                }
                return -1;
            }

            fds[subst_handled++] = fd; // On stocke le descripteur pour pouvoir fermer le fichier quand on en a plus besoin

            char fdPath[20];
            sprintf(fdPath, "/dev/fd/%d ", fd); // On construit le chemin du descripteur de fichier lié au tube anonyme
            currentCmd = commandStack[stack_index--]; // On dépile la commande en cours

            // On ajoute le chemin du descripteur de fichier à la commande en cours
            size_t totalSize = strlen(currentCmd) + strlen(fdPath) + 1;
            if (totalSize > 1024) {
                capacity = totalSize + 1024;
                char *tmp = realloc(currentCmd, capacity);
                if (!tmp) {
                    perror("realloc");
                    free(currentCmd);
                    return -1;
                }
                currentCmd = tmp;
            }
            strcat(currentCmd, fdPath);
        } else { // On est sur un token classique
            // On ajoute le token à la commande en cours (commande, argument, symbole de redirection, ...)
            size_t totalSize = strlen(currentCmd) + strlen(token) + 2;
            if (totalSize > capacity) {
                capacity = totalSize + 1024;
                char *tmp = realloc(currentCmd, capacity);
                if (!tmp) {
                    perror("realloc");
                    free(currentCmd);
                    return -1;
                }
                currentCmd = tmp;
            }
            strcat(currentCmd, token);
            strcat(currentCmd, " ");
        }
        token = strtok_r(NULL, " ", &saveptr);
    }

    // On exécute la commande finale
    if (strstr(currentCmd, " | ") != NULL) { // On est sur un pipeline
        handle_pipelines(currentCmd, false);
    }
    else { // On est sur une commande classique
        save_flows();
        char *cmdClean = extract_command(currentCmd);
        handle_redirections(currentCmd); // On gère les redirections éventuelles avant l'exécution
        execute_command(cmdClean, false);
        restore_flows();
        free(cmdClean);
    }

    // On ferme tous les descripteurs de fichier ouverts
    for (int i = 0; i <= subst_handled; ++i) {
        if (fds[i] != -1) {
            close(fds[i]);
        }        
    }

    free(currentCmd);
    return 0;
}