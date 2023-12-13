#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

#define MAX_JOBS 512



enum JobStatus {
    RUNNING, 
    STOPPED, 
    DETACHED, 
    KILLED,
    DONE
};

typedef struct {
    int id;
    pid_t pgid; 
    enum JobStatus status; 
    char *cmd;
} Job;

Job *init_job(pid_t pgid, enum JobStatus status, char *cmd);
char* statusToString(enum JobStatus status);
void print_job(Job *job);
int jobs();
void addJob(Job *job);
Job *getJob(int id);
void update_job_status();
void jobBecameDone();
void killJob(pid_t pid);
void stopJob(pid_t pid);
void continueJob(pid_t pid);
int getNbJobs();

#endif