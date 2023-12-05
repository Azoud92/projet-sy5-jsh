#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

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

Job *init_job(int id, pid_t pgid, enum JobStatus status, char *cmd);
char* statusToString(enum JobStatus status);
void print_job(Job *job);
int jobs();
int getJobIndex();
void setJobId(int id);
void addJob(Job *job);
Job *getJob(int id);
void update_job_status();

#endif