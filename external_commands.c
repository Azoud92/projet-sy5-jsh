#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "external_commands.h"

int execute_external_command(char *cmd) {
    pid_t pid = fork();
    int status;

    if (pid < 0) { // Erreur lors de la création du processus
        fprintf(stderr, "Erreur lors de la création du processus\n");
        return 1;
    } 
    else if (pid == 0) {
        char *args[50]; // tableau des arguments de la commande
        args[0] = cmd;
        int i = 1;
        char *arg;
        while ((arg = strtok(NULL, " ")) != NULL) {
            args[i] = arg;
            i++;
        }
        args[i] = NULL; // Le dernier élément du tableau est NULL
        execvp(cmd, args);

        // Si on arrive ici, alors execvp a échoué
        fprintf(stderr, "Erreur lors de l'exécution de la commande '%s'\n", cmd);
        exit(EXIT_FAILURE);
    } 
    else {
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return 1;
    }
    return 1;
}