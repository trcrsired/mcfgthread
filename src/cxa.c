// This file is part of MCF gthread.
// See LICENSE.TXT for licensing information.
// Copyleft 2022, LH_Mouse. All wrongs reserved.

#define __MCFGTHREAD_CXA_C_  1
#include "cxa.h"
#include "once.h"
#include "mutex.h"
#include "dtorque.h"
#include "thread.h"
#include "win32.h"

int
__MCF_cxa_guard_acquire(int64_t* guard)
  {
    return _MCF_once_wait_slow((_MCF_once*) guard, NULL);
  }

void
__MCF_cxa_guard_release(int64_t* guard)
  {
    _MCF_once_release((_MCF_once*) guard);
  }

void
__MCF_cxa_guard_abort(int64_t* guard)
  {
    _MCF_once_abort((_MCF_once*) guard);
  }

int
__MCF_cxa_atexit(__MCF_cxa_dtor_union dtor, void* this, void* dso)
  {
    // Push the element to the global queue.
    _MCF_mutex_lock(&__MCF_cxa_atexit_mutex, NULL);
    __MCF_dtorelem elem = { (__MCF_dtor_generic*) dtor.__cdecl_ptr, this, dso };
    int err = __MCF_dtorque_push(&__MCF_cxa_atexit_queue, &elem);
    _MCF_mutex_unlock(&__MCF_cxa_atexit_mutex);
    return err;
  }

int
__MCF_cxa_thread_atexit(__MCF_cxa_dtor_union dtor, void* this, void* dso)
  {
    // Push the element to the thread-specific queue.
    _MCF_thread* self = _MCF_thread_self();
    if(!self)
      return -1;

    __MCF_dtorelem elem = { (__MCF_dtor_generic*) dtor.__cdecl_ptr, this, dso };
    int err = __MCF_dtorque_push(&(self->__atexit_queue), &elem);
    return err;
  }

void
__MCF_cxa_finalize(void* dso)
  {
  }
