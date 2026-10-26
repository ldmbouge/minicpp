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
 * 
 * StaticBitSet Implementation: Tim Curry
 */

#include "bitset.hpp"

// The bitsets are header-only templates; instantiating the common word
// widths here makes the library build compile every member.
template class StaticBitSet<uint32_t>;
template class StaticBitSet<uint64_t>;
template class StaticBitSet<int>;
template class StaticBitSet<long>;
template class SparseBitSet<uint32_t>;
template class SparseBitSet<uint64_t>;
template class SparseBitSet<int>;
template class SparseBitSet<long>;
