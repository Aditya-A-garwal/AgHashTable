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

// #define LINEAR
// #define NO_LOCK
// #define TABLE_LOCK
#define BUCKET_LOCK

#define AG_DBG_MODE
#if defined (BUCKET_LOCK)
#define AG_HASH_TABLE_MULTITHREADED_MODE
#endif
#include "AgHashTable.h"

namespace   chrono = std::chrono;

AgHashTable<int32_t>    table;


class NoLock {

private:

    static std::mt19937 sGen;

public:

    static uint64_t sCntr;

    static void     Init ();

    static void     Insert ();
    static void     Erase ();
    static void     Find ();
};

std::mt19937 NoLock::sGen;
uint64_t NoLock::sCntr;

void
NoLock::Init ()
{
    NoLock::sCntr   = 0ULL;
}

void
NoLock::Insert ()
{
    for (int32_t i = 0; i < 10'000'000; ++i) {
        table.insert (NoLock::sGen () % 1'000'000);
    }
}

void
NoLock::Erase ()
{
    for (int32_t i = 0; i < 10'000'000; ++i) {
        table.erase (NoLock::sGen () % 1'000'000);
    }
}

void
NoLock::Find ()
{
    for (int32_t i = 0; i < 10'000'000; ++i) {
        NoLock::sCntr   += (uint64_t) table.find (NoLock::sGen () % 1'000'000);
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
    for (int32_t i = 0; i < 10'000'000; ++i) {
        sTableMutex.lock ();
        table.insert (YesLock::sGen () % 1'000'000);
        sTableMutex.unlock ();
    }
}

void
YesLock::Erase ()
{
    for (int32_t i = 0; i < 10'000'000; ++i) {
        sTableMutex.lock ();
        table.erase (YesLock::sGen () % 1'000'000);
        sTableMutex.unlock ();
    }
}

void
YesLock::Find ()
{
    for (int32_t i = 0; i < 10'000'000; ++i) {
        sTableMutex.lock ();
        YesLock::sCntr   += (uint64_t) table.find (YesLock::sGen () % 1'000'000);
        sTableMutex.unlock ();
    }

    std::cout << "WHOLE TABLE MUTEX'd: " << YesLock::sCntr << '\n';
}


int
main (int argc, char *argv[])
{
    if (!table.initialized ()) {
        std::cout << "Could not initialize table\n";
        return 1;
    }

    auto        start   = chrono::high_resolution_clock::now ();

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

    auto        end     = chrono::high_resolution_clock::now ();

    std::cout << "Time elapsed: " << (chrono::duration_cast<chrono::milliseconds>(end - start).count ()) << "us \n";

    return 0;
}

// a reader thread should add the reader lock
// a writer thread should add the writer lock
// a reader thread only waits on a writer lock
// a writer thread waits on both locks
