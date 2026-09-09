#ifndef __PACKING_HPP
#define __PACKING_HPP

#include <iostream>
#include <vector>

#include "device.hpp"
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

#include "global_constraints/bin_packing.hpp"
#include "global_constraints/lex_chain_bit.hpp"
#if __CUDACC__
#include "global_constraints_gpu/bin_packing.cuh"
#endif

namespace Factory {
   /**
    * Bin packing: `bin[i]` is the bin holding item `i` (of weight `w[i]`) and
    * `load[b]` is the total weight in bin `b`.
    */
   template<typename LoadVars, typename BinVars>
   inline Constraint::Ptr binPacking(LoadVars const & load,
                                     BinVars const & bin,
                                     std::vector<int> const & w,
                                     enum CDevice dev = CPU) {
      switch (dev) {
         case CPU:
            return new BinPackingLoad(load, bin, w);
         case GPU: default:
#ifdef __CUDACC__
            return new BinPackingLoadGPU(load, bin, w);
#else
            std::cerr << "Warning: this code was not compiled for NVIDIA libs. Falling back on bin packing on CPU.\n";
            return new BinPackingLoad(load, bin, w);
#endif
      }
   }

   /**
    * Symmetry breaking over interchangeable bins: bin `b` can only be used once
    * every bin before it is used, by an item of smaller index.
    */
   template<typename Vars>
   inline Constraint::Ptr lexChainBit(Vars const & rows) {
      return new LexChainBit(rows);
   }
};

#endif
