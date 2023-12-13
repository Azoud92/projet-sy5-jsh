#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
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


void print_job(Job *job) {
    char* status = statusToString(job->status);
    printf("[%d] %d  %s  %s\n", job->id, job->pgid, status, job->cmd);
}

int jobs() {
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (isJob[i]) {
            print_job(list_jobs[i]);
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
    job->cmd = cmd;
    updateJobsId();
    return job;
}



void addJob(Job *job) {
    int jobId = job -> id;
    list_jobs[jobId - 1] = job;
    isJob[jobId- 1] = true;
    ++nextJobId;
    
    updateJobsId();
    
    printf("[%d] %d\n", job->id, job->pgid);
}

Job *getJob(int id) {
    if (id > nextJobId || id < 1) {
        fprintf(stderr, "Invalid job id\n");
        return NULL;
        
    }
    if (id < 1){
        fprintf(stderr, "Invalid job id\n");
    }
    return list_jobs[id - 1];
}


void update_job_status() {
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
        pid_t result = waitpid(job->pgid, &status, WNOHANG | WUNTRACED);
        if (result == 0) {
            continue;
        } else
        if (result == -1) {
            perror("waitpid");
        } 
        else {
            if (WIFEXITED(status)) {
                job->status = DONE;
            } else if (WIFSIGNALED(status)) {
                printf ("killed\n");
                job->status = KILLED;
            } else if (WIFSTOPPED(status)) {
                job->status = STOPPED;
            } else if (WIFCONTINUED(status)) {
                job->status = RUNNING;
            }
        }
    }
    updateJobsId();
}

void jobBecameDone() {
    for (int i = 1; i < MAX_JOBS; ++i) {
        if (!isJob[i - 1]) {
            continue;
        }
        Job *job = getJob(i);
        if (job->status == DONE) {
            print_job(job);
            isJob[i - 1] = false;
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
            isJob[i - 1] = false;
            print_job(job);
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
            print_job(job);
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
