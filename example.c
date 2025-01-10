// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2018 Sen Han <00hnes@gmail.com>
 * Copyright 2025 Linaro Limited
 */

/*
 * This file illustrates the usage of the uco_*() coroutine functions.
 * It create a main coroutine and three secondary coroutines, to be scheduled
 * from the main one. The first of the three has its own stack, while the two
 * others share the same stack (thus requiring some copy of the stack content
 * when switching to a coroutine and resuming to the main one).
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "uco.h"

#define LOOPS 3

void foo(int ct)
{
	int *cnt = (int *)uco_get_arg();
	printf("co:%p save_stack:%p share_stack:%p in foo(%d), yielding\n",
		uco_get_co(), uco_get_co()->save_stack.ptr,
		uco_get_co()->share_stack->ptr, ct);
	uco_yield();
	(*cnt)++;
}

void co_fp0()
{
	int ct;
	struct uco *this_co = uco_get_co();

	assert(!uco_is_main_co(this_co));
	assert(this_co->fp == (void*)co_fp0);
	assert(this_co->is_end == 0);

	for (ct = 0 ; ct < LOOPS; ct++)
		foo(ct);

	printf("co:%p save_stack:%p share_stack:%p calling uco_exit()\n", this_co,
	       this_co->save_stack.ptr, this_co->share_stack->ptr);
	uco_exit();
	assert(0);
}

int main() {
	struct uco *main_co, *co, *co2, *co3;
	struct uco_share_stack *sstk, *sstk2;
	int co_ct_arg_point_to_me = 0;
	int co2_ct_arg_point_to_me = 0;
	int co3_ct_arg_point_to_me = 0;
	int ct;

	main_co = uco_create(NULL, NULL, 0, NULL, NULL);
	assert(main_co);

	sstk = uco_share_stack_new(0);
	assert(sstk);
	sstk2 = uco_share_stack_new(0);
	assert(sstk2);

	co = uco_create(main_co, sstk, 0, co_fp0, &co_ct_arg_point_to_me);
	assert(co);
	co2 = uco_create(main_co, sstk2, 0, co_fp0, &co2_ct_arg_point_to_me);
	assert(co2);
	co3 = uco_create(main_co, sstk2, 0, co_fp0, &co3_ct_arg_point_to_me);
	assert(co3);

	for (ct = 0; ct < LOOPS; ct++){
		assert(!co->is_end);
		printf("main_co: resuming co\n");
		uco_resume(co);
		assert(co_ct_arg_point_to_me == ct);

		assert(!co2->is_end);
		printf("main_co: resuming co2\n");
		uco_resume(co2);
		assert(co2_ct_arg_point_to_me == ct);

		assert(!co3->is_end);
		printf("main_co: resuming co3\n");
		uco_resume(co3);
		assert(co3_ct_arg_point_to_me == ct);
	}

	// Each secondary coroutine should have looped LOOPS times, next
	// time they are resumed they should exit
	printf("main_co: resuming co\n");
	uco_resume(co);
	assert(co_ct_arg_point_to_me == ct);
	assert(co->is_end);

	printf("main_co: resuming co2\n");
	uco_resume(co2);
	assert(co2_ct_arg_point_to_me == ct);
	assert(co2->is_end);

	printf("main_co: resuming co3\n");
	uco_resume(co3);
	assert(co3_ct_arg_point_to_me == ct);
	assert(co3->is_end);

	// co is using a shared stack shared that is actually shared with no
	// other coroutine, so no stack copy should have occured
	assert(co->save_stack.max_cpsz == 0);
	assert(co->save_stack.ct_save == 0);
	assert(co->save_stack.ct_restore == 0);

	// co2 and co3 however share the same stack so saving and restoring
	// stack content is expected to have happened on each yield and
	// resume
	assert(co2->save_stack.max_cpsz > 0);
	assert(co2->save_stack.ct_save == LOOPS);
	assert(co2->save_stack.ct_restore == LOOPS);
	assert(co3->save_stack.max_cpsz > 0);
	assert(co3->save_stack.ct_save == LOOPS);
	assert(co3->save_stack.ct_restore == LOOPS);

	uco_destroy(co);
	uco_destroy(co2);
	uco_destroy(co3);

	uco_share_stack_destroy(sstk);
	uco_share_stack_destroy(sstk2);

	uco_destroy(main_co);

	return 0;
}
