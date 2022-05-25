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

#ifdef AG_HASH_TABLE_MULTITHREADED_MODE
#define     MULTITHREADED_MODE(...)                 __VA_ARGS__
#else
#define     MULTITHREADED_MODE(...)
#endif

#include <mutex>
#include <shared_mutex>
#include <atomic>

#include <type_traits>
#include <limits>

#include "AgHashFunctions.hpp"

/**
 * @brief                   Default equals comparator to be used by AgHashTable for checking equivalance of keys
 *
 *                          Requires operator== to be overloaded for val_t
 *                          If C style strings (val_t is char * or val_t is const char *), then strcmp is used (unsafe, better to overload using strncmp)
 *
 * @tparam val_t            Type of value to compare
 *
 * @param pA                First operand
 * @param pB                Second operand
 *
 * @return true             If both the operands are equal
 * @return false            If both the operands are not equal
 */
template <typename val_t>
static bool
ag_hashtable_default_equals (const val_t &pA, const val_t &pB)
{
    if constexpr (std::is_same<val_t, char *>::value || std::is_same<val_t, const char *>::value) {
        // if the operands are C-style strings, don't compare pointer values, instead use strcmp to compare internal values
        return strcmp (pA, pB) == 0;
    }
    else {
        return (pA == pB);
    }
}

/**
 * @brief                   AgAVLTree is an implementation of the AVL tree data structure (a type of self balanced binary search tree)
 *
 * @tparam key_t            Type of keys held by the hash table
 * @tparam tHashFunc        Hash function to use (defaults to 16 bit pearson hash)
 * @tparam tEquals          Comparator to use while making equals comparisons (defaults to operator==)
 */
template <typename key_t, auto tHashFunc = ag_fnv1a<key_t, size_t>, auto tEquals = ag_hashtable_default_equals<key_t>>
class AgHashTable {



    protected:



    using       hash_t          = typename std::invoke_result<decltype (tHashFunc), const key_t *>::type;     /** Data type returned by the hash function (must be unsigned integral type */

    static_assert (std::is_unsigned<hash_t>::value, "Return type of hash functions must be unsigned integer");

    /**
     * @brief               Generic node in linked list which stores a key
     *
     */
    struct node_t {

        node_t              *nextPtr;                               /** Pointer to the next node in the linked list */
        key_t               key;                                    /** Key held by the node */

        ~node_t () { delete nextPtr; }                              /** The destructor recursively destroys all nodes in the linked list */
    };

    /**
     * @brief               Aggregate node representing a collection (linked list) of nodes containing keys which all have the same hash
     *
     */
    struct aggregate_node_t {

        aggregate_node_t    *nextPtr;                               /** Pointer to the next aggregate node in the linked list */
        uint64_t            keyCount;                               /** Number of nodes (which contain keys) in the aggregate node's linked list (all have the same hash) */
        hash_t              keyHash;                                /** Common hash value of the keys which the aggregate node represents */
        node_t              *nodePtr;                               /** Pointer to the linked list of nodes which this aggregate node respresents */

        ~aggregate_node_t () { delete nextPtr; delete nodePtr; }
    };

    /**
     * @brief               Bucket in the hash table, representing a collection of keys whose hash's have the same value modulo the number of buckets
     *
     */
    struct bucket_t {

        uint64_t            keyCount            {0ULL};             /** Number of keys in the bucket */
        uint64_t            distinctHashCount   {0ULL};             /** Number of distinct hashs in this bucket (= number of aggregate nodes in the bucket) */
        aggregate_node_t    *hashListHead       {nullptr};          /** Pointer to the linked list of aggregate nodes */
    };


    using       node_ptr_t      = node_t *;                                             /** Helper alias for pointers to linked list nodes */
    using       aggr_ptr_t      = aggregate_node_t *;                                   /** Helper alias for pointers to linked list of aggregate nodes */
    using       bucket_ptr_t    = bucket_t *;                                           /** Helper alias for pointers to buckets/arrays of buckets */


    static constexpr uint64_t   sHashBitness            = sizeof (hash_t) * 8ULL;       /** Bitness of the return type of the hash function */
    static constexpr uint64_t   sNumDistinctAllowed     = 1ULL;                         /** Number of distinct hashs allowed per bucket before resizing is considered */
    static constexpr uint64_t   sNumKeysAllowed         = 16ULL;                        /** Number of keys allowed in a bucket before resizing is considered */
    static constexpr uint64_t   sResizeFactor           = 8ULL;                         /** Factor by which the size of the hash table grows */

