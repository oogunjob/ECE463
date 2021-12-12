CC = gcc

CFLAGS = -std=gnu99 -g -O3

VAL = valgrind --tool=memcheck --track-origins=yes --log-file=memcheck.txt --leak-check=full --verbose

default: all

all: http_client

test1: http_client
	./http_client www.gnu.org 80 /software/make/manual/make.html

test2: http_client
	./http_client www.gnu.org 80 /software/make/manual/Tosin.pdf

test3: http_client
	./http_client sstatic.net 80 stackexchange/img/logos/so/so-logo-med.png

memory: http_client
	$(VAL) ./http_client www.gnu.org 80 /software/make/manual/Tosin.html

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

%: %.o
	$(CC) -o $@ $^

.PRECIOUS: %.o

.PHONY: default all

