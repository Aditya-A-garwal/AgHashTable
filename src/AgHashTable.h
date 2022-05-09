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
    if constexpr (std::is_same<val_t, char *>::value or std::is_same<val_t, const char *>::value) {
        // if the operands are C-style strings, don't compare pointer values, instead use strcmp to compare internal values
        return strcmp (pA, pB) == 0;
    }
    else {
        return (pA == pB);
    }
}

/**
 * @brief
 *
 * @tparam key_t
 * @tparam tHashFunc
 * @tparam tEquals
 */
template <typename key_t, auto tHashFunc = ag_pearson_16_hash, auto tEquals = ag_hashtable_default_equals<key_t>>
class AgHashTable {



    protected:



    using       hash_t          = typename std::invoke_result<decltype (tHashFunc), const uint8_t *, const uint64_t>::type;     /** */
    static_assert (std::is_unsigned<hash_t>::value, "Return type of hash functions must be unsigned integer");

    /**
     * @brief
     *
     */
    struct node_t {

        node_t              *nextPtr;                               /** */
        key_t               key;                                    /** */

        ~node_t () { delete nextPtr; }
    };

    /**
     * @brief
     *
     */
    struct aggregate_node_t {

        aggregate_node_t    *nextPtr;                               /** */
        uint64_t            keyCount;                               /** */
        hash_t              keyHash;                                /** */
        node_t              *nodePtr;                               /** */

        ~aggregate_node_t () { delete nextPtr; delete nodePtr; }
    };

    /**
     * @brief
     *
     */
    struct bucket_t {

        uint64_t            keyCount            {0ULL};             /** */
        uint64_t            distinctHashCount   {0ULL};             /** */
        aggregate_node_t    *hashListHead       {nullptr};          /** */
    };


    using       node_ptr_t      = node_t *;                                             /** */
    using       aggr_ptr_t      = aggregate_node_t *;                                   /** */
    using       bucket_ptr_t    = bucket_t *;                                           /** */


    static constexpr uint64_t   sHashBitness            = sizeof (hash_t) * 8ULL;       /** */
    static constexpr uint64_t   sNumDistinctAllowed     = 2ULL;                         /** */
    static constexpr uint64_t   sNumKeysAllowed         = 16ULL;                        /** */
    static constexpr uint64_t   sResizeFactor           = 8ULL;                         /** */



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

    void                init ();

    //  Destructors

    ~AgHashTable    ();

    //  Getters

    bool                initialized     () const;

    uint64_t            size            () const;
    uint64_t            getKeyCount     () const;

    uint64_t            getBucketCount  () const;

    // Testing and debugging

    DBG_MODE (
    uint64_t            getAllocAmount  () const;
    uint64_t            getAllocCount   () const;
    uint64_t            getDeleteCount  () const;

    uint64_t            getResizeCount  () const;
    )

    bool                find            (const key_t &pKey) const;

    //  Modifiers

    bool                insert          (const key_t &pKey);
    bool                erase           (const key_t &pKey);

    // Iterators and Iteration

    iterator            begin           () const;
    iterator            end             () const;



    private:



    // Getters

    bool        find_util       (const key_t &pKey, node_ptr_t pListElem) const;

    // Modifiers

    bool        insert_util     (const key_t &pKey, node_ptr_t *pListElem);
    bool        erase_util      (const key_t &pKey, node_ptr_t *pListElem);

    bool        resize          (const uint64_t &pNumBuckets);

    // Iterators

    aggr_ptr_t  getHashAggr     (const hash_t &pKeyHash) const;


    bucket_ptr_t        mBucketArray;

    MULTITHREADED_MODE (
    std::shared_mutex   *mLocks;
    )

    uint64_t            mKeyCount       {0ULL};
    uint64_t            mBucketCount    {256ULL};

    DBG_MODE (
    uint64_t            mAllocAmt       {0ULL};
    uint64_t            mAllocCnt       {0ULL};
    uint64_t            mDeleteCnt      {0ULL};

    uint64_t            mResizeCnt      {0ULL};
    )

};

/**
 * @brief Construct a new AgHashTable<key_t, tHashFunc, tEquals>::AgHashTable object
 *
 */
template <typename key_t, auto tHashFunc, auto tEquals>
AgHashTable<key_t, tHashFunc, tEquals>::AgHashTable ()
{
    init ();
}

template <typename key_t, auto tHashFunc, auto tEquals>
AgHashTable<key_t, tHashFunc, tEquals>::AgHashTable (const uint64_t &pBucketCount)
{
    mBucketCount        = pBucketCount;
    init ();
}

template <typename key_t, auto tHashFunc, auto tEquals>
void
AgHashTable<key_t, tHashFunc, tEquals>::init ()
{
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

    DBG_MODE (
    std::cout << "sizeof node_t: " << sizeof (node_t) << std::endl;
    std::cout << "sizeof aggregate_node_t: " << sizeof (aggregate_node_t) << std::endl;
    std::cout << "sizeof bucket_t: " << sizeof (bucket_t) << std::endl;
    )
}

