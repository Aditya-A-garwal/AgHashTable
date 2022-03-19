#include <gtest/gtest.h>
#include <type_traits>
#include <limits>

#define AG_TEST_MODE
#include "AgHashTable.h"


/**
 * @brief                       Simple function to get the first 16 bits of an integer
 *
 * @tparam val_t                Type of integer to accept as parameter
 *
 * @param pKey                  Key to get the hash of
 * @return uint16_t             First 16 bits of pKey
 */
template <typename val_t>
uint16_t
simpleIntHash (const val_t pKey)
{
    static_assert (std::is_unsigned<val_t>::value or std::is_signed<val_t>::value);
    return *(uint16_t *)&pKey;
}

TEST (Smoke, SmokeTest)
{
    constexpr int64_t                       lo  = -100'000;
    constexpr int64_t                       hi  = 100'000;

    AgHashTable<uint64_t, simpleIntHash<uint64_t>>    table;

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
