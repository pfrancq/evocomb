/*

    EvoComb

    RThreadPool.hpp

    Pool of threads - Header.

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
#pragma once


//------------------------------------------------------------------------------
// include files for ANSI C/C++
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>


//------------------------------------------------------------------------------
namespace evocomb {
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/**
 * The RThreadPool class provides a fixed pool of threads that execute a set of tasks in parallel. The threads are
 * created once (by the constructor) and reused for each call to run.
 *
 * The calling thread participates in the execution: a pool of n threads creates n-1 additional threads. With a single
 * thread, the tasks are simply executed by the calling thread (no additional thread is created).
 *
 * Each task receives the index of the thread executing it (in [0, nbThreads()[, 0 being the calling thread), so that it
 * can use data specific to that thread:
 * @code
 * evocomb::RThreadPool pool(4);
 * std::vector<Workspace> workspaces(pool.nbThreads());
 * pool.run(tasks.size(), [&](std::size_t thread, std::size_t task) {
 *     compute(tasks[task], workspaces[thread]);
 * });
 * @endcode
 * The tasks are distributed dynamically: which thread executes which task is not deterministic.
 * @short Pool of threads.
 */
class RThreadPool
{
public:

    //-----------------------------------------------------
    using Task = const std::function<void(std::size_t, std::size_t)>;

    /**
     * Construct the pool.
     * @param nbThreads          Number of threads (including the calling thread). It must be strictly positive.
     * @exception RException     The number of threads is null.
     */
    explicit RThreadPool(std::size_t nbThreads);

    /**
     * Destruct the pool (the threads are stopped).
     */
    ~RThreadPool();

    /**
     * Get the number of threads (including the calling thread).
     */
    [[nodiscard]] std::size_t nbThreads() const { return workers_.size() + 1; }

    /**
     * Execute a set of tasks in parallel. The method returns when all the tasks are done. If a task throws an
     * exception, the remaining tasks are not started and the first exception is rethrown (once the tasks being executed
     * are done).
     *
     * The method must not be called concurrently (or from a task).
     * @param nbTasks            Number of tasks.
     * @param task               Function executing a task. Its parameters are the index of the thread and the index of
     *                           the task (in [0, nbTasks[).
     */
    void run(std::size_t nbTasks, const Task& task);

    //-----------------------------------------------------
    RThreadPool(const RThreadPool&) = delete;
    RThreadPool& operator=(const RThreadPool&) = delete;

private:

    /**
     * Main loop of an additional thread.
     * @param thread             Index of the thread.
     */
    void worker(std::size_t thread);

    /**
     * Execute tasks of the current job until there are no more.
     * @param thread             Index of the thread.
     */
    void work(std::size_t thread);

    //-----------------------------------------------------
    std::vector<std::thread> workers_;      ///< Additional threads.
    std::mutex mutex_;                      ///< Mutex protecting the state.
    std::condition_variable start_;         ///< Signal a new task to the threads.
    std::condition_variable done_;          ///< Signal the end of the job to the caller.
    Task* task_ = nullptr;                  ///< Current task.
    std::size_t nbTasks_ = 0;               ///< Number of tasks of the current task.
    std::atomic<std::size_t> next_{0};   ///< Next task to execute.
    std::size_t generation_ = 0;            ///< Number of tasks started.
    std::size_t active_ = 0;                ///< Additional threads still working on the task.
    bool stop_ = false;                     ///< Must the threads stop?
    std::exception_ptr error_;              ///< First exception thrown by a task.
};


}  //------- End of namespace evocomb ----------------------------------------------
