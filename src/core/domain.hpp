/*
 * mini-cp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * mini-cp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c)  2018. by Laurent Michel, Pierre Schaus, Pascal Van Hentenryck
 */

#ifndef __BITDOMAIN_H
#define __BITDOMAIN_H

#include <assert.h>
#include "handle.hpp"
#include "trailable.hpp"
#include "store.hpp"

/**
 * @file domain.hpp
 * @brief Defines the event notification API for all the domains and define two domain implementations.
 */ 

/**
 * @brief Macro to get the bit at a specific position.
 * @param b The position of the bit to get.
 * @return True if the bit at position b is set, false otherwise.
 */
#define GETBIT(b) ((_dom[((b) - _imin)>>5] & (0x1 << (((b)-_imin) & 0x1f)))!=0)

/**
 * @brief Interface for integer notifiers.
 */
struct IntNotifier   {
    /**
     * @brief Notifies that the domain is empty.
     */
    virtual void empty() = 0;

    /**
     * @brief Notifies that the domain is bound.
     */
    virtual void bind() = 0;

    /**
     * @brief Notifies that the domain has changed.
     */
    virtual void change() = 0;

    /**
     * @brief Notifies that the minimum value of the domain has changed.
     */
    virtual void changeMin() = 0;

    /**
     * @brief Notifies that the maximum value of the domain has changed.
     */
    virtual void changeMax() = 0;
};

/**
 * @brief Class representing a domain with a bit vector implementation.
 */
class BitDomain {
   trail<int>*               _dom;
   trail<int>       _min,_max,_sz;
   const int        _imin;
   int count(int from,int to) const;
   int findMin(int from) const;
   int findMax(int from) const;
   void setZero(int at) const noexcept {
      at -= _imin;
      const int mw = at >> 5,  mb = at & 0x1f;
      _dom[mw] = _dom[mw] & ~(0x1 << mb);
   }
public:
    /**
     * @brief Type definition for a pointer to a BitDomain.
     */
    typedef handle_ptr<BitDomain>  Ptr;

    /**
     * @brief Constructor for BitDomain.
     * @param eng Pointer to the trailer.
     * @param store Pointer to the storage.
     * @param min The minimum value of the domain.
     * @param max The maximum value of the domain.
     */
    BitDomain(Trailer::Ptr eng,Storage::Ptr store,int min,int max);

    /**
     * @brief Gets the minimum value of the domain.
     * @return The minimum value of the domain.
     */
    int min() const { return _min;}

    /**
     * @brief Gets the maximum value of the domain.
     * @return The maximum value of the domain.
     */
    int max() const { return _max;}

    /**
     * @brief Gets the size of the domain.
     * @return The size of the domain.
     */
    int size() const { return _sz;}

    /**
     * @brief Checks if the domain is bound.
     * @return True if the domain is bound, false otherwise.
     */
    bool isBound() const { return _sz == 1;}

    /**
     * @brief Checks if a value is a member of the domain.
     * @param v The value to check.
     * @return True if the value is a member of the domain, false otherwise.
     */
    bool member(int v) const noexcept { return _min <= v && v <= _max && GETBIT(v);}

    /**
     * @brief Checks if a value is a member of the base domain.
     * @param v The value to check.
     * @return True if the value is a member of the base domain, false otherwise.
     */
    bool memberBase(int v) const noexcept { return GETBIT(v);}

    /**
     * @brief Checks if the domain has changed.
     * @return True if the domain has changed, false otherwise.
     */
    bool changed() const noexcept { return !_sz.fresh();}

    /**
     * @brief Assigns a value to the domain.
     * @param v The value to assign.
     * @param x The integer notifier.
     */
    void assign(int v,IntNotifier& x);

    /**
     * @brief Removes a value from the domain.
     * @param v The value to remove.
     * @param x The integer notifier.
     */
    void remove(int v,IntNotifier& x);

    /**
     * @brief Removes all values below a new minimum.
     * @param newMin The new minimum value.
     * @param x The integer notifier.
     */
    void removeBelow(int newMin,IntNotifier& x);

    /**
     * @brief Removes all values above a new maximum.
     * @param newMax The new maximum value.
     * @param x The integer notifier.
     */
    void removeAbove(int newMax,IntNotifier& x);

    /**
     * @brief Overloads the output stream operator for BitDomain.
     * @param os The output stream.
     * @param x The BitDomain object.
     * @return The output stream.
     */
    friend std::ostream& operator<<(std::ostream& os,const BitDomain& x);
};

/**
 * @brief Class representing a sparse set.
 */
