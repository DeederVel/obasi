CC = gcc
CFLAGS = -Wall -pedantic -std=c11 -D_POSIX_SOURCE -D_POSIX_C_SOURCE=199309L -D_GNU_SOURCE
execs = client server supervisor

.PHONY : clean test

all: server client supervisor

server :
	$(CC) $(CFLAGS) $@.c -o $@ -lpthread

client :
	$(CC) $(CFLAGS) $@.c -o $@

supervisor :
	$(CC) $(CFLAGS) $@.c -o $@ -lpthread

clean:
	@echo "Rimuovo gli eseguibili, gli oggetti e le lib"
	@-rm $(execs)

test:
	@echo "Avvio il test"
	@bash test.sh
