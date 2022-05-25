/**
 * @file            test.cpp
 * @author          Aditya Agarwal (aditya.agarwal@dumblebots.com)
 *
 * @brief           Unit tests for AgHashTable class
 *
 * @note            In all docstrings, the bucket at position i is referred to as the (i+1)-th bucket
 *                  This means that the bucket at position 0 is called the first bucket, while the bucket at position 1 is called the second bucket
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

    // insert 0 (hash=0, position=0) and check if the insertion was successful
    // the table should now have one key (={0}) and one aggregate node (={0})
    ASSERT_TRUE (table.insert (0));
    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // the first bucket should have the new key and its aggregate node
    ASSERT_EQ (table.get_bucket_key_count (0), 1);
    ASSERT_EQ (table.get_bucket_hash_count (0), 1);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), 0);
    EXPECT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = (int64_t)table.get_bucket_count ();

    // make sure no other bucket has any other keys
    for (int64_t bucket = 1; bucket < bucketCountInit; ++bucket) {
        ASSERT_EQ (table.get_bucket_key_count (bucket), 0);
        ASSERT_EQ (table.get_bucket_hash_count (bucket), 0);
    }

    // insert 1 (hash=1, position=1) and check if the insertion was successful
    // the table should now have two keys (={0, 1}) and two aggregate nodes (={0, 1})
    ASSERT_TRUE (table.insert (1));
    ASSERT_EQ (table.size (), 2);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the first bucket should have the new key and its aggregate node
    ASSERT_EQ (table.get_bucket_key_count (1), 1);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), 0);
    EXPECT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = (int64_t)table.get_bucket_count ();

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
    int64_t                                 resizeCountInit;
    int64_t                                 bucketCountInit;

    ASSERT_TRUE (table.initialized ());

    bucketCountInit     = (int64_t)table.get_bucket_count ();

    // insert 0 (hash=0, position=0) and check if the insertion was successful
    // the number of keys and aggregate nodes should have increased by 1 (a new aggregate node had to be created for this key)
    ASSERT_TRUE (table.insert (0));
    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // the first bucket should now have 1 key (={0}) and one aggregate node (={0})
    ASSERT_EQ (table.get_bucket_key_count (0), 1);
    ASSERT_EQ (table.get_bucket_hash_count (0), 1);

    // insert 0 (hash=0, position=0) and check if the insertion was successful
    // the number of keys should have increased by 1, while the number of aggregate nodes remain the same
    ASSERT_TRUE (table.insert (2));
    ASSERT_EQ (table.size (), 2);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // the first bucket should now have 1 key (={0, 2}) and one aggregate node (={0})
    ASSERT_EQ (table.get_bucket_key_count (0), 2);
    ASSERT_EQ (table.get_bucket_hash_count (0), 1);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), 0);
    EXPECT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = (int64_t)table.get_bucket_count ();
    resizeCountInit     = (int64_t)table.get_resize_count ();

    // make sure no other buckets have any keys
    for (int64_t bucket = 1; bucket < bucketCountInit; ++bucket) {
        ASSERT_EQ (table.get_bucket_key_count (bucket), 0);
        ASSERT_EQ (table.get_bucket_hash_count (bucket), 0);
    }

    // insert 1 (hash=1, position=1) and check if the insertion was successful
    // the number of keys and aggregate nodes should have increased by 1 (a new aggregate node had to be created for this key)
    ASSERT_TRUE (table.insert (1));
    ASSERT_EQ (table.size (), 3);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the second bucket should now have 1 key (={1}) and one aggregate node (={1})
    ASSERT_EQ (table.get_bucket_key_count (1), 1);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // insert 3 (hash=1, position=1) and check if the insertion was successful
    // the number of keys should have increased by 1, while the number of aggregate nodes remain the same
    ASSERT_TRUE (table.insert (3));
    ASSERT_EQ (table.size (), 4);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the second bucket should now have 1 key (={0, 2}) and one aggregate node (={0})
    ASSERT_EQ (table.get_bucket_key_count (1), 2);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), resizeCountInit);
    EXPECT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = (int64_t)table.get_bucket_count ();
    resizeCountInit     = (int64_t)table.get_resize_count ();

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

    // insert 1 (hash=1, position=1) and check if the insertion was successful
    // the number of keys and aggregate nodes in the table should have increased by 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (1));
    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // the second bucket should now have 1 key (={1}) and 1 aggregate node (={1})
    ASSERT_EQ (table.get_bucket_key_count (1), 1);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // insert -1 (hash=1, position=1) and check if the insertion was successful
    // the number of keys in the table should have increased by 1, while the number of aggregate nodes remain the same
    ASSERT_TRUE (table.insert (-1));
    ASSERT_EQ (table.size (), 2);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // the second bucket should now have 2 keys (={1, -1}) and 1 aggregate node (={1})
    ASSERT_EQ (table.get_bucket_key_count (1), 2);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // insert 2 (hash=2, position=2) and check if the insertion was successful
    // the number of keys and aggregate nodes in the table should have incerased by 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (2));
    ASSERT_EQ (table.size (), 3);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the third bucket should now have 1 key (={2}) and 1 aggregate node (={2})
    ASSERT_EQ (table.get_bucket_key_count (2), 1);
    ASSERT_EQ (table.get_bucket_hash_count (2), 1);

    // insert -2 (hash=2, position=2) and check if the insertion was successful
    // the number of keys in the table should have increased by 1, while the number of aggregate nodes remain the same
    ASSERT_TRUE (table.insert (-2));
    ASSERT_EQ (table.size (), 4);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the third bucket should now have 2 keys (={2, -2}) and 1 aggregate node (={2})
    ASSERT_EQ (table.get_bucket_key_count (2), 2);
    ASSERT_EQ (table.get_bucket_hash_count (2), 1);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), 0);
    EXPECT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = (int64_t)table.get_bucket_count ();
    resizeCountInit     = (int64_t)table.get_resize_count ();

    // make sure that no other buckets have any keys
    for (int64_t bucket = 0; bucket < bucketCountInit; ++bucket) {
        if (bucket != 1 && bucket != 2) {
            ASSERT_EQ (table.get_bucket_key_count (bucket), 0);
            ASSERT_EQ (table.get_bucket_hash_count (bucket), 0);
        }
    }

    // insert 1 + bucket count (hash=1 + bucket count, position=1) and check if the insertion was successful
    // the number of keys and aggregate nodes in the table should have increased by 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (1 + bucketCountInit));
    ASSERT_EQ (table.size (), 5);
    ASSERT_EQ (table.get_aggregate_count (), 3);

    // the second bucket should now have 3 keys (={1, -1, 1 + bucket count}) and 2 aggregate nodes (={1, 1 + bucket count})
    ASSERT_EQ (table.get_bucket_key_count (1), 3);
    ASSERT_EQ (table.get_bucket_hash_count (1), 2);

    // insert -1 - bucket count (hash=1 + bucket count, position=1) and check if the insertion was successful
    // the number of keys in the table should have increased by 1, while the number of aggregate nodes remain the same
    ASSERT_TRUE (table.insert (-1 - bucketCountInit));
    ASSERT_EQ (table.size (), 6);
    ASSERT_EQ (table.get_aggregate_count (), 3);

    // the second bucket should now have 4 keys (={1, -1, 1 + bucket count, -1 - bucket count}) and 2 aggregate nodes (={1, 1 + bucket count})
    ASSERT_EQ (table.get_bucket_key_count (1), 4);
    ASSERT_EQ (table.get_bucket_hash_count (1), 2);

    // insert 2 + bucket count (hash=2 + bucket count, position=2) and check if the insertion was successful
    // the number of keys and aggregate nodes in the table should have incerased by 1 (a new aggregate node should have been created for this key)
    ASSERT_TRUE (table.insert (2 + bucketCountInit));
    ASSERT_EQ (table.size (), 7);
    ASSERT_EQ (table.get_aggregate_count (), 4);

    // the third bucket should now have 3 keys (={2, -2, 2 + bucket count}) and 2 aggregate nodes (={2, 2 + bucket count})
    ASSERT_EQ (table.get_bucket_key_count (2), 3);
    ASSERT_EQ (table.get_bucket_hash_count (2), 2);

    // insert -2 - bucket count (hash=2, position=2) and check if the insertion was successful
    // the number of keys in the table should have increased by 1, while the number of aggregate nodes remain the same
    ASSERT_TRUE (table.insert (-2 - bucketCountInit));
    ASSERT_EQ (table.size (), 8);
    ASSERT_EQ (table.get_aggregate_count (), 4);

    // the third bucket should now have 4 key (={2, -2, 2 + bucket count, -2 - bucket count}) and 2 aggregate nodes (={2, 2 + bucket count})
    ASSERT_EQ (table.get_bucket_key_count (2), 4);
    ASSERT_EQ (table.get_bucket_hash_count (2), 2);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), resizeCountInit);
    EXPECT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = (int64_t)table.get_bucket_count ();
    resizeCountInit     = (int64_t)table.get_resize_count ();

    // make sure that no other buckets have any keys
    for (int64_t bucket = 0; bucket < bucketCountInit; ++bucket) {
        if (bucket != 1 && bucket != 2) {
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

    // insert 0 (hash=0, position=0) and check if the insertion was successful
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

/**
 * @brief                   Test deletion with one aggregate node (per bucket, not for all buckets) and multiple nodes per aggregate node (collisions)
 *
 */
