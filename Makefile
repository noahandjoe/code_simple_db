CC=gcc
CFLAGS=-Wall -Wextra -pedantic -o
sdb: sdb.c
	$(CC) $< $(CFLAGS) $@ -std=c99