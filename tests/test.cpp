/**
 * @file            test.cpp
 * @author          Aditya Agarwal (aditya.agarwal@dumblebots.com)
 *
 * @brief           Unit tests for AgHashTable class
 *
 */

#include <gtest/gtest.h>
#include <type_traits>
#include <limits>

#define AG_DBG_MODE
// #define AG_PRINT_INIT_INFO
#include "AgHashTable.h"

/**
 * @brief                   Returns the absoulute value of an integer
 *
 * @tparam int_t            Type of integer being supplied
 *
 * @param pKey              Pointer to the integer to get the absoulute value of
 *
 * @return uint64_t         Absoulute value of the given number
 */
template <typename int_t>
inline uint64_t
abs (const int_t *pKey)
{
    return (*pKey < 0) ? (-(*pKey)) : (*pKey);
}

/**
 * @brief                   Returns an integer modulo 2
 *
 * @tparam int_t            Type of integer being supplied
 *
 * @param pKey              Pointer to the integer whose modulo 2 is to be found
 *
 * @return uint8_t          Modulo 2 of the suppliec integer
 */
template <typename int_t>
inline uint8_t
mod2 (const int_t *pKey)
{
    return abs (pKey) & 1;
}

/**
 * @brief                   Returns an unsigned integer of any bitness as a an unsigned 64 bit integer
 *
 *                          Undefined behaviour if a signed integer is supplied
 *
 * @tparam uint_t           Type of unsigned integer begin supplied
 *
 * @param pKey              Pointer to the integer whose modulo 2 is to be found
 *
 * @return uint64_t         Given number as an unsigned 64 bit integer
 */
template <typename uint_t>
inline uint64_t
unsigned_identity (const uint_t *pKey)
{
    return (uint64_t)*pKey;
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
 * @brief                   Test inserting duplicate elements
 *
 */
TEST (Insert, duplicates)
{
    AgHashTable<int64_t>                    table;

    ASSERT_TRUE (table.insert (0));
    ASSERT_FALSE (table.insert (0));

    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_key_count (), 1);
    ASSERT_EQ (table.get_aggregate_count (), 1);
}

/**
 * @brief                   Test Insertion with one aggregate node (per bucket, not for all buckets) and one node per aggregate node (no collisions)
 *
 */
TEST (Insert, singleAggregateSingleNode)
{
    AgHashTable<int64_t, mod2<int64_t>>     table;
    int64_t                                 bucketCountInit;

    ASSERT_TRUE (table.initialized ());

    bucketCountInit     = (int64_t)table.get_bucket_count ();

    // insert 0 (hash=0, position=0) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (0));
    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), 0);
    EXPECT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = (int64_t)table.get_bucket_count ();

    // make sure the first bucket has the key
    ASSERT_EQ (table.get_bucket_key_count (0), 1);
    ASSERT_EQ (table.get_bucket_hash_count (0), 1);

    // make sure no other bucket has any other keys
    for (int64_t bucket = 1; bucket < bucketCountInit; ++bucket) {
        ASSERT_EQ (table.get_bucket_key_count (bucket), 0);
        ASSERT_EQ (table.get_bucket_hash_count (bucket), 0);
    }

    // insert 0 (hash=0, position=0) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (1));
    ASSERT_EQ (table.size (), 2);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), 0);
    EXPECT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = (int64_t)table.get_bucket_count ();

    // make sure the first bucket has the key
    ASSERT_EQ (table.get_bucket_key_count (1), 1);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // make sure no other bucket has any other keys
    for (int64_t bucket = 2; bucket < bucketCountInit; ++bucket) {
        ASSERT_EQ (table.get_bucket_key_count (bucket), 0);
        ASSERT_EQ (table.get_bucket_hash_count (bucket), 0);
    }
}

/**
 * @brief                   Test insertion with one aggregate node (per bucket, not for all buckets) but multiple nodes per aggregate node (collisions)
 *
 */
