#pragma once

#include <tuple>
#include <cuda_runtime_api.h>
#include "gfl/Types.hpp"
#include "gfl/Memory.hpp"
#include "gfl/Arrays.hpp"
#include "gfl/Vectors.hpp"
#include "gfl/DebugUtils.hpp"

#include <varitf.hpp>
#include <constraint.hpp>

#include "Vectors.hpp"
#include "global_constraints/cumulative.hpp"

#define MAX_INTERVALS_PER_ACTIVITY_PAIR 12
#define CBS 128
#define MAX_ACTIVITIES 128
#define INPUT_OUTPUT_MEMORY 16 * 1024 * 1024 // 16 MB

// References:
// - Constraint-Based Scheduling (ISBN: 978-1-4615-1479-4)
// - A New Characterization of Relevant Intervals for Energetic Reasoning (DOI: 10.1007/978-3-319-10428-7_22)

class CumulativeGPU : public Constraint
{
public:
    struct TimeInterval
    {
        gfl::i32 start;
        gfl::i32 end;
    };

    struct Domain {
        bool changed;
        gfl::i32 min;
        gfl::i32 max;
    };

    using ProcessingTimes = gfl::MirrorArray<gfl::i32>;
    using Requirements = gfl::MirrorArray<gfl::i32>;
    using RelevantIntervals = gfl::DeviceVector<TimeInterval>;
    using StartIntervals = gfl::MirrorArray<Domain>;

private:
    gfl::i32 _n;
    gfl::i32 _c;
    std::vector<var<int>::Ptr> _s;
    gfl::MirrorPool _roPool;
    gfl::MirrorPool _ioPool;
    gfl::MemoryPool<gfl::DeviceAllocator> _dPool;
    gfl::MirrorPtr<ProcessingTimes> _p;
    gfl::MirrorPtr<Requirements> _h;
    gfl::MirrorPtr<RelevantIntervals> _ri;
    gfl::MirrorPtr<StartIntervals> _si;
    gfl::MirrorPtr<bool> _fail;

    // CUDA
    gfl::i32 deviceId;
    gfl::u32 nSM; // Mow many multi-processors
    cudaStream_t cuStream; // a GPU queue to offload work
    cudaGraphExec_t cuGraph; // The executable kernel graph

public:
    template <typename Container>
    CumulativeGPU(Container& s, std::vector<int> const& p, std::vector<int> const& h, int c) :
        Constraint(s[0]->getSolver()), _n(gfl::scast<gfl::i32>(s.size ())), _c(c),_s(_n),
        _roPool(),
        _ioPool(),
        _dPool(),
        _p(_roPool.make<ProcessingTimes>(_roPool,_n)),
        _h(_roPool.make<Requirements>(_roPool,_n)),
        _ri(_ioPool.make<RelevantIntervals>(_dPool,MAX_INTERVALS_PER_ACTIVITY_PAIR*_n*_n)),
        _si(_ioPool.make<StartIntervals>(_ioPool,_n)),
        _fail(_ioPool.make<bool>())
    {

        gfl::checkOrAbort(p.size() == s.size(), "CumulativeGPU: |p| != |s|");
        gfl::checkOrAbort(h.size() == s.size(), "CumulativeGPU: |h| != |s|");

        for (auto i = 0; i < _n; i += 1)
        {
            _p->at(i) = p[i];
            _h->at(i) = h[i];
            _s[i] = s[i];
        }

        // CUDA initialization
        cudaGetDevice(&deviceId);
        cudaDeviceProp cuProp;
        cudaGetDeviceProperties(&cuProp, deviceId);
        nSM = cuProp.multiProcessorCount;
        cudaStreamCreate(&cuStream);
        setPriority(CLOW);
    }

    void post() override;
    void propagate() override;
};
