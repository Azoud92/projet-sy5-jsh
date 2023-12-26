#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cd.h"
#include "pwd.h"
#include "exit.h"
#include "external_commands.h"
#include "jobs.h"
#include "kill.h"
#include "bg.h"
#include "fg.h"

int lastExitCode = 0;

void execute_command (char *command) {
    char *saveptr;
    char *cmdCopy = strcpy(malloc(strlen(command) + 1), command);
    char *cmd = strtok_r(command, " ", &saveptr);    

    if (cmd != NULL){
        if (strcmp(cmd, "jobs") != 0) {
            jobsDone();
        }

        // --- COMMANDES INTERNES ---
        if (strcmp(cmd, "cd") == 0) { // Commande "cd"
            char *path = strtok_r(NULL, " ", &saveptr);
            char *extraArg = strtok_r(NULL, " ", &saveptr);

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
            char *exitCodeStr = strtok_r(NULL, " ", &saveptr);
            char *extraArg = strtok_r(NULL, " ", &saveptr);

            if (extraArg != NULL) { // Vérifie si la commande exit est utilisée avec trop d'arguments
                fprintf(stderr, "exit: too many arguments\n");
                lastExitCode = 1;
                return;
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
            }
            else if (getNbJobs() > 0){
                fprintf(stderr, "exit: Des jobs sont en cours d'exécution ou suspendus.\n");
                lastExitCode = 1;
            } 
            else {
                exitShell(exitCode);
            }
        }

        else if (strcmp(cmd, "?") == 0) { // Commande "?"
            printf("%d\n", lastExitCode);
            lastExitCode = 0;
        }

        else if (strcmp(cmd, "jobs") == 0) { // Commande "jobs"
            update_job_status(true);
            lastExitCode = jobs();
        }

        else if (strcmp(cmd, "kill") == 0) { // Commande "kill"
            lastExitCode = cmdKill(cmdCopy);
        }

        else if (strcmp(cmd, "bg") == 0) { // Commande "bg"
            lastExitCode = bg(cmdCopy);
        }
        else if (strcmp(cmd, "fg") == 0) { // Commande "fg"
            lastExitCode = fg(cmdCopy);
        }

        // --- COMMANDES EXTERNES ---
        else {            
            lastExitCode = execute_external_command(cmd, cmdCopy);
        }        
    }   

    free(cmdCopy);
}