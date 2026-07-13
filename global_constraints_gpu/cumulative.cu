#include "gfl/Arrays.hpp"
#include "gfl/Memory.hpp"
#include "gfl/CudaUtils.hpp"
#include "global_constraints_gpu/cumulative.cuh"

GFL_GLOBAL void calcRiKernel(
                  gfl::MirrorPtr<CumulativeGPU::StartIntervals> si,
                  gfl::MirrorPtr<CumulativeGPU::ProcessingTimes> p,
                  gfl::MirrorPtr<CumulativeGPU::RelevantIntervals> ri) {
    using namespace gfl;
    using TimeInterval = CumulativeGPU::TimeInterval;

    i32 const n = si->size();
    auto [ijBegin, ijEnd] = getBeginEnd<u32>(blockIdx.x, gridDim.x, n * n);
    for (u32 ijIdx = ijBegin + threadIdx.x; ijIdx < ijEnd; ijIdx += blockDim.x)
    {
        u32 const i = ijIdx / n;
        u32 const j = ijIdx % n;
        u32 nGoodIntervals = 0;
        TimeInterval intervalsToTest[MAX_INTERVALS_PER_ACTIVITY_PAIR];
        if (si->at(i).changed or si->at(j).changed)
        {
            i32 const pi = p->at(i);
            i32 const siMin = si->at(i).min;
            i32 const siMax = si->at(i).max;
            i32 const eiMax = siMax + pi;

            i32 const pj = p->at(j);
            i32 const sjMin = si->at(j).min;
            i32 const sjMax = si->at(j).max;
            i32 const ejMin = sjMin + pj;
            i32 const ejMax = sjMax + pj;

            // Case 1
            intervalsToTest[0] = {siMin, ejMin};
            intervalsToTest[1] = {siMin, ejMax};
            intervalsToTest[2] = {siMax, ejMin};
            intervalsToTest[3] = {siMax, ejMax};

            // Case 2
            intervalsToTest[4] = {siMin, sjMin + ejMax - siMin};
            intervalsToTest[5] = {siMin, sjMin + ejMax - siMax};
            intervalsToTest[6] = {siMax, sjMin + ejMax - siMin};
            intervalsToTest[7] = {siMax, sjMin + ejMax - siMax};

            // Case 3
            intervalsToTest[8]  = {siMin + eiMax - ejMin, ejMin};
            intervalsToTest[9]  = {siMin + eiMax - ejMin, ejMax};
            intervalsToTest[10] = {siMin + eiMax - ejMax, ejMin};
            intervalsToTest[11] = {siMin + eiMax - ejMax, ejMax};

            for (u32 k = 0; k < MAX_INTERVALS_PER_ACTIVITY_PAIR; k += 1) {
                if (intervalsToTest[k].start < intervalsToTest[k].end) {
                    intervalsToTest[nGoodIntervals] = intervalsToTest[k];
                    nGoodIntervals += 1;
                }
            }

            if (nGoodIntervals > 0) {
                auto const oldSize = ri->resizeByAtomic(nGoodIntervals);
                memcpy(ri->data() + oldSize, intervalsToTest, sizeof(TimeInterval) * nGoodIntervals);
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
    using Domain = CumulativeGPU::Domain;

    __shared__ Domain si_s[MAX_ACTIVITIES];

    if (*fail) return;

    i32 const n = si->size();
    for (auto a = threadIdx.x; a < n; a += blockDim.x)
        si_s[a] = si->at(a);
    __syncthreads();

    auto [iBegin, iEnd] = getBeginEnd<u32>(blockIdx.x, gridDim.x, scast<u32>(ri->size()));
    for (auto i = iBegin + threadIdx.x; i < iEnd; i += blockDim.x)
    {
        i32 const t1 = ri->at(i).start;
        i32 const t2 = ri->at(i).end;

        i32 w = 0;
        for (i32 a = 0; a < n; a += 1)
        {
            i32 const ha = h->at(a);
            i32 const saMin = si_s[a].min;
            i32 const saMax = si_s[a].max;
            i32 const pa = p->at(a);
            i32 const ls = max(0, min(saMin + pa, t2) - max(saMin, t1));
            i32 const rs = max(0, min(saMax + pa, t2) - max(saMax, t1));
            w += ha * min(ls, rs);
        }

        if (w > c * (t2 - t1)) { *fail = true; continue; }

        for (i32 a = 0; a < n; a += 1)
        {
            i32 const ha = h->at(a);
            i32 const saMin = si_s[a].min;
            i32 const saMax = si_s[a].max;
            i32 const pa = p->at(a);
            i32 const ls = max(0, min(saMin + pa, t2) - max(saMin, t1));
            i32 const rs = max(0, min(saMax + pa, t2) - max(saMax, t1));
            i32 const avail = c * (t2 - t1) - w + ha * min(ls, rs);
            if (avail < ha * ls) atomicMax_block(&si_s[a].min, t2 - (avail / ha));
            if (avail < ha * rs) atomicMin_block(&si_s[a].max, t1 + (avail / ha) - pa);
        }
    }
    __syncthreads();

    for (auto a = threadIdx.x; a < n; a += blockDim.x)
    {
        atomicMax(&si->at(a).min, si_s[a].min);
        atomicMin(&si->at(a).max, si_s[a].max);
    }
}


void CumulativeGPU::post()
{
    using namespace std;
    using namespace gfl;

    _ri->clear();
    *_fail = false;
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

void CumulativeGPU::propagate()
{
    _ri->clear();
    *_fail = false;
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