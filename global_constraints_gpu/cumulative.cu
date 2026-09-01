#include "gfl/Arrays.hpp"
#include "gfl/Memory.hpp"
#include "gfl/CudaUtils.hpp"
#include "global_constraints_gpu/cumulative.cuh"

GFL_GLOBAL void calcRiKernel(gfl::MirrorPtr<CumulativeGPU::StartIntervals> si,
                             gfl::MirrorPtr<CumulativeGPU::ProcessingTimes> p,
                             gfl::MirrorPtr<CumulativeGPU::RelevantIntervals> ri) {
    using namespace gfl;
    using TimeInterval =  CumulativeGPU::TimeInterval;

    i32 const n = si->size();
    auto [ijBegin, ijEnd] = getBeginEnd<u32>(blockIdx.x, gridDim.x, n * n);
    for (u32 ijIdx = ijBegin + threadIdx.x; ijIdx < ijEnd; ijIdx += blockDim.x){
        i32 const i = ijIdx / n;
        i32 const j = ijIdx % n;

        TimeInterval intervalsBuff[Cumulative::IntervalsPerActivityPair];
        VectorView<TimeInterval> intervals(intervalsBuff, Cumulative::IntervalsPerActivityPair);
        auto collectIfValid = [&](TimeInterval ti) { if (ti.start < ti.end) intervals.append(ti); };

        if (si->at(i).changed or si->at(j).changed) {
            i32 const pi = p->at(i);
            i32 const esti = si->at(i).min;
            i32 const lsti = si->at(i).max;
            i32 const lcti = lsti + pi;

            i32 const pj = p->at(j);
            i32 const estj = si->at(j).min;
            i32 const lstj = si->at(j).max;
            i32 const ectj = estj + pj;
            i32 const lctj = lstj + pj;

            // Case 1
            collectIfValid({esti, ectj});
            collectIfValid({esti, lctj});
            collectIfValid({lsti, ectj});
            collectIfValid({lsti, lctj});

            // Case 2
            collectIfValid({esti, estj + lctj - esti});
            collectIfValid({esti, estj + lctj - lsti});
            collectIfValid({lsti, estj + lctj - esti});
            collectIfValid({lsti, estj + lctj - lsti});

            // Case 3
            collectIfValid({esti + lcti - ectj, ectj});
            collectIfValid({esti + lcti - ectj, lctj});
            collectIfValid({esti + lcti - lctj, ectj});
            collectIfValid({esti + lcti - lctj, lctj});

            ri->appendAtomic(intervals.view());
        }
    }
}

GFL_GLOBAL void updateSiKernel(gfl::MirrorPtr<CumulativeGPU::Requirements> h,
                               gfl::MirrorPtr<CumulativeGPU::ProcessingTimes> p,
                               gfl::i32 const c,
                               gfl::MirrorPtr<CumulativeGPU::RelevantIntervals> ri,
                               gfl::MirrorPtr<CumulativeGPU::StartIntervals> si,
                               gfl::MirrorPtr<bool> fail)
{
    using namespace gfl;
    using Domain =  CumulativeGPU::Domain;

    __shared__ Domain si_s[CumulativeGPU::MaxActivities];

    if (*fail) return;

    i32 const n = si->size();
    for (auto i = threadIdx.x; i < n; i += blockDim.x)
        si_s[i] = si->at(i);
    __syncthreads();

    auto [iBegin, iEnd] = getBeginEnd<u32>(blockIdx.x, gridDim.x, scast<u32>(ri->size()));
    for (auto j = iBegin + threadIdx.x; j < iEnd and (not *fail); j += blockDim.x) {
        auto const [t1,t2] = ri->at(j);
        i32 w = 0;
        for (i32 i = 0; i < n; i += 1) {
            i32 const hi = h->at(i);
            i32 const pi = p->at(i);
            i32 const esti = si_s[i].min;
            i32 const lsti = si_s[i].max;
            i32 const ls = max(0, min(esti + pi, t2) - max(esti, t1));
            i32 const rs = max(0, min(lsti + pi, t2) - max(lsti, t1));
            w += hi * min(ls, rs);
        }

        if (w <= c * (t2 - t1))
            for (i32 i = 0; i < n; i += 1){
                i32 const hi = h->at(i);
                i32 const pi = p->at(i);
                i32 const esti = si_s[i].min;
                i32 const lsti = si_s[i].max;
                i32 const ls = max(0, min(esti + pi, t2) - max(esti, t1));
                i32 const rs = max(0, min(lsti + pi, t2) - max(lsti, t1));
                i32 const avail = c * (t2 - t1) - w + hi * min(ls, rs);
                if (avail < hi * ls)
                    atomicMax_block(&si_s[i].min, t2 - (avail / hi));
                if (avail < hi * rs)
                    atomicMin_block(&si_s[i].max, t1 + (avail / hi) - pi);
            }
        else
            *fail = true;
    }
    __syncthreads();

    if (not *fail)
        for (auto i = threadIdx.x; i < n; i += blockDim.x) {
            atomicMax(&si->at(i).min, si_s[i].min);
            atomicMin(&si->at(i).max, si_s[i].max);
        }
}

void CumulativeGPU::post()
{
    using namespace std;
    using namespace gfl;

    *_fail = false;
    _ri->clear();
    for (auto const & v : _x)
        v->propagateOnBoundChange(this);

    // Copy constants data on GPU
    _roRegion.copyToDeviceAsync(_cuStream);
    _ioRegion.copyToDeviceAsync(_cuStream);

    _cuGraph = gfl::capture(_cuStream,[this]() {
         _ioRegion.copyToDeviceAsync(_cuStream);
         calcRiKernel<<<_mp, BlockSize, 0, _cuStream>>>(_si, _p, _ri);
         updateSiKernel<<<_mp, BlockSize, 0, _cuStream>>>(_h, _p, _c, _ri, _si, _fail);
        _ioRegion.copyToHostAsync(_cuStream);
    });
}

void CumulativeGPU::propagate(){
    *_fail = false;
    _ri->clear();
    for (auto i = 0; i < _n; i += 1)
        _si->at(i) = {_x[i]->changed(), _x[i]->min(), _x[i]->max()};

    // Propagation
    cudaGraphLaunch(_cuGraph, _cuStream);
    cudaStreamSynchronize(_cuStream);

    // Filtering
    if (not *_fail)
        for (auto i = 0; i < _n; i += 1)
            _x[i]->updateBounds(_si->at(i).min,_si->at(i).max);
    else
        failNow();
}