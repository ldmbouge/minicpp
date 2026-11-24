// BinPackingLoad: load[b] == sum of w[i] over the items i with bin[i] == b.
// Propagation: load coherence, load tightening and item elimination/commitment
// (basic and knapsack-based, Shaw 2004), and a feasibility check with lower bounds
// from Dual-Feasible Functions applied to reductions of the current partial packing.
// The GPU variant (constraints/binpacking_gpu.cuh) computes the same lower bounds on the GPU.
// References:
// - A Constraint for Bin Packing (DOI: 10.1007/978-3-540-30201-8_47)
// - CP for Bin Packing with Multi-Core and GPUs (DOI: 10.4230/LIPIcs.CP.2024.28)

#pragma once

#include <algorithm>
#include <array>
#include <climits>
#include <tuple>
#include <vector>
#include "core/intvar.hpp"
#include "core/acstr.hpp"
#include "gfl/FunQual.hpp"

class BinPackingLoad : public Constraint {
public:
   BinPackingLoad(std::vector<var<int>::Ptr> const& load, std::vector<var<int>::Ptr> const& bin, std::vector<int> const& w);
   void post() override;
   void propagate() override;

   // Ceiling division for a >= 0, b > 0 (shared with the GPU kernels)
   GFL_HOST_DEVICE static constexpr int ceilDivPos(int a, int b) { return (a + b - 1) / b; }

protected:
   struct ItemInfo { int index; int weight; };

   // Read-only view of a vector, optionally hiding one element
   template <typename T>
   class VectorView {
      std::vector<T> const& v;
      std::size_t idxToHide;
   public:
      explicit VectorView(std::vector<T> const& v) : v(v), idxToHide(SIZE_MAX) {}
      VectorView(std::vector<T> const& v, std::size_t idx) : v(v), idxToHide(idx) {}
      std::size_t size() const { return v.size() - (idxToHide != SIZE_MAX); }
      T const& at(std::size_t idx) const { return v.at(idx + (idx >= idxToHide)); }
   };

   std::vector<var<int>::Ptr> const _load;
   std::vector<var<int>::Ptr> _bin;     // sorted by decreasing weight
   std::vector<int> _weights;           // sorted decreasingly
   int nBins;
   int nItems;
   int sum_weights;
   int sum_min_loads;
   int sum_max_loads;
   std::vector<int> bin_sizes;
   std::vector<int> load_sizes;
   std::vector<int> required_loads;
   std::vector<int> possible_loads;
   std::vector<int> packed_items;
   std::vector<std::vector<ItemInfo>> candidate_items;

   void calculateNumBins();
   void calculateRequiredPossibleCandidates();
   void propagateLoadCoherence();
   void calculateSumLoads();
   void propagateBasicLoadTightening();
   void propagateBasicItemEliminationCommitment();
   void checkPackableBins();
   void propagateKnapsackLoadTightening();
   void propagateKnapsackItemEliminationCommitment();
   void saveBinLoadSizes();
   bool checkBinLoadSizeChanges();
   virtual void checkLowerbounds();
   std::tuple<bool,int,int> noSum(VectorView<ItemInfo> const& items_info, int alpha, int beta);
   static int sumWeights(VectorView<ItemInfo> const& items_info);

   // Reductions
   static constexpr int REDUCTIONS_COUNT = 3;
   std::vector<int> weightsBaseReduction;
   std::vector<int> weightsCurrentReduction;
   std::array<int,REDUCTIONS_COUNT> deltaReductions;
   int capacityBaseReduction;

   void prepareReductions();

   // Dual-Feasible Functions
   static constexpr int DFFS_COUNT = 6;

   static int fCCM1(int w, int l, int c);
   static int fMT(int w, int l, int c);
   static int fBJ1(int w, int l, int c);
   static int fVB2Base(int w, int l, int c);
   static int fVB2(int w, int l, int c);
   static int fFS1(int w, int l, int c);
   static int fRAD2Base(int w, int l, int c);
   static int fRAD2(int w, int l, int c);

   // Lambdas
   static constexpr int LAMBDA_SAMPLES = 256;
   struct LambdaRange { int min; int max; };

   static LambdaRange lCCM1(int c) { return {1, c / 2}; }
   static LambdaRange lMT(int c)   { return {0, c / 2}; } // 0 is included to calculate the L0 bound
   static LambdaRange lBJ1(int c)  { return {1, c}; }
   static LambdaRange lVB2(int c)  { return {2, c}; }
   static LambdaRange lFS1(int)    { return {1, 100}; }
   static LambdaRange lRAD2(int c) { return {c / 4 + 1, c / 3}; }
   static LambdaRange sanitizeLambdaRange(LambdaRange lambda, int nWeights, int maxWeight);

   // Lower bounds
   static int calcL2(std::vector<int> const& weights, int capacity);

   template <int f(int,int,int)>
   static int calcDffLowerboundSingleLambda(std::vector<int> const& weights, int capacity, int lambda)
   {
      int sumTransformedWeights = 0;
      for (int w : weights)
         sumTransformedWeights += f(w, lambda, capacity);
      int const transformedCapacity = f(capacity, lambda, capacity);
      return ceilDivPos(sumTransformedWeights, transformedCapacity);
   }

   template <int f(int,int,int), LambdaRange l(int)>
   static int calcDffLowerbound(std::vector<int> const& weights, int capacity, int nWeights, int maxWeight, bool sanitize = false)
   {
      int fLowerbound = 0;
      LambdaRange lRange = l(capacity);
      lRange = sanitize ? sanitizeLambdaRange(lRange, nWeights, maxWeight) : lRange;
      int const lStep = ceilDivPos(std::max(0, lRange.max - lRange.min + 1), LAMBDA_SAMPLES + 1);
      if (lStep == 0) return 0;
      for (int lambda = lRange.min + lStep; lambda < lRange.max; lambda += lStep)
         fLowerbound = std::max(fLowerbound, calcDffLowerboundSingleLambda<f>(weights, capacity, lambda));
      return fLowerbound;
   }
};
