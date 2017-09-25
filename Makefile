#
#
#
#
#

CC    =gcc
CFLAGS=-g -Wall

.PHONY: test clean

test:
	$(CC) test/atom-prompt.c src/atom.c -o atom-prompt $(CFLAGS)
	$(CC) test/atom-viewer.c src/atom.c -o atom-viewer $(CFLAGS)

clean:
