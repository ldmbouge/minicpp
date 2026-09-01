#ifndef __SCHEDULING_HPP
#define __SCHEDULING_HPP

#include <iostream>
#include <fstream>
#include "solver.hpp"
#include "intvar.hpp"
#include "constraint.hpp"
#include "search.hpp"

#include "ttable.hpp"
#include "global_constraints/cumulative.hpp"
#if __CUDACC__
#include "global_constraints_gpu/cumulative.cuh"
#endif

namespace Factory {
   enum CDevice { CPU, GPU };

   inline Constraint::Ptr cumulativeER(Factory::Veci &s,
                                       std::vector<int> const &p,
                                       std::vector<int> const &h, int c,
                                       enum CDevice dev = CPU) {
      switch (dev) {
        case CPU:
           return new Cumulative(s,p,h,c);
      case GPU:default:         
#ifdef __CUDACC__
          return new CumulativeGPU(s,p,h,c);
#else
          std::cerr << "Warning: this code was not compiled for NVIDIA libs. Falling back on energetic on CPU.\n";
          return new Cumulative(s, p, h, c);
#endif
     }
   }

   inline Constraint::Ptr cumulativeTT(Factory::Veci &start,
                                       const std::vector<int> &dur,
                                       const std::vector<int> &reqs, int capa,
                                       enum CDevice dev = CPU) {
     switch (dev) {
        default: // not worth running timetable on a GPU.
           return new CumulativeTT(start, dur, reqs, capa);
     }           
   }  

};

#endif

