#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "cd.h"
#include "pwd.h"
#include "exit.h"
#include "external_commands.h"
#include "jobs.h"
#include "kill.h"
#define GREEN_COLOR "\001\033[32m\002"
#define YELLOW_COLOR "\001\033[33m\002"
#define NORMAL_COLOR "\001\033[00m\002"

int lastExitCode = 0;

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
    rl_outstream = stderr;
    int nbJobs;
    while (1) {
        jobBecameDone();
        update_job_status();
        nbJobs = getJobIndex() - 1;
        char *prompt = get_prompt(30, nbJobs);
        char *cmdLine = readline(prompt); // provisoire car pas de gestion des jobs pour l'instant
        if (cmdLine == NULL) {
            break;
        }
        add_history(cmdLine);
        char *cmdLineCopy = strcpy(malloc(strlen(cmdLine) + 1), cmdLine);

        char *cmd = strtok(cmdLine, " ");

        if (cmd != NULL){

            // --- COMMANDES INTERNES ---
            if (strcmp(cmd, "cd") == 0) { // Commande "cd"
                char *path = strtok(NULL, " ");
                char *extraArg = strtok(NULL, " ");

                if (extraArg != NULL) { // Vérifie si la commande cd est utilisée avec trop d'arguments
                    fprintf(stderr, "cd: too many arguments\n");
                    lastExitCode = 1;
                }
                else {
                    lastExitCode = cd(path);
                }
            }

            else if (strcmp(cmd, "pwd") == 0) { // Commande "pwd"
                lastExitCode = pwd();
            }

            else if (strcmp(cmd, "exit") == 0) { // Commande "exit"
                char *exitCodeStr = strtok(NULL, " ");
                char *extraArg = strtok(NULL, " ");

                if (extraArg != NULL) { // Vérifie si la commande exit est utilisée avec trop d'arguments
                    fprintf(stderr, "exit: too many arguments\n");
                    lastExitCode = 1;
                    continue;
                }
                
                if (exitCodeStr == NULL) { // Vérifie si la commande exit est utilisée sans argument
                    exitShell(lastExitCode);
                }

                int exitCode = 0;
                char *endptr;
                exitCode = (int) strtol(exitCodeStr, &endptr, 10); // on vérifie bien que le code fourni soit un entier

                if (*endptr != '\0' || exitCode < 0 || exitCode > 255) {
                    fprintf(stderr, "exit: invalid exit code\n");
                    lastExitCode = 1;
                } else {
                    exitShell(exitCode);
                }
            }

            else if (strcmp(cmd, "?") == 0) { // Commande "?"
                printf("%d\n", lastExitCode);
                lastExitCode = 0;
            }

            else if (strcmp(cmd, "jobs") == 0) { // Commande "jobs"
                update_job_status();
                lastExitCode = jobs();
            }

            else if (strcmp(cmd, "kill") == 0) { // Commande "kill"
                lastExitCode = cmdKill();
            }

            // --- COMMANDES EXTERNES ---
            else {
                lastExitCode = execute_external_command(cmd, cmdLineCopy);
            }          
        }

        free(prompt);
        free(cmdLine);
    }
    
    return lastExitCode;
}