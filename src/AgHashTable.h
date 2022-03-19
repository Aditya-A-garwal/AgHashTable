#ifndef AG_HASH_TABLE_GUARD_H
#define AG_HASH_TABLE_GUARD_H

#ifdef AG_TEST_MODE
#define TEST_MODE(...)                          __VA_ARGS__
#define NO_TEST_MODE(...)
#else
#define TEST_MODE(...)
#define NO_TEST_MODE(...)                       __VA_ARGS__
#endif

#include <type_traits>
#include <limits>

/**
 * @brief
 *
 * @tparam key_t
 * @tparam mHashFunc
 */
template <typename key_t, auto mHashFunc>
class AgHashTable {


NO_TEST_MODE(protected:)
TEST_MODE(public:)

    /**
     * @brief
     *
     */
    struct node_t {
        // pointer to next node in the linked list
        node_t      *mPtr   {nullptr};
        // the key value
        key_t       mKey;
    };

    /**
     * @brief
     *
     */
    struct bucket_t {
        // number of elements currently in the bucket
        uint64_t    mSize   {0};
        // pointer to the array storing the slots in the bucket
        node_t      **mAr   {nullptr};

        ~bucket_t () { delete[] mAr; };
    };

    using       node_ptr_t  = node_t *;

    // make sure that the hash function is callable and returns an unsigned integral type of not more than 16 bits
    static_assert (std::is_invocable <decltype (mHashFunc), key_t>::value, "hash function should be callable");
    using       hash_t      = typename std::invoke_result<decltype (mHashFunc), key_t>::type;
    static_assert (std::is_unsigned<hash_t>::value, "hash function should return unsigned type");
    static_assert (sizeof (hash_t) <= 2, "hash function should be 16 bit or less");

    static constexpr uint64_t   sMaxHashValueLog    = sizeof (hash_t) << 3;
    static constexpr uint64_t   sBucketSizeLog      = sMaxHashValueLog >> 1;
    static constexpr uint64_t   sBucketCountLog     = sMaxHashValueLog >> 1;

    static constexpr uint64_t   sMaxHashValue       = 1ULL << sMaxHashValueLog;
    static constexpr uint64_t   sBucketSize         = 1ULL << sBucketSizeLog;
    static constexpr uint64_t   sBucketCount        = 1ULL << sBucketCountLog;

    static_assert ((sBucketSize * sBucketCount) == sMaxHashValue);

    bucket_t        *mBuckets;

    uint64_t        mSize           {0};

    TEST_MODE(
    uint64_t        mSlots          {0};
    )


NO_TEST_MODE(public:)


    AgHashTable     ();
    AgHashTable     (const AgHashTable<key_t, mHashFunc> &pOther)   = delete;

    ~AgHashTable    ();


    bool                insert  (const key_t pKey);
    bool                find    (const key_t pKey) const;
    bool                erase   (const key_t pKey);

    uint64_t            size    () const;
};

/**
 * @brief                           Construct a new AgHashTable<val_t, mHashFunc>::AgHashTable object
 */
template <typename key_t, auto mHashFunc>
AgHashTable<key_t, mHashFunc>::AgHashTable ()
{
    // allocate array of buckets, return if could not allocate
    mBuckets        = new (std::nothrow) bucket_t[sBucketCount];
    if (mBuckets == nullptr) {
        TEST_MODE (std::cout << "Could not allocate buckets while constructing\n";)
        return;
    }
}

/**
 * @brief                           Destroy the AgHashTable<val_t, mHashFunc>::AgHashTable object
 */
template <typename val_t, auto mHashFunc>
AgHashTable<val_t, mHashFunc>::~AgHashTable ()
{
    delete[] mBuckets;
}

/**
 * @brief                           Attempts to insert a key into the hash table
 *
 * @param pKey                      Key to insert into the table
 *
 * @return true                     If no duplicate key was found and was inserted succesfully
 * @return false                    If key could not be inserted succesfully
 */
