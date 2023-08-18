CC=gcc
CFLAGS=-Wall -o
sdb: sdb.c
	$(CC) $< $(CFLAGS) $@ -std=c99