    static constexpr uint64_t   sMaxBucketsAllowed      = 1ULL << ((sHashBitness > 24)? /** Maxmimum number of buckets allowed in the hash table */
                                                                    (24)
                                                                    : (sHashBitness));



    public:



    struct iterator {

        protected:

        using table_ptr_t       = const AgHashTable<key_t, tHashFunc, tEquals> *;
        using ref_t             = const key_t &;

        node_ptr_t  mPtr        {nullptr};                                  /** Pointer to table node (nullptr if points to end()) */
        aggr_ptr_t  mAggrPtr    {nullptr};                                  /** Pointer to the aggregate node (nullptri f points to end() */
        table_ptr_t mTablePtr   {nullptr};                                  /** Pointer to table instance */

        public:

        iterator                (node_ptr_t pPtr, aggr_ptr_t pAggrPtr, table_ptr_t pTablePtr);
        iterator                () = default;

        iterator operator++     ();
        iterator operator++     (int);

        ref_t    operator*      () const;

        bool     operator==     (const iterator & pOther) const;
        bool     operator!=     (const iterator & pOther) const;

    };

    //  Constructors

    AgHashTable     ();
    AgHashTable     (const uint64_t &pBucketCount);
    AgHashTable     (const AgHashTable<key_t, tHashFunc, tEquals> &pOther) = delete;

    //  Destructors

    ~AgHashTable    ();

    //  Getters

    bool                initialized             () const;

    uint64_t            size                    () const;
    uint64_t            get_key_count           () const;

    uint64_t            get_bucket_count        () const;
    uint64_t            get_max_bucket_count    () const;

    uint64_t            get_bucket_key_count    (const uint64_t &pBucketId) const;
    uint64_t            get_bucket_hash_count   (const uint64_t &pBucketId) const;

    uint64_t            get_bucket_of_key       (const key_t &pKey) const;
    // Testing and debugging

    DBG_MODE (
    uint64_t            get_alloc_amount        () const;
    uint64_t            get_alloc_count         () const;
    uint64_t            get_delete_count        () const;

    uint64_t            get_resize_count        () const;

    uint64_t            get_aggregate_count     () const;
    )

    iterator            find                    (const key_t &pKey) const;
    bool                exists                  (const key_t &pkey) const;

    //  Modifiers

    bool                insert                  (const key_t &pKey);
    bool                erase                   (const key_t &pKey);

    // Iterators and Iteration

    iterator            begin                   () const;
    iterator            end                     () const;



    private:



    // Getters

    iterator            find_util               (const key_t &pKey, aggr_ptr_t pAggrElem) const;
    bool                exists_util             (const key_t &pKey, node_ptr_t pListElem) const;

    // Modifiers

    void                init                    ();

    bool                insert_util             (const key_t &pKey, node_ptr_t *pListElem);
    bool                erase_util              (const key_t &pKey, node_ptr_t *pListElem);

    bool                resize                  (const uint64_t &pNumBuckets);

    // Iterators

    aggr_ptr_t  getHashAggr                     (const hash_t &pKeyHash) const;


    bucket_ptr_t        mBucketArray;                                       /** Pointer to array of buckets, each containing a linked list of aggregate nodes */

    MULTITHREADED_MODE (
    std::shared_mutex   *mLocks;                                            /** Pointer to array of locks */
    )

    uint64_t            mKeyCount       {0ULL};                             /** Number of keys in the table */
    uint64_t            mBucketCount    {64ULL};                            /** Number of buckets in the table */

    DBG_MODE (
    uint64_t            mAllocAmt       {0ULL};                             /** Number of bytes allocated by the hash table (does not count allocations done by keys internally) */
    uint64_t            mAllocCnt       {0ULL};                             /** Number of times operator new/malloc has been used to perform a new allocation (does not count allocations done by keys internally) */
    uint64_t            mDeleteCnt      {0ULL};                             /** Number of times operator delete/free has been used to free up memory (does not count frees done by keys internally) */

    uint64_t            mResizeCnt      {0ULL};                             /** Number of times the bucket array of the table has been resized (expanded, specifically) */

    uint64_t            mAggregateCnt   {0ULL};                             /** Number of aggregate nodes in the table (=number of distinct hash values in the table) */
    )

};

