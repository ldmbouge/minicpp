/*
 * fzn-minicpp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * fzn-minicpp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c) 2022. by Fabio Tardivo
 */

#pragma once

#include <cassert>

#include "Alias.hpp"
#include "BitUtils.hpp"
#include "FunQual.hpp"
#include "MathUtils.hpp"

// Reachability and strongly connected components of a square boolean matrix,
// one bit per entry, worked on in 128x128 tiles: one u128 word per row, one
// thread per row, so a tile is exactly one 128-thread CUDA block.
//
// Two storage layouts for the same matrix, both `size` bits per side with
// `size` a multiple of 128 (64 is a special case handled by scc64):
//   - matrix:   plain row-major. Row r starts at word r * (size / 128), so a
//               tile is a strided gather of 128 words.
//   - arranged: tiles laid out one after another, so a tile is 128 contiguous
//               words. `arrange`/`dearrange` convert between the two.
//
// Bit order: column c of a row lives at bit 127 - c, so a row reads left to
// right and the lowest-numbered set column is a leading-zero count.

namespace gfl {
inline constexpr i32 BlockSide = 128;     // Rows (and threads) per tile
inline constexpr i32 HalfBlockSide = 64;  // The scc64 special case
inline constexpr i32 NoColumn = -1;       // firstSetColumn of an empty row

template<Unsigned Row> GFL_HOST_DEVICE constexpr i32 rowWidth() noexcept { return digits<Row>(); }
template<Unsigned Row> GFL_HOST_DEVICE constexpr Row columnMask(i32 const c) noexcept { return mask<Row>(rowWidth<Row>() - 1 - c); }
template<Unsigned Row> GFL_HOST_DEVICE constexpr bool testColumn(Row const row, i32 const c) noexcept { return test(row, rowWidth<Row>() - 1 - c); }
template<Unsigned Row> GFL_HOST_DEVICE constexpr i32 firstSetColumn(Row const row) noexcept { return row == 0 ? NoColumn : clz(row); }

// Sets or clears one entry of the matrix layout. Row is the word type the rows
// are made of: u64 when size is 64 (what scc64 reads), u128 otherwise.
template<Unsigned Row> GFL_HOST_DEVICE void setMatrixBit(u32* const matrix, u32 const size, u32 const r, u32 const c, bool const value) noexcept {
    assert(r < size and c < size);
    assert(size % rowWidth<Row>() == 0);
    i32 const wordsPerRow = scast<i32>(size) / rowWidth<Row>();
    Row& word = rcast<Row*>(matrix)[scast<i64>(r) * wordsPerRow + c / scast<u32>(rowWidth<Row>())];
    Row const bit = columnMask<Row>(scast<i32>(c) % rowWidth<Row>());
    word = value ? (word | bit) : (word & ~bit);
}

// Tile primitives. Each is one thread's share of the work on the device (one
// row per thread, blockDim.x == 128) and the whole tile serially on the host.
GFL_HOST_DEVICE void cpyMatrixToBlock(u32 row, u32 col, u32 size, u32 const* matrix, u128* block);
GFL_HOST_DEVICE void cpyArrangedToBlock(u32 row, u32 col, u32 size, u32 const* arranged, u128* block);
GFL_HOST_DEVICE void cpyBlockToMatrix(u32 row, u32 col, u32 size, u128 const* block, u32* matrix);
GFL_HOST_DEVICE void cpyBlockToArranged(u32 row, u32 col, u32 size, u128 const* block, u32* arranged);

GFL_HOST_DEVICE void transposeBlock(u64 const* block, u64* transposed);
GFL_HOST_DEVICE void transposeBlock(u128 const* block, u128* transposed);

GFL_HOST_DEVICE void bitwiseAndBlock(u128 const* aBlock, u128 const* bBlock, u128* cBlock);

GFL_HOST_DEVICE void findSccBlock(u32 col, u128 const* block, u32* scc);

// ijBlock |= iBlock * kBlock, the tile-level step of transitive closure.
// The first form tolerates ijBlock aliasing iBlock or kBlock (it syncs between
// the read and the write); the second is the faster no-aliasing case.
GFL_HOST_DEVICE void reachabilityBlock(u128* ijBlock, u128 const* iBlock, u128 const* kBlock);
GFL_HOST_DEVICE void reachabilityBlock3(u128* ijBlock, u128 const* iBlock, u128 const* kBlock);

// Whole-matrix operations, at most 128 nodes: closure and SCCs in one block
GFL_GLOBAL void scc64(u32 const* graph, u32* scc);
GFL_GLOBAL void scc128(u32 const* adj, u32* scc);

// Whole-matrix operations on the arranged layout. Each has a serial host
// version and a kernel; the kernel's grid is one block per tile (or per tile
// row), 128 threads each.
void arrange(u32 size, u32 const* matrix, u32* arranged);
GFL_GLOBAL void arrangeKernel(u32 size, u32 const* matrix, u32* arranged);

void dearrange(u32 size, u32 const* arranged, u32* matrix);
GFL_GLOBAL void dearrangeKernel(u32 size, u32 const* arranged, u32* matrix);

void transposeArranged(u32 size, u32 const* arranged, u32* transposed);
GFL_GLOBAL void transposeArrangedKernel(u32 size, u32 const* arranged, u32* transposed);

void bitwiseAnd(u32 size, u32 const* a, u32 const* b, u32* c);
GFL_GLOBAL void bitwiseAndKernel(u32 size, u32 const* a, u32 const* b, u32* c);

// Blocked Floyd-Warshall: for each diagonal tile i, close the tile itself
// (kernel 1), then the tile row and tile column through it (kernel 2), then
// every remaining tile (kernel 3). The three must run in that order.
void reachabilityArranged(u32 size, u32* reach);
GFL_GLOBAL void reachabilityArrangedKernel1(u32 i, u32 size, u32* reach);
GFL_GLOBAL void reachabilityArrangedKernel2(u32 i, u32 size, u32* reach);
GFL_GLOBAL void reachabilityArrangedKernel3(u32 i, u32 size, u32* reach);

// scc[n] is the smallest node mutually reachable with n, so two nodes share an
// SCC exactly when they share an scc[] value. Run initScc first.
void initScc(u32 size, u32* scc);
GFL_GLOBAL void initSccKernel(u32 size, u32* scc);
void findSccArranged(u32 size, u32 const* a, u32* scc);
GFL_GLOBAL void findSccArrangedKernel(u32 size, u32 const* a, u32* scc);

void profileReachability();
}
