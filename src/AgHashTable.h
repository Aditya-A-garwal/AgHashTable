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


template <typename key_t, auto mHashFunc>
class AgHashTable {


NO_TEST_MODE(protected:)
TEST_MODE(public:)


    struct node_t {
        // pointer to next node in the linked list
        node_t      *mPtr   {nullptr};
        // the key value
        key_t       mKey;
    };

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

    uint64_t        mSize;

    TEST_MODE(
    uint64_t        mSlots;
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


template <typename val_t, auto mHashFunc>
AgHashTable<val_t, mHashFunc>::AgHashTable ()
{
    mBuckets        = new (std::nothrow) bucket_t[sBucketCount];

    if (mBuckets == nullptr) {
        TEST_MODE(std::cout << "Could not allocate buckets\n";)
        return;
    }
}

template <typename val_t, auto mHashFunc>
AgHashTable<val_t, mHashFunc>::~AgHashTable ()
{
    delete[] mBuckets;
}

template <typename key_t, auto mHashFunc>
bool
AgHashTable<key_t, mHashFunc>::insert (const key_t pKey)
{
    uint64_t    keyHash;
    uint64_t    bucketId;
    uint64_t    bucketPos;

    node_ptr_t  *listElem;
    node_ptr_t  node;

    keyHash                     = mHashFunc (pKey);

    bucketId                    = keyHash >> sBucketSizeLog;
    bucketPos                   = keyHash & ((1ULL << sBucketSizeLog) - 1);

    if (mBuckets[bucketId].mAr == nullptr) {

        mBuckets[bucketId].mAr  = new (std::nothrow) node_ptr_t[1ULL << sBucketSizeLog];
        if (mBuckets[bucketId].mAr == nullptr) {
            TEST_MODE(
            std::cout << "Could not allocate buckets while inserting\n";
            )
            return false;
        }

        for (uint64_t i = 0; i < sBucketSize; ++i) {
            mBuckets[bucketId].mAr[i]   = nullptr;
        }
    }

    listElem                    = &mBuckets[bucketId].mAr[bucketPos];
    while ((*listElem) != nullptr) {

        if ((*listElem)->mKey == pKey) {
            return false;
        }

        listElem    = &((*listElem)->mPtr);
    }

    node                        = new (std::nothrow) node_t {nullptr, pKey};
    if (node == nullptr) {
        TEST_MODE(
        std::cout << "Could not allocate new node while inserting\n";
        )
        return false;
    }

    TEST_MODE(
    if (mBuckets[bucketId].mAr[bucketPos] == nullptr) {
        mSlots                  += 1;
    }
    )

    (*listElem)                 = node;

    mBuckets[bucketId].mSize    += 1;
    mSize                       += 1;

    return true;
}

template <typename key_t, auto mHashFunc>
bool
AgHashTable<key_t, mHashFunc>::find (const key_t pKey) const
{
    uint64_t    keyHash;
    uint64_t    bucketId;
    uint64_t    bucketPos;

    node_ptr_t  listElem;

    keyHash         = mHashFunc (pKey);

    bucketId        = keyHash >> sBucketSizeLog;
    bucketPos       = keyHash & ((1ULL << sBucketSizeLog) - 1);

    // this value does not exist in the hashtable
    if (mBuckets[bucketId].mAr == nullptr) {
        return false;
    }

    // get head of linked list at that position and iterate over it while trying to find the value
    listElem        = mBuckets[bucketId].mAr[bucketPos];

    while (listElem != nullptr) {

        if (listElem->mKey == pKey) {
            return true;
        }
        listElem    = listElem->mPtr;
    }

    return false;
}

template <typename key_t, auto mHashFunc>
bool
AgHashTable<key_t, mHashFunc>::erase (const key_t pKey)
{
    uint64_t    keyHash;
    uint64_t    bucketId;
    uint64_t    bucketPos;

    node_ptr_t  *listElem;
    node_ptr_t  node;

    keyHash         = mHashFunc (pKey);

    bucketId        = keyHash >> sBucketSizeLog;
    bucketPos       = keyHash & ((1ULL << sBucketSizeLog) - 1);

    if (mBuckets[bucketId].mAr == nullptr) {
        return false;
    }

    listElem        = &mBuckets[bucketId].mAr[bucketPos];

    while ((*listElem) != nullptr) {

        if ((*listElem)->mKey == pKey) {

            node                        = *listElem;
            (*listElem)                 = node->mPtr;

            mBuckets[bucketId].mSize    -= 1;
            mSize                       -= 1;

            TEST_MODE(
            if (mBuckets[bucketId].mAr[bucketPos] == nullptr) {
                mSlots                  -= 1;
            }
            )

            delete node;

            return true;
        }

        listElem    = &((*listElem)->mPtr);
    }

    return false;
}

template <typename key_t, auto mHashFunc>
uint64_t
AgHashTable<key_t, mHashFunc>::size () const
{
    return mSize;
}

#undef  TEST_MODE
#undef  NO_TEST_MODE

#endif      // Header Guard