TEST (Erase, singleAggregateMultiNode)
{
    AgHashTable<int64_t, mod2<int64_t>>     table;

    ASSERT_TRUE (table.initialized ());

    // insert 0 (hash=0, position=0) and check if the insertion was successful
    ASSERT_TRUE (table.insert (0));
    // insert 2 (hash=0, position=0) and check if the insertion was successful
    ASSERT_TRUE (table.insert (2));

    // insert 1 (hash=1, position=1) and check if the insertion was successful
    ASSERT_TRUE (table.insert (1));
    // Insert 3 (hash=1, position=1) and check if the insertion was successful
    ASSERT_TRUE (table.insert (3));

    // the table should have 4 keys (={0, 2, 1, 3}) and 2 aggregate nodes (={0, 1})
    ASSERT_EQ (table.size (), 4);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the first bucket should have 2 keys (={0, 2}) with one distinct hash value (={0})
    ASSERT_EQ (table.get_bucket_key_count (0), 2);
    ASSERT_EQ (table.get_bucket_hash_count (0), 1);

    // the second bucket should have 2 keys (={1, 3}) with one distinct hash value (={1})
    ASSERT_EQ (table.get_bucket_key_count (1), 2);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // try to erase 0 (hash=0, position=0) and check if the deletion was successful
    // the number of keys should have decreased by 1 while the number of aggregate nodes remain the same
    ASSERT_TRUE (table.erase (0));
    ASSERT_EQ (table.size (), 3);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the first bucket should have one less key (={2}) and the same number of aggregate nodes
    ASSERT_EQ (table.get_bucket_key_count (0), 1);
    ASSERT_EQ (table.get_bucket_hash_count (0), 1);

    // try to erase 2 (hash=0, position=0) and check if the deletion was successful
    // the number of keys and aggregate nodes should have each decreased by 1
    ASSERT_TRUE (table.erase (2));
    ASSERT_EQ (table.size (), 2);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // the first bucket should not have no keys or aggregate nodes
    ASSERT_EQ (table.get_bucket_key_count (0), 0);
    ASSERT_EQ (table.get_bucket_hash_count (0), 0);

    // try to erase 1 (hash=1, position=1) and check if the deletion was successful
    // the number of keys should have decreased by 1 while the number of aggregate nodes remain the same
    ASSERT_TRUE (table.erase (1));
    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // the first bucket should have one less key (={3}) and the same number of aggregate nodes
    ASSERT_EQ (table.get_bucket_key_count (1), 1);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // try to erase 3 (hash=1, position=1) and check if the deletion was successful
    // the number of keys and aggregate nodes should have each decreased by 1
    ASSERT_TRUE (table.erase (3));
    ASSERT_EQ (table.size (), 0);
    ASSERT_EQ (table.get_aggregate_count (), 0);

    // the first bucket should not have no keys or aggregate nodes
    ASSERT_EQ (table.get_bucket_key_count (1), 0);
    ASSERT_EQ (table.get_bucket_hash_count (1), 0);
}