/**
 * @brief                   Construct a new AgHashTable<key_t, tHashFunc, tEquals>::AgHashTable object
 *
 */
template <typename key_t, auto tHashFunc, auto tEquals>
AgHashTable<key_t, tHashFunc, tEquals>::AgHashTable ()
{
    init ();
}

/**
 * @brief Construct a new AgHashTable<key_t, tHashFunc, tEquals>::AgHashTable object
 *
 * @param pBucketCount      Number of buckets to initialize the hash table with
 */
template <typename key_t, auto tHashFunc, auto tEquals>
AgHashTable<key_t, tHashFunc, tEquals>::AgHashTable (const uint64_t &pBucketCount)
{
    mBucketCount        = pBucketCount;
    init ();
}

/**
 * @brief                   Initialize the hash table with the specified number of buckets
 *
 */
template <typename key_t, auto tHashFunc, auto tEquals>
void
AgHashTable<key_t, tHashFunc, tEquals>::init ()
{
    // try to allocate the array of buckets
    mBucketArray        = new (std::nothrow) bucket_t[mBucketCount];
    DBG_MODE (
    if (mBucketArray == nullptr) {
        std::cout << "Allocation of bucket array failed while constructing\n";
    }
    else {
        ++mAllocCnt;
        mAllocAmt       += sizeof (bucket_t) * mBucketCount;
    }
    )

    MULTITHREADED_MODE (

    mLocks              = new (std::nothrow) std::shared_mutex[mBucketCount];

    DBG_MODE (
    if (mLocks == nullptr) {
        std::cout << "Allocation of locks failed while constructing\n";
    }
    else {
        ++mAllocCnt;
        mAllocAmt       += sizeof (std::shared_mutex) * mBucketCount;
    }
    )
    )

#if defined (AG_DBG_MODE) && defined (AG_PRINT_INIT_INFO)
    std::cout << "sizeof node_t: " << sizeof (node_t) << std::endl;
    std::cout << "sizeof aggregate_node_t: " << sizeof (aggregate_node_t) << std::endl;
    std::cout << "sizeof bucket_t: " << sizeof (bucket_t) << std::endl;
#endif
}

/**
 * @brief                   Destroy the AgHashTable<key_t, tHashFunc, tEquals>::AgHashTable object
 *
 */
template <typename key_t, auto tHashFunc, auto tEquals>
AgHashTable<key_t, tHashFunc, tEquals>::~AgHashTable ()
{
    // iterate through each bucket in the array and delete it
    for (uint64_t bucketId = 0; bucketId < mBucketCount; ++bucketId) {
        delete mBucketArray[bucketId].hashListHead;
    }

    // delete the array of buckets
    delete mBucketArray;
}

