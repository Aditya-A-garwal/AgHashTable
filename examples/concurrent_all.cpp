/**
 * @file            concurrent_all.cpp
 * @author          Aditya Agarwal (aditya.agarwal@dumblebots.com)
 * @brief           Perform insert (modifier), erase (modifier) and find (non-modifier) operations simultaneously on the table.
 *
 *                  Simultaneously insert into, delete from and find keys in the table and report the results and the time taken.
 *                  The operations can be performed sequentially (no multi-threading involved) or concurrently.
 *                  If perfomed concurrently, the locking can be done at the table level or at the bucket level.
 *                  The behaviour of the program can be changed by changing macro definitions after the include directives.
 *
 *                  In case operations are performed concurrently, each type of operation is performed on a seperate thread, rather
 *                  than individual operations.
 */

#include <cstdio>
#include <iostream>

#include <thread>
#include <atomic>
#include <mutex>

#include <chrono>
#include <random>

// no multithreading - define linear
// mulithreading + no lock - define NO_LOCK
// mulithreading + table level locking - define TABLE_LOCK
// mulithreading + bucket level locking - define BUCKET_LOCK

#define LINEAR
// #define NO_LOCK
// #define TABLE_LOCK
// #define BUCKET_LOCK

#define AG_DBG_MODE
#if defined (BUCKET_LOCK)
#define AG_HASH_TABLE_MULTITHREADED_MODE
#endif

#include "AgHashTable.h"


namespace   chrono = std::chrono;


constexpr int32_t   NUM_ELEMENTS    = 10'000'000;       /** Number of keys to use per operation */
constexpr int32_t   ELEMENT_RANGE   = 1'000'000;        /** Range of keys, includes 0, does not include ELEMENT_RANGE */


AgHashTable<int32_t>    table;


class NoLock {

private:

    static std::mt19937 sGen;

public:

    static uint64_t     sCntr;

    static void     Init ();

    static void     Insert ();
    static void     Erase ();
    static void     Find ();
};

std::mt19937    NoLock::sGen;
uint64_t        NoLock::sCntr;

void
NoLock::Init ()
{
    NoLock::sCntr   = 0ULL;
}

void
NoLock::Insert ()
{
    for (int32_t i = 0; i < NUM_ELEMENTS; ++i) {
        table.insert (NoLock::sGen () % ELEMENT_RANGE);
    }
}

void
NoLock::Erase ()
{
    for (int32_t i = 0; i < NUM_ELEMENTS; ++i) {
        table.erase (NoLock::sGen () % ELEMENT_RANGE);
    }
}

void
NoLock::Find ()
{
    for (int32_t i = 0; i < NUM_ELEMENTS; ++i) {
        NoLock::sCntr   += (uint64_t) table.find (NoLock::sGen () % ELEMENT_RANGE);
    }

    std::cout << "NO LOCK: " << NoLock::sCntr << '\n';
}

class YesLock {

private:

    static std::mt19937 sGen;
    static std::mutex   sTableMutex;

public:

    static uint64_t sCntr;

    static void     Init ();

    static void     Insert ();
    static void     Erase ();
    static void     Find ();
};

std::mt19937    YesLock::sGen;
uint64_t        YesLock::sCntr;
std::mutex      YesLock::sTableMutex;

void
YesLock::Init ()
{
    YesLock::sCntr   = 0ULL;
}

void
YesLock::Insert ()
{
    for (int32_t i = 0; i < NUM_ELEMENTS; ++i) {
        sTableMutex.lock ();
        table.insert (YesLock::sGen () % ELEMENT_RANGE);
        sTableMutex.unlock ();
    }
}

void
YesLock::Erase ()
{
    for (int32_t i = 0; i < NUM_ELEMENTS; ++i) {
        sTableMutex.lock ();
        table.erase (YesLock::sGen () % ELEMENT_RANGE);
        sTableMutex.unlock ();
    }
}

void
YesLock::Find ()
{
    for (int32_t i = 0; i < NUM_ELEMENTS; ++i) {
        sTableMutex.lock ();
        YesLock::sCntr   += (uint64_t) table.find (YesLock::sGen () % ELEMENT_RANGE);
        sTableMutex.unlock ();
    }

    std::cout << "WHOLE TABLE MUTEX'd: " << YesLock::sCntr << '\n';
}


int
main (void)
{
    if (!table.initialized ()) {
        std::cout << "Could not initialize table\n";
        return 1;
    }

    chrono::high_resolution_clock::time_point   start;
    chrono::high_resolution_clock::time_point   end;

    start   = chrono::high_resolution_clock::now ();

#if defined (LINEAR)

    NoLock::Init ();

    NoLock::Insert ();
    NoLock::Find ();
    NoLock::Erase ();

#elif defined (NO_LOCK) || defined (BUCKET_LOCK)

    NoLock::Init ();

    std::thread     NoLockInsert (NoLock::Insert);
    std::thread     NoLockFind (NoLock::Find);
    std::thread     NoLockErase (NoLock::Erase);

    NoLockInsert.join ();
    NoLockErase.join ();
    NoLockFind.join ();

#elif defined (TABLE_LOCK)

    YesLock::Init ();

    std::thread     YesLockInsert (YesLock::Insert);
    std::thread     YesLockFind (YesLock::Find);
    std::thread     YesLockErase (YesLock::Erase);

    YesLockInsert.join ();
    YesLockErase.join ();
    YesLockFind.join ();

#endif

    end     = chrono::high_resolution_clock::now ();

    std::cout << "Time elapsed: " << (chrono::duration_cast<chrono::milliseconds>(end - start).count ()) << "us \n";
#if defined (AG_DBG_MODE)
    std::cout << "Allocations: " << table.getAllocCount () << '\n';
    std::cout << "Allocation amount: " << table.getAllocAmount () << '\n';
#endif

    return 0;
}