/**
 * @brief                   Test deletion with multiple aggregate nodes and multiple nodes per aggregate node
 *
 */
TEST (Erase, multiAggregateMultiNode)
{
    AgHashTable<int64_t, abs<int64_t>>      table;

    ASSERT_TRUE (table.initialized ());

    // insert 1 (hash=1, position=1) and check if the insertion was successful
    ASSERT_TRUE (table.insert (1));
    // insert -1 (hash=1, position=1) and check if the insertion was successful
    ASSERT_TRUE (table.insert (-1));

    // insert 1 + bucket count (hash=1 + bucket count, position=1) and check if the insertion was successful
    ASSERT_TRUE (table.insert (1 + (int64_t)table.get_bucket_count ()));
    // insert -1 - bucket count (hash=1 + bucket count, position=1) and check if the insertion was successful
    ASSERT_TRUE (table.insert (-1 - (int64_t)table.get_bucket_count ()));

    // insert 2 (hash=2, position=2) and check if the insertion was successful
    ASSERT_TRUE (table.insert (2));
    // insert -2 (hash=2, position=2) and check if the insertion was successful
    ASSERT_TRUE (table.insert (-2));

    // insert 2 + bucket count (hash=2 + bucket count, position=2) and check if the insertion was successful
    ASSERT_TRUE (table.insert (2 + (int64_t)table.get_bucket_count ()));
    // insert -2 - bucket count (hash=2, position=2) and check if the insertion was successful
    ASSERT_TRUE (table.insert (-2 - (int64_t)table.get_bucket_count ()));

    // the table should have 8 keys (={1, -1, 2, -2, 1 + bucket count, -1 - bucket count, 2 + bucket count, -2 - bucket count}) and 4 aggregate nodes (={1, 1 + bucket count, 2, 2 + bucket count})
    ASSERT_EQ (table.size (), 8);
    ASSERT_EQ (table.get_aggregate_count (), 4);

    // the second bucket should have 4 keys (={1, -1, 1 + bucket count, -1 - bucket count}) and 2 aggregate nodes (={1, 1 + bucket count})
    ASSERT_EQ (table.get_bucket_key_count (1), 4);
    ASSERT_EQ (table.get_bucket_hash_count (1), 2);

    // the third bucket should have 4 keys (={2, -2, 2 + bucket count, -2 - bucket count}) and 2 aggregate nodes (={2, 2 + bucket count})
    ASSERT_EQ (table.get_bucket_key_count (2), 4);
    ASSERT_EQ (table.get_bucket_hash_count (2), 2);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), 0);
    EXPECT_EQ (table.get_bucket_count (), (int64_t)table.get_bucket_count ());

    // erase 1 (hash=1, position=1) and check if the deletion was successful
    // the number of keys in the table should have decreased by 1 while the number of aggregate nodes remain the same
    ASSERT_TRUE (table.erase (1));
    ASSERT_EQ (table.size (), 7);
    ASSERT_EQ (table.get_aggregate_count (), 4);

    // the second bucket should have 3 keys (={-1, 1 + bucket count, -1 - bucket count}) and 2 aggregate nodes (={1, 1 + bucket count})
    ASSERT_EQ (table.get_bucket_key_count (1), 3);
    ASSERT_EQ (table.get_bucket_hash_count (1), 2);

    // erase -1 (hash=1, position=1) and check if the deletion was successful
    // the number of keys and aggregate nodes in the table should have decreased by 1
    ASSERT_TRUE (table.erase (-1));
    ASSERT_EQ (table.size (), 6);
    ASSERT_EQ (table.get_aggregate_count (), 3);

    // the second bucket should have 2 keys (={1 + bucket count, -1 - bucket count}) and 1 aggregate node (={1 + bucket count})
    ASSERT_EQ (table.get_bucket_key_count (1), 2);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // erase 1 + bucket count (hash=1 + bucket count, position=1) and check if the deletion was successful
    // the number of keys in the table should have decreased by 1 while the number of aggregate nodes remain the same
    ASSERT_TRUE (table.erase (1 + (int64_t)table.get_bucket_count ()));
    ASSERT_EQ (table.size (), 5);
    ASSERT_EQ (table.get_aggregate_count (), 3);

    // the second bucket should have 1 key (={-1 - bucket count}) and 1 aggregate node (={1 + bucket count})
    ASSERT_EQ (table.get_bucket_key_count (1), 1);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // erase -1 - bucket count (hash=1 + bucket count, position=1) and check if the deletion was successful
    // the number of keys and aggregate nodes in the table should have decreased by 1
    ASSERT_TRUE (table.erase (-1 - (int64_t)table.get_bucket_count ()));
    ASSERT_EQ (table.size (), 4);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the second bucket should have no keys left
    ASSERT_EQ (table.get_bucket_key_count (1), 0);
    ASSERT_EQ (table.get_bucket_hash_count (1), 0);

    // erase 2 (hash=2, position=2) and check if the deletion was successful
    // the number of keys in the table should have decreased by 1 while the number of aggregate nodes remain the same
    ASSERT_TRUE (table.erase (2));
    ASSERT_EQ (table.size (), 3);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the third bucket should have 3 keys (={-2, 2 + bucket count, -2 - bucket count}) and 2 aggregate node (={2, 2 + bucket count})
    ASSERT_EQ (table.get_bucket_key_count (2), 3);
    ASSERT_EQ (table.get_bucket_hash_count (2), 2);

    // erase -2 (hash=2, position=2) and check if the deletion was successful
    // the number of keys and aggregate nodes in the table should have decreased by 1
    ASSERT_TRUE (table.erase (-2));
    ASSERT_EQ (table.size (), 2);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // the second bucket should have 2 keys (={2 + bucket count, -2 - bucket count}) and 1 aggregate node (={2 + bucket count})
    ASSERT_EQ (table.get_bucket_key_count (2), 2);
    ASSERT_EQ (table.get_bucket_hash_count (2), 1);

    // erase 2 + bucket count (hash=2 + bucket count, position=2) and check if the deletion was successful
    // the number of keys in the table should have decreased by 1 while the number of aggregate nodes remain the same
    ASSERT_TRUE (table.erase (2 + (int64_t)table.get_bucket_count ()));
    ASSERT_EQ (table.size (), 1);
    ASSERT_EQ (table.get_aggregate_count (), 1);

    // the third bucket should have 1 keys (={-2 - bucket count}) and 1 aggregate node (={2 + bucket count})
    ASSERT_EQ (table.get_bucket_key_count (2), 1);
    ASSERT_EQ (table.get_bucket_hash_count (2), 1);

    // erase -2 = bucket count (hash=2 + bucket count, position=2) and check if the deletion was successful
    // the number of keys and aggregate nodes in the table should have decreased by 1
    ASSERT_TRUE (table.erase (-2 - (int64_t)table.get_bucket_count ()));
    ASSERT_EQ (table.size (), 0);
    ASSERT_EQ (table.get_aggregate_count (), 0);

    // the thid bucket should have no keys left
    ASSERT_EQ (table.get_bucket_key_count (2), 0);
    ASSERT_EQ (table.get_bucket_hash_count (2), 0);
}

