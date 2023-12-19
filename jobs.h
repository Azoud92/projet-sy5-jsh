#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>
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
    int id;
    pid_t pgid; 
    enum JobStatus status; 
    char *cmd;
} Job;


Job *init_job(pid_t pgid, enum JobStatus status, char *cmd);
void free_job(Job *job);
char* statusToString(enum JobStatus status);
void print_job(Job *job, bool isStderr);
int jobs();
void addJob(Job *job);
Job *getJob(int id);
void update_job_status(bool itsCommandJobs);
void jobsDone();
void JobsKilled();
void killJob(pid_t pid);
void stopJob(pid_t pid);
void continueJob(pid_t pid);
int getNbJobs();

#endif