/**
 * @brief                   Returns if the table could be successfully initialized
 *
 * @return true             If the table could be successfully initialized
 * @return false            If the table could not be successfully initialized
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::initialized () const
{
    if (mBucketArray == nullptr) {
        return false;
    }

    MULTITHREADED_MODE (
    if (mLocks == nullptr) {
        return false;
    }
    )

    return true;
}

/**
 * @brief                   Returns the number of keys in the hash table (identical to get_key_count())
 *
 * @return uint64_t         Number of keys in the hash table
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::size () const
{
    return mKeyCount;
}

/**
 * @brief                   Returns the number of keys in the hash table (identical to size())
 *
 * @return uint64_t         Number of keys in the hash table
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::get_key_count () const
{
    return mKeyCount;
}

/**
 * @brief                   Returns the number of buckets in the hash table
 *
 * @return uint64_t         Number of buckets in the hash table
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::get_bucket_count () const
{
    return mBucketCount;
}

/**
 * @brief                   Returns the maximum number of buckets which the hash table can have
 *
 * @return uint64_t         Maximum number of buckets which the hash table can have
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::get_max_bucket_count () const
{
    return sMaxBucketsAllowed;
}

DBG_MODE (

/**
 * @brief                   Returns the amount of memory currently allocated by the hash table
 *
 * @return uint64_t         Amount of memory (in bytes) allocated by the hash table
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::get_alloc_amount () const
{
    return mAllocAmt;
}

/**
 * @brief                   Returns the number of allocations performed by the hash table
 *
 * @return uint64_t         Number of allocations performed by the hash table
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::get_alloc_count () const
{
    return mAllocCnt;
}

/**
 * @brief                   Returns the number of times memory has been freed by the hash table
 *
 * @return uint64_t         Number of times memory has been freed by the hash table
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::get_delete_count () const
{
    return mDeleteCnt;
}

/**
 * @brief                   Returns the number of times the hash table has been resized (number of buckets have been changed)
 *
 * @return uint64_t         Number of times the hash table has been resized
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable <key_t, tHashFunc, tEquals>::get_resize_count () const
{
    return mResizeCnt;
}

template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable <key_t, tHashFunc, tEquals>::get_aggregate_count () const
{
    return mAggregateCnt;
}

)

/**
 * @brief                   Returns the number of keys in the supplied bucket
 *
 * @param pBucketId         Position of the bucket whose key count is to be found
 *
 * @return uint64_t         Number of keys in the bucket whose position is given
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::get_bucket_key_count (const uint64_t &pBucketId) const
{
    return (pBucketId < mBucketCount) ? (mBucketArray[pBucketId].keyCount) : (0ULL);
}

/**
 * @brief                   Returns the number of keys with unique hashs in the supplied bucket
 *
 * @param pBucketId         Position of the bucket whose unique hash count is to be returned
 *
 * @return uint64_t         Number of unique hashs in the bucket whose position is given
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::get_bucket_hash_count (const uint64_t &pBucketId) const
{
    return (pBucketId < mBucketCount) ? (mBucketArray[pBucketId].distinctHashCount) : (0ULL);
}

/**
 * @brief                   Returns the bucket in which a key will reside in after insertion
 *
 * @param pKey              Key to get the residing bucket of
 *
 * @return uint64_t         Bucket in which the supplied key will go into after insertion
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::get_bucket_of_key (const key_t &pKey) const
{
    hash_t              keyHash;                                    /** Hash value of the key */
    uint64_t            bucketId;                                   /** Position of the bucket in which to insert the key */

    // calculate the hash value of the key and find the bucket in which it should be insert into
    keyHash         = tHashFunc (&pKey);
    bucketId        = keyHash & (mBucketCount - 1);

    return bucketId;
}

/**
 * @brief                   Returns if a given key exists in the hash table
 *
 * @param pKey              Key to search for
 *
 * @return true             If the supplied key exists in the hash table
 * @return false            If the supplied key does not exist in the hash table
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::exists (const key_t &pKey) const
{
    hash_t              keyHash;                                    /** Hash value of the key */
    uint64_t            bucketId;                                   /** Position of the bucket in which to insert the key */

    aggr_ptr_t          aggrElem;                                   /** Pointer to the new aggregate node's predecessor's next-pointer */

    // calculate the hash value of the key and find the bucket in which it should be insert into
    keyHash         = tHashFunc (&pKey);
    bucketId        = keyHash & (mBucketCount - 1);

    // get a pointer to the pointer to the aggregate list's head
    aggrElem        = mBucketArray[bucketId].hashListHead;

    while (aggrElem != nullptr) {

        // if an aggregate node's representative hash value matches with the key's hash value.
        // try to find the new key in it's linked list
        if (aggrElem->keyHash == keyHash) {
            return exists_util (pKey, aggrElem->nodePtr);
        }

        // go to the next aggregate node
        aggrElem    = aggrElem->nextPtr;
    }

    // if node aggregate node with matching representative hash value is found, return failed find
    return false;
}

/**
 * @brief                   Searches for a given key in the hash table and returns an iterator to it (returns end() if no matching key is found)
 *
 * @param pKey              Key to search for
 *
 * @return true             If the supplied key exists in the hash table
 * @return false            If the supplied key does not exist in the hash table
 */
template <typename key_t, auto tHashFunc, auto tEquals>
typename AgHashTable<key_t, tHashFunc, tEquals>::iterator
AgHashTable<key_t, tHashFunc, tEquals>::find (const key_t &pKey) const
{
    hash_t              keyHash;                                    /** Hash value of the key */
    uint64_t            bucketId;                                   /** Position of the bucket in which to insert the key */

    aggr_ptr_t          aggrElem;                                   /** Pointer to the new aggregate node's predecessor's next-pointer */

    // calculate the hash value of the key and find the bucket in which it should be insert into
    keyHash         = tHashFunc (&pKey);
    bucketId        = keyHash & (mBucketCount - 1);

    // get a pointer to the pointer to the aggregate list's head
    aggrElem        = mBucketArray[bucketId].hashListHead;

    while (aggrElem != nullptr) {

        // if an aggregate node's representative hash value matches with the key's hash value.
        // try to find the new key in it's linked list
        if (aggrElem->keyHash == keyHash) {
            return find_util (pKey, aggrElem);
        }

        // go to the next aggregate node
        aggrElem    = aggrElem->nextPtr;
    }

    // if node aggregate node with matching representative hash value is found, return failed find
    return end ();
}

