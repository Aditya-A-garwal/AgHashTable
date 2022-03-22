#include <gtest/gtest.h>
#include <type_traits>
#include <limits>

#define AG_TEST_MODE
#include "AgHashTable.h"

TEST (Smoke, SmokeTest)
{
    constexpr int64_t               lo  = -100'000;
    constexpr int64_t               hi  = 100'000;

    AgHashTable<int64_t>            table;

    ASSERT_TRUE (table.initialized ());

    for (auto i = lo; i <= hi; ++i) {
        ASSERT_TRUE (table.insert (i)) << "i: " << i << '\n';
        ASSERT_EQ (table.size (), i - lo + 1);
    }

    for (auto i = lo; i <= hi; ++i) {
        ASSERT_TRUE (table.find (i)) << "i: " << i << '\n';
    }

    for (auto i = lo; i <= hi; ++i) {
        ASSERT_TRUE (table.erase (i)) << "i: " << i << '\n';
        ASSERT_FALSE (table.find (i)) << "i: " << i << '\n';
        ASSERT_EQ (table.size (), hi - i);
    }

    for (auto i = lo; i <= hi; ++i) {
        ASSERT_FALSE (table.find (i)) << "i: " << i << '\n';
    }
}
