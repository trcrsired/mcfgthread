/* This file is part of MCF Gthread.
 * See LICENSE.TXT for licensing information.
 * Copyleft 2022, LH_Mouse. All wrongs reserved.  */

#include "../src/sem.h"
#include "../src/clock.h"
#include <assert.h>
#include <stdio.h>

static _MCF_sem sem = __MCF_0_INIT;

int
main(void)
  {
    assert(_MCF_sem_get(&sem) == 0);

    assert(_MCF_sem_signal_some(&sem, __MCF_SEM_VALUE_MAX / 2) == 0);
    assert(_MCF_sem_get(&sem) == __MCF_SEM_VALUE_MAX / 2);

    assert(_MCF_sem_signal_some(&sem, __MCF_SEM_VALUE_MAX / 2) == 0);
    assert(_MCF_sem_get(&sem) == __MCF_SEM_VALUE_MAX - 1);

    assert(_MCF_sem_signal_some(&sem, __MCF_SEM_VALUE_MAX) == -2);
    assert(_MCF_sem_get(&sem) == __MCF_SEM_VALUE_MAX - 1);

    assert(_MCF_sem_signal_some(&sem, 1) == 0);
    assert(_MCF_sem_get(&sem) == __MCF_SEM_VALUE_MAX);

    assert(_MCF_sem_signal_some(&sem, 1) == -2);
    assert(_MCF_sem_get(&sem) == __MCF_SEM_VALUE_MAX);

    assert(_MCF_sem_wait(&sem, NULL) == 0);
    assert(_MCF_sem_get(&sem) == __MCF_SEM_VALUE_MAX - 1);

    assert(_MCF_sem_signal_some(&sem, 1) == 0);
    assert(_MCF_sem_get(&sem) == __MCF_SEM_VALUE_MAX);

    assert(_MCF_sem_wait(&sem, NULL) == 0);
    assert(_MCF_sem_get(&sem) == __MCF_SEM_VALUE_MAX - 1);

    assert(_MCF_sem_signal_some(&sem, 2) == -2);
    assert(_MCF_sem_get(&sem) == __MCF_SEM_VALUE_MAX - 1);
  }
