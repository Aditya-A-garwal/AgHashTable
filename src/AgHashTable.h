/**
 * @file            AgHashTable.h
 * @author          Aditya Agarwal (aditya.agarwal@dumblebots.com)
 *
 * @brief           AgHashTable class
 *
 */


#ifndef AG_HASH_TABLE_GUARD_H

#define     AG_HASH_TABLE_GUARD_H

#ifdef AG_DBG_MODE
#define     DBG_MODE(...)                           __VA_ARGS__
#define     NO_DBG_MODE(...)

#else

#define     DBG_MODE(...)
#define     NO_DBG_MODE(...)                        __VA_ARGS__

#endif

#include <type_traits>
#include <limits>

#include "AgHashFunctions.hpp"


/**
 * @brief                   AgHashtable is an implementation of the hashtable data structure
 *
 * @tparam key_t            Type of keys held by the hashtable
 * @tparam mHashFunc        Hash Function for calculating the hashes of keys
 */
template <typename key_t, auto mHashFunc = ag_pearson_16_hash>
class AgHashTable {


protected:

    /**
     * @brief               Node in the linked list at a slot
     *
     */
    struct node_t {

        node_t      *mPtr       {nullptr};      /* Pointer to next node in the linked list */
        key_t       mKey;                       /* The key value */

        ~node_t () { delete mPtr; }
    };

    /**
     * @brief               Structure for encapsulating buckets
     *
     */
    struct bucket_t {

        uint64_t    mSize       {0};            /* Number of elements currently in the bucket */
        node_t      **mAr       {nullptr};      /* Pointer to the array storing the slots in the bucket */
    };

    using       node_ptr_t  = node_t *;

    // make sure that the hash function is callable and returns an unsigned integral type of not more than 16 bits
    static_assert (std::is_invocable <decltype (mHashFunc), const uint8_t *, const uint64_t>::value, "hash function should be callable");
    using       hash_t      = typename std::invoke_result<decltype (mHashFunc), const uint8_t *, const uint64_t>::type;
    static_assert (std::is_unsigned<hash_t>::value, "hash function should return unsigned type");
    static_assert (sizeof (hash_t) <= 2, "hash function should be 16 bit or less");

    static constexpr uint64_t   sDistinctHash       = 1ULL << (sizeof (hash_t) * 8ULL);     /* Number of distinct hash values possible */
    static constexpr uint64_t   sBucketCapacity     = 1ULL << 8ULL;                         /* Number of slots in a bucket */
    static constexpr uint64_t   sBucketCount        = 1ULL << 8ULL;                         /* Number of distinct buckets */

    static_assert ((sBucketCapacity * sBucketCount) == sDistinctHash);

    bucket_t        *mBuckets;                                                              /* Member array to buckets */
    uint64_t        mSize           {0ULL};                                                 /* Number of elements in the table */
    uint64_t        mBucketsUsed    {0ULL};                                                 /* Number of buckets which have atleast one element */

    DBG_MODE (
    uint64_t        mSlotsUsed      {0ULL};                                                 /* Number of slots used in the table to accomodate keys*/
    )

public:

    //      Constructors

    AgHashTable                                 ();
    AgHashTable                                 (const AgHashTable<key_t, mHashFunc> &pOther)   = delete;

    //      Destructor

    ~AgHashTable                                ();

    //      Modifiers

    bool                initialize_if_not       ();

    bool                insert                  (const key_t pKey);
    bool                erase                   (const key_t pKey);

    //      Searching

    bool                find                    (const key_t pKey) const;

    //      Getters

    bool                initialized             () const;

    uint64_t            size                    () const;
    uint64_t            bucket_count            () const;
    uint64_t            buckets_used            () const;
    uint64_t            bucket_capacity         () const;
    uint64_t            bucket_size             (const uint64_t pIndex) const;

    DBG_MODE (
    uint64_t            slots_used              () const;
    )
};

/**
 * @brief                           Construct a new AgHashTable<key_t, mHashFunc>::AgHashTable object
 */
