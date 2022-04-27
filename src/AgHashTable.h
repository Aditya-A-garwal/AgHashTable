/**
 * @file            AgHashTable.h
 * @author          Aditya Agarwal (aditya.agarwal@dumblebots.com)
 *
 * @brief           AgHashTable class
 *
 */

//! if constexpr introduces a new scope, all locks are broken till this is resolved

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

        key_t           key;                            /** Stores the key in node */
        node_t          *nextPtr;                       /** Pointer to the next node in the linked list */

        ~node_t () { delete nextPtr; }
    };


    /**
     * @brief               Bucket (for recursively holding more buckets, or linked list containing keys)
     *
     */
    struct bucket_t {

        /**
         * Type to alias pointer to internal array of slots, which might be of linked lists or
         * buckets recursively. In case of linked lists, is of node_list_array_t and in case
         * of buckets, is of bucket_array_t
         */
        using           slot_t  = void *;

        uint64_t        keyCount        {0ULL};         /** Number of keys in the bucket (at the lowest level) */
        slot_t          slotArray       {nullptr};      /** Array of slots (for linked lists or buckets) */
    };


    using       slot_t              = typename bucket_t::slot_t;

    using       node_ptr_t          = node_t *;         /** Alias for node_t * for greater readability */
    using       node_list_t         = node_t *;         /** Alias for node_t * for explicit linked list instantiation */
    using       node_list_array_t   = node_list_t *;    /** Alias for node_t ** for explicit instantiation of linked list arrays */

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
    static constexpr uint64_t   sSlotCount      = 2ULL;
    /** log2(sSlotCount) used while calculating the slot id at each level */
    static constexpr uint64_t   sSlotCountLog   = 1ULL;
    /** Bitmask to use for extracting slot ID from hash at each level */
    static constexpr uint64_t   sBitMask        = (sSlotCountLog != 64) ? ((1ULL << sSlotCountLog) - 1ULL) : (0xffffffffffffffffULL);
    static_assert ((1ULL << sSlotCountLog) == sSlotCount, "sSLotCount should be 2 raised to sSlotCountLog");


    /** Number of levels of nesting for buckets (last level is contains linked lists) */
    static constexpr uint64_t   sLvlCount       = (sHashBitness / sSlotCountLog) + (uint64_t)((sHashBitness % sSlotCountLog) != 0);


    bucket_t                mBucket;                    /** Top level bucket (recursively holds more buckets or linked lists at each position) */

    MULTITHREADED_MODE (
    std::shared_mutex       *mLocks;                    /** Pointer to array storing the locks */
    )

    uint64_t                mKeyCount       {0ULL};     /** Number of keys present in the hashtable */



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

    //  Setters/Modifiers

    bool            insert              (const key_t pKey);
    bool            erase               (const key_t pKey);



private:



    // Getters

    template <uint64_t pLvl>
    bool    find_util           (const key_t &pKey, const hash_t &pKeyHash, const bucket_ptr_t pSlots) const;

    //  Setters/Modifiers

    template <uint64_t pLvl>
    bool    insert_util         (const key_t &pKey, const hash_t &pKeyHash, bucket_ptr_t pSlots);
    template <uint64_t pLvl>
    bool    erase_util          (const key_t &pKey, const hash_t &pKeyHash, bucket_ptr_t pSlots);

    template <uint64_t pLvl>
    void    delete_bucket       (bucket_ptr_t pBucket);
};

/**
 * @brief               Construct a new AgHashTable<key_t, mHashFunc, mEquals>::AgHashTable object
 *
 */
