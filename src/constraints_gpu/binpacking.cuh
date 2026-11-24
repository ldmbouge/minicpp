// BinPackingLoadGPU: BinPackingLoad whose Dual-Feasible-Function lower bounds run on the GPU.
// Every (reduction, DFF) pair is a kernel on its own stream; each thread evaluates one lambda
// (the CPU version samples LAMBDA_SAMPLES of them).
// The header is host-compilable: it can be included from plain .cpp files.
// Reference: CP for Bin Packing with Multi-Core and GPUs (DOI: 10.4230/LIPIcs.CP.2024.28)

#pragma once

#include <cuda_runtime_api.h>
#include "constraints/binpacking.hpp"

class BinPackingLoadGPU : public BinPackingLoad {
public:
   static constexpr int BlockSize = 128;

   // Mirrored on host (pinned) and device
   struct InstanceData {
      int lowerbound;
      int nBins;
      int nWeights;
      int capacityBaseReduction;
   };

private:
   InstanceData* _id_h;
   InstanceData* _id_d;
   int* _weights_h;
   int* _weights_d;
   std::size_t _sharedMemSize;
   cudaStream_t _streams[REDUCTIONS_COUNT * DFFS_COUNT];

public:
   BinPackingLoadGPU(std::vector<var<int>::Ptr> const& load, std::vector<var<int>::Ptr> const& bin, std::vector<int> const& w);
   ~BinPackingLoadGPU();

private:
   void checkLowerbounds() override;
};