template <typename key_t, auto mHashFunc>
AgHashTable<key_t, mHashFunc>::AgHashTable ()
{
    // allocate array of buckets, return if could not allocate
    mBuckets        = new (std::nothrow) bucket_t[sBucketCount];
    if (mBuckets == nullptr) {
        DBG_MODE (std::cout << "Could not allocate buckets while constructing\n";)
        return;
    }
}

/**
 * @brief                           Returns if the table could be succesfully initialized
 *
 * @note                            If the function returns false, using the table results in undefined behaviour
 *
 * @return true                     If the table was succesfully initialized, and is in a useable state
 * @return false                    If the table could not be succesfully initialized, and is not in a useable state
 */
template <typename key_t, auto mHashFunc>
bool
AgHashTable<key_t, mHashFunc>::initialized () const
{
    return (mBuckets != nullptr);
}

/**
 * @brief                           Tries to initialize the table if it could not be succesfully initialized
 *
 * @return true                     If the table was not already initialized and could be succesfully initialized
 * @return false                    If the table was already initialized or could not be initialized
 */
template <typename key_t, auto mHashFunc>
bool
AgHashTable<key_t, mHashFunc>::initialize_if_not ()
{
    if (mBuckets != nullptr) {
        DBG_MODE (std::cout << "Table has already been initialized\n";)
        return false;
    }

    mBuckets        = new (std::nothrow) bucket_t[sBucketCount];
    if (mBuckets == nullptr) {
        DBG_MODE (std::cout << "Could not allocate buckets while trying to re-init\n";)
        return false;
    }

    return true;
}

/**
 * @brief                           Destroy the AgHashTable<key_t, mHashFunc>::AgHashTable object
 */
template <typename key_t, auto mHashFunc>
AgHashTable<key_t, mHashFunc>::~AgHashTable ()
{
    for (uint64_t bucket = 0; bucket < sBucketCount; ++bucket) {

        // if the current bucket has never been allocated, skip it
        if (mBuckets[bucket].mAr == nullptr) {
            continue;
        }

        // go through every slot and delete the entire linked list at that slot
        for (uint64_t slot = 0; slot < sBucketCapacity; ++slot) {
            delete mBuckets[bucket].mAr[slot];
        }

        // delete this bucket
        delete[] mBuckets[bucket].mAr;
    }

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
    uint64_t    keyHash;                        // hash of the supplied key to insert
    uint64_t    bucketId;                       // bucket in which the key will be inserted
    uint64_t    bucketPos;                      // position in bucket in which the key will be inserted

    node_ptr_t  *listElem;                      // pointer to node ptr, for iterating over the linked list in the slot

    node_ptr_t  node;                           // pointer to allocated new node

    // calculate the hash of the key and find its bucket and position in the bucket
    // the bucket is the sBucketSizeLog Most Significant bits, while the
    // position in the bucket is the remaining Least Significant bits of the hash
    keyHash                     = mHashFunc ((uint8_t *)&pKey, sizeof (key_t));
    bucketId                    = keyHash / sBucketCapacity;
    bucketPos                   = keyHash % sBucketCapacity;

    // if the bucket has not been allocated yet, then it needs to be
    if (mBuckets[bucketId].mAr == nullptr) {

        // allocate the array of the current bucket, return failed insertion if could not allocate
        mBuckets[bucketId].mAr  = new (std::nothrow) node_ptr_t[sBucketCapacity];
        if (mBuckets [bucketId].mAr == nullptr) {
            DBG_MODE(std::cout << "Could not allocate bucket array while inserting\n";)
            return false;
        }

        // go over each slot in the bucket and make the linked list point to nullptr
        for (uint64_t i = 0; i < sBucketCapacity; ++i) {
            mBuckets[bucketId].mAr[i]   = nullptr;
        }
    }

    // get a pointer to the pointer to the next node
    // pointer to pointer allows for removing special head case
    listElem                    = &mBuckets[bucketId].mAr[bucketPos];

    DBG_MODE (

    if (mBuckets[bucketId].mAr[bucketPos] == nullptr) {
        mSlotsUsed              += 1;
    }
    )

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
        DBG_MODE (std::cout << "Could not allocate new node while inserting\n";)
        return false;
    }

    // place the new node at this position
    (*listElem)                 = node;

    // increment size of bucket and size of table
    if (mBuckets[bucketId].mSize == 0) {
        mBucketsUsed            += 1;
    }

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
    uint64_t    keyHash;                        // hash of the supplied key to insert
    uint64_t    bucketId;                       // bucket in which the key will be inserted
    uint64_t    bucketPos;                      // position in bucket in which the key will be inserted

    node_ptr_t  listElem ;                      // pointer to node in linked list

    // calculate the hash of they key and find its bucket and position in the bucket
    // the bucket is the sBucketSizeLog Most Significant bits, while the
    // position in the bucket is the remaining Least Significant bits of the hash
    keyHash         = mHashFunc ((uint8_t *)&pKey, sizeof (key_t));
    bucketId        = keyHash / sBucketCapacity;
    bucketPos       = keyHash % sBucketCapacity;

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
    uint64_t    keyHash;                        // hash of the supplied key to insert
    uint64_t    bucketId;                       // bucket in which the key will be inserted
    uint64_t    bucketPos;                      // position in bucket in which the key will be inserted

    node_ptr_t  *listElem;                      // pointer to node ptr, for iterating over the linked list in the slot

    node_ptr_t  node;                           // pointer to allocated new node

    // calculate the hash of they key and find its bucket and position in the bucket
    // the bucket is the sBucketSizeLog Most Significant bits, while the
    // position in the bucket is the remaining Least Significant bits of the hash
    keyHash                     = mHashFunc ((uint8_t *)&pKey, sizeof (key_t));
    bucketId                    = keyHash / sBucketCapacity;
    bucketPos                   = keyHash % sBucketCapacity;

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
            node->mPtr                  = nullptr;

            delete node;

            mBuckets[bucketId].mSize    -= 1;
            mSize                       -= 1;

            if (mBuckets[bucketId].mSize == 0) {
                mBucketsUsed            += 1;
            }

            DBG_MODE (

            if (mBuckets[bucketId].mAr[bucketPos] == nullptr) {
                mSlotsUsed              -= 1;
            }
            )

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

