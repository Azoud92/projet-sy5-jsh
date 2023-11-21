#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include "cd.h"
#include "pwd.h"
#include "exit.h"
#define GREEN_COLOR "\001\033[32m\002"
#define YELLOW_COLOR "\001\033[33m\002"
#define NORMAL_COLOR "\001\033[00m\002"

char *get_prompt(int trunc, int nbJobs) {
    char *prompt = malloc(trunc + 1 + strlen(GREEN_COLOR) + strlen(YELLOW_COLOR) + strlen(NORMAL_COLOR));

    char jobsPrompt[30];
    snprintf(jobsPrompt, sizeof(jobsPrompt), "%s[%d]%s", GREEN_COLOR, nbJobs, YELLOW_COLOR);

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        free(prompt);
        exit(EXIT_FAILURE);
    }
    printf("%s\n", cwd);

    int apparent_length = strlen(jobsPrompt) - (strlen(GREEN_COLOR) + strlen(YELLOW_COLOR)) + 2; // + 2 car on a le "$ " qui suit
    if (strlen(cwd) > trunc - apparent_length) {
        int decalage = trunc - apparent_length - 3; // - 3 pour les "..."

        memcpy(cwd, "...", 3);
        if (decalage < 0) {
            decalage = 0;
        }
        else {
            memmove(cwd + 3, cwd + strlen(cwd) - decalage, decalage);
        }
        cwd[decalage + 3] = '\0';
    }

    snprintf(prompt, trunc + 1 + strlen(GREEN_COLOR) + strlen(YELLOW_COLOR) + strlen(NORMAL_COLOR), "%s%s%s$ ",
             jobsPrompt, cwd, NORMAL_COLOR);

    return prompt;
}

int main() {
    while (1) {
        char *prompt = get_prompt(30, 0);
        char *cmdLine = readline(prompt); // provisoire car pas de gestion des jobs pour l'instant
        if (cmdLine == NULL) {
            break;
            }
        add_history(cmdLine);

        char *cmd = strtok(cmdLine, " ");
        if (cmd != NULL){
            if (strcmp(cmd, "cd") == 0) { // Vérifie si la commande est cd 
                char *path = strtok(NULL, " ");
                char *extraArg = strtok(NULL, " ");

                if (extraArg != NULL) { // Vérifie si la commande cd est utilisée avec trop d'arguments
                    fprintf(stderr, "cd: too many arguments\n");
                    free(path);
                    free(extraArg);
                    continue;
                }
                if (cd(path) != 0) { // Vérifie si le changement de répertoire s'est bien passé
                    fprintf(stderr, "Erreur lors du changement de répertoire vers '%s'\n", path);
                    free(path);
                    free(extraArg);
                    continue;
                }
                free(path);
                free(extraArg);
            }

            else if (strcmp(cmd, "pwd") == 0) { // Vérifie si la commande est pwd
                pwd();
            }

            else if (strcmp(cmd, "exit") == 0) { // Vérifier si la commande est exit
                char *exit_code_str = strtok(NULL, " ");
                char *extraArg = strtok(NULL, " ");

                if (extraArg != NULL) { // Vérifie si la commande exit est utilisée avec trop d'arguments
                    fprintf(stderr, "exit: too many arguments\n");
                    free(exit_code_str);
                    free(extraArg);
                    continue;
                }
                int exit_code = atoi(exit_code_str); // Conversion du code de sortie en int
                free(exit_code_str);
                free(extraArg);
                exit_shell(exit_code); // Fait quitter le programme avec le code de sortie donné
            }
            

            free(prompt);
            free(cmdLine);
        }

        else{
            free(prompt);
            free(cmdLine);
            continue;
        }
    }

    return EXIT_SUCCESS;
}