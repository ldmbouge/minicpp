#ifndef __CIRCUIT_HPP
#define __CIRCUIT_HPP

#include <iostream>
#include "device.hpp"
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"

#if __CUDACC__
#include "global_constraints_gpu/circuit.cuh"
#endif

namespace Factory {
   /**
    * Hamiltonian circuit over `xs`, with the alldifferent it rests on run on
    * `dev`. Both devices enforce domain consistency on that alldifferent and
    * filter identically, so they explore the same search tree; only the
    * strongly-connected-components phase moves. The one-argument
    * Factory::circuit keeps the cheaper value-consistent alldifferent instead.
    * @see AllDifferentGPU
    */
   template<typename Vars>
   inline Constraint::Ptr circuit(Vars const & xs, enum CDevice dev) {
      switch (dev) {
         case CPU:
            return new (xs[0]->getSolver()) CircuitAC(xs);
         case GPU: default:
#ifdef __CUDACC__
            return new (xs[0]->getSolver()) CircuitGPU(xs);
#else
            std::cerr << "Warning: this code was not compiled for NVIDIA libs. Falling back on circuit on CPU.\n";
            return new (xs[0]->getSolver()) CircuitAC(xs);
#endif
      }
   }
};

#endif
