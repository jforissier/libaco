/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright 2018 Sen Han <00hnes@gmail.com>
 * Copyright 2025 Linaro Limited
 */

#ifndef _UCO_H_
#define _UCO_H_

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define UCO_VERSION_MAJOR 1
#define UCO_VERSION_MINOR 2
#define UCO_VERSION_PATCH 4

#ifdef __i386__
#define UCO_REG_IDX_RETADDR 0
#define UCO_REG_IDX_SP 1
#define UCO_REG_IDX_BP 2
#elif __x86_64__
#define UCO_REG_IDX_RETADDR 4
#define UCO_REG_IDX_SP 5
#define UCO_REG_IDX_BP 7
#elif __aarch64__
#define UCO_REG_IDX_RETADDR 0
#define UCO_REG_IDX_SP 1
#else
#error Architecture no supported
#endif

struct uco_save_stack {
    void*  ptr;
    size_t sz;
    size_t valid_sz;
    /* max copy size in bytes */
    size_t max_cpsz;
    /* how many times the share stack was copied to this save stack */
    size_t ct_save;
    /*  how many times this save stack was copied to the share stack */
    size_t ct_restore;
};

struct uco_share_stack {
    void *ptr;
    size_t sz;
    void *align_highptr;
    void *align_retptr;
    size_t align_validsz;
    size_t align_limit;
    struct uco *owner;

    char guard_page_enabled;
    void *real_ptr;
    size_t real_sz;
};

struct uco {
    /*
     * CPU registers state (callee-savec plus SP, PC)
     */
#ifdef __i386__
        void*  reg[6];
#elif __x86_64__
        void*  reg[8];
#elif __aarch64__
	void *reg[14];  // pc, sp, x19-x29, x30 (lr)
#endif
	struct uco *main_co;
	void *arg;
	bool is_end;

	void (*fp)(void);

	struct uco_save_stack save_stack;
	struct uco_share_stack *share_stack;
};

#if defined(__i386__) || defined(__x86_64__)
#define UCO_THREAD __thread
#else
#define UCO_THREAD
#endif

extern UCO_THREAD struct uco *_uco_current_co;

struct uco *uco_create(struct uco *main_co,
		       struct uco_share_stack *share_stack,
		       size_t save_stack_sz, void (*fp)(void),
		       void *arg);

struct uco_share_stack *uco_share_stack_new(size_t sz);

void uco_share_stack_destroy(struct uco_share_stack *sstk);

void *_ucosw(struct uco *from_co, struct uco *to_co) __asm__("_ucosw");

static inline void _uco_yield_to_main_co(struct uco *yield_co)
{
    assert(yield_co);
    assert(yield_co->main_co);
    _ucosw(yield_co, yield_co->main_co);
}

static inline void uco_yield(void)
{
	_uco_yield_to_main_co(_uco_current_co);
}

static inline struct uco *uco_get_co()
{
	return _uco_current_co;
}
static inline void *uco_get_arg()
{
	return uco_get_co()->arg;
}

static inline bool uco_is_main_co(struct uco *co)
{
	return !co->main_co;
}

static inline void uco_exit(void)
{
	struct uco *co = uco_get_co();

	co->is_end = true;
	assert(co->share_stack->owner == co);
	co->share_stack->owner = NULL;
	co->share_stack->align_validsz = 0;
	_uco_yield_to_main_co(co);
	assert(false);
}

void uco_resume(struct uco *resume_co);
void uco_destroy(struct uco *co);

#endif /* _UCO_H_ */
