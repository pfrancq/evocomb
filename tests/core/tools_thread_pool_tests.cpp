/*

    EvoComb

    RThreadPoolTests.cpp

    Copyright 2026 by Pascal Francq (pascal@francq.info).

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2.0 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library, as a file COPYING.LIB; if not, write
    to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
    Boston, MA  02111-1307  USA

*/



//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <atomic>
#include <stdexcept>
#include <vector>


//------------------------------------------------------------------------------
// include files for EvoComb
#include "r_google_test.hpp"
#include "tools/r_exception.hpp"
#include "tools/r_thread_pool.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
class RThreadPoolTests : public RGoogleTest
{
};


//------------------------------------------------------------------------------
TEST_F(RThreadPoolTests, run)
{
    EXPECT_THROW(RThreadPool(0), RException);

    for (const std::size_t nbThreads : {1u, 4u})
    {
        RThreadPool pool(nbThreads);
        EXPECT_EQ(pool.nbThreads(), nbThreads);

        // Several jobs with the same pool: each task is done once, with a valid thread index.
        for (int job = 0; job < 20; ++job)
        {
            std::vector<int> done(1000, 0);
            std::atomic<bool> validThread{true};
            pool.run(done.size(), [&](const std::size_t thread, const std::size_t task) {
                if (thread >= nbThreads)
                    validThread = false;
                ++done[task];
            });
            EXPECT_TRUE(validThread);
            for (const auto d : done)
                EXPECT_EQ(d, 1);
        }

        // An exception thrown by a task is rethrown, and the pool is still usable afterwards.
        EXPECT_THROW(pool.run(100, [](std::size_t, const std::size_t task) {
            if (task == 50)
                throw std::runtime_error("task 50");
        }), std::runtime_error);
        std::atomic<std::size_t> count{0};
        pool.run(10, [&](std::size_t, std::size_t) { ++count; });
        EXPECT_EQ(count, 10u);
    }
}
