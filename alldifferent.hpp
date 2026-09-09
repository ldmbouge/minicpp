#ifndef __ALLDIFFERENT_HPP
#define __ALLDIFFERENT_HPP

#include <iostream>
#include "device.hpp"
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"

#if __CUDACC__
#include "global_constraints_gpu/all_different.cuh"
#endif

namespace Factory {
   /**
    * Domain-consistent alldifferent over `xs`. Both devices run the same
    * algorithm and filter identically; the GPU one only moves the
    * strongly-connected-components phase off the CPU, which pays off on a
    * dense residual graph (many variables sharing many values).
    */
   template<typename Vars>
   inline Constraint::Ptr allDifferentAC(Vars const & xs, enum CDevice dev) {
      switch (dev) {
         case CPU:
            return new (xs[0]->getSolver()) AllDifferentAC(xs);
         case GPU: default:
#ifdef __CUDACC__
            return new (xs[0]->getSolver()) AllDifferentGPU(xs);
#else
            std::cerr << "Warning: this code was not compiled for NVIDIA libs. Falling back on alldifferent on CPU.\n";
            return new (xs[0]->getSolver()) AllDifferentAC(xs);
#endif
      }
   }
};

#endif