/**
 * @brief                           Returns the maximum number of buckets which the table can have
 *
 * @return uint64_t                 Number of buckets which the hash table can have
 */
template <typename key_t, auto mHashFunc>
uint64_t
AgHashTable<key_t, mHashFunc>::bucket_count () const
{
    return sBucketCount;
}

/**
 * @brief                           Returns the number of buckets currently being used
 *
 * @return uint64_t                 Number of buckets currently being used
 */
template <typename key_t, auto mHashFunc>
uint64_t
AgHashTable<key_t, mHashFunc>::buckets_used () const
{
    return mBucketsUsed;
}

/**
 * @brief                           Returns the capacity of a bucket (maximum number of elements a bucket can have)
 *
 * @return uint64_t                 Number of elements which a bucket can have
 */
template <typename key_t, auto mHashFunc>
uint64_t
AgHashTable<key_t, mHashFunc>::bucket_capacity () const
{
    return sBucketCapacity;
}

/**
 * @brief                           Returns the current number of elements in a given bucket
 *
 * @param pIndex                    Index of the bucket whose size is to be returned
 * @return uint64_t                 Number of elements in bucket at position pIndex
 */
template <typename key_t, auto mHashFunc>
uint64_t
AgHashTable<key_t, mHashFunc>::bucket_size (const uint64_t pIndex) const
{
    return mBuckets[pIndex].mSize;
}

DBG_MODE (

/**
 * @brief                           Returns number of slots used in the hash table to accomodate keys
 *
 * @return uint64_t                 Number of slots used to accomodate present keys
 */
template <typename key_t, auto mHashFunc>
uint64_t
AgHashTable<key_t, mHashFunc>::slots_used () const
{
    return mSlotsUsed;
}
)

#undef  DBG_MODE
#undef  NO_DBG_MODE

#endif      // Header Guard
