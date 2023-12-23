#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "jobs.h"
int bg(char *cmd){
    int jobId;
    char *saveptr;
    char *arg = strtok_r(cmd, " ", &saveptr);
    arg = strtok_r(NULL, " ", &saveptr);
    if (arg == NULL) {
        fprintf(stderr, "bg: missing operand\n");
        return 1;
    }

    if (arg[0] == '%') {
        // Un numéro de job a été fourni
        char *endptr;
        jobId = (int) strtol(arg + 1, &endptr, 10);
        if (*endptr != '\0' || jobId < 0 ) {
            fprintf(stderr, "bg: invalid job number\n");
            return 1;
        }
    } 
    else {
        fprintf(stderr, "bg: invalid job number\n");
        return 1;
    }

    Job *job = getJob(jobId);
    if (job == NULL) {
        fprintf(stderr, "bg: job not found: %d\n", jobId);
        return -1;
    }
    kill(-job->pgid, SIGCONT);
    job->status = RUNNING;
    return 0;

}
