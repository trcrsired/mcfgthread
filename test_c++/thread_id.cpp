/* This file is part of MCF Gthread.
 * See LICENSE.TXT for licensing information.
 * Copyleft 2022, LH_Mouse. All wrongs reserved.  */

#include "../src/cxx11.hpp"
#include "../src/exit.h"
#include <assert.h>
#include <stdio.h>

#ifdef TEST_STD
#  include <thread>
namespace NS = std;
#else
namespace NS = ::_MCF;
#endif

int
main(void)
  {
    const auto main_tid = NS::this_thread::get_id();
    assert(main_tid._M_tid == ::_MCF_thread_self_tid());

    NS::thread thr(
      [&] {
        const auto my_tid = NS::this_thread::get_id();
        assert(my_tid == thr.get_id());
        assert(my_tid != main_tid);
        assert(my_tid < main_tid || my_tid > main_tid);
      });

    thr.join();
    assert(thr.get_id() == NS::thread::id());
  }
