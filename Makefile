OUTPUT_DIR=output
$(shell mkdir -p $(OUTPUT_DIR))

CFLAGS=-g -O2 -Wall -Werror -z noexecstack

ifneq (,$(CROSS_COMPILE))
CC=$(CROSS_COMPILE)gcc
endif

app_list=test_aco_tutorial_0 test_aco_tutorial_1 test_aco_tutorial_2 test_aco_tutorial_3 test_aco_tutorial_4 test_aco_tutorial_5 test_aco_tutorial_6 test_aco_synopsis test_aco_benchmark

all:

example: aco.c example.c acosw.S aco.h
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) aco.c example.c acosw.S -o example

clean-example:
	rm -f example

all: example

clean: clean-example

# $1 = binary name
# $2 = extra CFLAGS
# $3 = binary suffix
define generate_build_rule
$(OUTPUT_DIR)/$1$3: acosw.S aco.c $1.c aco.h
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $2 acosw.S aco.c $1.c -o $$@

all: $(OUTPUT_DIR)/$1$3


clean-$1$3:
	rm -f $(OUTPUT_DIR)/$1$3

clean: clean-$1$3

endef

define generate_build_rules
$(foreach app,$(app_list),$(call generate_build_rule,$(app),$1,$2))
endef

$(eval $(call generate_build_rules,,..no_valgrind.standaloneFPUenv))
$(eval $(call generate_build_rules,-DACO_USE_VALGRIND,..valgrind.standaloneFPUenv))
$(eval $(call generate_build_rules,-DACO_CONFIG_SHARE_FPU_MXCSR_ENV,..no_valgrind.shareFPUenv))
$(eval $(call generate_build_rules,-DACO_CONFIG_SHARE_FPU_MXCSR_ENV -DACO_USE_VALGRIND,..valgrind.shareFPUenv))
ifeq ($(shell uname -m),x86-64)
ifeq (,$(CROSS_COMPILE))
$(eval $(call generate_build_rules,,..m32.no_valgrind.standaloneFPUenv))
$(eval $(call generate_build_rules,-m32 -DACO_USE_VALGRIND,..m32.valgrind.standaloneFPUenv))
$(eval $(call generate_build_rules,-m32 -DACO_CONFIG_SHARE_FPU_MXCSR_ENV,..m32.no_valgrind.shareFPUenv))
$(eval $(call generate_build_rules,-m32 -DACO_CONFIG_SHARE_FPU_MXCSR_ENV -DACO_USE_VALGRIND,..m32.valgrind.shareFPUenv))
endif
endif

