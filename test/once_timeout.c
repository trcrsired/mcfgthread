/* This file is part of MCF Gthread.
 * See LICENSE.TXT for licensing information.
 * Copyleft 2022, LH_Mouse. All wrongs reserved.  */

#include "../src/once.h"
#include "../src/clock.h"
#include <assert.h>
#include <stdio.h>

static _MCF_once once;

int
main(void)
  {
    double now, delta;
    int r;

    now = _MCF_perf_counter();
    r = _MCF_once_wait(&once, (const int64_t[]){ _MCF_utc_now() + 1100 });  /* absolute  */
    assert(r == 1);
    delta = _MCF_perf_counter() - now;
    printf("delta = %.6f\n", delta);
    assert(delta <= 100);

    now = _MCF_perf_counter();
    r = _MCF_once_wait(&once, (const int64_t[]){ _MCF_utc_now() + 1100 });  /* absolute  */
    assert(r == -1);
    delta = _MCF_perf_counter() - now;
    printf("delta = %.6f\n", delta);
    assert(delta >= 1000);
    assert(delta <= 1200);

    now = _MCF_perf_counter();
    r = _MCF_once_wait(&once, (const int64_t[]){ -1100 });  /* relative  */
    assert(r == -1);
    delta = _MCF_perf_counter() - now;
    printf("delta = %.6f\n", delta);
    assert(delta >= 1000);
    assert(delta <= 1200);
  }
