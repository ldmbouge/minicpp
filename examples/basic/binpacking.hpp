// Bin Packing model and search shared by the CPU (basic/binpacking.cpp)
// and GPU (gpu/binpacking_gpu.cu) examples.
// Instances are in BPPLIB format (number of items, capacity, then one weight per line, decreasing).
// Reference: CP for Bin Packing with Multi-Core and GPUs (DOI: 10.4230/LIPIcs.CP.2024.28)

#pragma once

#include <sys/resource.h>
#include <algorithm>
#include <cassert>
#include <climits>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "core/solver.hpp"
#include "core/search.hpp"
#include "core/constraint.hpp"
#include "constraints/binpacking.hpp"
#include "constraints/lexchainbit.hpp"

/**
 * Solves a BPPLIB instance to optimality (or until the timeout) and prints the packing.
 * @param lowerbound name of the lower bound printed in the header (e.g. "CPU", "GPU")
 * @param makeLoad   (cp, binsLoad, itemsBin, weights) -> the bin packing constraint to post
 */
template <class MakeLoad>
int solveBinPacking(std::string const& instance, int timeout, bool printStats,
                    std::string const& lowerbound, MakeLoad makeLoad)
{
   using namespace std;
   using namespace Factory;

   // Deep searches recurse a lot: raise the stack limit to 256 MB
   struct rlimit limit = {256 * 1024 * 1024, 256 * 1024 * 1024};
   setrlimit(RLIMIT_STACK, &limit);

   // Read instance
   ifstream file(instance);
   int nItems, capacity;
   file >> nItems >> capacity;
   vector<int> weights(nItems);
   for (auto& w : weights)
      file >> w;
   // The symmetry breaking and the search assume equal weights are contiguous and decreasing
   stable_sort(weights.begin(), weights.end(), greater<int>());

   cout << "Timeout: " << timeout << " s" << endl
        << "Lowerbound: " << lowerbound << endl
        << "File: " << instance << endl
        << "---" << endl
        << "Capacity: " << capacity << endl
        << "Items (" << nItems << "): ";
   for (int w : weights) cout << w << " ";
   cout << endl;

   SearchStatistics stats;
   auto cp = makeSolver();

   // Bounds on the number of bins
   int const sumWeights = accumulate(weights.begin(), weights.end(), 0);
   int const lb = BinPackingLoad::ceilDivPos(sumWeights, capacity);
   int const ub = lb * 2;

   // Variables
   auto maxBin = makeIntVar(cp, lb - 1, ub);   // index of the last used bin
   vector<var<int>::Ptr> itemsBin;
   for (int i = 0; i < nItems; i += 1)
      itemsBin.push_back(makeIntVar(cp, 0, ub));
   vector<var<int>::Ptr> binsLoad;
   for (int b = 0; b <= ub; b += 1)
      binsLoad.push_back(makeIntVar(cp, 0, capacity));

   // Constraints
   for (int i = 0; i < nItems; i += 1)
      cp->post(new (cp) LessOrEqual(itemsBin[i], maxBin));
   cp->post(new (cp) LexChainBit(itemsBin));
   cp->post(makeLoad(cp, binsLoad, itemsBin, weights));

   // Search: complete decreasing best fit, with dominance rules
   // Working memory is preallocated
   vector<int> residualCapacity(nItems, capacity);
   struct BinInfo { int index; int residual; };
   vector<BinInfo> binsResidualDecreasing(nItems);
   vector<int> candidates(nItems, 0);

   auto completeDecreasingBestFit = [&]() -> Branches {
      // Residual capacity of every bin
      int const nBins = maxBin->max() + 1;
      fill(residualCapacity.begin(), residualCapacity.end(), capacity);
      for (int i = 0; i < nItems; i += 1)
         if (itemsBin[i]->isBound())
            residualCapacity[itemsBin[i]->min()] -= weights[i];

      // Dominance rules
      bool someItemAssigned = true;
      while (someItemAssigned) {
         someItemAssigned = false;

         // Perfect fit
         for (int b = 0; b < nBins; b += 1) {
            auto& residual = residualCapacity[b];
            for (int i = 0; i < nItems; i += 1) {
               auto& x = itemsBin[i];
               if (not x->isBound() and x->contains(b) and residual == weights[i]) {
                  residual = 0;
                  x->assign(b);
                  someItemAssigned = true;
                  break;
               }
            }
         }

         // Only one item fits
         binsResidualDecreasing.clear();
         for (int b = 0; b < nBins; b += 1)
            binsResidualDecreasing.push_back({b, residualCapacity[b]});
         sort(binsResidualDecreasing.begin(), binsResidualDecreasing.end(),
              [](auto const& l, auto const& r) { return l.residual > r.residual; });

         for (int br = 0; br < nBins; br += 1) {
            int const b = binsResidualDecreasing[br].index;
            auto& residual = residualCapacity[b];
            candidates.clear();
            for (int i = 0; i < nItems; i += 1) {
               auto& x = itemsBin[i];
               if (not x->isBound() and x->contains(b) and residual >= weights[i])
                  candidates.push_back(i);
            }
            int const nCandidates = static_cast<int>(candidates.size());
            if (nCandidates > 1 and
                weights[candidates[nCandidates - 1]] + weights[candidates[nCandidates - 2]] > residual) {
               int const biggest = candidates[0];
               residual -= weights[biggest];
               itemsBin[biggest]->assign(b);
               someItemAssigned = true;
            }
         }
      }

      cp->fixpoint();

      // Biggest unassigned item
      int iIdx = -1;
      for (int i = 0; i < nItems; i += 1)
         if (itemsBin[i]->size() > 1 and (iIdx < 0 or weights[i] > weights[iIdx]))
            iIdx = i;
      if (iIdx < 0)
         return Branches({});
      auto itemBin = itemsBin[iIdx];
      int const iWeight = weights[iIdx];

      // Best fit bin
      int bestBin = INT_MIN;
      int bestResidual = INT_MAX;
      for (int b = 0; b < nBins; b += 1) {
         if (itemBin->contains(b)) {
            int const r = residualCapacity[b] - iWeight;
            if (0 <= r and r < bestResidual) {
               bestBin = b;
               bestResidual = r;
            }
         }
      }

      // Bins with the same residual capacity as the best one (residualCapacity is overwritten deeper in the search)
      vector<int> equivalentBins;
      if (bestBin >= 0)
         for (int b = 0; b < nBins; b += 1)
            if (residualCapacity[b] == residualCapacity[bestBin])
               equivalentBins.push_back(b);

      return Branches({
         // Put the item in the best bin
         [=] { cp->post(itemBin == bestBin); },
         // Or in none of the bins equivalent to it, for all the items equivalent to it
         [=, &itemsBin, &weights] {
            for (int b : equivalentBins)
               for (int j = iIdx; j < nItems and weights[j] == iWeight; j += 1)
                  if (not itemsBin[j]->isBound())
                     itemsBin[j]->remove(b);
            cp->fixpoint();
         }
      });
   };

   // Try the number of bins in increasing order, then pack the items: the first solution is optimal
   auto boundSearch = [&]() -> Branches {
      return maxBin->isBound() ? Branches({}) : indomain_min(cp, maxBin);
   };
   DFSearch search(cp, land(boundSearch, completeDecreasingBestFit));

   search.onSolution([&]() {
      cout << "Objective: " << maxBin->min() + 1 << endl;
      cout << "Packing: ";
      for (auto const& x : itemsBin)
         cout << x->min() << " ";
      cout << endl;
      cout << "Loads: ";
      vector<int> load(maxBin->min() + 1, 0);
      for (int i = 0; i < nItems; i += 1)
         load[itemsBin[i]->min()] += weights[i];
      for (int l : load) {
         assert(0 < l and l <= capacity);
         cout << l << " ";
      }
      cout << endl;
   });

   auto startTime = RuntimeMonitor::now();
   search.solve(stats, [&](SearchStatistics const& s) {
      return RuntimeMonitor::elapsedSeconds(startTime) >= timeout or s.numberOfSolutions() > 0;
   });

   if (printStats) {
      cout << fixed << setprecision(3)
           << "---" << endl
           << "Time: " << RuntimeMonitor::elapsedSeconds(startTime) << " s" << endl
           << "Propagations: " << cp->getPropagations() << endl
           << "Nodes: " << stats.numberOfNodes() << endl
           << "Failures: " << stats.numberOfFailures() << endl;
   }
   return 0;
}
