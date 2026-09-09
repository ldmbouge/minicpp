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

#include "SqrBitMatrix.cuh"

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "Allocators.hpp"
#include "DebugUtils.hpp"

// Each tile primitive below is written once for both targets: on the device
// thread t owns row t of the tile, on the host one loop walks all 128 rows.
// GFL_ROW opens that loop off-device and names the row either way.
#ifdef __CUDA_ARCH__
#define GFL_ROW(name) u32 const name = threadIdx.x;
#define GFL_SYNC_ROWS() __syncthreads()
#else
#define GFL_ROW(name) for (u32 name = 0; name < gfl::scast<u32>(gfl::BlockSide); name += 1)
#define GFL_SYNC_ROWS()
#endif

namespace gfl {
namespace {
// Words per row of the matrix layout, and tiles per row of either layout
GFL_HOST_DEVICE i32 wordsPerRow(u32 const size) noexcept { return scast<i32>(size) / BlockSide; }

GFL_HOST_DEVICE void checkTile(u32 const row, u32 const col) noexcept {
#ifdef __CUDA_ARCH__
    assert(blockDim.x == BlockSide);
#endif
    assert(row % BlockSide == 0);
    assert(col % BlockSide == 0);
}
}

GFL_HOST_DEVICE void cpyMatrixToBlock(u32 const row, u32 const col, u32 const size, u32 const* const matrix, u128* const block) {
    checkTile(row, col);
    i32 const stride = wordsPerRow(size);
    u128 const* const m = rcast<u128 const*>(matrix) + (row * stride + col / BlockSide);
    GFL_ROW(bRow) { block[bRow] = m[bRow * stride]; }
}

GFL_HOST_DEVICE void cpyArrangedToBlock(u32 const row, u32 const col, u32 const size, u32 const* const arranged, u128* const block) {
    checkTile(row, col);
    i32 const tilesPerRow = wordsPerRow(size);
    u128 const* const a = rcast<u128 const*>(arranged) + ((row / BlockSide * tilesPerRow + col / BlockSide) * BlockSide);
    GFL_ROW(bRow) { block[bRow] = a[bRow]; }
}

GFL_HOST_DEVICE void cpyBlockToMatrix(u32 const row, u32 const col, u32 const size, u128 const* const block, u32* const matrix) {
    checkTile(row, col);
    i32 const stride = wordsPerRow(size);
    u128* const m = rcast<u128*>(matrix) + (row * stride + col / BlockSide);
    GFL_ROW(bRow) { m[bRow * stride] = block[bRow]; }
}

GFL_HOST_DEVICE void cpyBlockToArranged(u32 const row, u32 const col, u32 const size, u128 const* const block, u32* const arranged) {
    checkTile(row, col);
    i32 const tilesPerRow = wordsPerRow(size);
    u128* const a = rcast<u128*>(arranged) + ((row / BlockSide * tilesPerRow + col / BlockSide) * BlockSide);
    GFL_ROW(bRow) { a[bRow] = block[bRow]; }
}

GFL_HOST_DEVICE void transposeBlock(u64 const* const block, u64* const transposed) {
#ifdef __CUDA_ARCH__
    assert(blockDim.x == HalfBlockSide);
    u64 const tRow = threadIdx.x;
#else
    for (u64 tRow = 0; tRow < asSize(HalfBlockSide); tRow += 1)
#endif
    {
        u64 buffer = 0;
        for (i32 bRow = 0; bRow < HalfBlockSide; bRow += 1)
            if (testColumn(block[bRow], scast<i32>(tRow))) buffer |= columnMask<u64>(bRow);
        transposed[tRow] = buffer;
    }
}

GFL_HOST_DEVICE void transposeBlock(u128 const* const block, u128* const transposed) {
#ifdef __CUDA_ARCH__
    assert(blockDim.x == BlockSide);
#endif
    GFL_ROW(tRow) {
        u128 buffer = 0;
        for (i32 bRow = 0; bRow < BlockSide; bRow += 1)
            if (testColumn(block[bRow], scast<i32>(tRow))) buffer |= columnMask<u128>(bRow);
        transposed[tRow] = buffer;
    }
}

GFL_HOST_DEVICE void bitwiseAndBlock(u128 const* const aBlock, u128 const* const bBlock, u128* const cBlock) {
#ifdef __CUDA_ARCH__
    assert(blockDim.x == BlockSide);
#endif
    GFL_ROW(bRow) { cBlock[bRow] = aBlock[bRow] & bBlock[bRow]; }
}

GFL_HOST_DEVICE void findSccBlock(u32 const col, u128 const* const block, u32* const scc) {
#ifdef __CUDA_ARCH__
    assert(blockDim.x == BlockSide);
#endif
    GFL_ROW(bRow) {
        i32 const c = firstSetColumn(block[bRow]);
        u32 const candidate = c != NoColumn ? col + scast<u32>(c) : highest<u32>();
        scc[bRow] = min<u32>(scc[bRow], candidate);
    }
}

GFL_HOST_DEVICE void reachabilityBlock(u128* const ijBlock, u128 const* const iBlock, u128 const* const kBlock) {
#ifdef __CUDA_ARCH__
    assert(blockDim.x == BlockSide);
#endif
    // ijBlock may be iBlock or kBlock, so every row's read of step k has to
    // happen before any row's write of step k, and vice versa
    for (i32 k = 0; k < BlockSide; k += 1) {
        GFL_ROW(i) {
            u128 const kRow = testColumn(iBlock[i], k) ? kBlock[k] : u128{0};
            GFL_SYNC_ROWS();
            ijBlock[i] |= kRow;
            GFL_SYNC_ROWS();
        }
    }
}

GFL_HOST_DEVICE void reachabilityBlock3(u128* const ijBlock, u128 const* const iBlock, u128 const* const kBlock) {
#ifdef __CUDA_ARCH__
    assert(blockDim.x == BlockSide);
#endif
    // The three tiles are distinct here, so a row can accumulate in a register
    GFL_ROW(i) {
        u128 ijRow = ijBlock[i];
        u128 const iRow = iBlock[i];
        for (i32 k = 0; k < BlockSide; k += 1)
            if (testColumn(iRow, k)) ijRow |= kBlock[k];
        ijBlock[i] = ijRow;
    }
}

// Layout conversion ----------------------------------------------------------

void arrange(u32 const size, u32 const* const matrix, u32* const arranged) {
    u128 block[BlockSide];
    for (u32 row = 0; row < size; row += BlockSide)
        for (u32 col = 0; col < size; col += BlockSide) {
            cpyMatrixToBlock(row, col, size, matrix, block);
            cpyBlockToArranged(row, col, size, block, arranged);
        }
}

GFL_GLOBAL void arrangeKernel(u32 const size, u32 const* const matrix, u32* const arranged) {
    assert(gridDim.x * BlockSide == size);
    assert(gridDim.y * BlockSide == size);

    __shared__ u128 block[BlockSide];
    u32 const row = blockIdx.y * BlockSide;
    u32 const col = blockIdx.x * BlockSide;
    cpyMatrixToBlock(row, col, size, matrix, block);
    cpyBlockToArranged(row, col, size, block, arranged);
}

void dearrange(u32 const size, u32 const* const arranged, u32* const matrix) {
    u128 block[BlockSide];
    for (u32 row = 0; row < size; row += BlockSide)
        for (u32 col = 0; col < size; col += BlockSide) {
            cpyArrangedToBlock(row, col, size, arranged, block);
            cpyBlockToMatrix(row, col, size, block, matrix);
        }
}

GFL_GLOBAL void dearrangeKernel(u32 const size, u32 const* const arranged, u32* const matrix) {
    assert(gridDim.x * BlockSide == size);
    assert(gridDim.y * BlockSide == size);

    __shared__ u128 block[BlockSide];
    u32 const row = blockIdx.y * BlockSide;
    u32 const col = blockIdx.x * BlockSide;
    cpyArrangedToBlock(row, col, size, arranged, block);
    cpyBlockToMatrix(row, col, size, block, matrix);
}

void transposeArranged(u32 const size, u32 const* const arranged, u32* const transposed) {
    u128 aBlock[BlockSide];
    u128 tBlock[BlockSide];
    for (u32 row = 0; row < size; row += BlockSide)
        for (u32 col = 0; col < size; col += BlockSide) {
            cpyArrangedToBlock(row, col, size, arranged, aBlock);
            transposeBlock(aBlock, tBlock);
            cpyBlockToArranged(col, row, size, tBlock, transposed);
        }
}

GFL_GLOBAL void transposeArrangedKernel(u32 const size, u32 const* const arranged, u32* const transposed) {
    assert(gridDim.x * BlockSide == size);
    assert(gridDim.y * BlockSide == size);

    __shared__ u128 arrangedBlock[BlockSide];
    __shared__ u128 transposedBlock[BlockSide];
    u32 const row = blockIdx.y * BlockSide;
    u32 const col = blockIdx.x * BlockSide;
    cpyArrangedToBlock(row, col, size, arranged, arrangedBlock);
    __syncthreads();
    transposeBlock(arrangedBlock, transposedBlock);
    __syncthreads();
    cpyBlockToArranged(col, row, size, transposedBlock, transposed);
}

void bitwiseAnd(u32 const size, u32 const* const a, u32 const* const b, u32* const c) {
    u128 aBlock[BlockSide];
    u128 bBlock[BlockSide];
    u128 cBlock[BlockSide];
    for (u32 row = 0; row < size; row += BlockSide)
        for (u32 col = 0; col < size; col += BlockSide) {
            cpyArrangedToBlock(row, col, size, a, aBlock);
            cpyArrangedToBlock(row, col, size, b, bBlock);
            bitwiseAndBlock(aBlock, bBlock, cBlock);
            cpyBlockToArranged(row, col, size, cBlock, c);
        }
}

GFL_GLOBAL void bitwiseAndKernel(u32 const size, u32 const* const a, u32 const* const b, u32* const c) {
    assert(gridDim.x * BlockSide == size);
    assert(gridDim.y * BlockSide == size);

    __shared__ u128 aBlock[BlockSide];
    __shared__ u128 bBlock[BlockSide];
    __shared__ u128 cBlock[BlockSide];
    u32 const row = blockIdx.y * BlockSide;
    u32 const col = blockIdx.x * BlockSide;
    cpyArrangedToBlock(row, col, size, a, aBlock);
    cpyArrangedToBlock(row, col, size, b, bBlock);
    __syncthreads();
    bitwiseAndBlock(aBlock, bBlock, cBlock);
    __syncthreads();
    cpyBlockToArranged(row, col, size, cBlock, c);
}

// Single-tile matrices -------------------------------------------------------

GFL_GLOBAL void scc64(u32 const* const graph, u32* const scc) {
    assert(gridDim.x == 1);
    assert(blockDim.x == HalfBlockSide);

    __shared__ u64 block[HalfBlockSide];

    i32 const i = scast<i32>(threadIdx.x);
    block[i] = rcast<u64 const*>(graph)[i];
    __syncthreads();

    // Transitive closure
    for (i32 k = 0; k < HalfBlockSide; k += 1) {
        u64 const kRow = testColumn(block[i], k) ? block[k] : u64{0};
        __syncthreads();
        block[i] |= kRow;
        __syncthreads();
    }

    // Mutually reachable nodes share the smallest of their indices
    u32 smallest = scast<u32>(i);
    u64 const iRow = block[i];
    for (i32 j = 0; j < HalfBlockSide; j += 1)
        if (testColumn(iRow, j) and testColumn(block[j], i))
            smallest = min<u32>(smallest, scast<u32>(j));
    scc[i] = smallest;
}

GFL_GLOBAL void scc128(u32 const* const adj, u32* const scc) {
    assert(gridDim.x == 1);
    assert(blockDim.x == BlockSide);

    __shared__ u128 block[BlockSide];

    i32 const i = scast<i32>(threadIdx.x);
    block[i] = rcast<u128 const*>(adj)[i];
    __syncthreads();

    // Transitive closure
    for (i32 k = 0; k < BlockSide; k += 1) {
        u128 const kRow = testColumn(block[i], k) ? block[k] : u128{0};
        __syncthreads();
        block[i] |= kRow;
        __syncthreads();
    }

    // Mutually reachable nodes share the smallest of their indices
    u32 smallest = scast<u32>(i);
    u128 const iRow = block[i];
    for (i32 j = 0; j < BlockSide; j += 1)
        if (testColumn(iRow, j) and testColumn(block[j], i))
            smallest = min<u32>(smallest, scast<u32>(j));
    scc[i] = smallest;
}

// Blocked transitive closure -------------------------------------------------

void reachabilityArranged(u32 const size, u32* const reach) {
    u128 block1[BlockSide];
    u128 block2[BlockSide];
    u128 block3[BlockSide];
    for (u32 i = 0; i < size; i += BlockSide) {
        // Phase 1: close the diagonal tile
        cpyArrangedToBlock(i, i, size, reach, block1);
        reachabilityBlock(block1, block1, block1);
        cpyBlockToArranged(i, i, size, block1, reach);

        // Phase 2: close its tile row, then its tile column, through it
        for (u32 x = 0; x < size; x += BlockSide)
            if (x != i) {
                cpyArrangedToBlock(i, x, size, reach, block2);
                reachabilityBlock(block2, block1, block2);
                cpyBlockToArranged(i, x, size, block2, reach);
            }
        for (u32 x = 0; x < size; x += BlockSide)
            if (x != i) {
                cpyArrangedToBlock(x, i, size, reach, block2);
                reachabilityBlock(block2, block2, block1);
                cpyBlockToArranged(x, i, size, block2, reach);
            }

        // Phase 3: close every remaining tile through the two just closed
        for (u32 col = 0; col < size; col += BlockSide)
            if (col != i) {
                cpyArrangedToBlock(i, col, size, reach, block3);
                for (u32 row = 0; row < size; row += BlockSide)
                    if (row != i) {
                        cpyArrangedToBlock(row, i, size, reach, block2);
                        cpyArrangedToBlock(row, col, size, reach, block1);
                        reachabilityBlock3(block1, block2, block3);
                        cpyBlockToArranged(row, col, size, block1, reach);
                    }
            }
    }
}

GFL_GLOBAL void reachabilityArrangedKernel1(u32 const i, u32 const size, u32* const reach) {
    assert(gridDim.x == 1);
    assert(blockDim.x == BlockSide);

    __shared__ u128 block[BlockSide];
    cpyArrangedToBlock(i, i, size, reach, block);
    __syncthreads();
    reachabilityBlock(block, block, block);
    __syncthreads();
    cpyBlockToArranged(i, i, size, block, reach);
}

GFL_GLOBAL void reachabilityArrangedKernel2(u32 const i, u32 const size, u32* const reach) {
    assert(gridDim.x == size / BlockSide);
    assert(gridDim.y == 2);
    assert(blockDim.x == BlockSide);

    __shared__ u128 diagonal[BlockSide];
    __shared__ u128 block[BlockSide];
    u32 const x = blockIdx.x * BlockSide;
    bool const horizontal = blockIdx.y == 0;

    if (x == i) return;

    cpyArrangedToBlock(i, i, size, reach, diagonal);
    cpyArrangedToBlock(horizontal ? i : x, horizontal ? x : i, size, reach, block);
    __syncthreads();
    if (horizontal)
        reachabilityBlock(block, diagonal, block);
    else
        reachabilityBlock(block, block, diagonal);
    __syncthreads();
    cpyBlockToArranged(horizontal ? i : x, horizontal ? x : i, size, block, reach);
}

GFL_GLOBAL void reachabilityArrangedKernel3(u32 const i, u32 const size, u32* const reach) {
    assert(gridDim.x == size / BlockSide);
    assert(gridDim.y == size / BlockSide);
    assert(blockDim.x == BlockSide);

    __shared__ u128 ijBlock[BlockSide];
    __shared__ u128 iBlock[BlockSide];
    __shared__ u128 kBlock[BlockSide];

    u32 const row = blockIdx.y * BlockSide;
    u32 const col = blockIdx.x * BlockSide;

    if (col == i or row == i) return;

    cpyArrangedToBlock(i, col, size, reach, kBlock);
    cpyArrangedToBlock(row, i, size, reach, iBlock);
    cpyArrangedToBlock(row, col, size, reach, ijBlock);
    __syncthreads();
    reachabilityBlock3(ijBlock, iBlock, kBlock);
    __syncthreads();
    cpyBlockToArranged(row, col, size, ijBlock, reach);
}

// Strongly connected components ----------------------------------------------

void initScc(u32 const size, u32* const scc) {
    for (u32 i = 0; i < size; i += 1) scc[i] = i;
}

GFL_GLOBAL void initSccKernel(u32 const size, u32* const scc) {
    assert(gridDim.x * BlockSide == size);
    assert(blockDim.x == BlockSide);

    for (u32 i = threadIdx.x; i < size; i += blockDim.x) scc[i] = i;
}

void findSccArranged(u32 const size, u32 const* const a, u32* const scc) {
    u128 block[BlockSide];
    u32 rowScc[BlockSide];
    for (u32 row = 0; row < size; row += BlockSide) {
        for (i32 bRow = 0; bRow < BlockSide; bRow += 1) rowScc[bRow] = scc[row + bRow];
        for (u32 col = 0; col < size; col += BlockSide) {
            cpyArrangedToBlock(row, col, size, a, block);
            findSccBlock(col, block, rowScc);
        }
        for (i32 bRow = 0; bRow < BlockSide; bRow += 1) scc[row + bRow] = rowScc[bRow];
    }
}

GFL_GLOBAL void findSccArrangedKernel(u32 const size, u32 const* const a, u32* const scc) {
    assert(gridDim.x * BlockSide == size);
    assert(blockDim.x == BlockSide);

    __shared__ u128 block[BlockSide];
    __shared__ u32 rowScc[BlockSide];
    u32 const row = blockIdx.x * BlockSide;
    u32 const bRow = threadIdx.x;

    rowScc[bRow] = scc[row + bRow];
    for (u32 col = 0; col < size; col += BlockSide) {
        cpyArrangedToBlock(row, col, size, a, block);
        __syncthreads();
        findSccBlock(col, block, rowScc);
        __syncthreads();
    }
    scc[row + bRow] = rowScc[bRow];
}

// Profiling ------------------------------------------------------------------

void profileReachability() {
    constexpr u32 MaxSize = 16384;
    i64 const bytes = as<i64>(MaxSize) * MaxSize / 8;

    // A random matrix, dense enough that the closure does real work
    u32* const m_h = HostAllocator{}.allocate<u32>(bytes / as<i64>(sizeof(u32)));
    for (i64 i = 0; i < bytes / as<i64>(sizeof(u32)); i += 1) m_h[i] = scast<u32>(std::rand());
    u32* const m_d = DeviceAllocator{}.allocate<u32>(bytes / as<i64>(sizeof(u32)));
    CHECK_CUDA_ERROR(cudaMemcpy(m_d, m_h, asSize(bytes), cudaMemcpyHostToDevice));

    cudaEvent_t start;
    cudaEvent_t stop;
    CHECK_CUDA_ERROR(cudaEventCreate(&start));
    CHECK_CUDA_ERROR(cudaEventCreate(&stop));

    for (u32 nNodes = BlockSide; nNodes <= MaxSize; nNodes *= 2) {
        u32 const tiles = nNodes / BlockSide;
        dim3 const tileRow = dim3(tiles, 2);
        dim3 const allTiles = dim3(tiles, tiles);
        CHECK_CUDA_ERROR(cudaEventRecord(start));
        for (u32 i = 0; i < nNodes; i += BlockSide) {
            reachabilityArrangedKernel1<<<1, BlockSide>>>(i, nNodes, m_d);
            reachabilityArrangedKernel2<<<tileRow, BlockSide>>>(i, nNodes, m_d);
            reachabilityArrangedKernel3<<<allTiles, BlockSide>>>(i, nNodes, m_d);
        }
        CHECK_CUDA_ERROR(cudaEventRecord(stop));
        CHECK_CUDA_ERROR(cudaEventSynchronize(stop));
        f32 milliseconds = 0;
        CHECK_CUDA_ERROR(cudaEventElapsedTime(&milliseconds, start, stop));
        printf("%ux%u -> %.3f us\n", nNodes, nNodes, milliseconds * 1000.0f);
    }

    CHECK_CUDA_ERROR(cudaEventDestroy(start));
    CHECK_CUDA_ERROR(cudaEventDestroy(stop));
    DeviceAllocator{}.deallocate(m_d, bytes);
    HostAllocator{}.deallocate(m_h, bytes);
}
}

#undef GFL_ROW
#undef GFL_SYNC_ROWS