/**
 * @brief                   Attempts to insert a new key into the hash table
 *
 * @param pKey              Key to insert
 *
 * @return true             If the key could successfully be inserted
 * @return false            If the key could not be inserted (duplicate key found or allocation failure)
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::insert (const key_t &pKey)
{
    hash_t              keyHash;                                    /** Hash value of the key */
    uint64_t            bucketId;                                   /** Position of the bucket in which to insert the key */

    aggr_ptr_t          *aggrElem;                                  /** Pointer to the new aggregate node's predecessor's next-pointer */
    aggr_ptr_t          newAggr;                                    /** Pointer to new aggregate node */

    bool                insertionState;                             /** Stores if insert_util could successfully insert the key into the aggregate node's linked list */

    // calculate the hash value of the key and find the bucket in which it should be insert into
    keyHash         = tHashFunc (&pKey);
    bucketId        = keyHash & (mBucketCount - 1);

    // get a pointer to the pointer to the aggregate list's head
    aggrElem        = &(mBucketArray[bucketId].hashListHead);

    while ((*aggrElem) != nullptr) {

        // if the current aggregate node's representative hash value matches with the key's hash value,
        // try to insert the new key into it's linked list
        if ((*aggrElem)->keyHash == keyHash) {
            insertionState  = insert_util (pKey, &((*aggrElem)->nodePtr));

            // if the insertion was succesful, increment the key counters
            if (insertionState) {
                ++mKeyCount;
                ++(*aggrElem)->keyCount;
                ++mBucketArray[bucketId].keyCount;

                // resize the table if the bucket has too many keys with different hashs
                // dont resize in case the number of keys are > maximum allowed but all have the same hash, since
                // this would still cause all the keys to fall in the same bucket, causing repeated resizing at every subsequent insert
                if ((mBucketArray[bucketId].distinctHashCount > sNumDistinctAllowed)
                    && (mBucketArray[bucketId].keyCount > sNumKeysAllowed)
                    && ((mBucketCount * sResizeFactor) < sMaxBucketsAllowed)) {
                    resize (mBucketCount * sResizeFactor);
                }
            }

            return insertionState;
        }

        // go to the next aggregate node
        aggrElem    = &((*aggrElem)->nextPtr);
    }

    // if the loop was completed but no position was found, an entirely new aggregate node needs to be created for
    // this hash value

    // create a new aggregate node (return failed insertion on failure)
    newAggr         = new (std::nothrow) aggregate_node_t {nullptr, 0ULL, keyHash, nullptr};
    if (newAggr == nullptr) {
        return false;
    }
    else {
        DBG_MODE (
        ++mAllocCnt;
        mAllocAmt   += sizeof (aggregate_node_t);
        )
    }

    // place the new node at the last position and try to insert the new key into it's linked list
    (*aggrElem)     = newAggr;

    insertionState  = insert_util (pKey, &((*aggrElem)->nodePtr));

    // if the insertion was successfull, increment the corresponding key counters
    if (insertionState) {
        ++mKeyCount;
        ++(*aggrElem)->keyCount;
        ++mBucketArray[bucketId].keyCount;
        ++mBucketArray[bucketId].distinctHashCount;
        DBG_MODE (
        ++mAggregateCnt;
        )

        // resize the table if the bucket has too many keys with different hashs
        // dont resize in case the number of keys are > maximum allowed but all have the same hash, since
        // this would still cause all the keys to fall in the same bucket, causing repeated resizing at every subsequent insert
        if ((mBucketArray[bucketId].distinctHashCount > sNumDistinctAllowed)
            && (mBucketArray[bucketId].keyCount > sNumKeysAllowed)
            && ((mBucketCount * sResizeFactor) < sMaxBucketsAllowed)) {
            resize (mBucketCount * sResizeFactor);
        }
    }
    // if the insertion failed, then remove the newly created aggregate node as well
    else {
        // get it and set its predecessor's successor to it's successor
        newAggr             = *aggrElem;
        *aggrElem           = newAggr->nextPtr;

        // set its successor to NULL and delete it
        newAggr->nextPtr    = nullptr;
        delete newAggr;
    }

    return insertionState;
}

