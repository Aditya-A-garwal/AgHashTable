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

#include <type_traits>
#include <limits>

#include "AgHashFunctions.hpp"

/**
 * @brief                   Default equals comparator to be used by AgHashTable for checking equivalance of keys
 *
 * Requires operator== to be overloaded for val_t
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
    return (pA == pB);
}


/**
 * @brief                   AgHashtable is an implementation of the hashtable data structure
 *
 * @tparam key_t            Type of keys held by the hashtable
 * @tparam mHashFunc        Hash Function for calculating the hashes of keys
 */
template <typename key_t, auto mHashFunc = ag_pearson_16_hash, auto mEquals = ag_hashtable_default_equals<key_t>>
class AgHashTable {



protected:



    /**
     * @brief               Node in the linked list at each position for storing keys
     *
     */
    struct node_t {

        node_t          *nextPtr;                       /** Pointer to the next node in the linked list */
        key_t           key;                            /** Stores the key in node */

        ~node_t () { delete nextPtr; }
    };


    /**
     * @brief               Bucket (for recursively holding more buckets, or linked list containing keys)
     *
     */
    struct bucket_t {

        /**
         * Type to alias pointer to internal array of slots, which might be of linked lists or
         * buckets recursively. In case of linked lists, is of node_list_ar_t and in case of buckets,
         * is of bucket_array_t
         */
        using           slot_t  = void *;

        // uint64_t        keyCount        {0ULL};         /** Number of keys in the bucket (at the lowest level) */
        slot_t          slotArray       {nullptr};      /** Array of slots (for linked lists or buckets) */
    };


    using       slot_t              = typename bucket_t::slot_t;

    using       node_ptr_t          = node_t *;         /** Alias for node_t * for greater readability */
    using       node_list_t         = node_t *;         /** Alias for node_t * for explicit linked list instantiation */
    using       node_list_ar_t      = node_list_t *;    /** Alias for node_t ** for explicit instantiation of linked list arrays */

    using       bucket_ptr_t        = bucket_t *;       /** Alias for bucket_t * for greater readability */
    using       bucket_array_t      = bucket_t *;       /** Alias for bucket_t * for explicit instantiation of bucket arrays */


    /**
     * Type of integer returned by given hash function, must be unsigned
     */
    using       hash_t              = typename std::invoke_result<decltype (mHashFunc), const uint8_t *, const uint64_t>::type;
    static_assert (std::is_unsigned<hash_t>::value, "Return type of hash function must be unsigned integer");


    /** Bitness of the hash function being used */
    static constexpr uint64_t   sHashBitness    = sizeof (hash_t) * 8ULL;
    /** Number of slots in each bucket */
    static constexpr uint64_t   sSlotCount      = 4ULL;
    /** log2(sSlotCount) used while calculating the slot id at each level */
    static constexpr uint64_t   sSlotCountLog   = 2ULL;
    /** Bitmask to use for extracting slot ID from hash at each level */
    static constexpr uint64_t   sBitMask        = (sSlotCountLog != 64) ? ((1ULL << sSlotCountLog) - 1ULL) : (0xffffffffffffffffULL);
    static_assert ((1ULL << sSlotCountLog) == sSlotCount, "sSLotCount should be 2 raised to sSlotCountLog");


    /** Number of levels of nesting for buckets (last level is contains linked lists) */
    static constexpr uint64_t   sLvlCount       = (sHashBitness / sSlotCountLog) + (uint64_t)((sHashBitness % sSlotCountLog) != 0);


    bucket_ptr_t            mBucket;                    /** Top level bucket (recursively holds more buckets or linked lists at each position) */

    MULTITHREADED_MODE (
    std::shared_mutex       *mLocks;                    /** Pointer to array storing the locks */
    )

    uint64_t                mKeyCount       {0ULL};     /** Number of keys present in the hashtable */

