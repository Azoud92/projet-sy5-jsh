#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>



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

Job *list_jobs[250];
bool isJob[250];
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
    for (int i = 0; i < nextJobId - 1; i++) {
        if (isJob[i]) {
            print_job(list_jobs[i]);
            if (list_jobs[i]->status == DONE) {
                isJob[i] = false;
            }
        }
                
    }
    return 0;

}


Job * init_job(int id, pid_t pgid, enum JobStatus status, char *cmd) {
    Job *job = malloc(sizeof(Job));
    job->id = id;
    job->pgid = pgid;
    job->status = status;
    job->cmd = cmd;
    return job;
}

int getJobIndex() {
    return nextJobId;
}

void setJobId(int id) {
    nextJobId = id;
}

void addJob(Job *job) {
    list_jobs[nextJobId - 1] = job;
    isJob[nextJobId - 1] = true;
    nextJobId++;
    
    printf("[%d] %d\n", job->id, job->pgid);
}

Job * getJob(int id) {
    if (id > nextJobId - 1 || id < 1) {
        return NULL;
    }
    return list_jobs[id - 1];
}


void update_job_status() {
    for (int i = 1; i < getJobIndex(); i++) {
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
}

void jobBecameDone() {
    for (int i = 1; i < getJobIndex(); i++) {
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
    for (int i = 1; i < getJobIndex(); i++) {
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
    for (int i = 1; i < getJobIndex(); i++) {
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
    for (int i = 1; i < getJobIndex(); i++) {
        if (!isJob[i - 1]) {
            continue;
        }
        Job *job = getJob(i);
        if (job->pgid == pid) {
            job->status = RUNNING;
        }
    }
}
