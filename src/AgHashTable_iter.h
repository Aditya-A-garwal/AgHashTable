/**
 * @file            AgHashTable_iter.h
 * @author          Aditya Agarwal (aditya.agarwal@dumblebots.com)
 * @brief           Implementation of AgHashTable forward and reverse iterator methods
 */

/**
 * @brief                   Construct a new AgHashTable<key_t, tHashFunc, tEquals>::iterator object
 *
 * @param pPtr              Pointer to node to be encapsulated
 * @param pAggrPtr          Pointer to corresponding aggregate node
 * @param pTablePtr         Pointer to the table which contains the node
 */
template <typename key_t, auto tHashFunc, auto tEquals>
AgHashTable<key_t, tHashFunc, tEquals>::iterator::iterator (node_ptr_t pPtr, aggr_ptr_t pAggrPtr, table_ptr_t pTablePtr) :
    mPtr {pPtr}, mAggrPtr {pAggrPtr}, mTablePtr {pTablePtr}
{
}

/**
 * @brief                   Prefix increment operator (increments the iterator if not end() and returns it)
 *
 * @return AgHashTable<key_t, tHashFunc, tEquals>::iterator
 */
template <typename key_t, auto tHashFunc, auto tEquals>
typename AgHashTable<key_t, tHashFunc, tEquals>::iterator
AgHashTable<key_t, tHashFunc, tEquals>::iterator::operator++ ()
{
    hash_t          newHash;                                    /** Hash value of the next key to use (only used if all keys with the current hash value have been iterated over) */
    aggr_ptr_t      newAggrPtr;                                 /** Pointer to the new aggregate node to use (only used if all keys with the current hash value have been iterated over) */

    // if this is the end, return itseld
    if (mPtr == nullptr || mAggrPtr == nullptr || mTablePtr == nullptr) {
        return *this;
    }

    // if another node exists after this one in the linked list, use it and return self
    if (mPtr->nextPtr != nullptr) {
        mPtr    = mPtr->nextPtr;
        return *this;
    }

    // try to find the next hash which is present in the table and use the aggregate node corresponding to it
    newHash     = mAggrPtr->keyHash;
    while (true) {

        // if all hashs have been used, return end()
        if (++newHash == 0) {
            mPtr        = nullptr;
            mAggrPtr    = nullptr;

            return *this;
        }

        // try to find an aggregate node with the given hash
        newAggrPtr      = mTablePtr->getHashAggr (newHash);

        // if such an aggregate node exists, break the loop
        if (newAggrPtr != nullptr) {
            break;
        }
    }

    // use the found aggregate node and the first node in its linked list and return
    mPtr        = newAggrPtr->nodePtr;
    mAggrPtr    = newAggrPtr;

    return *this;
}

/**
 * @brief                   Suffix increment operator (increments the iterator if not end() and returns a copy of the old one)
 *
 * @return AgHashTable<key_t, tHashFunc, tEquals>::iterator
 */
template <typename key_t, auto tHashFunc, auto tEquals>
typename AgHashTable<key_t, tHashFunc, tEquals>::iterator
AgHashTable<key_t, tHashFunc, tEquals>::iterator::operator++ (int)
{
    iterator    res {mPtr, mAggrPtr, mTablePtr};

    ++(*this);

    return res;
}

/**
 * @brief                   Dereferences and returns the value held by the encapsulated node
 *
 * @return AgHashTable<key_t, tHashFunc, tEquals>::iterator::ref_t
 */
template <typename key_t, auto tHashFunc, auto tEquals>
typename AgHashTable<key_t, tHashFunc, tEquals>::iterator::ref_t
AgHashTable<key_t, tHashFunc, tEquals>::iterator::operator* () const
{
    return mPtr->key;
}

/**
 * @brief                   Checks if two iterators point to the same node in the same table
 *
 * @param pOther            Iterator to compare to
 *
 * @return true             If both iterators point to the same node in the same table
 * @return false            If both iterators point to different nodes or different tables
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::iterator::operator== (const iterator &pOther) const
{
    return (mPtr == pOther.mPtr) && (mTablePtr == pOther.mTablePtr);
}

/**
 * @brief                   Checks if two iterators point to different nodes
 *
 * @param pOther            Iterator to compare to
 *
 * @return true             If both iterators point to different nodes (or different tables)
 * @return false            If both iterators point to the same node in the same table
 */
template <typename key_t, auto tHashFunc, auto tEquals>
bool
AgHashTable<key_t, tHashFunc, tEquals>::iterator::operator!= (const iterator &pOther) const
{
    return (mPtr != pOther.mPtr) || (mTablePtr != pOther.mTablePtr);
}
