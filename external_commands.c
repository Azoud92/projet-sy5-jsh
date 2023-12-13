#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "jobs.h"
#include "external_commands.h"

int execute_external_command(char *cmd, char *cmdLine) {
    int status;
    bool bg = false;
    pid_t pid;

    char *args[50]; // tableau des arguments de la commande
        args[0] = cmd;
        int i = 1;
        char *arg;
        while ((arg = strtok(NULL, " ")) != NULL) {
            args[i] = arg;
            i++;
        }
        args[i] = NULL; // Le dernier élément du tableau est NULL

        if (i > 1 && strcmp(args[i-1], "&") == 0) {
            bg = true;
            args[i-1] = NULL;
        }

        switch (pid = fork()){
            case -1: // Erreur lors de la création du processus
                fprintf(stderr, "Erreur lors de la création du processus\n");
                return 1;
            
            case 0:
                execvp(cmd, args);
                // Si on arrive ici, alors execvp a échoué
                fprintf(stderr, "Erreur lors de l'exécution de la commande '%s'\n", cmd);
                exit(EXIT_FAILURE);

            default:
                if (bg) {
                    // Créer et ajouter le job à la liste des jobs
                    Job *job = init_job(pid, RUNNING, cmdLine);
                    int pgid = setpgid(pid, pid);
                    if (pgid == -1) {
                        fprintf(stderr, "Erreur lors de la création du groupe de processus\n");
                        return 1;
                    }
                    addJob(job);
                }

                else {
                    waitpid(pid, &status, 0);
                }
                if (WIFEXITED(status)) {
                    return WEXITSTATUS(status);
                }
                return 1;



    }
    return 1;
}