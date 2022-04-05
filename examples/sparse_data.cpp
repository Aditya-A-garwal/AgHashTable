#include <cstdio>
#include <iostream>

#include <chrono>

// #define AG_DBG_MODE
#include "AgHashTable.h"

namespace chrono = std::chrono;

uint64_t
id (const void *pPtr, const uint64_t pLen)
{
    return *(uint64_t *) pPtr;
}

int
main (int argc, char *argv[])
{

    AgHashTable<uint64_t, id>   table;

    if (!table.initialized ()) {
        std::cout << "Could not initialize table\n";
        return 1;
    }

    uint64_t                    numInsertions       {0};

    auto                        start               = chrono::high_resolution_clock::now ();

    for (uint64_t i = 0; i < 10'000'000ULL; ++i) {
        numInsertions       += (uint64_t) table.insert (100'000'000ULL * i);
    }

    auto                        end                 = chrono::high_resolution_clock::now ();

    std::cout << "Successful insertions: " << numInsertions << '\n';
    std::cout << "Time elapsed: " << chrono::duration_cast<chrono::milliseconds> (end - start).count () << "us \n";

    return 0;
}
