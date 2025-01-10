CFLAGS = -g -O2 -Wall -Werror -z noexecstack
ARCH ?= $(shell uname -m)

ifneq (,$(CROSS_COMPILE))
CC=$(CROSS_COMPILE)gcc
endif

all: example

clean: clean-example

example: uco.c example.c arch/$(ARCH)/ucosw.S uco.h
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) uco.c example.c arch/$(ARCH)/ucosw.S -o $@

clean-example:
	rm -f example

cscope:
	rm -f cscope.*
	find . -name "*.[chSs]" > cscope.files
	cscope -b -q -k

cscope-clean:
	rm -f cscope.*

distclean: clean cscope-clean
