/*
 * fzn-minicpp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License  v3
 * as published by the Free Software Foundation.
 *
 * fzn-minicpp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 * See the GNU Lesser General Public License  for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with mini-cp. If not, see http://www.gnu.org/licenses/lgpl-3.0.en.html
 *
 * Copyright (c) 2022. by Fabio Tardivo
 */

#include "global_constraints_gpu/all_different.cuh"

#include <cstring>

#include "gfl/Allocators.hpp"
#include "gfl/DebugUtils.hpp"
#include "gfl/MathUtils.hpp"
#include "gfl/ParallelUtils.hpp"

using namespace gfl;

AllDifferentGPU::~AllDifferentGPU()
{
    HostAllocator{}.deallocate(_graph_h, _matrixBytes);
    HostAllocator{}.deallocate(_scc_h, _nPadded * scast<i64>(sizeof(u32)));
    DeviceAllocator{}.deallocate(_reach_d, _matrixBytes);
    DeviceAllocator{}.deallocate(_reachT_d, _matrixBytes);
    DeviceAllocator{}.deallocate(_scratch_d, _matrixBytes);
    DeviceAllocator{}.deallocate(_scc_d, _nPadded * scast<i64>(sizeof(u32)));
    cudaGraphExecDestroy(_cuGraph);
    cudaStreamDestroy(_cuStream);
}

void AllDifferentGPU::allocate()
{
    // The kernels come in a 64-node form, a 128-node form, and a tiled form
    // that wants a multiple of 128
    u32 const nNodes = scast<u32>(_nNodes);
    _nPadded = nNodes <= 64 ? 64u : ceilDiv<u32>(nNodes, BlockSide) * BlockSide;
    _matrixBytes = scast<i64>(_nPadded) * _nPadded / 8;

    // Pinned, so the staging copies can overlap with the kernels
    _graph_h = HostAllocator{}.allocate<u32>(_matrixBytes / scast<i64>(sizeof(u32)));
    _scc_h = HostAllocator{}.allocate<u32>(_nPadded);
    _reach_d = DeviceAllocator{}.allocate<u32>(_matrixBytes / scast<i64>(sizeof(u32)));
    _reachT_d = DeviceAllocator{}.allocate<u32>(_matrixBytes / scast<i64>(sizeof(u32)));
    _scratch_d = DeviceAllocator{}.allocate<u32>(_matrixBytes / scast<i64>(sizeof(u32)));
    _scc_d = DeviceAllocator{}.allocate<u32>(_nPadded);
}

void AllDifferentGPU::post()
{
    init();
    allocate();
    CHECK_CUDA_ERROR(cudaStreamCreate(&_cuStream));
    recordGraph2scc();
    for (int i = 0; i < _nVar; i += 1)
        _x[i]->propagateOnDomainChange(this);
    propagate();
}

template<typename Row> void AllDifferentGPU::setEdge(u32 const from, u32 const to)
{
    setMatrixBit<Row>(_graph_h, _nPadded, from, to, true);
}

void AllDifferentGPU::domains2graph()
{
    std::memset(_graph_h, 0, asSize(_matrixBytes));

    // Same residual graph PGraph walks: every value a variable could still take
    // other than its matched one, every value pointing at what it is matched to
    // (or at the sink when it is free), and the sink pointing back at every
    // matched value.
    auto edge = [this](int const from, int const to) {
        if (_nPadded == 64)
            setEdge<u64>(scast<u32>(from), scast<u32>(to));
        else
            setEdge<u128>(scast<u32>(from), scast<u32>(to));
    };

    int const sink = _nNodes - 1;

    for (int i = 0; i < _nVar; i += 1) {
        int const ub = _x[i]->max();
        for (int v = _x[i]->min(); v <= ub; v += 1)
            if (v != _match[i] and _x[i]->containsBase(v))
                edge(i, valNode(v));
    }

    for (int v = _minVal; v <= _maxVal; v += 1) {
        int const matchedVar = _varFor[v - _minVal];
        edge(valNode(v), matchedVar == -1 ? sink : matchedVar);
        if (matchedVar != -1)
            edge(sink, valNode(v));
    }
}

void AllDifferentGPU::recordGraph2scc()
{
    // One capture, replayed at every propagation: the shape of the pipeline
    // only depends on _nPadded, and every buffer it names is already allocated
    _cuGraph = capture(_cuStream, [this]() { graph2scc(); });
}

void AllDifferentGPU::graph2scc()
{
    i64 const sccBytes = _nPadded * scast<i64>(sizeof(u32));
    CHECK_CUDA_ERROR(cudaMemcpyAsync(_reach_d, _graph_h, asSize(_matrixBytes), cudaMemcpyHostToDevice, _cuStream));

    if (_nPadded <= scast<u32>(BlockSide)) {
        // Small enough to close and label inside a single block
        if (_nPadded == 64)
            scc64<<<1, 64, 0, _cuStream>>>(_reach_d, _scc_d);
        else
            scc128<<<1, BlockSide, 0, _cuStream>>>(_reach_d, _scc_d);
    } else {
        u32 const tiles = _nPadded / BlockSide;
        dim3 const tileRow = dim3(tiles, 2);
        dim3 const allTiles = dim3(tiles, tiles);

        // Closure of the arranged matrix, one diagonal tile at a time
        arrangeKernel<<<allTiles, BlockSide, 0, _cuStream>>>(_nPadded, _reach_d, _scratch_d);
        for (u32 i = 0; i < _nPadded; i += BlockSide) {
            reachabilityArrangedKernel1<<<1, BlockSide, 0, _cuStream>>>(i, _nPadded, _scratch_d);
            reachabilityArrangedKernel2<<<tileRow, BlockSide, 0, _cuStream>>>(i, _nPadded, _scratch_d);
            reachabilityArrangedKernel3<<<allTiles, BlockSide, 0, _cuStream>>>(i, _nPadded, _scratch_d);
        }

        // Two nodes are in one component when each reaches the other, so the
        // components are the closure AND its transpose
        transposeArrangedKernel<<<allTiles, BlockSide, 0, _cuStream>>>(_nPadded, _scratch_d, _reachT_d);
        bitwiseAndKernel<<<allTiles, BlockSide, 0, _cuStream>>>(_nPadded, _scratch_d, _reachT_d, _reach_d);
        initSccKernel<<<tiles, BlockSide, 0, _cuStream>>>(_nPadded, _scc_d);
        findSccArrangedKernel<<<tiles, BlockSide, 0, _cuStream>>>(_nPadded, _reach_d, _scc_d);
    }

    CHECK_CUDA_ERROR(cudaMemcpyAsync(_scc_h, _scc_d, asSize(sccBytes), cudaMemcpyDeviceToHost, _cuStream));
}

void AllDifferentGPU::propagate()
{
    if (calcMatch() < _nVar)
        failNow();
    calcVarFor();

    domains2graph();
    CHECK_CUDA_ERROR(cudaGraphLaunch(_cuGraph, _cuStream));
    CHECK_CUDA_ERROR(cudaStreamSynchronize(_cuStream));

    // The labels are node indices rather than the CPU's component counter, but
    // filterDomains only ever compares two of them for equality
    for (int n = 0; n < _nNodes; n += 1)
        _scc[n] = scast<int>(_scc_h[n]);
    filterDomains();
}
