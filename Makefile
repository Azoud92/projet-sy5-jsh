CC=gcc
CFLAGS=-Wall
LDFLAGS=-lreadline
DEPS= # on ajoutera les fichiers d'en-tête .h
EXEC=runner

all: $(EXEC)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $<

runner: jsh.o # on ajoutera les fichiers utilisés par le main
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(EXEC) *.o
