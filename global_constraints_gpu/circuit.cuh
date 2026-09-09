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

/**
 * @brief Circuit whose alldifferent runs on the GPU.
 *
 * Only the alldifferent changes: the successor/predecessor bookkeeping that
 * rules out subtours stays on the CPU, in the base class. The propagator it
 * posts filters exactly like CircuitAC's, so the two explore the same search
 * tree.
 * @see AllDifferentGPU
 */
class CircuitGPU : public Circuit
{
    public:
        // Low priority, matching AllDifferentGPU: the alldifferent it posts is
        // the expensive half, and deferring it lets the subtour bookkeeping
        // narrow the domains first
        template <class Vec> CircuitGPU(Vec const & x) : Circuit(x) { setPriority(CLOW);}
    protected:
        void postAllDifferent() override;
};
