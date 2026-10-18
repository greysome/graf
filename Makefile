CC=gcc
CFLAGS=-std=c23 -g3 -Wall -Wextra -Wpedantic -Wconversion -Wdouble-promotion \
	-Wno-unused-parameter -Wno-unused-function \
	-fsanitize=address,undefined
OUTS=libgraf.a graf-info

.PHONY: all clean afl

all: $(OUTS)

afl:
	$(MAKE) clean
	$(MAKE) CC=afl-cc all

graf-info: graf-info.c libgraf.a

libgraf.a: graf.c graf.h
	$(CC) $(CFLAGS) -c $< -o graf.o
	ar rcs $@ graf.o
	rm -f graf.o

clean:
	rm -f $(OUTS)
