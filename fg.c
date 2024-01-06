#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "jobs.h"
#include <unistd.h>
#include "signals.h"

int fg(char *cmd){
    int jobId;
    char *saveptr;

    char *arg = strtok_r(cmd, " ", &saveptr);
    arg = strtok_r(NULL, " ", &saveptr);
    if (arg == NULL) {
        fprintf(stderr, "fg: missing operand\n");
        return 1;
    }

    if (arg[0] == '%') {
        // Un numéro de job a été fourni
        char *endptr;
        jobId = (int) strtol(arg + 1, &endptr, 10);
        if (*endptr != '\0' || jobId < 0 ) {
            fprintf(stderr, "fg: invalid job number\n");
            return 1;
        }
    } 
    else {
        fprintf(stderr, "fg: invalid job number\n");
        return 1;
    }

    Job *job = getJob(jobId);
    if (job == NULL) {
        fprintf(stderr, "fg: job not found: %d\n", jobId);
        return 1;
    }

    pid_t jPid = job->pgid;
    printName(job);

    tcsetpgrp(STDIN_FILENO, jPid);
    kill(-jPid, SIGCONT);

    int status;
    
    do {
        waitpid(jPid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSTOPPED(status) && !WIFSIGNALED(status));

    tcsetpgrp(STDIN_FILENO, getpgrp());

    if (WIFSTOPPED(status)) {       
        stopJob(jPid);

        // Ignorer temporairement SIGTTOU
        ignore_sigttou();

        // Rendre le contrôle du terminal au shell
        tcsetpgrp(STDIN_FILENO, getpgrp());

        // Rétablir le gestionnaire de signal par défaut pour SIGTTOU
        restore_sigttou();

        return WEXITSTATUS(status);
    }
    
    if (WIFEXITED(status)) {
        removeJob(jPid);
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        removeJob(jPid);
        return WTERMSIG(status);
    }

    removeJob(jPid);

    return 0;
}