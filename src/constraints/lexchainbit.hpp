// LexChainBit: symmetry breaking for item-to-bin assignments.
// Bin j can be used by item i only if bin j-1 is already usable by an item before i,
// so bins are opened in order (an item never opens bin j while bin j-1 is still empty).
// Reference: CP for Bin Packing with Multi-Core and GPUs (DOI: 10.4230/LIPIcs.CP.2024.28)

#pragma once

#include <vector>
#include "core/intvar.hpp"
#include "core/acstr.hpp"

class LexChainBit : public Constraint {
   std::vector<var<int>::Ptr> _rows;
   int const _nRows;
   int const _nCols;
   std::vector<int> _colsUpperbound;
public:
   explicit LexChainBit(std::vector<var<int>::Ptr> const& rows);
   void post() override;
   void propagate() override;
};
