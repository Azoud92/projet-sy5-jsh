#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include "jobs.h"
#include "signals.h"

int execute_external_command(char *cmd, char *cmdLine, bool isPipeBg) {
    int status;
    bool bg = false;
    pid_t pid;

    char *cmdLineCopy = strdup(cmdLine);

    char *args[50]; // tableau des arguments de la commande
    args[0] = cmd;

    int i = 1;
    char *saveptr;
    char *arg = strtok_r(cmdLine, " ", &saveptr);    

    while ((arg = strtok_r(NULL, " ", &saveptr)) != NULL) {
        args[i] = arg;
        i++;
    }
    args[i] = NULL; // Le dernier élément du tableau est NULL

    if (i > 1 && strcmp(args[i-1], "&") == 0) {
        bg = true;
        args[i-1] = NULL;
    }
    
    if (isPipeBg){
        execvp(cmd, args);
        // Si on arrive ici, alors execvp a échoué
        fprintf(stderr, "Erreur lors de l'exécution de la commande '%s'\n", cmd);
        free(cmdLineCopy);
        return 1;
    }


    switch (pid = fork()){
        case -1: // Erreur lors de la création du processus
            fprintf(stderr, "Erreur lors de la création du processus\n");
            free(cmdLineCopy);
            return 1;      

        case 0:
            setpgid(0, 0);            

            execvp(cmd, args);
            // Si on arrive ici, alors execvp a échoué
            fprintf(stderr, "Erreur lors de l'exécution de la commande '%s'\n", cmd);
            free(cmdLineCopy);
            exit(EXIT_FAILURE);

        default:
            if (bg) {
                // Créer et ajouter le job à la liste des jobs
                char *andPos = strrchr(cmdLineCopy, '&');
                if (andPos && andPos > cmdLineCopy) {
                    char *pos = andPos - 1;
                    while (pos > cmdLineCopy && *pos == ' ') { // suppression du "&" et des espaces avant
                        --pos;
                    }
                    *(pos + 1) = '\0';
                }

                Job *job = init_job(pid, RUNNING, cmdLineCopy);
                int pgid = setpgid(pid, pid);
                if (pgid == -1) {
                    fprintf(stderr, "Erreur lors de la création du groupe de processus\n");
                    free(cmdLineCopy);
                    return 1;
                }
                addJob(job);
                free(cmdLineCopy);
                return 0;
            }

            else {
                setpgid(pid, pid);

                ignore_sigttou();
                tcsetpgrp(STDIN_FILENO, pid);
                restore_sigttou();

                do {
                    waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSTOPPED(status) && !WIFSIGNALED(status));

                ignore_sigttou();
                tcsetpgrp(STDIN_FILENO, getpgrp());
                restore_sigttou();

                if (WIFSTOPPED(status)) {       
                    //suppression des espaces et saut de ligne de fin de commande
                    remove_spaces_and_newline(cmdLineCopy);
                    Job *job = init_job(pid, STOPPED, cmdLineCopy);
                    addJob(job);
                    free(cmdLineCopy);
                    return WEXITSTATUS(status);
                }
                
                if (WIFEXITED(status)) {
                    free(cmdLineCopy);
                    return WEXITSTATUS(status);
                }

                if (WIFSIGNALED(status)) {
                    free(cmdLineCopy);
                    return WTERMSIG(status);
                }

                free(cmdLineCopy);
                return 1;
            }
    }
    free(cmdLineCopy);
    return 1;
}