/**
 * @brief                   Test searching with one aggregate node (per bucket, not for all buckets) and one node per aggregate node (no collisions)
 *
 */
TEST (Find, singleAggregateSingleNode)
{
    AgHashTable<int64_t, mod2<int64_t>>     table;

    ASSERT_TRUE (table.initialized ());

    // make sure that 0 can not be found in the table before insertion
    ASSERT_FALSE (table.exists (0));
    ASSERT_EQ (table.find (0), table.end ());

    // make sure that 1 can not be found in the table before insertion
    ASSERT_FALSE (table.exists (1));
    ASSERT_EQ (table.find (1), table.end ());

    // insert 0 (hash=0, position=0) and check if the insertion was successful
    ASSERT_TRUE (table.insert (0));
    // insert 1 (hash=1, position=1) and check if the insertion was successful
    ASSERT_TRUE (table.insert (1));

    // the table should have 2 keys (={0, 1}) and 2 aggregate nodes (={0, 1})
    ASSERT_EQ (table.size (), 2);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the first bucket should have 1 key (={0}) with one distinct hash value (={0})
    ASSERT_EQ (table.get_bucket_key_count (0), 1);
    ASSERT_EQ (table.get_bucket_hash_count (0), 1);

    // the second bucket should have 1 key (={1}) with one distinct hash value (={1})
    ASSERT_EQ (table.get_bucket_key_count (1), 1);
    ASSERT_EQ (table.get_bucket_hash_count (1), 1);

    // the key 0 should be found in succesfully in the table
    ASSERT_TRUE (table.exists (0));
    ASSERT_NE (table.find (0), table.end ());
    ASSERT_EQ (*(table.find (0)), 0);

    // the key 1 should be found in succesfully in the table
    ASSERT_TRUE (table.exists (1));
    ASSERT_NE (table.find (1), table.end ());
    ASSERT_EQ (*(table.find (1)), 1);

    // make sure both the keys are erased from the table
    ASSERT_TRUE (table.erase (0));
    ASSERT_TRUE (table.erase (1));

    // make sure that 0 can not be found in the table after deletion
    ASSERT_FALSE (table.exists (0));
    ASSERT_EQ (table.find (0), table.end ());

    // make sure that 1 can not be found in the table after deletion
    ASSERT_FALSE (table.exists (1));
    ASSERT_EQ (table.find (1), table.end ());
}

