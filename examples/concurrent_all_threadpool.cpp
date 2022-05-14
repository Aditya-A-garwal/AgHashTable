/**
 * @file            concurrent_all.cpp
 * @author          Aditya Agarwal (aditya.agarwal@dumblebots.com)
 * @brief           Perform insert (modifier), erase (modifier) and find (non-modifier) operations simultaneously on the table.
 *
 *                  Simultaneously insert into, delete from and find keys in the table and report the results and the time taken.
 *                  Each operations is off loaded to a threadpool (Each operations takes place on an individual thread), and are intended to be executed concurrently.
 *
 *                  Locking is done at the level of individual buckets.
 */

#include <cstdio>
#include <iostream>

#include <thread>
#include <atomic>
#include <mutex>

#include <chrono>
#include <random>

#include <queue>

#include <cassert>

// #define AG_DBG_MODE
#define AG_MULTITHREADED_MODE
#include "AgHashTable.h"

namespace   chrono  = std::chrono;

constexpr int32_t   NUM_ELEMENTS    = 1'000'000;        /** Number of keys to use per operation */
// constexpr int32_t   ELEMENT_RANGE   = 1'000'000;        /** Range of keys, includes 0, does not include ELEMENT_RANGE */

template <typename first_t, typename second_t>
struct pair {

    first_t     first;
    second_t    second;

    pair () = default;

    pair (const first_t &pFirst, const second_t &pSecond) : first {pFirst}, second {pSecond}
    {}
};

template <typename func_t, typename arg_t>
class ThreadPool {


public:


    ThreadPool (uint64_t pNumThreads) : mNumThreads {pNumThreads} {
        mThreads        = (std::thread *) malloc (sizeof (std::thread) * pNumThreads);

        if (mThreads == nullptr) {
            std::cout << "Could not init threadpool (allocation failed)!\n";
            std::exit (1);
        }

        for (uint64_t i = 0; i < pNumThreads; ++i) {
            new (mThreads + i) std::thread ([this] () { startJob (); });
        }

        mFinished.store (false, std::memory_order_relaxed);
    }

    ~ThreadPool () {

        for (uint64_t i = 0; i < mNumThreads; ++i) {
            mThreads[i].join ();
            mThreads[i].std::thread::~thread ();
        }

        free ((void *)mThreads);
    }

    void
    queueJob (func_t func, arg_t arg)
    {
        mJobsMutex.lock ();
        mJobs.emplace (func, arg);
        mJobsMutex.unlock ();
    }

    void
    waitOnQueue ()
    {
        while (1) {

            mJobsMutex.lock ();

            if (!mJobs.empty ()) {
                mJobsMutex.unlock ();
                std::this_thread::yield ();
                continue;
            }

            mJobsMutex.unlock ();
            break;
        }

        mFinished.store (true, std::memory_order_seq_cst);
    }

    uint64_t
    numThreads ()
    {
        return mNumThreads;
    }


private:


    uint64_t                            mNumThreads;

    std::queue<pair<func_t, arg_t>>     mJobs;
    std::mutex                          mJobsMutex;

    std::atomic<bool>                   mFinished;

    std::thread                         *mThreads;

    void
    startJob () {

        pair<func_t, arg_t> job;
        // func_t              func;
        // arg_t               arg;

        while (!mFinished.load (std::memory_order_seq_cst)) {

            mJobsMutex.lock ();

            if (mJobs.empty ()) {
                mJobsMutex.unlock ();

                std::this_thread::yield ();
            }
            else {
                job     = mJobs.front ();
                mJobs.pop ();

                mJobsMutex.unlock ();

                job.first (job.second);
            }
        }
    }
};

AgHashTable<int32_t>    table;
std::atomic<int32_t>    cntr;

void
insert (int32_t &pVal)
{
    assert (table.insert (pVal));
}

void
find (int32_t &pVal)
{
    cntr.fetch_add ((int32_t) table.exists (pVal), std::memory_order_seq_cst);
}

void
erase (int32_t &pVal)
{
    assert (table.erase (pVal));
}

int
main (void)
{
    chrono::high_resolution_clock::time_point   start;
    chrono::high_resolution_clock::time_point   end;

    ThreadPool<decltype (&insert), int32_t>      pool (std::thread::hardware_concurrency () / 2);

    cntr.store (0, std::memory_order_seq_cst);

    std::cout << "Using " << pool.numThreads () << " Threads\n";

    start       = chrono::high_resolution_clock::now ();

    for (int32_t i = 0; i < NUM_ELEMENTS; ++i) {
        pool.queueJob (insert, i);
        pool.queueJob (find, i);
        pool.queueJob (erase, i);
    }

    pool.waitOnQueue ();

    end         = chrono::high_resolution_clock::now ();

    std::cout << cntr << std::endl;
    std::cout << (chrono::duration_cast<chrono::milliseconds> (end - start).count ()) << " us\n";
#if defined (AG_DBG_MODE)
    std::cout << "Allocations: " << table.getAllocCount () << '\n';
    std::cout << "Allocation amount: " << table.getAllocAmount () << '\n';
#endif

    return 0;
}