class SparseSet {
    std::vector<int> _values;
    std::vector<int> _indexes;
    trail<int>       _size,_min,_max;
    int              _ofs,_n;
    void exchangePositions(int val1,int val2);
    bool checkVal(int val) const { assert(val <= (int)_values.size()-1);return true;}
    bool internalContains(int val) const {
        if (val < 0 || val >= _n)
            return false;
        else return _indexes[val] < _size;
    }
    void updateBoundsValRemoved(int val);
    void updateMaxValRemoved(int val);
    void updateMinValRemoved(int val);
public:
    /**
     * @brief Constructor for SparseSet.
     * @param eng Pointer to the trailer.
     * @param n The number of elements.
     * @param ofs The offset.
     */
    SparseSet(Trailer::Ptr eng,int n,int ofs);

    /**
     * @brief Checks if the sparse set is empty.
     * @return True if the sparse set is empty, false otherwise.
     */
    bool isEmpty() const { return _size == 0;}

    /**
     * @brief Gets the size of the sparse set.
     * @return The size of the sparse set.
     */
    int size() const { return _size;}

    /**
     * @brief Gets the minimum value of the sparse set.
     * @return The minimum value of the sparse set.
     */
    int min() const { return _min + _ofs;}

    /**
     * @brief Gets the maximum value of the sparse set.
     * @return The maximum value of the sparse set.
     */
    int max() const { return _max + _ofs;}

    /**
     * @brief Checks if a value is contained in the sparse set.
     * @param val The value to check.
     * @return True if the value is contained in the sparse set, false otherwise.
     */
    bool contains(int val) const {
        val -= _ofs;
        if (val < 0 || val >= _n) return false;
        return _indexes[val] < _size;
    }

    /**
     * @brief Checks if the sparse set has changed.
     * @return True if the sparse set has changed, false otherwise.
     */
    bool changed() const noexcept { return _size.fresh();}

    /**
     * @brief Removes a value from the sparse set.
     * @param val The value to remove.
     * @return True if the value was successfully removed, false otherwise.
     */
    bool remove(int val);

    /**
     * @brief Removes all values except a specific value.
     * @param v The value to keep.
     */
    void removeAllBut(int v);

    /**
     * @brief Removes all values from the sparse set.
     */
    void removeAll() { _size = 0;}

    /**
     * @brief Removes all values below a specific value.
     * @param value The value below which to remove all values.
     */
    void removeBelow(int value);

    /**
     * @brief Removes all values above a specific value.
     * @param value The value above which to remove all values.
     */
    void removeAbove(int value);
};

/**
 * @brief Class representing a domain with a sparse set implementation.
 */
class SparseSetDomain {
    SparseSet _dom;
public:
    /**
     * @brief Type definition for a pointer to a SparseSetDomain.
     */
    typedef handle_ptr<SparseSetDomain>  Ptr;

    /**
     * @brief Constructor for SparseSetDomain.
     * @param trail Pointer to the trailer.
     * @param store Pointer to the storage.
     * @param min The minimum value of the domain.
     * @param max The maximum value of the domain.
     */
    SparseSetDomain(Trailer::Ptr trail,Storage::Ptr store,int min,int max)
        : _dom(trail,max - min + 1,min) {}

    /**
     * @brief Gets the minimum value of the domain.
     * @return The minimum value of the domain.
     */
    int min() const { return _dom.min();}

    /**
     * @brief Gets the maximum value of the domain.
     * @return The maximum value of the domain.
     */
    int max() const { return _dom.max();}

    /**
     * @brief Gets the size of the domain.
     * @return The size of the domain.
     */
    int size() const { return _dom.size();}

    /**
     * @brief Checks if a value is a member of the domain.
     * @param v The value to check.
     * @return True if the value is a member of the domain, false otherwise.
     */
    bool member(int v) const { return _dom.contains(v);}

    /**
     * @brief Checks if the domain is bound.
     * @return True if the domain is bound, false otherwise.
     */
    bool isBound() const { return _dom.size() == 1;}

    /**
     * @brief Checks if the domain has changed.
     * @return True if the domain has changed, false otherwise.
     */
    bool changed() const noexcept { return _dom.changed();}

    /**
     * @brief Assigns a value to the domain.
     * @param v The value to assign.
     * @param x The integer notifier.
     */
    void assign(int v,IntNotifier& x);

    /**
     * @brief Removes a value from the domain.
     * @param v The value to remove.
     * @param x The integer notifier.
     */
    void remove(int v,IntNotifier& x);

    /**
     * @brief Removes all values below a new minimum.
     * @param newMin The new minimum value.
     * @param x The integer notifier.
     */
    void removeBelow(int newMin,IntNotifier& x);

    /**
     * @brief Removes all values above a new maximum.
     * @param newMax The new maximum value.
     * @param x The integer notifier.
     */
    void removeAbove(int newMax,IntNotifier& x);

    /**
     * @brief Overloads the output stream operator for SparseSetDomain.
     * @param os The output stream.
     * @param x The SparseSetDomain object.
     * @return The output stream.
     */
    friend std::ostream& operator<<(std::ostream& os,const SparseSetDomain& x);
};

#endif
