CFLAGS=-g -O2 -Wall -Werror -z noexecstack

ifneq (,$(CROSS_COMPILE))
CC=$(CROSS_COMPILE)gcc
endif

all:

example: aco.c example.c acosw.S aco.h
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) aco.c example.c acosw.S -o example

clean-example:
	rm -f example

all: example

clean: clean-example
