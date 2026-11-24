#include <algorithm>
#include <cassert>
#include "constraints_gpu/binpacking.cuh"

// Dual-Feasible Functions (branch-less, mirror the CPU versions in constraints/binpacking.cpp)

__device__ static int fMT_d(int w, int l, int c)
{
   int const c0 = w > c - l;
   int const c1 = l <= w and w <= c - l;
   int const c2 = w < l;
   return (c0 * c) + (c1 * w) + (c2 * 0);
}

__device__ static int fCCM1_d(int w, int l, int c)
{
   int const c0 = 2 * w > c;  // w > c / 2
   int const c1 = 2 * w == c; // w == c / 2
   int const c2 = 2 * w < c;  // w < c / 2
   int const v0 = 2 * (c / l - (c - w) / l);
   int const v1 = c / l;
   int const v2 = 2 * (w / l);
   return (c0 * v0) + (c1 * v1) + (c2 * v2);
}

__device__ static int fFS1_d(int w, int l, int c)
{
   int const c0 = (w * (l + 1)) % c == 0;
   int const c1 = (w * (l + 1)) % c != 0;
   int const v0 = w * l;
   int const v1 = ((w * (l + 1)) / c) * c;
   return (c0 * v0) + (c1 * v1);
}

__device__ static int fBJ1_d(int w, int l, int c)
{
   int const p = l - (c % l);
   int const c0 = w % l <= c % l;
   int const c1 = w % l > c % l;
   int const v0 = (w / l) * p;
   int const v1 = (w / l) * p + (w % l) - (c % l);
   return (c0 * v0) + (c1 * v1);
}

__device__ static int fVB2Base_d(int w, int l, int c)
{
   int const v = BinPackingLoad::ceilDivPos(l * w, c);
   return v > 0 ? v - 1 : 0;
}

__device__ static int fVB2_d(int w, int l, int c)
{
   int const c0 = 2 * w > c;
   int const c1 = 2 * w == c;
   int const c2 = 2 * w < c;
   int const t0 = fVB2Base_d(c, l, c);
   int const t1 = fVB2Base_d(w, l, c);
   int const t2 = fVB2Base_d(c - w, l, c);
   return (c0 * (2 * t0 - 2 * t2)) + (c1 * t0) + (c2 * 2 * t1);
}

__device__ static int fRAD2Base_d(int w, int l, int c)
{
   int const c1 = l <= w and w <= c - 2 * l;
   int const c2 = c - 2 * l < w and w < 2 * l;
   return (c1 * (c / 3)) + (c2 * (c / 2));
}

__device__ static int fRAD2_d(int w, int l, int c)
{
   int const c0 = w < 2 * l;
   int const c1 = 2 * l <= w;
   return (c0 * fRAD2Base_d(w, l, c)) + (c1 * (c - fRAD2Base_d(c - w, l, c)));
}

// One thread per lambda in [minLambda, maxLambda]; the best bound is atomically maxed into id->lowerbound
template <int f(int, int, int)>
__global__ void calcDffLowerboundKernel(BinPackingLoadGPU::InstanceData* id, int const* weightsBase,
                                        int deltaReduction, int minLambda, int maxLambda)
{
   extern __shared__ int weights_s[];
   __shared__ int lowerbound_s;
   __shared__ bool active_s;

   int const capacity = id->capacityBaseReduction + deltaReduction;
   int const nWeights = id->nWeights;
   int const nBins = id->nBins;

   // id->lowerbound is raised concurrently by other kernels: read it once per block
   if (threadIdx.x == 0) {
      lowerbound_s = 0;
      active_s = id->lowerbound <= nBins and capacity > 0;
   }
   __syncthreads();

   if (active_s) {
      // Reduction in shared memory
      for (int wIdx = threadIdx.x; wIdx < nWeights; wIdx += blockDim.x)
         weights_s[wIdx] = weightsBase[wIdx] + (wIdx < nBins ? deltaReduction : 0);
      __syncthreads();

      int lowerbound = 0;
      int const lambda = minLambda + static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
      if (lambda <= maxLambda) {
         int sumTransformedWeights = 0;
         for (int wIdx = 0; wIdx < nWeights; wIdx += 1)
            sumTransformedWeights += f(weights_s[wIdx], lambda, capacity);
         int const transformedCapacity = f(capacity, lambda, capacity);
         lowerbound = BinPackingLoad::ceilDivPos(sumTransformedWeights, transformedCapacity);
      }

      // Warp, then block, maximum
      int const lowerbound_w = __reduce_max_sync(__activemask(), lowerbound);
      if (threadIdx.x % 32 == 0)
         atomicMax_block(&lowerbound_s, lowerbound_w);
   }
   __syncthreads();

   if (threadIdx.x == 0)
      atomicMax(&id->lowerbound, lowerbound_s);
}

