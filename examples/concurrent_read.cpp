#include <cstdio>
#include <iostream>

#include <thread>
#include <atomic>
#include <mutex>

#include <chrono>
#include <random>

#define AG_HASH_TABLE_MULTITHREADED_MODE
#include "AgHashTable.h"

#define NUM_ELEMENTS                        8'000'000
#define NUM_THREADS                         8

namespace   chrono = std::chrono;

AgHashTable<int32_t>    table;
std::thread             *threads;

std::mutex              gStdOutMutex;

void
find (int32_t pLo, int32_t pHi, int32_t pId = 0)
{
    int32_t     res     {0};

    for (int32_t i = pLo; i < pHi; ++i) {
        res += (int32_t) table.find (i);
    }

    gStdOutMutex.lock ();
    std::cout << "Successful searches on #" << pId << ": " << res << '\n';
    gStdOutMutex.unlock ();
}

int
main (void)
{

    if (!table.initialized ()) {
        std::cout << "Could not initialize table\n";
        return 1;
    }

    for (int32_t i = 0; i < (NUM_ELEMENTS); ++i) {
        if (!table.insert (i)) {
            std::cout << "Could not insert " << i << '\n';
            return 1;
        }
    }

    threads         = (std::thread *) malloc ((NUM_THREADS) * sizeof (std::thread));

    if (threads == nullptr) {
        std::cout << "Could not allocate array of threads\n";
        return 1;
    }

    auto    start   = chrono::high_resolution_clock::now ();

    for (int32_t i = 0; i < (NUM_THREADS); ++i) {
        new (threads + i) std::thread (find, 0, (NUM_ELEMENTS), i);
    }

    for (int32_t i = 0; i < (NUM_THREADS); ++i) {
        threads[i].join ();
    }

    auto    end     = chrono::high_resolution_clock::now ();

    for (int32_t i = 0; i < (NUM_THREADS); ++i) {
        threads[i].std::thread::~thread ();
    }
    free (threads);

    std::cout << "Time elapsed: " << chrono::duration_cast<chrono::milliseconds> (end - start).count () << '\n';

    return 0;
}
