CC=gcc
CFLAGS=-Wall
LDFLAGS=-lreadline
DEPS= cd.h pwd.h exit.h external_commands.h jobs.h kill.h redirections.h # on ajoutera les fichiers d'en-tête .h
EXEC=jsh

all: $(EXEC)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $<

jsh: jsh.o cd.o pwd.o exit.o external_commands.o jobs.o kill.o redirections.o # on ajoutera les fichiers utilisés par le main
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(EXEC) *.o