template <typename key_t, auto mHashFunc>
bool
AgHashTable<key_t, mHashFunc>::insert (const key_t pKey)
{
    // hash of the supplied key to insert
    uint64_t    keyHash;
    // bucket in which the key will be inserted
    uint64_t    bucketId;
    // position in bucket in which the key will be inserted
    uint64_t    bucketPos;

    // pointer to node ptr, used while iterating over the linked list in the slot
    node_ptr_t  *listElem;

    // pointer to allocated new node, used after valid empty position has been found
    node_ptr_t  node;

    // calculate the hash of they key and find its bucket and position in the bucket
    // the bucket is the sBucketSizeLog Most Significant bits, while the
    // position in the bucket is the remaining Least Significant bits of the hash
    keyHash                     = mHashFunc (pKey);
    bucketId                    = keyHash >> sBucketSizeLog;
    bucketPos                   = keyHash & ((1ULL << sBucketSizeLog) - 1);

    // if the bucket has not been allocated yet, then it needs to be
    if (mBuckets[bucketId].mAr == nullptr) {

        // allocate the array of the current bucket, return failed insertion if could not allocate
        mBuckets[bucketId].mAr  = new (std::nothrow) node_ptr_t[sBucketSize];
        if (mBuckets [bucketId].mAr == nullptr) {
            TEST_MODE(std::cout << "Could not allocate bucket array while inserting\n";)
            return false;
        }

        // go over each slot in the bucket and make the linked list point to nullptr
        for (uint64_t i = 0; i < sBucketSize; ++i) {
            mBuckets[bucketId].mAr[i]   = nullptr;
        }
    }

    // get a pointer to the pointer to the next node
    // pointer to pointer allows for removing special head case
    listElem                    = &mBuckets[bucketId].mAr[bucketPos];

    // while the pointer being pointed to, does not point to null, the end of the list has not been reached
    while ((*listElem) != nullptr) {

        // in case a matching element was found, return failed insertion (duplicates are not allowed)
        if ((*listElem)->mKey == pKey) {
            return false;
        }

        listElem    = &((*listElem)->mPtr);
    }

    // allocate a new node containing the key, return failed insertion if could not allocate
    node                        = new (std::nothrow) node_t {nullptr, pKey};
    if (node == nullptr) {
        TEST_MODE (std::cout << "Could not allocate new node while inserting\n";)
        return false;
    }

    // place the new node at this position
    (*listElem)                 = node;

    // increment size of bucket and size of table
    mBuckets[bucketId].mSize    += 1;
    mSize                       += 1;

    return true;
}

/**
 * @brief                           Searches for a key in the hashtable
 *
 * @param pKey                      Key to search for
 *
 * @return true                     If key exists in the hashtable
 * @return false                    If key does not exist in the hashtable
 */
template <typename key_t, auto mHashFunc>
bool
AgHashTable<key_t, mHashFunc>::find (const key_t pKey) const
{
    // hash of the supplied key to insert
    uint64_t    keyHash;
    // bucket in which the key will be inserted
    uint64_t    bucketId;
    // position in bucket in which the key will be inserted
    uint64_t    bucketPos;

    // pointer to node, used while iterating over linked list to find matching key
    node_ptr_t  listElem;

    // calculate the hash of they key and find its bucket and position in the bucket
    // the bucket is the sBucketSizeLog Most Significant bits, while the
    // position in the bucket is the remaining Least Significant bits of the hash
    keyHash         = mHashFunc (pKey);
    bucketId        = keyHash >> sBucketSizeLog;
    bucketPos       = keyHash & ((1ULL << sBucketSizeLog) - 1);

    // the bucket itself does not exist, search unsuccesful
    if (mBuckets[bucketId].mAr == nullptr) {
        return false;
    }

    // get head of linked list at that position and iterate over it while trying to find the value
    listElem        = mBuckets[bucketId].mAr[bucketPos];
    while (listElem != nullptr) {

        // if matching key was found, return succesful search
        if (listElem->mKey == pKey) {
            return true;
        }
        listElem    = listElem->mPtr;
    }

    return false;
}

/**
 * @brief                           Attempts to erase a key from the hash table
 *
 * @param pKey                      Key to erase from the table
 *
 * @return true                     If the key was succesfully found and removed
 * @return false                    If a matching key could not be found or removed
 */
template <typename key_t, auto mHashFunc>
bool
AgHashTable<key_t, mHashFunc>::erase (const key_t pKey)
{
    // hash of the supplied key to erase
    uint64_t    keyHash;
    // bucket in which the key should be present
    uint64_t    bucketId;
    // position in bucket at which they key should be present
    uint64_t    bucketPos;

    // pointer to node ptr, used while iterating over the linked list in the slot
    node_ptr_t  *listElem;

    // pointer to allocated new node, used while deleting the key's node
    node_ptr_t  node;

    // calculate the hash of they key and find its bucket and position in the bucket
    // the bucket is the sBucketSizeLog Most Significant bits, while the
    // position in the bucket is the remaining Least Significant bits of the hash
    keyHash         = mHashFunc (pKey);
    bucketId        = keyHash >> sBucketSizeLog;
    bucketPos       = keyHash & ((1ULL << sBucketSizeLog) - 1);

    // if the bucket itself does not exist, return failed erase
    if (mBuckets[bucketId].mAr == nullptr) {
        return false;
    }

    // get a pointer to the pointer to the next node
    // pointer to pointer allows for removing special head case
    listElem                    = &mBuckets[bucketId].mAr[bucketPos];

    // while the pointer being pointed to, does not point to null, the end of the list has not been reached
    while ((*listElem) != nullptr) {

        // if matching key was found
        if ((*listElem)->mKey == pKey) {

            // take out the current node and make the previous node point to the next node, delete the node
            node                        = *listElem;
            (*listElem)                 = node->mPtr;

            delete node;

            mBuckets[bucketId].mSize    -= 1;
            mSize                       -= 1;

            return true;
        }

        listElem    = &((*listElem)->mPtr);
    }

    return false;
}

/**
 * @brief                           Returns the number of elements in the hashtable
 *
 * @return uint64_t                 Number of elements in the table
 */
template <typename key_t, auto mHashFunc>
uint64_t
AgHashTable<key_t, mHashFunc>::size () const
{
    return mSize;
}

#undef  TEST_MODE
#undef  NO_TEST_MODE

#endif      // Header Guard
