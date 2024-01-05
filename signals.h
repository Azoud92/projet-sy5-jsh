#ifndef SIGNALS_H
#define SIGNALS_H

void ignore_signals();
void restore_signals();
void ignore_sigttou();
void restore_sigttou();

#endif