/**
 * @brief                   Attempts to erase a given key from the hash table
 *
 * @param pKey              Key to erase
 *
 * @return true             If the key was successfully found and removed
 * @return false            If the key could not be removed (no matching key was found)
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::erase (const key_t &pKey)
{
    hash_t              keyHash;                                    /** Hash value of the key */
    uint64_t            bucketId;                                   /** Position of the bucket in which to insert the key */

    aggr_ptr_t          *aggrElem;                                  /** Pointer to the new aggregate node's predecessor's next-pointer */
    aggr_ptr_t          toRem;                                      /** Pointer to the aggregate node to be removed (only used in case the key was the only key with it's hash value) */

    bool                eraseState;                                 /** Stores if erase_util could successfully erase the node from the aggregate node's linked list */

    // calculate the hash value of the key and find the bucket in which it should be insert into
    keyHash         = tHashFunc (&pKey);
    bucketId        = keyHash & (mBucketCount - 1);

    // get a pointer to the pointer to the aggregate list's head
    aggrElem        = &(mBucketArray[bucketId].hashListHead);

    while ((*aggrElem) != nullptr) {

        // if the current aggregate node's representative hash value matches with the key's hash value,
        // try to insert the new key into it's linked list
        if ((*aggrElem)->keyHash == keyHash) {
            eraseState  = erase_util (pKey, &((*aggrElem)->nodePtr));

            // if successfully erased the key, decrement all key counters
            if (eraseState) {
                --mKeyCount;
                --(*aggrElem)->keyCount;
                --mBucketArray[bucketId].keyCount;

                // if this aggregate node has no elements, delete it and decrement the number of distinct hashs in the bucket
                if ((*aggrElem)->keyCount == 0) {

                    --mBucketArray[bucketId].distinctHashCount;
                    DBG_MODE (
                    --mAggregateCnt;
                    )

                    // take the node out and put it's successor in it's place
                    toRem           = *aggrElem;
                    *aggrElem       = (*aggrElem)->nextPtr;

                    // set it's successor to NULL (to avoid recursive deletion)
                    toRem->nextPtr  = nullptr;

                    // delete it
                    delete toRem;
                }
            }
            return eraseState;
        }

        // go to the next aggregate node
        aggrElem    = &((*aggrElem)->nextPtr);
    }

    // if the loop was completed, but no aggregate node with matching representative hash value could be found,
    // return failed erase
    return false;
}

/**
 * @brief                   Utility function to check if a key exists in an aggregate node's linked list
 *
 * @param pKey              Key to find
 * @param pListElem         Linked list to search in
 *
 * @return true             If the key could successfully be found
 * @return false            If the key could not be found
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::exists_util (const key_t &pKey, node_ptr_t pListElem) const
{
    // iterator through all elements of the linked list
    while (pListElem != nullptr) {

        // if a matching key has been found, return successful find
        if (tEquals (pKey, pListElem->key)) {
            return true;
        }

        // go to the next node
        pListElem   = pListElem->nextPtr;
    }

    // if no duplicate key could be found, return failed find
    return false;
}

/**
 * @brief                   Utility function to search for a key in an aggregate node's linked list and return an iterator to it (end() if no matching key is found)
 *
 * @param pKey              Key to find
 * @param pListElem         Linked list to search in
 *
 * @return true             If the key could successfully be found
 * @return false            If the key could not be found
 */
template <typename key_t, auto tHashFunc, auto tEquals>
typename AgHashTable<key_t, tHashFunc, tEquals>::iterator
AgHashTable<key_t, tHashFunc, tEquals>::find_util (const key_t &pKey, aggr_ptr_t pAggrPtr) const
{
    node_ptr_t      pListElem;                      /** Pointer to nodes in the linked list (used while iterating over the linked list to find a matching key) */

    pListElem       = pAggrPtr->nodePtr;

    // iterator through all elements of the linked list
    while (pListElem != nullptr) {

        // if a matching key has been found, return successful find
        if (tEquals (pKey, pListElem->key)) {
            return iterator {pListElem, pAggrPtr, this};
        }

        // go to the next node
        pListElem   = pListElem->nextPtr;
    }

    // if no duplicate key could be found, return failed find
    return end ();
}

