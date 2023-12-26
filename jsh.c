#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <signal.h>
#include "external_commands.h"
#include "redirections.h"
#include "commands.h"
#include "jobs.h"
#include "exit.h"
#define GREEN_COLOR "\001\033[32m\002"
#define YELLOW_COLOR "\001\033[33m\002"
#define NORMAL_COLOR "\001\033[00m\002"

char *get_prompt(int trunc, int nbJobs) {
    char *prompt = malloc(trunc + 1 + strlen(GREEN_COLOR) + strlen(YELLOW_COLOR) + strlen(NORMAL_COLOR));

    char jobsPrompt[30];
    snprintf(jobsPrompt, sizeof(jobsPrompt), "%s[%d]%s", GREEN_COLOR, nbJobs, YELLOW_COLOR);

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        free(prompt);
        exit(EXIT_FAILURE);
    }

    int apparent_length = strlen(jobsPrompt) - (strlen(GREEN_COLOR) + strlen(YELLOW_COLOR)) + 2; // + 2 car on a le "$ " qui suit
    if (strlen(cwd) > trunc - apparent_length) {
        int decalage = trunc - apparent_length - 3; // - 3 pour les "..."

        memcpy(cwd, "...", 3);
        if (decalage < 0) {
            decalage = 0;
        }
        else {
            memmove(cwd + 3, cwd + strlen(cwd) - decalage, decalage);
        }
        cwd[decalage + 3] = '\0';
    }

    snprintf(prompt, trunc + 1 + strlen(GREEN_COLOR) + strlen(YELLOW_COLOR) + strlen(NORMAL_COLOR), "%s%s%s$ ",
             jobsPrompt, cwd, NORMAL_COLOR);

    return prompt;
}

void handle_command (char *command) {
    // On obtient la commande sans les redirections
    char *cleanedCmd = extract_command(command);
    if (cleanedCmd == NULL) {
        lastExitCode = 1;
        return;
    }

    if (save_flows() != 0) { // sauvegarde des flux
        free(cleanedCmd);
        lastExitCode = 1;
        return;
    }

    if (handle_redirections(command) != 0) { // gestion des redirections
        restore_flows();
        free(cleanedCmd);
        lastExitCode = 1;
        return;
    }

    execute_command(cleanedCmd);    

    restore_flows();
    free(cleanedCmd);
}


void sigttou_handler(int sig) {
    tcsetpgrp(STDIN_FILENO, getpgrp());
}

int main() {

    struct sigaction sa;
    sa.sa_handler = sigttou_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTTOU, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }


    rl_outstream = stderr;
    int nbJobs;
    while (1) {
        nbJobs = getNbJobs();
        char *prompt = get_prompt(30, nbJobs);
        char *cmdLine = readline(prompt);
        
        JobsKilled();

        if (cmdLine == NULL) {
            free(prompt);
            free(cmdLine);
            exitShell(lastExitCode);
            break;
        }

        add_history(cmdLine);

        if (strstr(cmdLine, " | ") != NULL) {
            handle_pipelines(cmdLine);
        } else {
            handle_command(cmdLine);
        }
        
        update_job_status(false);

        free(prompt);
        free(cmdLine);
    }
    
    return lastExitCode;
}