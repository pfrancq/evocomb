/*

    EvoComb

    RThreadPool.cpp

    Pool of threads - Implementation.

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
// include files for EvoComb
#include "tools/r_thread_pool.hpp"
#include "app/r_logger.hpp"
#include "tools/r_exception.hpp"
using namespace evocomb;


//------------------------------------------------------------------------------
mLogModule("r.core");



//------------------------------------------------------------------------------
//
// class RThreadPool
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
RThreadPool::RThreadPool(const std::size_t nbThreads)
{
    if (nbThreads == 0)
        mThrowRException("The number of threads must be strictly positive");
    workers_.reserve(nbThreads - 1);
    for (std::size_t thread = 1; thread < nbThreads; ++thread)
        workers_.emplace_back(&RThreadPool::worker, this, thread);
}


//------------------------------------------------------------------------------
RThreadPool::~RThreadPool()
{
    {
        const std::lock_guard lock(mutex_);
        stop_ = true;
    }
    start_.notify_all();
    for (auto& worker : workers_)
        worker.join();
}


//------------------------------------------------------------------------------
void RThreadPool::run(const std::size_t nbTasks, const std::function<void(std::size_t thread, std::size_t task)>& task)
{
    if (nbTasks == 0)
        return;

    // Single thread (or single task): execute directly.
    if (workers_.empty() || (nbTasks == 1))
    {
        for (std::size_t i = 0; i < nbTasks; ++i)
            task(0, i);
        return;
    }

    // Publish the job and wake up the threads
    {
        const std::lock_guard lock(mutex_);
        task_ = &task;
        nbTasks_ = nbTasks;
        next_.store(0);
        error_ = nullptr;
        active_ = workers_.size();
        ++generation_;
    }
    start_.notify_all();

    // The calling thread works too
    work(0);

    // Wait for the other threads
    std::exception_ptr error;
    {
        std::unique_lock lock(mutex_);
        done_.wait(lock, [this] { return active_ == 0; });
        task_ = nullptr;
        error = error_;
        error_ = nullptr;
    }
    if (error)
        std::rethrow_exception(error);
}


//------------------------------------------------------------------------------
void RThreadPool::worker(const std::size_t thread)
{
    std::size_t seen = 0;
    while (true)
    {
        {
            std::unique_lock lock(mutex_);
            start_.wait(lock, [this, seen] { return stop_ || (generation_ != seen); });
            if (stop_)
                return;
            seen = generation_;
        }
        work(thread);
        {
            const std::lock_guard lock(mutex_);
            if (--active_ == 0)
                done_.notify_one();
        }
    }
}


//------------------------------------------------------------------------------
void RThreadPool::work(const std::size_t thread)
{
    while (true)
    {
        const auto i = next_.fetch_add(1);
        if (i >= nbTasks_)
            return;
        try
        {
            (*task_)(thread, i);
        }
        catch (...)
        {
            // Keep the first exception and prevent the remaining tasks from being started.
            const std::lock_guard lock(mutex_);
            if (!error_)
                error_ = std::current_exception();
            next_.store(nbTasks_);
        }
    }
}