/**
 * @brief Destroy the AgHashTable<key_t, tHashFunc, tEquals>::AgHashTable object
 *
 */
template <typename key_t, auto tHashFunc, auto tEquals>
AgHashTable<key_t, tHashFunc, tEquals>::~AgHashTable ()
{
    for (uint64_t bucketId = 0; bucketId < mBucketCount; ++bucketId) {
        delete mBucketArray[bucketId].hashListHead;
    }

    delete mBucketArray;
}

/**
 * @brief
 *
 * @return true
 * @return false
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
 * @brief
 *
 * @return uint64_t
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::size () const
{
    return mKeyCount;
}

/**
 * @brief
 *
 * @return uint64_t
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::getKeyCount () const
{
    return mKeyCount;
}

/**
 * @brief
 *
 * @return uint64_t
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::getBucketCount () const
{
    return mBucketCount;
}

DBG_MODE (

/**
 * @brief
 *
 * @return uint64_t
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::getAllocAmount () const
{
    return mAllocAmt;
}

/**
 * @brief
 *
 * @return uint64_t
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::getAllocCount () const
{
    return mAllocCnt;
}

/**
 * @brief
 *
 * @return uint64_t
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable<key_t, tHashFunc, tEquals>::getDeleteCount () const
{
    return mDeleteCnt;
}

/**
 * @brief
 *
 * @return uint64_t
 */
template <typename key_t, auto tHashFunc, auto tEquals>
uint64_t
AgHashTable <key_t, tHashFunc, tEquals>::getResizeCount () const
{
    return mResizeCnt;
}

)

/**
 * @brief
 *
 * @param pKey
 *
 * @return true
 * @return false
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::find (const key_t &pKey) const
{
    hash_t              keyHash;                                    /** Hash value of the key */
    uint64_t            bucketId;                                   /** Position of the bucket in which to insert the key */

    aggr_ptr_t          aggrElem;                                   /** Pointer to the new aggregate node's predecessor's next-pointer */

    // calculate the hash value of the key and find the bucket in which it should be insert into
    keyHash         = tHashFunc ((uint8_t *)&pKey, sizeof (key_t));
    bucketId        = keyHash & (mBucketCount - 1);

    // get a pointer to the pointer to the aggregate list's head
    aggrElem        = mBucketArray[bucketId].hashListHead;

    while (aggrElem != nullptr) {

        // if an aggregate node's representative hash value matches with the key's hash value.
        // try to find the new key in it's linked list
        if (aggrElem->keyHash == keyHash) {
            return find_util (pKey, aggrElem->nodePtr);
        }

        // go to the next aggregate node
        aggrElem    = aggrElem->nextPtr;
    }

    // if node aggregate node with matching representative hash value is found, return failed find
    return false;
}

