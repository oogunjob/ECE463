CC = gcc

CFLAGS = -std=gnu99 -g -O3

VAL = valgrind --tool=memcheck --log-file=memcheck.txt --leak-check=full --verbose

default: all

all: http_client

memory: http_client
	$(VAL) ./http_client www.gnu.org 80 /software/make/manual/make.html

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

%: %.o
	$(CC) -o $@ $^

.PRECIOUS: %.o

.PHONY: default all