    DBG_MODE (
    uint64_t                mAllocCnt       {0ULL};     /** Total number of allocations performed */
    uint64_t                mDeallocCnt     {0ULL};     /** Total number of frees performed */
    uint64_t                mAllocAmt       {0ULL};     /** Total number of bytes allocated by the hashtable */
    )



public:



    //  Constructors

    AgHashTable     ();
    AgHashTable     (const AgHashTable<key_t, mHashFunc, mEquals> &) = delete;

    //  Destructors

    ~AgHashTable    ();

    //  Sanity checking

    bool            initialized         () const;

    //  Getters

    bool            find                (const key_t pKey) const;
    uint64_t        size                () const;
    uint64_t        keyCount            () const;

    DBG_MODE (
    uint64_t        getAllocAmount      () const;
    uint64_t        getAllocCount       () const;
    uint64_t        getFreeCount        () const;
    )

    //  Setters/Modifiers

    bool            insert              (const key_t pKey);
    bool            erase               (const key_t pKey);



private:



    // Getters

    template <uint64_t pLvl>
    bool    find_util                   (const key_t &pKey, const hash_t &pKeyHash, const bucket_ptr_t pSlots) const;

    //  Setters/Modifiers

    template <uint64_t pLvl>
    bool    insert_util                 (const key_t &pKey, const hash_t &pKeyHash, bucket_ptr_t pSlots);
    template <uint64_t pLvl>
    bool    erase_util                  (const key_t &pKey, const hash_t &pKeyHash, bucket_ptr_t pSlots);

    template <uint64_t pLvl>
    void    delete_bucket_contents      (bucket_ptr_t pBucket);
};

/**
 * @brief               Construct a new AgHashTable<key_t, mHashFunc, mEquals>::AgHashTable object
 *
 */
template <typename key_t, auto mHashFunc, auto mEquals>
AgHashTable<key_t, mHashFunc, mEquals>::AgHashTable ()
{

    DBG_MODE (
    std::cout << "sizeof node_t:        " << sizeof (node_t) << std::endl;
    std::cout << "sizeof bucket_t:      " << sizeof (bucket_t) << std::endl;

    std::cout << "hash bitness:         " << sHashBitness << std::endl;
    std::cout << "slots in bucket:      " << sSlotCount << std::endl;

    std::cout << "levels in table:      " << sLvlCount << std::endl;
    )

    mBucket     = new (std::nothrow) bucket_t;

    DBG_MODE (
    ++mAllocCnt;
    mAllocAmt   += sizeof (bucket_t);
    if (mBucket == nullptr) {
        std::cout << "Allocation of top level bucket failed while constructing\n";
    }
    )

    MULTITHREADED_MODE (
    mLocks      = new (std::nothrow) std::shared_mutex[sSlotCount];
    DBG_MODE (
    ++mAllocCnt;
    mAllocAmt   += sizeof (std::shared_mutex) * sSlotCount;
    if (mLocks == nullptr) {

        std::cout << "Allocation of locks failed while constructing\n";
    }
    )
    )
}

/**
 * @brief              Destroy the AgHashTable<key_t, mHashFunc, mEquals>::AgHashTable object
 *
 */
template <typename key_t, auto mHashFunc, auto mEquals>
AgHashTable<key_t, mHashFunc, mEquals>::~AgHashTable ()
{
    // delete the contents of the top level bucket and then the bucket itself
    delete_bucket_contents<0> (mBucket);
    delete mBucket;

    DBG_MODE (
    ++mDeallocCnt;
    mAllocAmt   -= sizeof (bucket_t);
    mAllocAmt   -= sizeof (node_t) * mKeyCount;
    )

    mKeyCount   = 0;
}

/**
 * @brief               Check if the hashtable could be successfuly initialized
 *
 * @return true         If the hash table could be successfuly initialized
 * @return false        If the hash table could not be successfuly initialized
 */
