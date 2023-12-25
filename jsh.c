#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <signal.h>
#include "cd.h"
#include "pwd.h"
#include "exit.h"
#include "external_commands.h"
#include "jobs.h"
#include "kill.h"
#include "redirections.h"
#include "bg.h"
#include "fg.h"
#define GREEN_COLOR "\001\033[32m\002"
#define YELLOW_COLOR "\001\033[33m\002"
#define NORMAL_COLOR "\001\033[00m\002"

int lastExitCode = 0;

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

    // On crée une copie pour les redirections
    char *redirectionCopy = strdup(command);

    if (save_flows() != 0) { // sauvegarde des flux
        free(cleanedCmd);
        free(redirectionCopy);
        lastExitCode = 1;
        return;
    }

    if (handle_redirections(redirectionCopy) != 0) { // gestion des redirections
        restore_flows();
        free(cleanedCmd);
        free(redirectionCopy);
        lastExitCode = 1;
        return;
    }

    char *saveptr;
    char *cmdLineCopy = strcpy(malloc(strlen(cleanedCmd) + 1), cleanedCmd);
    char *strtokCopy = strcpy(malloc(strlen(cleanedCmd) + 1), cleanedCmd);
    char *cmd = strtok_r(strtokCopy, " ", &saveptr);   

    if (cmd != NULL){
        if (strcmp(cmd, "jobs") != 0) {
            jobsDone();
        }

        // --- COMMANDES INTERNES ---
        if (strcmp(cmd, "cd") == 0) { // Commande "cd"
            char *path = strtok_r(NULL, " ", &saveptr);
            char *extraArg = strtok_r(NULL, " ", &saveptr);

            if (extraArg != NULL) { // Vérifie si la commande cd est utilisée avec trop d'arguments
                fprintf(stderr, "cd: too many arguments\n");
                lastExitCode = 1;
            }
            else {
                lastExitCode = cd(path);
            }
        }

        else if (strcmp(cmd, "pwd") == 0) { // Commande "pwd"
            lastExitCode = pwd();
        }

        else if (strcmp(cmd, "exit") == 0) { // Commande "exit"
            char *exitCodeStr = strtok_r(NULL, " ", &saveptr);
            char *extraArg = strtok_r(NULL, " ", &saveptr);

            if (extraArg != NULL) { // Vérifie si la commande exit est utilisée avec trop d'arguments
                fprintf(stderr, "exit: too many arguments\n");
                lastExitCode = 1;
                return;
            }
            
            if (exitCodeStr == NULL) { // Vérifie si la commande exit est utilisée sans argument
                exitShell(lastExitCode);
            }

            int exitCode = 0;
            char *endptr;
            exitCode = (int) strtol(exitCodeStr, &endptr, 10); // on vérifie bien que le code fourni soit un entier

            if (*endptr != '\0' || exitCode < 0 || exitCode > 255) {
                fprintf(stderr, "exit: invalid exit code\n");
                lastExitCode = 1;
            }
            else if (getNbJobs() > 0){
                fprintf(stderr, "exit: Des jobs sont en cours d'exécution ou suspendus.\n");
                lastExitCode = 1;
            } 
            else {
                exitShell(exitCode);
            }
        }

        else if (strcmp(cmd, "?") == 0) { // Commande "?"
            printf("%d\n", lastExitCode);
            lastExitCode = 0;
        }

        else if (strcmp(cmd, "jobs") == 0) { // Commande "jobs"
            update_job_status(true);
            lastExitCode = jobs();
        }

        else if (strcmp(cmd, "kill") == 0) { // Commande "kill"
            lastExitCode = cmdKill(cmdLineCopy);
        }

        else if (strcmp(cmd, "bg") == 0) { // Commande "bg"
            lastExitCode = bg(cmdLineCopy);
        }
        else if (strcmp(cmd, "fg") == 0) { // Commande "fg"
            lastExitCode = fg(cmdLineCopy);
        }


        // --- COMMANDES EXTERNES ---
        else {            
            lastExitCode = execute_external_command(cmd, cmdLineCopy);
        }        
    }   

    restore_flows();
    free(cleanedCmd);
    free(redirectionCopy);
    free(cmdLineCopy);
    free(strtokCopy);
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

        handle_command(cmdLine);
        
        update_job_status(false);

        free(prompt);
        free(cmdLine);
    }
    
    return lastExitCode;
}