template <typename key_t, auto mHashFunc, auto mEquals>
AgHashTable<key_t, mHashFunc, mEquals>::AgHashTable ()
{
    MULTITHREADED_MODE (
    mLocks      = new (std::nothrow) std::shared_mutex[sSlotCount];
    DBG_MODE (
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
    delete_bucket<0> (&mBucket);
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
    hash_t      keyHash;

    keyHash     = mHashFunc ((uint8_t *)&pKey, sizeof (key_t));

    return  find_util<0> (pKey, keyHash, (const bucket_ptr_t)&mBucket);
}

/**
 * @brief               Return the number of keys in the hastable (identical to keyCount method)
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
 * @brief               Return the number of keys in the hastable (identical to size method)
 *
 * @return uint64_t     Number of keys in the table
 */
template <typename key_t, auto mHashFunc, auto mEquals>
uint64_t
AgHashTable<key_t, mHashFunc, mEquals>::keyCount () const
{
    return mKeyCount;
}

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
    hash_t      keyHash;

    keyHash     = mHashFunc ((uint8_t *)&pKey, sizeof (key_t));

    return  insert_util<0> (pKey, keyHash, &mBucket);
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
    hash_t      keyHash;

    keyHash     = mHashFunc ((uint8_t *)&pKey, sizeof (key_t));

    return  erase_util<0> (pKey, keyHash, &mBucket);
}

/**
 * @brief
 *
 * @tparam pLvl
 *
 * @param pKey
 * @param pKeyHash
 * @param pSlots
 *
 * @return true
 * @return false
 */
template <typename key_t, auto mHashFunc, auto mEquals>
template <uint64_t pLvl>
bool
AgHashTable<key_t, mHashFunc, mEquals>::find_util (const key_t &pKey, const hash_t &pKeyHash, const bucket_ptr_t pBucket) const
{
    uint64_t    slotId;

    slotId      = (pKeyHash >> (pLvl * sSlotCountLog)) & (sBitMask);

    if constexpr (pLvl == 0) {
        MULTITHREADED_MODE (
        // lock this entire bucket, since this is the top level
        std::shared_lock<std::shared_mutex> scopedLock (mLocks[slotId]);
        )
    }

    // if the slots in the bucket have not been init yet, the key obviously does not exist
    if (pBucket->slotArray == nullptr) {
        return false;
    }

    // not reached bottom layer (current layer contains buckets)
    if constexpr (pLvl != (sLvlCount - 1)) {
        return find_util<pLvl + 1> (pKey, pKeyHash, &(((bucket_ptr_t)(pBucket->slotArray))[slotId]));
    }
    // reached bottom layer (current layer contains linked lists)
    else {

        node_list_t     *slotArray;
        node_ptr_t      listElem;

        slotArray   = (node_list_t *)(pBucket->slotArray);
        listElem    = slotArray[slotId];

        // iterate through the linked list at this position, trying to find a matching key
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
 * @brief
 *
 * @tparam pLvl
 *
 * @param pKey
 * @param pKeyHash
 * @param pBucket
 *
 * @return true
 * @return false
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

        bucket_ptr_t    bucketArray;

        bucketArray     = (bucket_ptr_t)(pBucket->slotArray);

        // if the slot has not been init yet, try to init and return false on failure
        if (pBucket->slotArray == nullptr) {

            bucketArray         = new (std::nothrow) bucket_t[sSlotCount];
            pBucket->slotArray  = (slot_t)(bucketArray);
            if (bucketArray == nullptr) {
                return false;
            }

            for (uint64_t slot = 0; slot < sSlotCount; ++slot) {
                bucketArray[slot] = {0ULL, nullptr};
            }
        }

        // try to recursively insert, and increment current bucket's key count if successful
        if (insert_util<pLvl + 1> (pKey, pKeyHash, &(bucketArray[slotId]))) {
            ++(pBucket->keyCount);
            return true;
        }

        // if recursive insertion failed, return false to indicate failure
        return false;
    }
    // reached bottom layer (current layer contains linked lists)
    else {

        node_list_t     *listArray;
        node_ptr_t      *listElem;
        node_ptr_t      newKey;

        listArray       = (node_list_t *)(pBucket->slotArray);

        // if the slot has not been init yet, try to init and return false on failure
        if (pBucket->slotArray == nullptr) {

            listArray           = new (std::nothrow) node_list_t[sSlotCount];
            pBucket->slotArray  = (slot_t)(listArray);
            if (listArray == nullptr) {
                return false;
            }

            for (uint64_t slot = 0; slot < sSlotCount; ++slot) {
                listArray[slot]     = nullptr;
            }
        }

        newKey          = new (std::nothrow) node_t {pKey, nullptr};
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
        ++(pBucket->keyCount);
        ++mKeyCount;

        return true;
    }
}

/**
 * @brief
 *
 * @tparam pLvl
 *
 * @param pKey
 * @param pKeyHash
 * @param pSlots
 *
 * @return true
 * @return false
 */
template <typename key_t, auto mHashFunc, auto mEquals>
template <uint64_t pLvl>
bool
AgHashTable<key_t, mHashFunc, mEquals>::erase_util (const key_t &pKey, const hash_t &pKeyHash, bucket_ptr_t pBucket)
{
    uint64_t    slotId;

    slotId      = (pKeyHash >> (pLvl * sSlotCountLog)) & (sBitMask);

    if constexpr (pLvl == 0) {
        MULTITHREADED_MODE (
        // lock this entire bucket, since this is the top level
        std::lock_guard<std::shared_mutex> scopedLock (mLocks[slotId]);
        )
    }

    if constexpr (pLvl != (sLvlCount - 1)) {

        // if the slot has not been init yet, the key obviously does not exist, return false
        if (pBucket->slotArray == nullptr) {
            return false;
        }

        // try to recursively erase, and decrement current bucket's key count if successful
        if (erase_util<pLvl + 1> (pKey, pKeyHash, &(((bucket_ptr_t)(pBucket->slotArray))[slotId]))) {
            --(pBucket->keyCount);
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

                --(pBucket->keyCount);
                --mKeyCount;

                return true;
            }
            listElem        = &((*listElem)->nextPtr);
        }

        return false;
    }
}

template <typename key_t, auto mHashFunc, auto mEquals>
template <uint64_t pLvl>
void
AgHashTable<key_t, mHashFunc, mEquals>::delete_bucket (bucket_ptr_t pBucket)
{
    if (pBucket->slotArray == nullptr) {
        return;
    }

    if constexpr (pLvl != (sLvlCount - 1)) {

        bucket_ptr_t    bucketArray;

        bucketArray     = (bucket_ptr_t)(pBucket->slotArray);

        for (uint64_t slot = 0; slot < sSlotCount; ++slot) {
            delete_bucket<pLvl + 1> (&(bucketArray[slot]));
        }

        delete bucketArray;
    }
    else {

        node_list_t     *listArray;

        listArray       = (node_list_t *)(pBucket->slotArray);

        for (uint64_t slot = 0; slot < sSlotCount; ++slot) {
            delete listArray[slot];
        }

        delete listArray;
    }
}

#undef  DBG_MODE
#undef  NO_DBG_MODE
#undef  MULTITHREADED_MODE

#endif          // Header Guard