/**
 * @brief
 *
 * @param pKey
 *
 * @return true
 * @return false
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::insert (const key_t &pKey)
{
    hash_t              keyHash;                                    /** Hash value of the key */
    uint64_t            bucketId;                                   /** Position of the bucket in which to insert the key */

    aggr_ptr_t          *aggrElem;                                  /** Pointer to the new aggregate node's predecessor's next-pointer */
    aggr_ptr_t          newAggr;                                    /** Pointer to new aggregate node */

    bool                insertionState;                             /** */

    // calculate the hash value of the key and find the bucket in which it should be insert into
    keyHash         = tHashFunc ((uint8_t *)&pKey, sizeof (key_t));
    bucketId        = keyHash & (mBucketCount - 1);

    // get a pointer to the pointer to the aggregate list's head
    aggrElem        = &(mBucketArray[bucketId].hashListHead);

    while ((*aggrElem) != nullptr) {

        // if the current aggregate node's representative hash value matches with the key's hash value,
        // try to insert the new key into it's linked list
        if ((*aggrElem)->keyHash == keyHash) {
            insertionState  = insert_util (pKey, &((*aggrElem)->nodePtr));

            if (insertionState) {
                ++mKeyCount;
                ++(*aggrElem)->keyCount;
                if ((mBucketArray[bucketId].distinctHashCount > sNumDistinctAllowed) && (++mBucketArray[bucketId].keyCount > sNumKeysAllowed)) {
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

    if (insertionState) {
        ++mKeyCount;
        ++(*aggrElem)->keyCount;
        if ((++mBucketArray[bucketId].distinctHashCount > sNumDistinctAllowed) && (++mBucketArray[bucketId].keyCount > sNumKeysAllowed)) {
            resize (mBucketCount * sResizeFactor);
        }
    }

    return insertionState;
}

/**
 * @brief
 *
 * @param pKey
 *
 * @return true
 * @return false
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::erase (const key_t &pKey)
{
    hash_t              keyHash;                                    /** Hash value of the key */
    uint64_t            bucketId;                                   /** Position of the bucket in which to insert the key */

    aggr_ptr_t          *aggrElem;                                  /** Pointer to the new aggregate node's predecessor's next-pointer */

    bool                eraseState;                                 /** */

    // calculate the hash value of the key and find the bucket in which it should be insert into
    keyHash         = tHashFunc ((uint8_t *)&pKey, sizeof (key_t));
    bucketId        = keyHash & (mBucketCount - 1);

    // get a pointer to the pointer to the aggregate list's head
    aggrElem        = &(mBucketArray[bucketId].hashListHead);

    while ((*aggrElem) != nullptr) {

        // if the current aggregate node's representative hash value matches with the key's hash value,
        // try to insert the new key into it's linked list
        if ((*aggrElem)->keyHash == keyHash) {
            eraseState  = erase_util (pKey, &((*aggrElem)->nodePtr));
            if (eraseState) {
                --mKeyCount;
                --(*aggrElem)->keyCount;
                --mBucketArray[bucketId].keyCount;
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
 * @brief
 *
 * @param pKey
 * @param pListElem
 *
 * @return true
 * @return false
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::find_util (const key_t &pKey, node_ptr_t pListElem) const
{
    while (pListElem != nullptr) {

        // if a matching key has been found, return successful find
        if (tEquals (pKey, pListElem->key)) {
            return true;
        }

        pListElem   = pListElem->nextPtr;
    }

    // if no duplicate key could be found, return failed find
    return false;
}

/**
 * @brief
 *
 * @param pKey
 * @param pKeyHash
 * @param pListElem
 *
 * @return true
 * @return false
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
 * @brief
 *
 * @param pKey
 * @param pKeyHash
 * @param pListElem
 *
 * @return true
 * @return false
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
 * @brief
 *
 * @param pNumBuckets
 *
 * @return true
 * @return false
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::resize (const uint64_t &pNumBuckets)
{
    bucket_ptr_t        newArray;
    aggr_ptr_t          *aggrElem;
    aggr_ptr_t          *aggrInsertElem;

    uint64_t            newPosition;

    DBG_MODE (
    ++mResizeCnt;
    )

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

    for (uint64_t bucketId = 0ULL; bucketId < mBucketCount; ++bucketId) {

        aggrInsertElem  = &(mBucketArray[bucketId].hashListHead);

        while (*aggrInsertElem != nullptr) {

            newPosition                     = (*aggrInsertElem)->keyHash & (pNumBuckets - 1);
            aggrElem                        = &(newArray[newPosition].hashListHead);

            while (*aggrElem != nullptr) {
                aggrElem    = &((*aggrElem)->nextPtr);
            }

            *aggrElem                       = *aggrInsertElem;
            ++newArray[newPosition].distinctHashCount;
            newArray[newPosition].keyCount  += (*aggrElem)->keyCount;

            *aggrInsertElem                 = (*aggrInsertElem)->nextPtr;
            (*aggrElem)->nextPtr            = nullptr;
        }
    }

    delete mBucketArray;
    DBG_MODE (
    ++mDeleteCnt;
    mAllocAmt       -= sizeof (bucket_t) * mBucketCount;
    )

    mBucketArray    = newArray;
    mBucketCount    = pNumBuckets;

    return true;
}

/**
 * @brief
 *
 * @param pKeyHash
 *
 * @return aggr_ptr_t
 */
template<typename key_t, auto tHashFunc, auto tEquals>
typename AgHashTable<key_t, tHashFunc, tEquals>::aggr_ptr_t
AgHashTable<key_t, tHashFunc, tEquals>::getHashAggr (const hash_t &pKeyHash) const
{
    uint64_t        bucketId;
    aggr_ptr_t      aggrElem;

    bucketId        = pKeyHash & (mBucketCount - 1);
    aggrElem        = mBucketArray[bucketId].hashListHead;

    while (aggrElem != nullptr) {
        if (aggrElem->keyHash == pKeyHash) {
            break;
        }
        aggrElem    = aggrElem->nextPtr;
    }

    return aggrElem;
}

/**
 * @brief
 *
 * @return AgHashTable<key_t, tHashFunc, tEquals>::iterator
 */
template <typename key_t, auto tHashFunc, auto tEquals>
typename AgHashTable<key_t, tHashFunc, tEquals>::iterator
AgHashTable<key_t, tHashFunc, tEquals>::begin () const
{
    aggr_ptr_t  newAggrPtr;
    hash_t      newHash;

    if (mKeyCount == 0) {
        return end ();
    }

    newHash     = 0;

    while (true) {

        newAggrPtr  = getHashAggr (newHash);

        if (newAggrPtr != nullptr) {
            break;
        }

        if (++newHash == 0 || mKeyCount == 0) {
            return end ();
        }
    }

    return iterator {newAggrPtr->nodePtr, newAggrPtr, this};
}

/**
 * @brief
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
