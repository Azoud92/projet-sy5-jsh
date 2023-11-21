CC=gcc
CFLAGS=-Wall
LDFLAGS=-lreadline
DEPS= cd.h pwd.h # on ajoutera les fichiers d'en-tête .h
EXEC=jsh

all: $(EXEC)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $<

jsh: jsh.o cd.o pwd.o # on ajoutera les fichiers utilisés par le main
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(EXEC) *.o
