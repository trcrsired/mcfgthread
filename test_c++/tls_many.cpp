/* This file is part of MCF Gthread.
 * See LICENSE.TXT for licensing information.
 * Copyleft 2022, LH_Mouse. All wrongs reserved.  */

#include "../src/cxx11.hpp"
#include "../src/sem.h"
#include <assert.h>
#include <stdio.h>
#include <deque>

#ifdef TEST_STD
namespace NS = std;
#else
namespace NS = ::_MCF;
#endif

int
main()
  {
#ifdef TEST_STD
    return 77;  // skipped
#else
    std::deque<NS::thread_specific_ptr<int>> keys;

    constexpr std::size_t NKEYS = 1000U;
    while(keys.size() < NKEYS) {
      keys.emplace_back(nullptr);  // no destructor
      assert(!keys.back());
      assert(keys.back().get() == nullptr);
    }

    constexpr std::size_t NVALS = 10000U;
    for(std::size_t v = 0;  v != NVALS;  ++v) {
      // set and get
      for(std::size_t k = 0;  k != NKEYS;  ++k)
        keys.at(k).reset((int*) v);

      for(std::size_t k = 0;  k != NKEYS;  ++k)
        assert(keys.at(k).get() == (int*) v);
    }
#endif
  }
