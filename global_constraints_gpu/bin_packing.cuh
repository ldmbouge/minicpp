#pragma once

#include "global_constraints/bin_packing.hpp"
#include "gfl/Alias.hpp"
#include "gfl/ArrayView.hpp"
#include "gfl/MathUtils.hpp"
#include "gfl/Mirror.hpp"
#include "gfl/ParallelUtils.hpp"

// GPU counterpart of BinPackingLoad: only the dual-feasible-function lower
// bounds are offloaded, one kernel per (reduction, DFF) pair, each sampling the
// lambda range in parallel.

class BinPackingLoadGPU : public BinPackingLoad
{
    public:
        static constexpr gfl::i32 BlockSize = 128;
        static constexpr gfl::i32 KernelsCount = BinPackingLoad::REDUCTIONS_COUNT * BinPackingLoad::DFFS_COUNT;
        // Slack covering the alignment padding of the three pool allocations
        static constexpr gfl::i64 PoolPadding = 256;
        using Weights = gfl::MirrorArrayView<gfl::i32>;

        struct InstanceData
        {
            gfl::i32 lowerbound;
            gfl::i32 nBins;
            gfl::i32 nWeights;
            gfl::i32 capacityBaseReduction;
        };

    private:
        gfl::MirrorPool _mPool;
        gfl::MirrorPtr<InstanceData> _id;
        gfl::MirrorPtr<Weights> _w;
        gfl::MirrorRegion _ioRegion;
        gfl::u32 _sharedMemSize;
        cudaStream_t _cuStreams[KernelsCount];

    public:
        template<typename LoadVars, typename BinVars>
        BinPackingLoadGPU(LoadVars const & load, BinVars const & bin, std::vector<int> const & w) :
            BinPackingLoad(load, bin, w),
            _mPool(sizeof(InstanceData) + sizeof(Weights) + (sizeof(gfl::i32) * (nItems + nBins)) + PoolPadding)
        {
            using namespace gfl;

            _ioRegion = _mPool.record([this]() {
                _id = _mPool.make<InstanceData>();
                _w = _mPool.make<Weights>(_mPool, nItems + nBins);
            });

            // weights_s in calcDffLowerboundKernel
            _sharedMemSize = scast<u32>(sizeof(i32) * (nItems + nBins));

            for (i32 sIdx = 0; sIdx < KernelsCount; sIdx += 1)
                cudaStreamCreate(&_cuStreams[sIdx]);
        }

    private:
        void checkLowerbounds() override;
};

// Kernels --------------------------------------------------------------------

template<int f(int, int, int)>
GFL_GLOBAL void calcDffLowerboundKernel(gfl::MirrorPtr<BinPackingLoadGPU::InstanceData> id,
                                        gfl::MirrorPtr<BinPackingLoadGPU::Weights> w,
                                        gfl::i32 const deltaReduction,
                                        gfl::i32 const minLambda,
                                        gfl::i32 const maxLambda)
{
    using namespace gfl;

    extern __shared__ i32 weights_s[];
    __shared__ i32 lowerbound_s;

    if (threadIdx.x == 0)
        lowerbound_s = 0;
    __syncthreads();

    i32 const capacity_r = id->capacityBaseReduction + deltaReduction;
    i32 const nWeights_r = id->nWeights;

    if (id->lowerbound <= id->nBins and capacity_r > 0)
    {
        // Calculate the reduction in shared memory
        for (i32 wIdx = scast<i32>(threadIdx.x); wIdx < nWeights_r; wIdx += scast<i32>(blockDim.x))
            weights_s[wIdx] = w->at(wIdx) + (wIdx < id->nBins ? deltaReduction : 0);
        __syncthreads();

        // One lambda per thread
        i32 const tIndex = scast<i32>(blockIdx.x * blockDim.x + threadIdx.x);
        i32 const lambda = minLambda + tIndex;
        i32 lowerbound_r = 0;
        if (lambda <= maxLambda)
        {
            // Transform and sum the weights
            i32 sumTransformedWeights = 0;
            for (i32 wIdx = 0; wIdx < nWeights_r; wIdx += 1)
                sumTransformedWeights += f(weights_s[wIdx], lambda, capacity_r);

            // Transform the capacity
            i32 const transformedCapacity = f(capacity_r, lambda, capacity_r);
            if (transformedCapacity > 0)
                lowerbound_r = ceilDiv<i32>(sumTransformedWeights, transformedCapacity);
        }

        // Calculate the biggest lowerbound for the current warp. The guarding
        // condition is block-uniform, so the whole warp is active here.
        constexpr u32 warpMask = 0xFFFFFFFFu;
        i32 lowerbound_w = lowerbound_r;
        for (i32 offset = warpSize / 2; offset > 0; offset /= 2)
            lowerbound_w = max<i32>(lowerbound_w, __shfl_down_sync(warpMask, lowerbound_w, offset));

        // Update the lowerbound in shared memory
        if (laneIdx() == 0)
            atomicMax_block(&lowerbound_s, lowerbound_w);
    }
    __syncthreads();

    // Update the lowerbound in global memory
    if (threadIdx.x == 0)
        atomicMax(&id->lowerbound, lowerbound_s);
}
