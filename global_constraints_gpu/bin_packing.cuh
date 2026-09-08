#pragma once

#include "global_constraints/bin_packing.hpp"
#include "gfl/Types.hpp"
#include "gfl/MathUtils.hpp"
#include "gfl/Memory.hpp"
#include "gfl/Arrays.hpp"
#include "gfl/CudaUtils.hpp"

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
        using Weights = gfl::MirrorArray<gfl::i32>;

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

            setPriority(CLOW);

            _ioRegion = _mPool.record([this]() {
                _id = _mPool.make<InstanceData>();
                _w = _mPool.make<Weights>(_mPool, nItems + nBins);
            });

            // weights_s in calcDffLowerboundKernel
            _sharedMemSize = scast<u32>(sizeof(i32) * (nItems + nBins));

            for (i32 sIdx = 0; sIdx < KernelsCount; sIdx += 1)
                cudaStreamCreate(&_cuStreams[sIdx]);
        }

        void post() override;
        void propagate() override;

    private:
        void checkLowerbounds();
};

// Dual-Feasible Functions ----------------------------------------------------

GFL_DEVICE inline gfl::i32 fMT_d(gfl::i32 w, gfl::i32 l, gfl::i32 c)
{
    using namespace gfl;

    // Branch-less f0 transformation
    i32 const c0 = w > c - l;
    i32 const c1 = l <= w and w <= c - l;
    i32 const c2 = w < l;

    i32 const v0 = c;
    i32 const v1 = w;
    i32 const v2 = 0;

    return (c0 * v0) + (c1 * v1) + (c2 * v2);
}

GFL_DEVICE inline gfl::i32 fCCM1_d(gfl::i32 w, gfl::i32 l, gfl::i32 c)
{
    using namespace gfl;

    // Branch-less fCCM1 transformation
    i32 const c0 = 2 * w > c;  // x > c / 2
    i32 const c1 = 2 * w == c; // x == c / 2
    i32 const c2 = 2 * w < c;  // x < c / 2

    i32 const v0 = 2 * (c / l - (c - w) / l);
    i32 const v1 = c / l;
    i32 const v2 = 2 * (w / l);

    return (c0 * v0) + (c1 * v1) + (c2 * v2);
}

GFL_DEVICE inline gfl::i32 fFS1_d(gfl::i32 w, gfl::i32 l, gfl::i32 c)
{
    using namespace gfl;

    // Branch-less fFS1 transformation
    i32 const c0 = (w * (l + 1)) % c == 0;
    i32 const c1 = (w * (l + 1)) % c != 0;

    i32 const v0 = w * l;
    i32 const v1 = ((w * (l + 1)) / c) * c;

    return (c0 * v0) + (c1 * v1);
}

GFL_DEVICE inline gfl::i32 fBJ1_d(gfl::i32 w, gfl::i32 l, gfl::i32 c)
{
    using namespace gfl;

    // Branch-less fBJ1 transformation
    i32 const p = l - (c % l);

    i32 const c0 = w % l <= c % l;
    i32 const c1 = w % l > c % l;

    i32 const v0 = (w / l) * p;
    i32 const v1 = (w / l) * p + (w % l) - (c % l);

    return (c0 * v0) + (c1 * v1);
}

GFL_DEVICE inline gfl::i32 fVB2Base_d(gfl::i32 w, gfl::i32 l, gfl::i32 c)
{
    gfl::i32 const v = gfl::ceilDiv<gfl::i32>(l * w, c);
    return v > 0 ? v - 1 : 0;
}

GFL_DEVICE inline gfl::i32 fVB2_d(gfl::i32 w, gfl::i32 l, gfl::i32 c)
{
    using namespace gfl;

    i32 const c0 = 2 * w > c;
    i32 const c1 = 2 * w == c;
    i32 const c2 = 2 * w < c;

    i32 const t0 = fVB2Base_d(c, l, c);
    i32 const t1 = fVB2Base_d(w, l, c);
    i32 const t2 = fVB2Base_d(c - w, l, c);

    i32 const v0 = 2 * t0 - 2 * t2;
    i32 const v1 = t0;
    i32 const v2 = 2 * t1;

    return (c0 * v0) + (c1 * v1) + (c2 * v2);
}

GFL_DEVICE inline gfl::i32 fRAD2Base_d(gfl::i32 w, gfl::i32 l, gfl::i32 c)
{
    using namespace gfl;

    i32 const c0 = w < l;
    i32 const c1 = l <= w and w <= c - 2 * l;
    i32 const c2 = c - 2 * l < w and w < 2 * l;

    i32 const v0 = 0;
    i32 const v1 = c / 3;
    i32 const v2 = c / 2;

    return (c0 * v0) + (c1 * v1) + (c2 * v2);
}

GFL_DEVICE inline gfl::i32 fRAD2_d(gfl::i32 w, gfl::i32 l, gfl::i32 c)
{
    using namespace gfl;

    i32 const c0 = w < 2 * l;
    i32 const c1 = 2 * l <= w;

    i32 const v0 = fRAD2Base_d(w, l, c);
    i32 const v1 = c - fRAD2Base_d(c - w, l, c);

    return (c0 * v0) + (c1 * v1);
}

// Kernels --------------------------------------------------------------------

template<gfl::i32 f(gfl::i32, gfl::i32, gfl::i32)>
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
