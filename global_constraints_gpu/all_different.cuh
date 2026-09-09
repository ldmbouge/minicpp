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

#pragma once

#include "constraint.hpp"
#include "intvar.hpp"

#include "gfl/Alias.hpp"
#include "gfl/SqrBitMatrix.cuh"

/**
 * @brief GPU counterpart of AllDifferentAC.
 *
 * Only the strongly-connected-components phase is offloaded: the matching and
 * the filtering stay on the CPU, in the base class. The residual graph goes to
 * the GPU as a bit matrix -- one bit per (node, node) pair, in the node
 * numbering AllDifferentAC documents -- where the SCCs come out of a blocked
 * transitive closure (see gfl/SqrBitMatrix.cuh) instead of Tarjan's algorithm.
 * That trades Tarjan's O(V + E) for O(V^3 / 128) bit operations, which pays off
 * once the graph is dense enough for the width to beat the asymptotics.
 *
 * The node count is padded up to what the tiled kernels want (64, 128, or a
 * multiple of 128); the padding nodes have no edges, so each is its own
 * component and none of them is ever consulted by the filtering.
 */
class AllDifferentGPU : public AllDifferentAC
{
    private:
        gfl::u32 _nPadded;      // Node count rounded up for the kernels
        gfl::i64 _matrixBytes;  // _nPadded * _nPadded bits

        gfl::u32 * _graph_h;    // Residual graph, staged in pinned host memory
        gfl::u32 * _reach_d;    // Closure, then closure AND its transpose
        gfl::u32 * _reachT_d;   // Scratch: arranged closure, then its transpose
        gfl::u32 * _scratch_d;  // Scratch: the arranged layout
        gfl::u32 * _scc_h;      // Component label per node, back from the GPU
        gfl::u32 * _scc_d;

        cudaStream_t _cuStream;
        cudaGraphExec_t _cuGraph; // The captured graph-to-SCC pipeline

    public:
        template<class Vec> AllDifferentGPU(Vec const & x) : AllDifferentAC(x) {}
        ~AllDifferentGPU() override;

        void post() override;
        void propagate() override;

    private:
        void allocate();
        void domains2graph();
        void recordGraph2scc();
        void graph2scc();
        template<typename Row> void setEdge(gfl::u32 from, gfl::u32 to);
};