/**
 * @brief                   Utility function to insert a key in an aggregate node's linked list
 *
 * @param pKey              Key to insert
 * @param pKeyHash          Hash of the given key
 * @param pListElem         Linked list to insert the key into
 *
 * @return true             If the key could successfully be inserted
 * @return false            If the key could not be inserted
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::insert_util (const key_t &pKey, node_ptr_t *pListElem)
{
    node_ptr_t          newNode;                                    /** Pointer to new node */

    while ((*pListElem) != nullptr) {

        // if a duplicate key is found to already exist, return failed insertion
        if (tEquals (pKey, (*pListElem)->key)) {
            return false;
        }

        // go to the next node
        pListElem       = &((*pListElem)->nextPtr);
    }

    // if the loop finished executing, no duplicate key exists, so try to insert this
    // try to create a new node (return failed insertion on failure)
    newNode         = new (std::nothrow) node_t {nullptr, pKey};
    if (newNode == nullptr) {
        return false;
    }
    else {
        DBG_MODE (
        ++mAllocCnt;
        mAllocAmt   += sizeof (node_t);
        )
    }

    // place the new node at the vacant position
    (*pListElem)    = newNode;

    return true;
}

/**
 * @brief                   Utility function to erase a key from an aggregate node's linked list
 *
 * @param pKey              Key to erase
 * @param pKeyHash          Hash of the given key
 * @param pListElem         Linked list to erase the key from
 *
 * @return true             If the key could successfully be erased
 * @return false            If the key could not be erased
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::erase_util (const key_t &pKey, node_ptr_t *pListElem)
{
    node_ptr_t          foundNode;                                  /** Pointer to node with matching key */

    while ((*pListElem) != nullptr) {

        // if a node is found with matching key, erase it
        if (tEquals (pKey, (*pListElem)->key)) {

            // take the current node out and set it's predecessor's successor to it's successor
            foundNode           = *pListElem;
            *pListElem          = foundNode->nextPtr;

            // set it's successor to nullptr and delete it
            foundNode->nextPtr  = nullptr;

            delete foundNode;

            DBG_MODE (
            ++mDeleteCnt;
            mAllocAmt           -= sizeof (node_t);
            )

            return true;
        }

        // go to the next node
        pListElem   = &((*pListElem)->nextPtr);
    }

    return true;
}

/**
 * @brief                   Resizes the hash table to have the supplied number of buckets
 *
 *                          Creates a new array of buckets of the specified size and moves aggregate nodes from
 *                          the old bucket array to the new one, after which the old array is deleted
 *
 * @param pNumBuckets       Number of buckets the hash table be resized to
 *
 * @return true             If the hash table could be resized successfully
 * @return false            If the hash table could not be resized successfully (allocation failure)
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::resize (const uint64_t &pNumBuckets)
{
    bucket_ptr_t        newArray;                                   /** New array of buckets to use */
    aggr_ptr_t          *aggrElem;                                  /** Pointer to pointer to an aggregate node (used while iterating over aggregate nodes in the new bucket) */
    aggr_ptr_t          *aggrInsertElem;                            /** Pointer to pointer to an aggregate node (used while iterating over aggregate nodes in the old bucket) */

    uint64_t            newPosition;                                /** For each aggregate node, stores the position in which it will be inserted in the new bucket */

    DBG_MODE (
    ++mResizeCnt;
    )

    // allocate the new array (return failed resize in case of failure)
    newArray        = new (std::nothrow) bucket_t[pNumBuckets];
    if (newArray == nullptr) {
        DBG_MODE (
        std::cout << "Allocation of new bucket array failed while resizing" << std::endl;
        std::cout << "Present Size: " << mBucketCount << std::endl;
        std::cout << "Target Size: " << pNumBuckets << std::endl;
        )

        return false;
    }
    else {
        DBG_MODE (
        ++mAllocCnt;
        mAllocAmt       += sizeof (bucket_t) * pNumBuckets;
        )
    }

    // iterare through all buckets in the old array for moving the aggregate nodes
    for (uint64_t bucketId = 0ULL; bucketId < mBucketCount; ++bucketId) {

        // get a pointer to the pointer to the head of the current bucket's aggregate node list
        aggrInsertElem  = &(mBucketArray[bucketId].hashListHead);

        // while an aggregate node exists in the current list, move it to the new bucket array
        while (*aggrInsertElem != nullptr) {

            // using the hash value of the aggregate node, calculate its position in the new array and get a pointer to it's pointer
            newPosition                     = (*aggrInsertElem)->keyHash & (pNumBuckets - 1);
            aggrElem                        = &(newArray[newPosition].hashListHead);

            // move to the end of the aggregate node list of the new array to find a vacant spot
            while (*aggrElem != nullptr) {
                aggrElem    = &((*aggrElem)->nextPtr);
            }

            // make the found empty spot point to the current aggregate node and accordingly increment the new bucket's distinct hash and key counts
            *aggrElem                       = *aggrInsertElem;
            ++newArray[newPosition].distinctHashCount;
            newArray[newPosition].keyCount  += (*aggrElem)->keyCount;

            // move the aggregate node's successor in its place, and disconnect it from it's successor
            *aggrInsertElem                 = (*aggrInsertElem)->nextPtr;
            (*aggrElem)->nextPtr            = nullptr;
        }
    }

    // delete the old bucket array (which should not have any aggregate nodes left)
    delete mBucketArray;
    DBG_MODE (
    ++mDeleteCnt;
    mAllocAmt       -= sizeof (bucket_t) * mBucketCount;
    )

    // make the current bucket array point to the new array and update the bucket count
    mBucketArray    = newArray;
    mBucketCount    = pNumBuckets;

    return true;
}

