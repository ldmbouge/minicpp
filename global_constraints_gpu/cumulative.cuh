#pragma once

#include "varitf.hpp"
#include "constraint.hpp"
#include "global_constraints/cumulative.hpp"
#include "gfl/Types.hpp"
#include "gfl/Memory.hpp"
#include "gfl/Arrays.hpp"
#include "gfl/Vectors.hpp"

// References:
// - Constraint-Based Scheduling (ISBN: 978-1-4615-1479-4)
// - A New Characterization of Relevant Intervals for Energetic Reasoning (DOI: 10.1007/978-3-319-10428-7_22)

class CumulativeGPU : public Constraint {
public:
    static constexpr gfl::i32 BlockSize = 128;
    static constexpr gfl::i32 MaxActivities = 128;
    using TimeInterval = Cumulative::TimeInterval;
    using Domain = Cumulative::Domain;
    using ProcessingTimes = gfl::MirrorArray<gfl::i32>;
    using Requirements = gfl::MirrorArray<gfl::i32>;
    using StartIntervals = gfl::MirrorArray<Domain>;
    using RelevantIntervals = gfl::DeviceVector<TimeInterval>;

private:
    gfl::i32 _n;
    std::vector<var<int>::Ptr> _x;
    gfl::MirrorPtr<ProcessingTimes> _p;
    gfl::MirrorPtr<Requirements> _h;
    gfl::i32 _c;
    gfl::MirrorPtr<StartIntervals> _si;
    gfl::MirrorPtr<bool> _fail;
    gfl::MirrorPtr<RelevantIntervals> _ri;

    gfl::MirrorPool _mPool;
    gfl::DevicePool _dPool;
    gfl::MirrorRegion _roRegion;
    gfl::MirrorRegion _ioRegion;

    gfl::i32 _device;
    gfl::u32 _mp; // Mow many multi-processors
    cudaStream_t _cuStream; // a GPU queue to offload work
    cudaGraphExec_t _cuGraph; // The executable kernel graph

public:
    template<typename Container>
    CumulativeGPU(Container& x, std::vector<int> const& p, std::vector<int> const& h, int c) :
        Constraint(x[0]->getSolver()), _n(x.size()), _x(x.begin(),x.end()), _c(c)
    {
        assert(p.size() == x.size());
        assert(h.size() == x.size());

        _roRegion = _mPool.record([this]() {
            _p = _mPool.make<ProcessingTimes>(_mPool, _n);
            _h = _mPool.make<Requirements>(_mPool, _n);
        });

        _ioRegion = _mPool.record([this]() {
            _si = _mPool.make<StartIntervals>(_mPool, _n);
            _fail = _mPool.make<bool>();
            _ri = _mPool.make<RelevantIntervals>(_dPool, Cumulative::IntervalsPerActivityPair * _n * _n);
        });

        _p->loadFrom(p);
        _h->loadFrom(h);

        // CUDA initialization
        cudaGetDevice(&_device);
        cudaDeviceProp cuProp;
        cudaGetDeviceProperties(&cuProp, _device);
        _mp = cuProp.multiProcessorCount;
        cudaStreamCreate(&_cuStream);
        setPriority(CLOW);
    }

    void post() override;
    void propagate() override;
};