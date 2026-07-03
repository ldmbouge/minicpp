#pragma once

#include <tuple>
#include <cuda_runtime_api.h>
#include "gfl/Types.hpp"
#include "gfl/Memory.hpp"
#include "gfl/ArenaAllocator.hpp"

#include <varitf.hpp>
#include <constraint.hpp>
#include "global_constraints/cumulative.hpp"

#define MAX_INTERVALS_PER_ACTIVITY_PAIR 12
#define CUMULATIVE_BLOCK_SIZE 128
#define MAX_ACTIVITIES 1024
#define INPUT_OUTPUT_MEMORY 128 * 1024 // 128 KB

// References:
// - Constraint-Based Scheduling (ISBN: 978-1-4615-1479-4)
// - A New Characterization of Relevant Intervals for Energetic Reasoning (DOI: 10.1007/978-3-319-10428-7_22)

class CumulativeGPU : public Cumulative {
private:
  gfl::i32 * p_d; // Processing time
  gfl::i32 * h_d; // Height
  gfl::i32 * nIntervals_d;
  Interval * i_d;
  bool * isConsistent_h;
  StartInterval * si_h;
  bool * isConsistent_d;
  StartInterval * si_d;
  //  gfl::ArenaAllocator* allocator_h; // memory allocator host-side
  //  gfl::ArenaAllocator* allocator_d; // memory allocator device-side
  // CUDA
  gfl::u32 sm_count;      // how many multi-processors
  cudaStream_t cu_stream; // a GPU queue to offload work
  cudaGraph_t cu_graph;   // object to represent a kernel graph
  cudaGraphExec_t propagate_low_latency; // The executable kernel graph
  gfl::MPool pool;
  gfl::Region region;
  void init();
public:
  template <typename Container>
  CumulativeGPU(Container & sa, std::vector<int> const & p, std::vector<int> const & h, int c) : Cumulative(sa,p,h,c),pool(INPUT_OUTPUT_MEMORY) {
    init();
  }
    
  //CumulativeGPU(std::vector<var<int>::Ptr> & s, std::vector<int> const & p, std::vector<int> const & h, int c);
  void post() override;
  void propagate() override;
private:
  void initPropagateLowLatency();
  void propagateBase();
};
