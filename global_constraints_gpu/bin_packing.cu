#include "global_constraints_gpu/bin_packing.cuh"

#include <algorithm>

void BinPackingLoadGPU::checkLowerbounds()
{
    using namespace gfl;

    // Prepare the data to copy on the GPU
    i32 const nWeights = scast<i32>(weightsBaseReduction.size());
    _id->lowerbound = 0;
    _id->nBins = nBins;
    _id->nWeights = nWeights;
    _id->capacityBaseReduction = capacityBaseReduction;
    _w->loadFrom(weightsBaseReduction);

    // Copy the data on the GPU
    _ioRegion.copyToDeviceAsync(_cuStreams[0]);
    cudaStreamSynchronize(_cuStreams[0]);

    // Launch the kernels
    i32 kernelIdx = 0;
    for (i32 rIdx = 0; rIdx < REDUCTIONS_COUNT; rIdx += 1)
    {
        // Analyze the reduction. The kernels rebuild its weights in shared
        // memory, so there is nothing to materialize here
        i32 const delta = deltaReductions[rIdx];
        i32 const capacity = reductionCapacity(rIdx);
        Reduction const reduction = analyzeReduction(rIdx, false);

        // One kernel per dual-feasible function, each on a stream of its own
        Dffs::forEach([&](auto dff)
        {
            using D = decltype(dff);
            cudaStream_t const stream = _cuStreams[kernelIdx];
            kernelIdx += 1;

            LambdaRange lRange = D::lambdaRange(capacity);
            lRange = D::needsSanitizing ? sanitizeLambdaRange(lRange, reduction.notZeroCount, reduction.maxWeight) : lRange;
            i32 const lRangeSize = lRange.max - lRange.min + 1;
            if (lRangeSize <= 0)
                return;

            u32 const nBlocks = max<u32>(1u, ceilDiv<u32>(lRangeSize, BlockSize));
            calcDffLowerboundKernel<D::transform><<<nBlocks, BlockSize, _sharedMemSize, stream>>>(_id, _w, delta, lRange.min, lRange.max);
        });
    }

    // Wait for the kernels and copy the lowerbound back
    for (i32 sIdx = 0; sIdx < KernelsCount; sIdx += 1)
        cudaStreamSynchronize(_cuStreams[sIdx]);
    cudaMemcpy(&_id.h()->lowerbound, &_id.d()->lowerbound, sizeof(i32), cudaMemcpyDeviceToHost);

    if (_id->lowerbound > nBins)
        failNow();
}