/**
 * @brief                   Test searching with on aggregate node (per bucket, not for all buckets) and multiple nodes per aggregate node (collisions)
 *
 */
TEST (Find, singleAggregateMultiNode)
{
    AgHashTable<int64_t, abs<int64_t>>      table;
    int64_t                                 bucketCountInit;
    int64_t                                 resizeCountInit;

    ASSERT_TRUE (table.initialized ());

    bucketCountInit     = (int64_t)table.get_bucket_count ();

    // make sure none of the keys are found before insertion
    for (int64_t i = 1; i < 3; ++i) {
        for (auto &e : {i, -i, i + bucketCountInit, -i - bucketCountInit}) {
            ASSERT_FALSE (table.exists (e));
            ASSERT_EQ (table.find (e), table.end ());
        }
    }

    // insert the following keys into the table
    // 1                    (hash=1,                position=1)
    // -1                   (hash=1,                position=1)
    // 2                    (hash=2,                position=2)
    // -2                   (hash=2,                position=2)
    for (int64_t i = 1; i < 3; ++i) {
        for (auto &e : {i, -i}) {
            ASSERT_TRUE (table.insert (e));
        }
    }

    // make sure the size of the table and the number of aggregate nodes are correct
    ASSERT_EQ (table.size (), 4);
    ASSERT_EQ (table.get_aggregate_count (), 2);

    // the table should not have been resized, but it's okay if it was
    EXPECT_EQ (table.get_resize_count (), 0);
    EXPECT_EQ (table.get_bucket_count (), bucketCountInit);

    bucketCountInit     = (int64_t)table.get_bucket_count ();
    resizeCountInit     = (int64_t)table.get_resize_count ();

    // check if all keys can be found
    for (int64_t i = 1; i < 3; ++i) {
        for (auto &e : {i, -i}) {
            ASSERT_TRUE (table.exists (e));
            ASSERT_NE (table.find (e), table.end ());
            ASSERT_EQ (*(table.find (e)), e);
        }
    }
}

