#pragma once

#include <tuple>
#include <cuda_runtime_api.h>
#include "gfl/Types.hpp"
#include "gfl/Memory.hpp"
#include "gfl/ArenaAllocator.hpp"
#include "gfl/Scalar.hpp"

#include <varitf.hpp>
#include <constraint.hpp>
#include "global_constraints/cumulative.hpp"

#define MAX_INTERVALS_PER_ACTIVITY_PAIR 12
#define CBS 128
#define MAX_ACTIVITIES 128
#define INPUT_OUTPUT_MEMORY 16 * 1024 * 1024 // 16 MB

// References:
// - Constraint-Based Scheduling (ISBN: 978-1-4615-1479-4)
// - A New Characterization of Relevant Intervals for Energetic Reasoning (DOI: 10.1007/978-3-319-10428-7_22)

class CumulativeGPU : public Cumulative {
private:
  gfl::i32 * p_d; // Processing time
  gfl::i32 * h_d; // Height
  gfl::i32 * nIntervals_d;
  Interval * i_d;
  gfl::Scalar<bool> isConsistent_h;
  StartInterval * si_h;
  gfl::Scalar<bool> isConsistent_d;
  StartInterval * si_d;
  // CUDA
  gfl::u32 sm_count;      // how many multi-processors
  cudaStream_t cu_stream; // a GPU queue to offload work
  cudaGraphExec_t propagateGraph; // The executable kernel graph
  gfl::MPool pool;
  gfl::Region region;
  void init();
public:
  template <typename Container>
  CumulativeGPU(Container & sa, std::vector<int> const & p, std::vector<int> const & h, int c)
    : Cumulative(sa,p,h,c),pool(INPUT_OUTPUT_MEMORY) {
    init();
  }    
  void post() override;
  void propagate() override;
};
