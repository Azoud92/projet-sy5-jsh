#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void ignore_signals() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGTTIN, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTTOU, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
}

void sigttou_handler(int sig) {
    tcsetpgrp(STDIN_FILENO, getpgrp());
}

void restore_signals() {
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    struct sigaction sa_ttou;
    sa_ttou.sa_handler = sigttou_handler;
    sa_ttou.sa_flags = 0;
    sigemptyset(&sa_ttou.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGTTIN, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTTOU, &sa_ttou, NULL);
    sigaction(SIGTSTP, &sa, NULL);
}

void ignore_sigttou() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigaction(SIGTTOU, &sa, NULL);
}

void restore_sigttou() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigaction(SIGTTOU, &sa, NULL);
}