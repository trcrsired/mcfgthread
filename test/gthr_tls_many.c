/* This file is part of MCF Gthread.
 * See LICENSE.TXT for licensing information.
 * Copyleft 2022, LH_Mouse. All wrongs reserved.  */

#include "../src/gthr.h"
#include <assert.h>

int
main(void)
  {
#define NKEYS  1000U
    __gthread_key_t keys[NKEYS];
    for(size_t k = 0;  k != NKEYS;  ++k) {
      int r = __gthread_key_create(&keys[k], NULL);
      assert(r == 0);
      assert(keys[k]);
    }

#define NVALS  10000U
    for(size_t v = 0;  v != NVALS;  ++v) {
      for(size_t k = 0;  k != NKEYS;  ++k) {
        int r = __gthread_setspecific(keys[k], (void*) v);
        assert(r == 0);
      }

      for(size_t k = 0;  k != NKEYS;  ++k) {
        size_t p = (size_t) __gthread_getspecific(keys[k]);
        assert(p == v);
      }
    }
  }