/**
 * @brief                   Returns a pointer to the aggregate node which contains nodes with the given hash value
 *
 * @param pKeyHash          Hash value to search for
 *
 * @return aggr_ptr_t       Pointer to aggregate node with the matching hash value
 */
template<typename key_t, auto tHashFunc, auto tEquals>
typename AgHashTable<key_t, tHashFunc, tEquals>::aggr_ptr_t
AgHashTable<key_t, tHashFunc, tEquals>::getHashAggr (const hash_t &pKeyHash) const
{
    uint64_t        bucketId;                                       /** Index of the bucket in which the aggregate node should lie */
    aggr_ptr_t      aggrElem;                                       /** Used to iterator over elements in the linked list of the aggregate node list */

    // calculate the bucket in which the aggregate node should lie in (assuming it exists)
    bucketId        = pKeyHash & (mBucketCount - 1);
    aggrElem        = mBucketArray[bucketId].hashListHead;

    // iterate through all the aggregate nodes in the bucket until one with the maching hash is found
    while (aggrElem != nullptr) {
        if (aggrElem->keyHash == pKeyHash) {
            break;
        }
        aggrElem    = aggrElem->nextPtr;
    }

    // return the found aggregate node (return null in case no such node is found)
    return aggrElem;
}

/**
 * @brief                   Returns an iterator to the key with the smallest hash value in the table
 *
 * @return AgHashTable<key_t, tHashFunc, tEquals>::iterator
 */
template <typename key_t, auto tHashFunc, auto tEquals>
typename AgHashTable<key_t, tHashFunc, tEquals>::iterator
AgHashTable<key_t, tHashFunc, tEquals>::begin () const
{
    //! if the smallest hash value present in the table is too high, then the function may take a lot of time
    //! try to use the fact that the size of the table can't become too big
    //! also use this while incrementing the iterators

    aggr_ptr_t  aggrPtr;                                            /** Pointer to the aggregate node with the minimum hash value in the table */
    hash_t      newHash;                                            /** Minimum hash value present in the table */

    // if no keys are present, return end() iterator
    if (mKeyCount == 0) {
        return end ();
    }

    // start with a hash of 0 and keep incrementing it while no aggregate node is found
    newHash     = 0;
    while (true) {

        aggrPtr     = getHashAggr (newHash);

        if (aggrPtr != nullptr) {
            break;
        }

        // if the keycount suddenyl becomes 0, return end()
        if (mKeyCount == 0) {
            return end ();
        }
    }

    return iterator {aggrPtr->nodePtr, aggrPtr, this};
}

/**
 * @brief                   Returns an iterator to the logical key after the last key
 *
 * @return AgHashTable<key_t, tHashFunc, tEquals>::iterator
 */
template <typename key_t, auto tHashFunc, auto tEquals>
typename AgHashTable<key_t, tHashFunc, tEquals>::iterator
AgHashTable<key_t, tHashFunc, tEquals>::end () const
{
    return iterator {nullptr, nullptr, this};
}

#include "AgHashTable_iter.h"

#undef  DBG_MODE
#undef  NO_DBG_MODE
#undef  MULTITHREADED_MODE

#endif          // Header Guard
