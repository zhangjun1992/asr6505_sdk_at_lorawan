CC = gcc
CFLAGS = -O0 -g -Wall -Wextra -std=c99 -pedantic-errors -I inc/ -I parson/

all: lcfg

.PHONY: lcfg
lcfg: src/main.c parson/parson.c
	$(CC) $(CFLAGS) -o $@ src/main.c parson/parson.c

clean:
	rm -f lcfg *.o

