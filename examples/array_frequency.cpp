/**
 * @file                    array_distinct.cpp
 * @author                  Aditya Agarwal (aditya,agarwal@dumblebots.com)
 *
 * In this example, given an array, we need to find the number of distinct elements inside it without
 * modifying it
 * All the elements from the array be iterated over and inserted into a hash table one by one
 * Since only one copy of each element will be succesfully inserted into the table, the number of elements
 * in the tree after iterating over the array, is the answer
*/

#include <iostream>
#include <chrono>

#include <unordered_set>

#define AG_DBG_MODE
#include "AgHashTable.h"

namespace chrono = std::chrono;

uint64_t
id (const uint8_t *pPtr, const uint64_t &pLen)
{
    return (*(uint64_t *)pPtr) + pLen;
}

int
main (void)
{
    AgHashTable<uint64_t, id>   table;
    // std::unordered_set<uint64_t>    table;
    uint64_t                    cntr    {0ULL};

    auto start              = chrono::high_resolution_clock::now();

    for (uint64_t i = 0; i < 10'000'000; ++i) {
        cntr += (uint64_t) table.insert (i);
        // cntr += (uint64_t) (table.insert (i).second);
    }

    auto end              = chrono::high_resolution_clock::now();

    std::cout << "Time elapsed: " << (chrono::duration_cast<chrono::milliseconds>(end - start).count ()) << "us \n";
#if defined (AG_DBG_MODE)
    std::cout << "Allocations: " << table.getAllocCount () << '\n';
    std::cout << "Deletions: " << table.getDeleteCount () << '\n';
    std::cout << "Allocation amount: " << table.getAllocAmount () << '\n';
    std::cout << "Resizes: " << table.getResizeCount () << '\n';
#endif

    return 0;
}
