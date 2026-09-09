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

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

#include "gfl/Allocators.hpp"
#include "gfl/DebugUtils.hpp"
#include "gfl/MathUtils.hpp"
#include "gfl/ParallelUtils.hpp"
#include "profiling.hpp"

using namespace gfl;

AllDifferentGPU::~AllDifferentGPU()
{
    HeapAllocator{}.deallocate(_dumpBuf, _wNVal / 8);
    HeapAllocator{}.deallocate(_sccLocal, _nPadded * scast<i64>(sizeof(u32)));
    HostAllocator{}.deallocate(_graph_h, _matrixBytes);
    HostAllocator{}.deallocate(_scc_h, _nPadded * scast<i64>(sizeof(u32)));
    DeviceAllocator{}.deallocate(_reach_d, _matrixBytes);
    DeviceAllocator{}.deallocate(_reachT_d, _matrixBytes);
    DeviceAllocator{}.deallocate(_scratch_d, _matrixBytes);
    DeviceAllocator{}.deallocate(_scc_d, _nPadded * scast<i64>(sizeof(u32)));
    cudaGraphExecDestroy(_cuGraph);
    cudaStreamDestroy(_cuStream);
}

void AllDifferentGPU::windowBounds()
{
    // Snap the value range out to whole 32-bit words. The window is fixed from
    // here on: domains only ever shrink, so it stays a superset of every live
    // range, and the node numbering it defines never has to move.
    i32 lo = highest<i32>(), hi = lowest<i32>();
    for (int i = 0; i < _nVar; i += 1) {
        lo = std::min(lo, _x[i]->min());
        hi = std::max(hi, _x[i]->max());
    }
    // Arithmetic shift, so a bound below zero still rounds down rather than
    // toward it -- gfl's roundDown wants a non-negative argument, and a view
    // like q[i] - i reaches well into the negatives
    _wMinVal = (lo >> 5) << 5;
    _wNVal = roundUp<i32>(hi - _wMinVal + 1, 32);
    _wMaxVal = _wMinVal + _wNVal - 1;
    _wNNodes = _wNVal + _nVar + 1;
}

void AllDifferentGPU::allocate()
{
    // The kernels come in a 64-node form, a 128-node form, and a tiled form
    // that wants a multiple of 128
    u32 const nNodes = scast<u32>(_wNNodes);
    _nPadded = nNodes <= 64 ? 64u : ceilDiv<u32>(nNodes, BlockSide) * BlockSide;
    _matrixBytes = scast<i64>(_nPadded) * _nPadded / 8;

    _dumpBuf = HeapAllocator{}.allocate<u32>(_wNVal / 32);
    _sccLocal = HeapAllocator{}.allocate<u32>(_nPadded);

    // Pinned, so the staging copies can overlap with the kernels
    _graph_h = HostAllocator{}.allocate<u32>(_matrixBytes / scast<i64>(sizeof(u32)));
    _scc_h = HostAllocator{}.allocate<u32>(_nPadded);
    _reach_d = DeviceAllocator{}.allocate<u32>(_matrixBytes / scast<i64>(sizeof(u32)));
    _reachT_d = DeviceAllocator{}.allocate<u32>(_matrixBytes / scast<i64>(sizeof(u32)));
    _scratch_d = DeviceAllocator{}.allocate<u32>(_matrixBytes / scast<i64>(sizeof(u32)));
    _scc_d = DeviceAllocator{}.allocate<u32>(_nPadded);
}

// How dense the residual graph has to be before the device is worth the trip.
// Tarjan's algorithm costs O(V + E), while the device pipeline is a chain of
// 3 * V / 128 dependent kernels whose cost barely moves with E, so the two
// curves cross at some edge count -- one that depends on the card, which is why
// it is left settable. The default is the figure the constraint's authors were
// measuring against.
static gfl::i64 minEdgesForDevice()
{
    static gfl::i64 const threshold = [] {
        char const * const env = std::getenv("MINICPP_ADGPU_MIN_EDGES");
        return env != nullptr ? std::strtoll(env, nullptr, 10) : scast<i64>(256 * 256);
    }();
    return threshold;
}

void AllDifferentGPU::post()
{
    init();
    windowBounds();
    allocate();
    if (profiling()) {
        std::string const tag = " (" + std::to_string(_nPadded) + " padded)";
        _tagMatch = "alldifferent gpu match" + tag;
        _tagBuild = "alldifferent gpu build graph" + tag;
        _tagDevice = "alldifferent gpu device" + tag;
        _tagFilter = "alldifferent gpu filter" + tag;
        _tagHost = "alldifferent gpu scc on host" + tag;
    }
    _minEdges = minEdgesForDevice();
    CHECK_CUDA_ERROR(cudaStreamCreate(&_cuStream));
    recordGraph2scc();
    for (int i = 0; i < _nVar; i += 1)
        _x[i]->propagateOnDomainChange(this);
    propagate();
}

