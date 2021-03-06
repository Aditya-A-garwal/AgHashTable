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
id (const uint64_t *pPtr)
{
    return *pPtr;
}

int
main (void)
{
    AgHashTable<uint64_t, id>   table;
    // std::unordered_set<uint64_t>    table;
    // uint64_t                    cntr    {0ULL};

    auto start              = chrono::high_resolution_clock::now();

    for (uint64_t i = 0; i < 10'000'000; ++i) {
        table.insert (i);
        // cntr += (uint64_t) table.insert (i);
        // cntr += (uint64_t) (table.insert (i).second);
    }

    auto end              = chrono::high_resolution_clock::now();

    std::cout << "Time elapsed: " << (chrono::duration_cast<chrono::milliseconds>(end - start).count ()) << "us \n";
#if defined (AG_DBG_MODE)
    std::cout << "Allocations: " << table.get_alloc_count () << '\n';
    std::cout << "Deletions: " << table.get_delete_count () << '\n';
    std::cout << "Allocation amount: " << table.get_alloc_amount () << '\n';
    std::cout << "Resizes: " << table.get_resize_count () << '\n';
#endif

    return 0;
}
