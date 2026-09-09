#pragma once

#include <cassert>

#include "Alias.hpp"
#include "Backend.hpp"
#include "BitUtils.hpp"
#include "DebugUtils.hpp"
#include "FunQual.hpp"
#include "MathUtils.hpp"

namespace gfl {
template<typename T> GFL_HOST_DEVICE constexpr tuple<T, T> calcSlice(i64 const wIdx, i64 const nWorkers, i64 const nJobs) noexcept {
    assert(nWorkers > 0);
    assert(0 <= wIdx && wIdx < nWorkers);
    T begin = 0;
    T end = 0;
    if (nJobs > 0) { // There are reasonable cases where nJobs can be negative
        auto const jobsPerWorker = nJobs / nWorkers;
        auto const remainder = nJobs % nWorkers;
        // First 'remainder' workers get one extra job
        auto const extra = (wIdx < remainder) ? 1 : 0;
        begin = wIdx * jobsPerWorker + min<i64>(wIdx, remainder);
        end = begin + jobsPerWorker + extra;
    }
    return {begin, end};
}

#ifdef __CUDACC__
inline constexpr u32 FullMask = fullMask<u32>();
inline constexpr u32 WarpSize = 32;
inline constexpr u32 DefaultBlockSize = 256;
inline constexpr u32 DefaultBlocksPerMP = 4;
inline constexpr u32 DefaultWarpsPerBlock = DefaultBlockSize / WarpSize;
GFL_DEVICE inline i32 warpIdx() { return threadIdx.x / WarpSize; }
GFL_DEVICE inline i32 laneIdx() { return threadIdx.x % WarpSize; }
GFL_DEVICE inline i32 getShrMemSize() noexcept {
    i32 size;
    asm volatile("mov.u32 %0, %%dynamic_smem_size;" : "=r"(size));
    return size;
}
// Records the stream work a block issues into a replayable CUDA graph, so a
// fixed sequence of copies and kernels costs one launch instead of several
template<typename Block> inline cudaGraphExec_t capture(cudaStream_t const stream, Block&& b) noexcept {
    cudaGraph_t graph;
    cudaGraphExec_t executable;
    CHECK_CUDA_ERROR(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal));
    b();
    CHECK_CUDA_ERROR(cudaStreamEndCapture(stream, &graph));
    CHECK_CUDA_ERROR(cudaGraphInstantiate(&executable, graph, nullptr, nullptr, 0));
    return executable;
}
#endif
}