template <typename key_t, auto mHashFunc, auto mEquals>
bool
AgHashTable<key_t, mHashFunc, mEquals>::initialized () const
{
    if (mBucket == nullptr) {
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
 * @brief               Search for the supplied key in the hashtable
 *
 * @param pKey          Key to search for
 *
 * @return true         If the supplied key is present in the table
 * @return false        If the supplied key is not present in the table
 */
template <typename key_t, auto mHashFunc, auto mEquals>
bool
AgHashTable<key_t, mHashFunc, mEquals>::find (const key_t pKey) const
{
    bucket_array_t  bucketArray;                                    /** Array of buckets in the top level bucket (alias for slot array) */

    hash_t          keyHash;                                        /** Hash value of the key */
    uint64_t        slotId;                                         /** Position in the top level bucket in which to recursively search*/

    keyHash         = mHashFunc ((uint8_t *)&pKey, sizeof (key_t));
    slotId          = keyHash & sBitMask;

    // lock the entire tree of buckets which are descendants of the corresponding top level bucket
    MULTITHREADED_MODE (
    std::shared_lock<std::shared_mutex> scopedLock (mLocks[slotId]);
    )

    // cast the array of slots into an array of buckets
    bucketArray     = (bucket_array_t)(mBucket->slotArray);

    // if the top level buckets have not been allocated, return false straight away
    if (bucketArray == nullptr) {
        return false;
    }

    return  find_util<1> (pKey, keyHash, (const bucket_ptr_t)(&(bucketArray[slotId])));
}

/**
 * @brief               Return the number of keys in the hastable (identical to keyCount())
 *
 * @return uint64_t     Number of keys in the table
 */
template <typename key_t, auto mHashFunc, auto mEquals>
uint64_t
AgHashTable<key_t, mHashFunc, mEquals>::size () const
{
    return mKeyCount;
}

/**
 * @brief               Return the number of keys in the hastable (identical to size())
 *
 * @return uint64_t     Number of keys in the table
 */
template <typename key_t, auto mHashFunc, auto mEquals>
uint64_t
AgHashTable<key_t, mHashFunc, mEquals>::keyCount () const
{
    return mKeyCount;
}

DBG_MODE (
/**
 * @brief               Return the number of bytes allocated by the hashtable
 *
 * @return uint64_t     Number of bytes allocated by the hashtable
 */
template <typename key_t, auto mHashFunc, auto mEquals>
uint64_t
AgHashTable<key_t, mHashFunc, mEquals>::getAllocAmount () const
{
    return mAllocAmt;
}

/**
 * @brief               Return the number of times memory has been allocated by the hashtable
 *
 * @return uint64_t     Number of bytes allocated by the hashtable
 */
template <typename key_t, auto mHashFunc, auto mEquals>
uint64_t
AgHashTable<key_t, mHashFunc, mEquals>::getAllocCount () const
{
    return mAllocCnt;
}

/**
 * @brief               Return the number of times memory has been freed by the hashtable
 *
 * @return uint64_t     Number of bytes allocated by the hashtable
 */
template <typename key_t, auto mHashFunc, auto mEquals>
uint64_t
AgHashTable<key_t, mHashFunc, mEquals>::getFreeCount () const
{
    return mDeallocCnt;
}
)

/**
 * @brief               Attempt to insert the supplied key in the hashtable
 *
 * @param pKey          Key to insert
 *
 * @return true         If the supplied key could be successfuly inserted
 * @return false        If the supplied key could not be successfuly inserted (allocation failed at some step or duplicate found)
 */
template <typename key_t, auto mHashFunc, auto mEquals>
bool
AgHashTable<key_t, mHashFunc, mEquals>::insert (const key_t pKey)
{
    hash_t          keyHash;                                        /** Array of buckets in the top level bucket (alias for slot array) */
    uint64_t        slotId;                                         /** Hash value of the key */

    bucket_array_t  bucketArray;                                    /** Position in the top level bucket in which to recursively search*/

    keyHash     = mHashFunc ((uint8_t *)&pKey, sizeof (key_t));
    slotId      = keyHash & sBitMask;

    // lock the entire tree of buckets which are descendants of the corresponding top level bucket
    MULTITHREADED_MODE (
    std::lock_guard<std::shared_mutex> scopedLock (mLocks[slotId]);
    )

    // cast the array of slots into an array of buckets
    bucketArray     = (bucket_array_t)(mBucket->slotArray);

    // attempt to allocate the array of buckets (if not already)
    if (bucketArray == nullptr) {

        bucketArray         = new (std::nothrow) bucket_t[sSlotCount];
        DBG_MODE (
        ++mAllocCnt;
        mAllocAmt           += sizeof (bucket_t) * sSlotCount;
        )
        mBucket->slotArray  = (slot_t)(bucketArray);

        // if allocation fails, return false
        if (bucketArray == nullptr) {
            return false;
        }
    }

    return  insert_util<1> (pKey, keyHash, &(bucketArray[slotId]));
}

/**
 * @brief               Attempt to erase the supplied key from the hashtable
 *
 * @param pKey          Key to erase
 *
 * @return true         If the supplied key could be successfuly erased
 * @return false        If the supplied key could not be successfuly erased (no matching key found)
 */
template <typename key_t, auto mHashFunc, auto mEquals>
bool
AgHashTable<key_t, mHashFunc, mEquals>::erase (const key_t pKey)
{
    bucket_array_t  bucketArray;                                    /** Array of buckets in the top level bucket (alias for slot array) */

    hash_t          keyHash;                                        /** Hash value of the key */
    uint64_t        slotId;                                         /** Position in the top level bucket in which to recursively search*/

    keyHash     = mHashFunc ((uint8_t *)&pKey, sizeof (key_t));
    slotId      = keyHash & sBitMask;

    // lock the entire tree of buckets which are descendants of the corresponding top level bucket
    MULTITHREADED_MODE (
    std::lock_guard<std::shared_mutex> scopedLock (mLocks[slotId]);
    )

    // cast the array of slots into an array of buckets
    bucketArray     = (bucket_array_t)(mBucket->slotArray);

    // if the top level buckets have not been allocated yet, return false straight away
    if (bucketArray == nullptr) {
        return false;
    }

    return  erase_util<1> (pKey, keyHash, &(bucketArray[slotId]));
}

/**
 * @brief               Utility method that implements main search algorithm
 *
 * @tparam pLvl         Level at which the supplied bucket lies in
 *
 * @param pKey          Constant reference to key being searched for
 * @param pKeyHash      Hash value of the supplied key
 * @param pBucket       Pointer to the bucket being searched in (must be allocated, i.e. cannot be null)
 *
 * @return true         If the recursive search was succesful (matching key was found)
 * @return false        If the recursive search failed (matching key could not be found)
 */
template <typename key_t, auto mHashFunc, auto mEquals>
template <uint64_t pLvl>
bool
AgHashTable<key_t, mHashFunc, mEquals>::find_util (const key_t &pKey, const hash_t &pKeyHash, const bucket_ptr_t pBucket) const
{
    uint64_t    slotId;

    slotId      = (pKeyHash >> (pLvl * sSlotCountLog)) & (sBitMask);

    // if the slots in the bucket have not been init yet, the key obviously does not exist
    if (pBucket->slotArray == nullptr) {
        return false;
    }

    // not reached bottom layer (current layer contains buckets)
    if constexpr (pLvl != (sLvlCount - 1)) {

        // cast the array of generic slots to an array of buckets and get the pointer to the bucket
        // at a specific position, recursively calling the search until the final level has been reached
        return find_util<pLvl + 1> (pKey, pKeyHash, &(((bucket_ptr_t)(pBucket->slotArray))[slotId]));
    }
    // reached bottom layer (current layer contains linked lists)
    else {
        node_list_ar_t  slotArray;                          /** Array of linked lists in the current bucket */
        node_ptr_t      listElem;                           /** Variable for iterating over each element */

        // get the array as an array of linked lists (from an array of slots) and get the head
        slotArray   = (node_list_t *)(pBucket->slotArray);
        listElem    = slotArray[slotId];

        // iterate through the linked list, trying to find a matching key
        while (listElem != nullptr) {

            // if a matching key is found, return true to indicate successful search
            if (mEquals (listElem->key, pKey)) {
                return true;
            }

            listElem    = listElem->nextPtr;
        }

        return false;
    }
}

/**
 * @brief               Utility method that implements the main insert algorithm
 *
 * @tparam pLvl         Level at which the supplied bucket lies in
 *
 * @param pKey          Constant reference to the key being inserted
 * @param pKeyHash      Hash value of the supplied key
 * @param pBucket       Pointer to the bucket being recursively inserted in (must be allocated, i.e. cannot be null)
 *
 * @return true         If the recursive insertion was successful (no matching key)
 * @return false        If the recursive insertion failed (matching key found or allocation failed)
 */
template <typename key_t, auto mHashFunc, auto mEquals>
template <uint64_t pLvl>
bool
AgHashTable<key_t, mHashFunc, mEquals>::insert_util (const key_t &pKey, const hash_t &pKeyHash, bucket_ptr_t pBucket)
{
    uint64_t    slotId;

    slotId      = (pKeyHash >> (pLvl * sSlotCountLog)) & (sBitMask);

    // not reached bottom layer (current layer contains buckets)
    if constexpr (pLvl != (sLvlCount - 1)) {

        bucket_array_t  bucketArray;                            /** Array of buckets withing the current bucket */

        // cast the array of slots into an array of buckets
        bucketArray     = (bucket_array_t)(pBucket->slotArray);

        // if the slot has not been init yet, try to init and return false on failure
        if (pBucket->slotArray == nullptr) {

            bucketArray         = new (std::nothrow) bucket_t[sSlotCount];
            DBG_MODE (
            ++mAllocCnt;
            mAllocAmt           += sizeof (bucket_t) * sSlotCount;
            )
            pBucket->slotArray  = (slot_t)(bucketArray);
            if (bucketArray == nullptr) {
                return false;
            }
        }

        // try to recursively insert, and increment current bucket's key count if successful
        if (insert_util<pLvl + 1> (pKey, pKeyHash, &(bucketArray[slotId]))) {

            // ++(pBucket->keyCount);
            return true;
        }

        // if recursive insertion failed, return false to indicate failure
        return false;
    }
    // reached bottom layer (current layer contains linked lists)
    else {

        node_list_ar_t  listArray;                              /** Array of linked lists in the current bucket */
        node_ptr_t      *listElem;                              /** Pointer to each element's predecessor;s nextPtr, removed head special case */
        node_ptr_t      newKey;                                 /** New node to be inserted into the linked list */

        // cast the bucket's array of slots into an array of linked lists
        listArray       = (node_list_ar_t)(pBucket->slotArray);

        // if the slot has not been init yet, try to init and return false on failure
        if (pBucket->slotArray == nullptr) {

            listArray           = new (std::nothrow) node_list_t[sSlotCount];
            DBG_MODE (
            ++mAllocCnt;
            mAllocAmt           += sizeof (node_list_t) * sSlotCount;
            )
            pBucket->slotArray  = (slot_t)(listArray);
            if (listArray == nullptr) {
                return false;
            }

            // zero out the entire array
            for (uint64_t slot = 0; slot < sSlotCount; ++slot) {
                listArray[slot]     = nullptr;
            }
        }

        newKey          = new (std::nothrow) node_t {nullptr, pKey};
        DBG_MODE (
        ++mAllocCnt;
        mAllocAmt       += sizeof (node_t);
        )
        listElem        = &(listArray[slotId]);

        // go the end of the linked list, while making sure no duplicate key exists
        while ((*listElem) != nullptr) {

            // if a duplicate key is found, then return false to indicate failed insertion
            if (mEquals ((*listElem)->key, pKey)) {
                return false;
            }

            listElem    = &((*listElem)->nextPtr);
        }

        (*listElem)     = newKey;
        // ++(pBucket->keyCount);
        ++mKeyCount;

        return true;
    }
}

/**
 * @brief               Utility method that implements the main erase algorithm
 *
 * @tparam pLvl         Level at which the supplied bucket lies in
 *
 * @param pKey          Constant reference to the key being erased
 * @param pKeyHash      Hash value of the supplied key
 * @param pSlots        Pointer to the bucket being recursively erased from (must be allocated, i.e. cannot be null)
 *
 * @return true         If the deletion was successful (matching key was found)
 * @return false        If the deletion failed (no matching key)
 */
template <typename key_t, auto mHashFunc, auto mEquals>
template <uint64_t pLvl>
bool
AgHashTable<key_t, mHashFunc, mEquals>::erase_util (const key_t &pKey, const hash_t &pKeyHash, bucket_ptr_t pBucket)
{
    uint64_t    slotId;

    slotId      = (pKeyHash >> (pLvl * sSlotCountLog)) & (sBitMask);

    if constexpr (pLvl != (sLvlCount - 1)) {

        // if the slot has not been init yet, the key obviously does not exist, return false
        if (pBucket->slotArray == nullptr) {
            return false;
        }

        // try to recursively erase, and decrement current bucket's key count if successful
        if (erase_util<pLvl + 1> (pKey, pKeyHash, &(((bucket_ptr_t)(pBucket->slotArray))[slotId]))) {
            // --(pBucket->keyCount);
            return true;
        }

        // if recursive deletion failed, return false to indicate failure
        return false;
    }
    else {

        node_ptr_t  *listElem;
        node_ptr_t  resultant;

        // if the slot has not been init yet, the key obviously does not exist, return false
        if (pBucket->slotArray == nullptr) {
            return false;
        }

        listElem    = &(((node_list_t *)(pBucket->slotArray))[slotId]);

        // iterate over the linked list, trying to find a matching node
        while ((*listElem) != nullptr) {

            // if a duplicate key is found, then return true to indicate successful deletion
            if (mEquals ((*listElem)->key, pKey)) {

                resultant           = *listElem;
                (*listElem)         = (*listElem)->nextPtr;
                resultant->nextPtr  = nullptr;

                delete resultant;
                DBG_MODE (
                ++mDeallocCnt;
                mAllocAmt           -= sizeof (node_t);
                )

                // --(pBucket->keyCount);
                --mKeyCount;

                return true;
            }
            listElem        = &((*listElem)->nextPtr);
        }

        return false;
    }
}

/**
 * @brief               Utility method to recursively delete a bucket's contents
 *
 * @tparam pLvl         The level at which the supplied bucket lies
 *
 * @param pBucket       Pointer to the bucket whose contents are to be deleted
 */
template <typename key_t, auto mHashFunc, auto mEquals>
template <uint64_t pLvl>
void
AgHashTable<key_t, mHashFunc, mEquals>::delete_bucket_contents (bucket_ptr_t pBucket)
{
    if (pBucket->slotArray == nullptr) {
        return;
    }

    if constexpr (pLvl != (sLvlCount - 1)) {

        bucket_ptr_t    bucketArray;

        bucketArray     = (bucket_ptr_t)(pBucket->slotArray);

        for (uint64_t slot = 0; slot < sSlotCount; ++slot) {
            delete_bucket_contents<pLvl + 1> (&(bucketArray[slot]));
        }

        delete[] bucketArray;
        DBG_MODE (
        ++mDeallocCnt;
        mAllocAmt       -= sizeof (bucket_t) * sSlotCount;
        )
    }
    else {

        node_list_t     *listArray;

        listArray       = (node_list_t *)(pBucket->slotArray);

        for (uint64_t slot = 0; slot < sSlotCount; ++slot) {
            delete listArray[slot];
        }

        delete[] listArray;
        DBG_MODE (
        ++mDeallocCnt;
        mAllocAmt       -= sizeof (node_list_t) * sSlotCount;
        )
    }
}

#undef  DBG_MODE
#undef  NO_DBG_MODE
#undef  MULTITHREADED_MODE

#endif          // Header Guard