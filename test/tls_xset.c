/* This file is part of MCF Gthread.
 * See LICENSE.TXT for licensing information.
 * Copyleft 2022, LH_Mouse. All wrongs reserved.  */

#include "../src/thread.h"
#include <assert.h>
#include <stdio.h>

static _MCF_tls_key* key;
static _MCF_thread* thrd;
static int dso_1, dso_2;

static
void
thread_proc(_MCF_thread* self)
  {
    void* p;
    int r;

    p = _MCF_tls_get(key);
    assert(p == NULL);

    p = &r;
    r = _MCF_tls_xset(key, &p, &dso_2);
    assert(r == 0);
    assert(p == NULL);

    p = _MCF_tls_get(key);
    assert(p == &dso_2);

    p = &r;
    r = _MCF_tls_xset(key, &p, NULL);
    assert(r == 0);
    assert(p == &dso_2);

    _MCF_sleep((const int64_t[]) { -1000 });
    printf("thread %d quitting\n", self->__tid);
  }

int
main(void)
  {
    void* p;
    int r;

    key = _MCF_tls_key_new(NULL);
    assert(key);
    assert(_MCF_tls_key_get_destructor(key) == NULL);

    p = _MCF_tls_get(key);
    assert(p == NULL);

    p = &r;
    r = _MCF_tls_xset(key, &p, NULL);
    assert(r == 0);
    assert(p == NULL);

    p = &r;
    r = _MCF_tls_xset(key, &p, &dso_1);
    assert(r == 0);
    assert(p == NULL);

    p = _MCF_tls_get(key);
    assert(p == &dso_1);

    p = &r;
    r = _MCF_tls_xset(key, &p, &dso_2);
    assert(r == 0);
    assert(p == &dso_1);

    thrd = _MCF_thread_new(thread_proc, NULL, sizeof(int));
    assert(thrd);

    printf("main waiting\n");
    _MCF_thread_wait(thrd, NULL);
    printf("main wait finished\n");

    p = _MCF_tls_get(key);
    assert(p == &dso_2);

    p = &r;
    r = _MCF_tls_xset(key, &p, NULL);
    assert(r == 0);
    assert(p == &dso_2);
  }
