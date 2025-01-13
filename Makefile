CFLAGS=-g -O2 -Wall -Werror -z noexecstack

ifneq (,$(CROSS_COMPILE))
CC=$(CROSS_COMPILE)gcc
endif

all: example

clean: clean-example

example: aco.c example.c acosw.S aco.h
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) aco.c example.c acosw.S -o example

clean-example:
	rm -f example

cscope:
	rm -f cscope.*
	find . -name "*.[chSs]" > cscope.files
	cscope -b -q -k

cscope-clean:
	rm -f cscope.*

distclean: clean cscope-clean
