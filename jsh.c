#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include "cd.h"
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

        if (cmd != NULL && strcmp(cmd, "cd") == 0) {
            char *path = strtok(NULL, " ");
            char *extraArg = strtok(NULL, " ");

            if (extraArg != NULL) {
                fprintf(stderr, "cd: too many arguments\n");
                continue;
            if (cd(path) != 0) {
                fprintf(stderr, "Erreur lors du changement de répertoire vers '%s'\n", path);
                continue;
            }
        }

        
        free(prompt);
        free(cmdLine);
    }

    return EXIT_SUCCESS;
}