#include "global_constraints_gpu/bin_packing.cuh"

#include <algorithm>

void BinPackingLoadGPU::post()
{
    for (auto const & l : _load)
        l->propagateOnBoundChange(this);

    for (auto const & b : _bin)
        b->propagateOnDomainChange(this);

    propagate();
}

void BinPackingLoadGPU::propagate()
{
    calculateNumBins();
    saveBinLoadSizes();
    calculateRequiredPossibleCandidates();
    propagateLoadCoherence();
    calculateSumLoads();
    propagateBasicLoadTightening();
    propagateBasicItemEliminationCommitment();
    if (not checkBinLoadSizeChanges())
    {
        checkPackableBins();
        propagateKnapsackLoadTightening();
        propagateKnapsackItemEliminationCommitment();

        if (not checkBinLoadSizeChanges())
        {
            prepareReductions();
            checkLowerbounds();
        }
    }
}

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
    for (i32 rIdx = 0; rIdx < REDUCTIONS_COUNT; rIdx += 1)
    {
        // Calculate the reduction parameters
        i32 const delta = deltaReductions[rIdx];
        i32 const capacity = capacityBaseReduction + delta;

        // Analyze the reduction
        i32 notZeroCount = 0;
        i32 maxWeight = 0;
        for (i32 wIdx = 0; wIdx < nWeights; wIdx += 1)
        {
            i32 const weight = weightsBaseReduction[wIdx] + (wIdx < nBins ? delta : 0);
            notZeroCount += weight != 0;
            maxWeight = std::max(maxWeight, weight);
        }

        auto launch = [&](auto kernel, LambdaRange lRange, i32 kIdx) {
            i32 const lRangeSize = lRange.max - lRange.min + 1;
            if (lRangeSize <= 0)
                return;
            u32 const nBlocks = max<u32>(1u, ceilDiv<u32>(lRangeSize, BlockSize));
            cudaStream_t const stream = _cuStreams[rIdx * DFFS_COUNT + kIdx];
            kernel<<<nBlocks, BlockSize, _sharedMemSize, stream>>>(_id, _w, delta, lRange.min, lRange.max);
        };

        launch(calcDffLowerboundKernel<fCCM1_d>, lCCM1(capacity), 0);
        launch(calcDffLowerboundKernel<fMT_d>,   lMT(capacity),   1);
        launch(calcDffLowerboundKernel<fBJ1_d>,  lBJ1(capacity),  2);
        launch(calcDffLowerboundKernel<fVB2_d>,  sanitizeLambdaRange(lVB2(capacity), notZeroCount, maxWeight), 3);
        launch(calcDffLowerboundKernel<fFS1_d>,  sanitizeLambdaRange(lFS1(capacity), notZeroCount, maxWeight), 4);
        launch(calcDffLowerboundKernel<fRAD2_d>, lRAD2(capacity), 5);
    }

    // Wait for the kernels and copy the lowerbound back
    for (i32 sIdx = 0; sIdx < KernelsCount; sIdx += 1)
        cudaStreamSynchronize(_cuStreams[sIdx]);
    cudaMemcpy(&_id.h()->lowerbound, &_id.d()->lowerbound, sizeof(i32), cudaMemcpyDeviceToHost);

    if (_id->lowerbound > nBins)
        failNow();
}
