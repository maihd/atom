#
#
#
#
#

CC    =gcc
CFLAGS=-g -Wall

WORKER=worker/atom-worker.c src/atom.c worker/jsmn/jsmn.c

.PHONY: test worker clean


test:
	$(CC) test/atom-prompt.c src/atom.c -o atom-prompt $(CFLAGS)
	$(CC) test/atom-viewer.c src/atom.c -o atom-viewer $(CFLAGS)

worker:
	$(CC) $(WORKER) -o atom-worker $(CFLAGS)

clean:
