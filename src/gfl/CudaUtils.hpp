#pragma once

#include <cassert>
#include "Types.hpp"
#include "FunQual.hpp"
#include "Backend.hpp"
#include "MathUtils.hpp"
#ifdef __CUDACC__
#include <cuda_runtime_api.h>
#endif

namespace gfl {

#ifdef __CUDACC__
GFL_DEVICE inline
i32 sharedMemSize() noexcept {
    i32 size;
    asm volatile ("mov.u32 %0, %dynamic_smem_size;" : "=r"(size));
    return size;
}

template<typename T>
GFL_DEVICE
tuple<T, T> getBeginEnd(const i64 index, const i64 workers, const i64 jobs) noexcept {
    assert(index < workers);
    assert(0 <= workers);
    assert(0 <= jobs);
    T begin, end;
    auto const jobsPerWorker = jobs / workers;
    auto const remainder = jobs % workers;
    auto const extra = (index < remainder) ? 1 : 0;
    begin = index * jobsPerWorker + gfl::min<i64>(index, remainder);
    end = begin + jobsPerWorker + extra;
    return make_tuple(begin, end);
}

GFL_DEVICE inline
i32 laneIdx() noexcept {
    unsigned int lIdx;
    asm volatile("mov.u32 %0, %laneid;" : "=r"(lIdx));
    return lIdx;
}

template<typename Block>
cudaGraphExec_t capture(cudaStream_t stream, Block b) noexcept{
    cudaGraphExec_t ge;
    cudaGraph_t g;
    cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal);
    b();
    cudaStreamEndCapture(stream, &g);
    cudaGraphInstantiate(&ge, g, nullptr, nullptr, 0);
    return ge;
}
#endif
}