TEST (Insert, singleAggregateMultiNode)
{
    AgHashTable<int64_t, mod2<int64_t>>     table;
    int64_t                                 bucketCountInit;

    ASSERT_TRUE (table.initialized ());

    bucketCountInit     = (int64_t)table.get_bucket_count ();

    // insert 0 (hash=0, position=0) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (0));
    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // insert 2 (hash=0, position=0) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 2 (no new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (2));
    ASSERT_EQ (table.size (), 2);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), 0);
    EXPECT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = (int64_t)table.get_bucket_count ();

    // make sure the first bucket has the newly inserted keys
    ASSERT_EQ (table.get_bucket_key_count (0), 2);
    ASSERT_EQ (table.get_bucket_hash_count (0), 1);

    // make sure no other buckets have any keys
    for (int64_t bucket = 1; bucket < bucketCountInit; ++bucket) {
        ASSERT_EQ (table.get_bucket_key_count (bucket), 0);
        ASSERT_EQ (table.get_bucket_hash_count (bucket), 0);
    }

    // insert 1 (hash=1, position=1) and check if the insertion was successful and table key counts are consistent
    // the key count in the second bucket should now be 1 (a new aggreagte node should have been created for this key)
    ASSERT_TRUE (table.insert (1));
    ASSERT_EQ (table.size (), 3);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // Insert 3 (hash=1, position=1) and check if the insertion was successful and table key counts are consistent
    // the kkey count in the second bucket should now be 2 (no new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (3));
    ASSERT_EQ (table.size (), 4);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), 0);
    EXPECT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = (int64_t)table.get_bucket_count ();

    // make sure the second bucket has newly inserted keys
    ASSERT_EQ (table.get_bucket_key_count (1), 2);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // make sure no other buckets have any keys
    for (int64_t bucket = 2; bucket < bucketCountInit; ++bucket) {
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
    AgHashTable<int64_t, abs<int64_t>>      table;
    int64_t                                 bucketCountInit;
    int64_t                                 resizeCountInit;

    ASSERT_TRUE (table.initialized ());

    bucketCountInit     = (int64_t)table.get_bucket_count ();

    // insert 1 (hash=1, position=1) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (1));
    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // insert 2 (hash=2, position=2) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 2 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (2));
    ASSERT_EQ (table.size (), 2);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // insert -1 (hash=1, position=1) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 1 (no new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (-1));
    ASSERT_EQ (table.size (), 3);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // insert -2 (hash=2, position=2) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 2 (no new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (-2));
    ASSERT_EQ (table.size (), 4);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), 0);
    EXPECT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = (int64_t)table.get_bucket_count ();
    resizeCountInit     = (int64_t)table.get_resize_count ();

    // make sure that only the second and third buckets have all the keys
    for (int64_t bucket = 0; bucket < bucketCountInit; ++bucket) {
        if (bucket == 1 || bucket == 2) {
            ASSERT_EQ (table.get_bucket_key_count (bucket), 2);
            ASSERT_EQ (table.get_bucket_hash_count (bucket), 1);
        }
        else {
            ASSERT_EQ (table.get_bucket_key_count (bucket), 0);
            ASSERT_EQ (table.get_bucket_hash_count (bucket), 0);
        }
    }

    // insert 1 + bucket count (hash=1 + bucket count, position=1) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (1 + bucketCountInit));
    ASSERT_EQ (table.size (), 5);
    ASSERT_EQ (table.get_aggregate_count (), 3);

    // insert 2 + bucket count (hash=2 + bucket count, position=2) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 2 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (2 + bucketCountInit));
    ASSERT_EQ (table.size (), 6);
    ASSERT_EQ (table.get_aggregate_count (), 4);

    // insert -(1 + bucket count) (hash=1 + bucket count, position=1) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 1 (no new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (-(1 + bucketCountInit)));
    ASSERT_EQ (table.size (), 7);
    ASSERT_EQ (table.get_aggregate_count (), 4);

    // insert -(2 + bucket count) (hash=2 + bucket count, position=2) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 2 (no new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (-(2 + bucketCountInit)));
    ASSERT_EQ (table.size (), 8);
    ASSERT_EQ (table.get_aggregate_count (), 4);

    // make sure that the table was not resized
    ASSERT_EQ (table.get_resize_count (), resizeCountInit);
    ASSERT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = table.get_bucket_count ();

    // make sure that only the second and third buckets have all the keys
    for (int64_t bucket = 0; bucket < bucketCountInit; ++bucket) {
        if (bucket == 1 || bucket == 2) {
            ASSERT_EQ (table.get_bucket_key_count (bucket), 4);
            ASSERT_EQ (table.get_bucket_hash_count (bucket), 2);
        }
        else {
            ASSERT_EQ (table.get_bucket_key_count (bucket), 0);
            ASSERT_EQ (table.get_bucket_hash_count (bucket), 0);
        }
    }
}

/**
 * @brief                   Test Erasing duplicate elements
 *
 */
TEST (Erase, duplicates)
{
    AgHashTable<int64_t>                    table;

    // make sure that a non-existent key is not erased from the table
    ASSERT_FALSE (table.erase (0));
    ASSERT_EQ (table.size (), 0);
    ASSERT_EQ (table.get_aggregate_count (), 0);

    // insert 0 into the table and make sure all key counts are consistent
    ASSERT_TRUE (table.insert (0));
    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // erasing 0 from the table should be successful and all key counts should be consistent
    ASSERT_TRUE (table.erase (0));
    ASSERT_EQ (table.size (), 0);
    ASSERT_EQ (table.get_aggregate_count (), 0);

    // a non existant key should not be erased from the table
    ASSERT_FALSE (table.erase (0));
    ASSERT_EQ (table.size (), 0);
    ASSERT_EQ (table.get_aggregate_count (), 0);
}


/**
 * @brief                   Test deletion with one aggregate node (per bucket, not for all buckets) and one node per aggregate node (no collisions)
 *
 */
TEST (Erase, singleAggregateSingleNode)
{
    AgHashTable<int64_t, mod2<int64_t>>     table;

    ASSERT_TRUE (table.initialized ());

    // insert 0 (hash=0, position=0) and check if the insertion was successful
    ASSERT_TRUE (table.insert (0));

    // make sure the first bucket has the key
    ASSERT_EQ (table.get_bucket_key_count (0), 1);
    ASSERT_EQ (table.get_bucket_hash_count (0), 1);

    // insert 0 (hash=0, position=0) and check if the insertion was successful and table key counts are consistent
    // the key count in the first bucket should now be 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (1));

    // make sure the first bucket has the key
    ASSERT_EQ (table.get_bucket_key_count (1), 1);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // erase 0 (hash=0, position=0) and check if the deletion was successful and the key counts are consistent
    ASSERT_TRUE (table.erase (0));
    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // make sure 0 can not be erased again
    ASSERT_FALSE (table.erase (0));

    // make sure the first bucket does not have any keys
    ASSERT_EQ (table.get_bucket_key_count (0), 0);
    ASSERT_EQ (table.get_bucket_hash_count (0), 0);

    // erase 1 (hash=1, position=1) and check if the deletion was successful and the key counts are consistent
    ASSERT_TRUE (table.erase (1));
    ASSERT_EQ (table.size (), 0);
    ASSERT_EQ (table.get_aggregate_count (), 0);

    // make sure 1 can not be erased again
    ASSERT_FALSE (table.erase (0));

    // make sure the first bucket does not have any keys
    ASSERT_EQ (table.get_bucket_key_count (1), 0);
    ASSERT_EQ (table.get_bucket_hash_count (1), 0);
}