BinPackingLoadGPU::BinPackingLoadGPU(std::vector<var<int>::Ptr> const& load, std::vector<var<int>::Ptr> const& bin, std::vector<int> const& w)
   : BinPackingLoad(load, bin, w)
{
   setPriority(CLOW);

   std::size_t const nWeights = weightsBaseReduction.size(); // nItems + nBins
   cudaMallocHost(&_id_h, sizeof(InstanceData));
   cudaMalloc(&_id_d, sizeof(InstanceData));
   cudaMallocHost(&_weights_h, sizeof(int) * nWeights);
   cudaMalloc(&_weights_d, sizeof(int) * nWeights);
   _sharedMemSize = sizeof(int) * nWeights;

   for (auto& s : _streams)
      cudaStreamCreate(&s);
}

BinPackingLoadGPU::~BinPackingLoadGPU()
{
   for (auto& s : _streams)
      cudaStreamDestroy(s);
   cudaFreeHost(_id_h);
   cudaFree(_id_d);
   cudaFreeHost(_weights_h);
   cudaFree(_weights_d);
}

void BinPackingLoadGPU::checkLowerbounds()
{
   auto blocksFor = [](LambdaRange r) {
      int const jobs = std::max(0, r.max - r.min + 1);
      return std::max(1, ceilDivPos(jobs, BlockSize));
   };

   // Copy the base reduction to the GPU
   int const nWeights = static_cast<int>(weightsBaseReduction.size());
   _id_h->lowerbound = 0;
   _id_h->nBins = nBins;
   _id_h->nWeights = nWeights;
   _id_h->capacityBaseReduction = capacityBaseReduction;
   std::copy(weightsBaseReduction.begin(), weightsBaseReduction.end(), _weights_h);
   cudaMemcpy(_id_d, _id_h, sizeof(InstanceData), cudaMemcpyHostToDevice);
   cudaMemcpy(_weights_d, _weights_h, sizeof(int) * nWeights, cudaMemcpyHostToDevice);

   // One kernel per (reduction, DFF)
   for (int rIdx = 0; rIdx < REDUCTIONS_COUNT; rIdx += 1) {
      int const delta = deltaReductions[rIdx];
      int const capacity = capacityBaseReduction + delta;

      int notZeroCount = 0;
      int maxWeight = 0;
      for (int wIdx = 0; wIdx < nWeights; wIdx += 1) {
         int const weight = weightsBaseReduction[wIdx] + (wIdx < nBins ? delta : 0);
         notZeroCount += weight != 0;
         maxWeight = std::max(maxWeight, weight);
      }

      cudaStream_t* s = &_streams[rIdx * DFFS_COUNT];
      auto launch = [&](auto kernel, LambdaRange r, cudaStream_t stream) {
         kernel<<<blocksFor(r), BlockSize, _sharedMemSize, stream>>>(_id_d, _weights_d, delta, r.min, r.max);
      };
      launch(calcDffLowerboundKernel<fCCM1_d>, lCCM1(capacity), s[0]);
      launch(calcDffLowerboundKernel<fMT_d>, lMT(capacity), s[1]);
      launch(calcDffLowerboundKernel<fBJ1_d>, lBJ1(capacity), s[2]);
      launch(calcDffLowerboundKernel<fVB2_d>, sanitizeLambdaRange(lVB2(capacity), notZeroCount, maxWeight), s[3]);
      launch(calcDffLowerboundKernel<fFS1_d>, sanitizeLambdaRange(lFS1(capacity), notZeroCount, maxWeight), s[4]);
      launch(calcDffLowerboundKernel<fRAD2_d>, lRAD2(capacity), s[5]);
   }

   for (auto& s : _streams)
      cudaStreamSynchronize(s);
   cudaMemcpy(&_id_h->lowerbound, &_id_d->lowerbound, sizeof(int), cudaMemcpyDeviceToHost);

   if (_id_h->lowerbound > nBins)
      failNow();
}
