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
    hash_t          newHash;
    aggr_ptr_t      newAggrPtr;

    if (mPtr == nullptr || mAggrPtr == nullptr || mTablePtr == nullptr) {
        return *this;
    }

    if (mPtr->nextPtr != nullptr) {
        mPtr    = mPtr->nextPtr;
        return *this;
    }

    newHash     = mAggrPtr->keyHash;

    while (true) {

        if (++newHash == 0) {
            mPtr        = nullptr;
            mAggrPtr    = nullptr;

            return *this;
        }

        newAggrPtr      = mTablePtr->getHashAggr (newHash);

        if (newAggrPtr != nullptr) {
            break;
        }
    }

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
    return (mPtr == pOther.mPtr) && (mTablePtr == pOther.mtablePtr);
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
