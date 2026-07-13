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
        i32 goodIntervals = 0;
        TimeInterval intervalsToConsider[MAX_INTERVALS_PER_ACTIVITY_PAIR];

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
            intervalsToConsider[0] = {esti, ectj};
            intervalsToConsider[1] = {esti, lctj};
            intervalsToConsider[2] = {lsti, ectj};
            intervalsToConsider[3] = {lsti, lctj};

            // Case 2
            intervalsToConsider[4] = {esti, estj + lctj - esti};
            intervalsToConsider[5] = {esti, estj + lctj - lsti};
            intervalsToConsider[6] = {lsti, estj + lctj - esti};
            intervalsToConsider[7] = {lsti, estj + lctj - lsti};

            // Case 3
            intervalsToConsider[8]  = {esti + lcti - ectj, ectj};
            intervalsToConsider[9]  = {esti + lcti - ectj, lctj};
            intervalsToConsider[10] = {esti + lcti - lctj, ectj};
            intervalsToConsider[11] = {esti + lcti - lctj, lctj};

            for (u32 k = 0; k < MAX_INTERVALS_PER_ACTIVITY_PAIR; k += 1) {
                if (intervalsToConsider[k].start < intervalsToConsider[k].end) {
                    intervalsToConsider[goodIntervals] = intervalsToConsider[k];
                    goodIntervals += 1;
                }
            }

            if (goodIntervals > 0) {
                auto const oldSize = ri->resizeByAtomic(goodIntervals);
                memcpy(ri->data() + oldSize, intervalsToConsider, sizeof(TimeInterval) * goodIntervals);
            }
        }
    }
}

__global__ void updateSiKernel(gfl::MirrorPtr<CumulativeGPU::Requirements> h,
                               gfl::MirrorPtr<CumulativeGPU::ProcessingTimes> p,
                               gfl::i32 const c,
                               gfl::MirrorPtr<CumulativeGPU::RelevantIntervals> ri,
                               gfl::MirrorPtr<CumulativeGPU::StartIntervals> si,
                               gfl::MirrorPtr<bool> fail)
{
    using namespace gfl;
    using Domain =  CumulativeGPU::Domain;

    __shared__ Domain si_s[MAX_ACTIVITIES];

    if (*fail) return;

    i32 const n = si->size();
    for (auto i = threadIdx.x; i < n; i += blockDim.x)
        si_s[i] = si->at(i);
    __syncthreads();

    auto [iBegin, iEnd] = getBeginEnd<u32>(blockIdx.x, gridDim.x, scast<u32>(ri->size()));
    for (auto j = iBegin + threadIdx.x; j < iEnd and (not *fail); j += blockDim.x) {
        i32 const t1 = ri->at(j).start;
        i32 const t2 = ri->at(j).end;
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
                if (avail < hi * ls) atomicMax_block(&si_s[i].min, t2 - (avail / hi));
                if (avail < hi * rs) atomicMin_block(&si_s[i].max, t1 + (avail / hi) - pi);
            }
        else
            *fail = true;
    }
    __syncthreads();

    for (auto i = threadIdx.x; i < n; i += blockDim.x){
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
    for (auto const & v : _s)
        v->propagateOnBoundChange(this);

    // Copy constants data on GPU
    _ioPool.copyToDeviceAsync(cuStream);
    _roPool.copyToDeviceAsync(cuStream);

    cuGraph = gfl::capture(cuStream,[this]() {
         _ioPool.copyToDeviceAsync(cuStream);
         calcRiKernel<<<nSM, CBS, 0, cuStream>>>(_si, _p, _ri);
         updateSiKernel<<<nSM, CBS, 0, cuStream>>>(_h, _p, _c, _ri, _si, _fail);
        _ioPool.copyToHostAsync(cuStream);
    });
}

void CumulativeGPU::propagate(){
    *_fail = false;
    _ri->clear();
    for (auto i = 0; i < _n; i += 1)
        _si->at(i) = {_s[i]->changed(), _s[i]->min(), _s[i]->max()};

    // Propagation
    cudaGraphLaunch(cuGraph, cuStream);
    cudaStreamSynchronize(cuStream);

    // Filtering
    if (not *_fail)
        for (auto i = 0; i < _n; i += 1) {
            _s[i]->removeBelow(_si->at(i).min);
            _s[i]->removeAbove(_si->at(i).max);
        }
    else
        failNow();
}