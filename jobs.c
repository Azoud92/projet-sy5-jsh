#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#define MAX_JOBS 512

enum JobStatus {
    RUNNING, 
    STOPPED, 
    DETACHED, 
    KILLED,
    DONE
};

typedef struct {
    int id; // Numéro du job
    pid_t pgid; // Identifiant du groupe de processus
    enum JobStatus status; // État du job
    char *cmd; // Ligne de commande
} Job;

Job *list_jobs[MAX_JOBS];
bool isJob[MAX_JOBS];
int nextJobId = 1;
Job *fgJob = NULL;

void free_job(Job *job) {
    free(job->cmd);
    free(job);
}

char* statusToString(enum JobStatus status) {
    switch (status) {
        case RUNNING:
            return "Running";
        case STOPPED:
            return "Stopped";
        case DETACHED:
            return "Detached";
        case KILLED:
            return "Killed";
        case DONE:
            return "Done";
        default:
            return "Unknown";   
    }
}

void print_job(Job *job, bool isStderr) {
    char* status = statusToString(job->status);

    if (isStderr) fprintf(stderr, "[%d] %d  %s  %s\n", job->id, job->pgid, status, job->cmd);
    else fprintf(stdout, "[%d] %d  %s  %s\n", job->id, job->pgid, status, job->cmd);
}

int jobs() {
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (isJob[i]) {
            print_job(list_jobs[i], false);
            if (list_jobs[i]->status == DONE) {
                isJob[i] = false;
            }
        }                
    }
    return 0;
}

int getNbJobs() {
    int nb = 0;
    for (int i = 1; i < MAX_JOBS; ++i) {
        if (isJob[i - 1]) {
            ++nb;
        }
    }
    return nb;
}

void updateJobsId() {
    int lastFalseId = 0;
    for (int i = MAX_JOBS - 1; i >= 0; --i){
        if (!isJob[i]) lastFalseId = i;
    } 
    nextJobId = lastFalseId + 1;
}

Job *init_job(pid_t pgid, enum JobStatus status, char *cmd) {
    Job *job = malloc(sizeof(Job));
    job->id = nextJobId;
    job->pgid = pgid;
    job->status = status;
    job->cmd = strdup(cmd);
    updateJobsId();
    return job;
}

void addJob(Job *job) {
    int jobId = job -> id;
    list_jobs[jobId - 1] = job;
    isJob[jobId- 1] = true;
    ++nextJobId;
    
    updateJobsId();
    
    print_job(job, true);
}

Job *getJob(int id) {
    if (id > MAX_JOBS || id < 1) {
        fprintf(stderr, "Invalid job id: %d\n", id);
        return NULL;        
    }
    if (id < 1) {
        fprintf(stderr, "Invalid job id: %d\n", id);
    }
    return list_jobs[id - 1];
}

void update_job_status(bool itsCommandJobs) {
    for (int i = 1; i < MAX_JOBS; ++i) {
        if (!isJob[i - 1]) {
            continue;
        }

        Job *job = getJob(i);
        if (job->status == DONE || job->status == KILLED) {
            // Le job est déjà terminé, donc nous n'avons pas besoin d'appeler waitpid
            continue;
        }

        int status;
        pid_t result = waitpid(job->pgid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (result == 0) {
            continue;
        }
        else if (result == -1) {
            perror("waitpid");
        } 
        else {
            if (WIFEXITED(status)) {
                job->status = DONE;
                isJob[i - 1] = false;
                print_job(job, !itsCommandJobs);
                free_job(job);

                
            }
            else if (WIFSIGNALED(status)) {
                job->status = KILLED;
                isJob[i - 1] = false;
                print_job(job, !itsCommandJobs);
                free_job(job);

            }
            else if (WIFSTOPPED(status)) {
                job->status = STOPPED;
                print_job(job, !itsCommandJobs);

            }
            else if (WIFCONTINUED(status)) {
                job->status = RUNNING;
                print_job(job, !itsCommandJobs);
            }
        }
    }
    updateJobsId();
}

void jobsDone() {
    for (int i = 1; i < MAX_JOBS; ++i) {
        if (!isJob[i - 1]) {
            continue;
        }

        Job *job = getJob(i);
        if (job->status == DONE) {
            print_job(job, true);
            isJob[i - 1] = false;
            free_job(job);
        }
    }
}

void JobsKilled() {
    for (int i = 1; i < MAX_JOBS; ++i) {
        if (!isJob[i - 1]) {
            continue;
        }

        Job *job = getJob(i);
        if (job->status == KILLED) {
            print_job(job, true);
            isJob[i - 1] = false;
            free_job(job);
        }
    }
} 

void killJob(pid_t pid){
    for (int i = 1; i < MAX_JOBS; ++i) {
        if (!isJob[i - 1]) {
            continue;
        }

        Job *job = getJob(i);
        if (job->pgid == pid) {
            job->status = KILLED;
        }
    }
}

void stopJob(pid_t pid){
    for (int i = 1; i < MAX_JOBS; ++i) {
        if (!isJob[i - 1]) {
            continue;
        }

        Job *job = getJob(i);
        if (job->pgid == pid) {
            job->status = STOPPED;
            print_job(job, true);
        }
    }
}

void continueJob(pid_t pid){
    for (int i = 1; i < MAX_JOBS; ++i) {
        if (!isJob[i - 1]) {
            continue;
        }

        Job *job = getJob(i);
        if (job->pgid == pid) {
            job->status = RUNNING;
        }
    }
}


void removeJob(pid_t pid) {
    for (int i = 1; i < MAX_JOBS; ++i) {
        if (!isJob[i - 1]) {
            continue;
        }

        Job *job = getJob(i);
        if (job->pgid == pid) {
            isJob[i - 1] = false;
            free_job(job);
        }
    }
}

void printName(Job *job) {
    fprintf(stderr, "%s\n", job->cmd);
}