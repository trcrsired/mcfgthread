// This file is part of MCF gthread.
// See LICENSE.TXT for licensing information.
// Copyleft 2022, LH_Mouse. All wrongs reserved.

#define __MCFGTHREAD_COND_C_  1
#include "cond.h"
#include "win32.h"

static void
do_wait_cleanup_common(_MCF_cond_unlock_callback* unlock_opt, intptr_t unlocked,
        _MCF_cond_relock_callback* relock_opt, intptr_t lock_arg)
  {
    if(unlock_opt && relock_opt)
      relock_opt(lock_arg, unlocked);
  }

int
_MCF_cond_wait(_MCF_cond* cond, _MCF_cond_unlock_callback* unlock_opt,
        _MCF_cond_relock_callback* relock_opt, intptr_t lock_arg,
        const int64_t* timeout_opt)
  {
    _MCF_cond new, old;
    NTSTATUS status;
    LARGE_INTEGER timeout = { 0 };
    LARGE_INTEGER* use_timeout = __MCF_initialize_timeout(&timeout, timeout_opt);

    // Allocate a count for the current thread.
    __atomic_load(cond, &old, __ATOMIC_RELAXED);
    do {
      new = old;
      new.__nsleep = (old.__nsleep + 1) & __MCF_COND_NSLEEP_M;
    }
    while(!__atomic_compare_exchange(cond, &old, &new, TRUE, __ATOMIC_RELAXED, __ATOMIC_RELAXED));

    // Now, invoke the unlock callback.
    // If another thread attempts to signal this one, it shall block.
    intptr_t unlocked = 42;
    if(unlock_opt)
      unlocked = unlock_opt(lock_arg);

    // Try waiting.
    status = NtWaitForKeyedEvent(NULL, cond, FALSE, use_timeout);
    __MCFGTHREAD_ASSERT(NT_SUCCESS(status));
    if(!use_timeout) {
      // The wait operation was infinite.
      do_wait_cleanup_common(unlock_opt, unlocked, relock_opt, lock_arg);
      return 0;
    }

    while(status == STATUS_TIMEOUT) {
      // Tell another thread which is going to signal this condition variable
      // that an old waiter has left by decrementing the number of sleeping
      // threads. But see below...
      __atomic_load(cond, &old, __ATOMIC_RELAXED);
      do {
        if(old.__nsleep == 0)
          break;

        new = old;
        new.__nsleep = (old.__nsleep - 1) & __MCF_COND_NSLEEP_M;
      }
      while(!__atomic_compare_exchange(cond, &old, &new, TRUE, __ATOMIC_RELAXED, __ATOMIC_RELAXED));

      if(old.__nsleep != 0) {
        // The operation has timed out.
        do_wait_cleanup_common(unlock_opt, unlocked, relock_opt, lock_arg);
        return -1;
      }

      // ... It is possible that a second thread has already decremented the
      // counter. If this does take place, it is going to release the keyed
      // event soon. We must wait again, otherwise we get a deadlock in the
      // second thread. Again, a third thread could start waiting for this
      // keyed event before us, so we set the timeout to zero. If we time out
      // again, the third thread will have incremented the number of sleeping
      // threads and we can try decrementing it again.
      LARGE_INTEGER zero = { 0 };
      status = NtWaitForKeyedEvent(NULL, cond, FALSE, &zero);
      __MCFGTHREAD_ASSERT(NT_SUCCESS(status));
    }

    // After the wakeup, relock the associated mutex, if any.
    do_wait_cleanup_common(unlock_opt, unlocked, relock_opt, lock_arg);
    return 0;
  }

size_t
_MCF_cond_signal_some(_MCF_cond* cond, size_t max)
  {
    // Get the number of threads to wake up.
    size_t nwoken;
    _MCF_cond new, old;

    __atomic_load(cond, &old, __ATOMIC_RELAXED);
    do {
      new = old;
      nwoken = _MCF_minz(old.__nsleep, max);
      new.__nsleep = (old.__nsleep - nwoken) & __MCF_COND_NSLEEP_M;
    }
    while(!__atomic_compare_exchange(cond, &old, &new, TRUE, __ATOMIC_RELAXED, __ATOMIC_RELAXED));

    return __MCF_batch_release_common(cond, old.__nsleep);
  }

size_t
_MCF_cond_signal_all(_MCF_cond* cond)
  {
    // Swap out all data.
    _MCF_cond new = { 0 };
    _MCF_cond old;
    __atomic_exchange(cond, &new, &old, __ATOMIC_RELAXED);

    return __MCF_batch_release_common(cond, old.__nsleep);
  }
