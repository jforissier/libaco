// SPDX-License-Identifier: Apache-2.0

// Copyright 2018 Sen Han <00hnes@gmail.com>
// Copyright 2025 Linaro Limited

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "uco.h"

/* Current co-routine */
UCO_THREAD struct uco *_uco_current_co;

struct uco_share_stack *uco_share_stack_new(size_t sz)
{
	struct uco_share_stack *p = calloc(1, sizeof(*p));
	uintptr_t u_p;

	if (!p)
		return NULL;

	if (!sz)
		sz = 1024 * 1024 * 2;
	if(sz < 4096)
		sz = 4096;

	assert(sz > 0);

	p->sz = sz;
	p->ptr = malloc(sz);
	if (!p->ptr) {
		free(p);
		return NULL;
	}

	p->owner = NULL;
	u_p = (uintptr_t)(p->sz - (sizeof(void*) << 1) + (uintptr_t)p->ptr);
	u_p = (u_p >> 4) << 4;
	p->align_highptr = (void*)u_p;
	p->align_retptr  = (void*)(u_p - sizeof(void*));
	assert(p->sz > (16 + (sizeof(void*) << 1) + sizeof(void*)));
	p->align_limit = p->sz - 16 - (sizeof(void*) << 1);

	return p;
}

void uco_share_stack_destroy(struct uco_share_stack *sstk){
	assert(sstk != NULL && sstk->ptr != NULL);
	free(sstk->ptr);
	sstk->ptr = NULL;
	free(sstk);
}

struct uco *uco_create(struct uco *main_co,
		       struct uco_share_stack *share_stack,
		       size_t save_stack_sz,
		       void (*fp)(void), void *arg)
{
	struct uco *p = malloc(sizeof(*p));
	assert(p);
	memset(p, 0, sizeof(*p));

	if (main_co) {
		assert(share_stack);
		p->share_stack = share_stack;
#ifdef __i386__
		// POSIX.1-2008 (IEEE Std 1003.1-2008) - General Information - Data Types - Pointer Types
		// http://pubs.opengroup.org/onlinepubs/9699919799.2008edition/functions/V2_chap02.html#tag_15_12_03
		p->reg[UCO_REG_IDX_RETADDR] = (void*)fp;
		// push retaddr
		p->reg[UCO_REG_IDX_SP] = p->share_stack->align_retptr;
#elif  __x86_64__
		p->reg[UCO_REG_IDX_RETADDR] = (void*)fp;
		p->reg[UCO_REG_IDX_SP] = p->share_stack->align_retptr;
#elif __aarch64__
		p->reg[UCO_REG_IDX_RETADDR] = (void *)fp;
		// FIXME setting to align_retptr causes a crash
		p->reg[UCO_REG_IDX_SP] = p->share_stack->align_highptr;
#endif
		p->main_co = main_co;
		p->arg = arg;
		p->fp = fp;
		if (!save_stack_sz)
			save_stack_sz = 64;
		p->save_stack.ptr = malloc(save_stack_sz);
		assert(p->save_stack.ptr);
		p->save_stack.sz = save_stack_sz;
		p->save_stack.valid_sz = 0;
	} else {
		p->main_co = NULL;
		p->arg = arg;
		p->fp = fp;
		p->share_stack = NULL;
		p->save_stack.ptr = NULL;
	}
	return p;
}

static void grab_share_stack(struct uco *resume_co)
{
	struct uco *owner_co = resume_co->share_stack->owner;

	if (owner_co) {
		assert(owner_co->share_stack == resume_co->share_stack);
		assert((uintptr_t)(owner_co->share_stack->align_retptr) >=
		       (uintptr_t)(owner_co->reg[UCO_REG_IDX_SP]));
		assert((uintptr_t)owner_co->share_stack->align_highptr -
				(uintptr_t)owner_co->share_stack->align_limit
			<= (uintptr_t)owner_co->reg[UCO_REG_IDX_SP]);
		owner_co->save_stack.valid_sz =
			(uintptr_t)owner_co->share_stack->align_retptr -
			(uintptr_t)owner_co->reg[UCO_REG_IDX_SP];
		if (owner_co->save_stack.sz < owner_co->save_stack.valid_sz) {
			free(owner_co->save_stack.ptr);
			owner_co->save_stack.ptr = NULL;
			do {
				owner_co->save_stack.sz <<= 1;
				assert(owner_co->save_stack.sz > 0);
			} while (owner_co->save_stack.sz <
				 owner_co->save_stack.valid_sz);
			owner_co->save_stack.ptr =
				malloc(owner_co->save_stack.sz);
			assert(owner_co->save_stack.ptr);
		}
		if (owner_co->save_stack.valid_sz > 0) {
			memcpy(owner_co->save_stack.ptr,
			       owner_co->reg[UCO_REG_IDX_SP],
			       owner_co->save_stack.valid_sz);
			owner_co->save_stack.ct_save++;
		}
		if (owner_co->save_stack.valid_sz >
		    owner_co->save_stack.max_cpsz)
			owner_co->save_stack.max_cpsz =
				owner_co->save_stack.valid_sz;
		owner_co->share_stack->owner = NULL;
		owner_co->share_stack->align_validsz = 0;
	}
	assert(!resume_co->share_stack->owner);
	assert(resume_co->save_stack.valid_sz <=
	       resume_co->share_stack->align_limit - sizeof(void *));
	if (resume_co->save_stack.valid_sz > 0) {
		memcpy((void*)
		       (uintptr_t)(resume_co->share_stack->align_retptr) -
				resume_co->save_stack.valid_sz,
		       resume_co->save_stack.ptr,
		       resume_co->save_stack.valid_sz);
		resume_co->save_stack.ct_restore++;
	}
	if (resume_co->save_stack.valid_sz > resume_co->save_stack.max_cpsz)
		resume_co->save_stack.max_cpsz = resume_co->save_stack.valid_sz;
	resume_co->share_stack->align_validsz =
		resume_co->save_stack.valid_sz + sizeof(void *);
	resume_co->share_stack->owner = resume_co;
}

void uco_resume(struct uco *resume_co)
{
	assert(resume_co && resume_co->main_co && !resume_co->is_end);

	if (resume_co->share_stack->owner != resume_co)
		grab_share_stack(resume_co);

	_uco_current_co = resume_co;
	_ucosw(resume_co->main_co, resume_co);
	_uco_current_co = resume_co->main_co;
}

void uco_destroy(struct uco *co){
	assert(co);
	if(uco_is_main_co(co)){
		free(co);
	} else {
		if(co->share_stack->owner == co){
			co->share_stack->owner = NULL;
			co->share_stack->align_validsz = 0;
		}
		free(co->save_stack.ptr);
		co->save_stack.ptr = NULL;
		free(co);
	}
}
