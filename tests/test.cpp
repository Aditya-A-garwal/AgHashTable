//! collect more data on aggregagte nodes
//! test allocations as well
//! it should be possible to manually specify the bitness of the hash function

#include <gtest/gtest.h>
#include <type_traits>
#include <limits>

#define AG_DBG_MODE
#include "AgHashTable.h"

/**
 * @brief
 *
 * @tparam int_t
 *
 * @param pKey
 *
 * @return uint8_t
 */
template <typename int_t>
uint8_t
is_odd (const int_t *pKey)
{
    return (uint8_t)((*pKey) & 1);
}

/**
 * @brief                   Smoke test
 *
 */
TEST (Smoke, SmokeTest)
{
    constexpr int64_t               lo  = -100'000;
    constexpr int64_t               hi  = 100'000;

    AgHashTable<int64_t>            table;

    ASSERT_TRUE (table.initialized ());

    for (auto i = lo; i <= hi; ++i) {
        ASSERT_TRUE (table.insert (i)) << "i: " << i << '\n';
        ASSERT_EQ (table.size (), i - lo + 1);
        ASSERT_EQ (table.get_key_count (), i - lo + 1);
    }

    for (auto i = lo; i <= hi; ++i) {
        ASSERT_TRUE (table.exists (i)) << "i: " << i << '\n';
    }

    for (auto i = lo; i <= hi; ++i) {
        ASSERT_TRUE (table.erase (i)) << "i: " << i << '\n';
        ASSERT_FALSE (table.exists (i)) << "i: " << i << '\n';
        ASSERT_EQ (table.size (), hi - i) << i << '\n';
        ASSERT_EQ (table.get_key_count (), hi - i) << i << '\n';
    }

    for (auto i = lo; i <= hi; ++i) {
        ASSERT_FALSE (table.exists (i)) << "i: " << i << '\n';
        ASSERT_EQ (table.find (i), table.end ());
    }

    ASSERT_LE (table.get_bucket_count (), table.get_max_bucket_count ());
}

/**
 * @brief                   Test Insertion with one aggregate node per bucket and one node per aggregate node (no collisions)
 *
 */
TEST (Insert, singleAggregateSingleNode)
{
    AgHashTable<int64_t, is_odd<int64_t>>   table;
    uint64_t                                bucketCountInit;

    ASSERT_TRUE (table.initialized ());

    bucketCountInit     = table.get_bucket_count ();

    // insert 0 (hash=0) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (0));
    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_key_count (), 1);

    // make sure the table has not been resized unecessarily
    ASSERT_EQ (table.get_resize_count (), 0);
    ASSERT_EQ (table.get_bucket_count (), bucketCountInit);

    // make sure the first bucket has the key
    ASSERT_EQ (table.get_bucket_key_count (0), 1);
    ASSERT_EQ (table.get_bucket_hash_count (0), 1);

    // make sure no other bucket has any other keys
    for (uint64_t bucket = 1; bucket < bucketCountInit; ++bucket) {
        ASSERT_EQ (table.get_bucket_key_count (bucket), 0);
        ASSERT_EQ (table.get_bucket_hash_count (bucket), 0);
    }

    // insert 0 (hash=0) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (1));
    ASSERT_EQ (table.size (), 2);
    ASSERT_EQ (table.get_key_count (), 2);

    // make sure the table has not been resized unecessarily
    ASSERT_EQ (table.get_resize_count (), 0);
    ASSERT_EQ (table.get_bucket_count (), bucketCountInit);

    // make sure the first bucket has the key
    ASSERT_EQ (table.get_bucket_key_count (1), 1);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // make sure no other bucket has any other keys
    for (uint64_t bucket = 2; bucket < bucketCountInit; ++bucket) {
        ASSERT_EQ (table.get_bucket_key_count (bucket), 0);
        ASSERT_EQ (table.get_bucket_hash_count (bucket), 0);
    }
}

/**
 * @brief                   Test insertion with one aggregate node per bucket but multiple nodes per aggregate node (collisions)
 *
 */
TEST (Insert, singleAggregateMultiNode)
{
    AgHashTable<int64_t, is_odd<int64_t>>   table;
    uint64_t                                bucketCountInit;

    ASSERT_TRUE (table.initialized ());

    bucketCountInit     = table.get_bucket_count ();

    // insert 0 (hash=0) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (0));
    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_key_count (), 1);

    // insert 2 (hash=0) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 2 (no new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (2));
    ASSERT_EQ (table.size (), 2);
    ASSERT_EQ (table.get_key_count (), 2);

    // make sure that the table was not resized unecessarily
    ASSERT_EQ (table.get_resize_count (), 0);
    ASSERT_EQ (table.get_bucket_count (), bucketCountInit);

    // make sure the first bucket has the newly inserted keys
    ASSERT_EQ (table.get_bucket_key_count (0), 2);
    ASSERT_EQ (table.get_bucket_hash_count (0), 1);

    // make sure no other buckets have any keys
    for (uint64_t bucket = 1; bucket < bucketCountInit; ++bucket) {
        ASSERT_EQ (table.get_bucket_key_count (bucket), 0);
        ASSERT_EQ (table.get_bucket_hash_count (bucket), 0);
    }

    // insert 1 (hash=1) and check if the insertion was successful and table key counts are consistent
    // the key count in the second bucket should now be 1 (a new aggreagte node should have been created for this key)
    ASSERT_TRUE (table.insert (1));
    ASSERT_EQ (table.size (), 3);
    ASSERT_EQ (table.get_key_count (), 3);

    // Insert 3 (hash=1) and check if the insertion was successful and table key counts are consistent
    // the kkey count in the second bucket should now be 2 (no new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (3));
    ASSERT_EQ (table.size (), 4);
    ASSERT_EQ (table.get_key_count (), 4);

    // make sure that the table was not resized unecessarily
    ASSERT_EQ (table.get_resize_count (), 0);
    ASSERT_EQ (table.get_bucket_count (), bucketCountInit);

    // make sure the second bucket has newly inserted keys
    ASSERT_EQ (table.get_bucket_key_count (1), 2);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // make sure no other buckets have any keys
    for (uint64_t bucket = 2; bucket < bucketCountInit; ++bucket) {
        ASSERT_EQ (table.get_bucket_key_count (bucket), 0);
        ASSERT_EQ (table.get_bucket_hash_count (bucket), 0);
    }
}

/**
 * @brief                   Test Insertion with multiple aggregate nodes per bucket and multiple nodes per aggregate node
 *
 */
TEST (Insert, multiAggregateMultiNode)
{

}
