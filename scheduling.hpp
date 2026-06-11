#ifndef __SCHEDULING_HPP
#define __SCHEDULING_HPP

#include "ttable.hpp"
#include "global_constraints/cumulative.hpp"
#if GPU==on
#pragma message("Compiling with CUDA...")
#include "gpu_constraints/cumulative.cuh"
#endif

namespace Factory {
   enum CDevice { CPU, GPU };

   inline Constraint::Ptr cumulativeEN(Factory::Veci &s,
                                       std::vector<int> const &p,
                                       std::vector<int> const &h, int c,
                                       enum CDevice dev = CPU) {
      switch (dev) {
        case CPU:
           return new Cumulative(s,p,h,c);
        case GPU:         
#if GPU==on
          return new CumulativeGPU(s, p, h, c);
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

