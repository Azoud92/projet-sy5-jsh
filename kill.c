#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "jobs.h"

int cmdKill(char *cmd) {
    int sig = SIGTERM; // Le signal par défaut est SIGTERM
    pid_t pid;
    int jobNum;
    bool isNbJob = false;
    char *saveptr;

    char *arg = strtok_r(cmd, " ", &saveptr);
    arg = strtok_r(NULL, " ", &saveptr);
    if (arg == NULL) {
        fprintf(stderr, "kill: missing operand\n");
        return 1;
    }

    if (arg[0] == '-') {
        char *endptr;
        sig = (int) strtol(arg + 1, &endptr, 10);
        if (*endptr != '\0' || sig < 0 ) {
            fprintf(stderr, "kill: invalid signal\n");
            return 1;
        }

        arg = strtok_r(NULL, " ", &saveptr);
        if (arg == NULL) {
            fprintf(stderr, "kill: missing operand\n");
            return 1;
        }
    }

    if (arg[0] == '%') {
        // Un numéro de job a été fourni
        isNbJob = true;
        char *endptr;
        jobNum = (int) strtol(arg + 1, &endptr, 10);
        if (*endptr != '\0' || jobNum < 0 ) {
            fprintf(stderr, "kill: invalid job number\n");
            return 1;
        }
    } 
    else {
        // Un PID a été fourni
        char *endptr;
        pid = (int) strtol(arg, &endptr, 10);
        if (*endptr != '\0' || pid < 0 ) {            
            fprintf(stderr, "kill: invalid PID\n");
            return 1;
        }
    }
    

    if (isNbJob) {
        // Envoyer le signal à tous les processus du job
        Job *job = getJob(jobNum);
        if (job != NULL) {
            pid = job->pgid;
            int k = kill(-pid, sig);
            if (k == -1) {
                perror("kill");
                return 1;
            }
        } else {
            fprintf(stderr, "Job de numéro %d non trouvé\n", jobNum);
            return 1;
        }
    } 
    else {
        // Envoyer le signal au processus
        int k = kill(pid, sig);
        if (k == -1) {
            perror("kill");
            return 1;
        }
    }
    return 0;
}