void AllDifferentGPU::setEdge(u32 const from, u32 const to, bool const value)
{
    if (_nPadded == 64)
        setMatrixBit<u64>(_graph_h, _nPadded, from, to, value);
    else
        setMatrixBit<u128>(_graph_h, _nPadded, from, to, value);
}

void AllDifferentGPU::domains2graph()
{
    std::memset(_graph_h, 0, asSize(_matrixBytes));

    // A row is one u64 or u128 word per 64 or 128 columns, read most
    // significant bit first, so on a little endian host the 32-bit halves of a
    // row word sit in reverse order: the dump's word w belongs in the slot
    // that many places from the end of its group.
    u32 const wordsPerRow = _nPadded / 32;
    u32 const groupWords = _nPadded == 64 ? 2u : 4u;
    u32 const sink = scast<u32>(_wNVal + _nVar);

    // Same residual graph PGraph walks, built the other way round: every
    // variable points at its whole domain and every value at the sink, then the
    // matched edges are turned around one at a time.
    for (int i = 0; i < _nVar; i += 1) {
        _x[i]->dump(_wMinVal, _wMaxVal, _dumpBuf);
        u32 * const row = _graph_h + scast<i64>(_wNVal + i) * wordsPerRow;
        u32 const from = scast<u32>(_x[i]->min() - _wMinVal) / 32;
        u32 const to = scast<u32>(_x[i]->max() - _wMinVal) / 32;
        for (u32 w = from; w <= to; w += 1)
            row[(w / groupWords) * groupWords + (groupWords - 1 - w % groupWords)] = _dumpBuf[w];
    }

    for (i32 v = _wMinVal; v <= _wMaxVal; v += 1)
        setEdge(scast<u32>(v - _wMinVal), sink, true);

    for (int i = 0; i < _nVar; i += 1) {
        u32 const valNode = scast<u32>(_match[i] - _wMinVal);
        u32 const varNode = scast<u32>(_wNVal + i);
        setEdge(varNode, valNode, false);
        setEdge(valNode, varNode, true);
        setEdge(valNode, sink, false);
        setEdge(sink, valNode, true);
    }
}

void AllDifferentGPU::recordGraph2scc()
{
    // One capture, replayed at every propagation: the shape of the pipeline
    // only depends on _nPadded, and every buffer it names is already allocated
    _cuGraph = capture(_cuStream, [this]() { graph2scc(); });

    // Upload once, up front. A launch has to get the graph onto the device
    // first, and a model that posts several of these leaves each one launching
    // a different graph every time, which is the case where a launch is most
    // likely to pay that cost again
    CHECK_CUDA_ERROR(cudaGraphUpload(_cuGraph, _cuStream));
    CHECK_CUDA_ERROR(cudaStreamSynchronize(_cuStream));
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
    {
        Phase const p(_tagMatch);
        if (calcMatch() < _nVar)
            failNow();
    }
    // A sparse graph is cheaper to label here than to send anywhere
    i64 nEdges = 0;
    for (int i = 0; i < _nVar; i += 1)
        nEdges += _x[i]->size();
    if (nEdges < _minEdges) {
        {
            Phase const p(_tagHost);
            calcSCC();
        }
        // A single component covers every edge, so nothing can be filtered
        if (_nSCC > 1) {
            Phase const p(_tagFilter);
            filterDomains();
        }
        return;
    }

    {
        Phase const p(_tagBuild);
        domains2graph();
    }
    {
        Phase const p(_tagDevice);
        CHECK_CUDA_ERROR(cudaGraphLaunch(_cuGraph, _cuStream));
        CHECK_CUDA_ERROR(cudaStreamSynchronize(_cuStream));
    }
    Phase const p(_tagFilter);

    // One sequential read of the staging page rather than a scattered one: the
    // device has just written it over PCIe, so every line of it is cold, and
    // the filtering below touches it twice per value
    std::memcpy(_sccLocal, _scc_h, asSize(_nPadded * scast<i64>(sizeof(u32))));

    // Same filtering as the base class -- drop every value whose edge stays
    // inside no component -- but read against this class's own node numbering
    // rather than copying the labels over into the base class's
    for (int i = 0; i < _nVar; i += 1) {
        u32 const varScc = _sccLocal[_wNVal + i];
        int const ub = _x[i]->max();
        for (int v = _x[i]->min(); v <= ub; v += 1)
            if (_match[i] != v and varScc != _sccLocal[v - _wMinVal] and _x[i]->containsBase(v))
                _x[i]->remove(v);
    }
}
