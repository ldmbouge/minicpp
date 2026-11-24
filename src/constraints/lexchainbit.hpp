// LexChainBit: symmetry breaking for item-to-bin assignments.
// Each row is an item whose value is its bin, i.e. a row of a 0/1 matrix with a single 1.
// The columns (bins) are lexicographically decreasing: the first item in bin c comes before
// the first item in bin c+1, and an empty bin is followed only by empty bins.
// Propagates constraints (S4) of Hadj Salem and Kieffer: an item can be in a bin after c only if
// an earlier item is in bin c. They are also applied to the first item (forced into bin 0), as the
// model has no (S0) "bins are used in order" constraint.
// References:
// - An Experimental Study on Symmetry Breaking Constraints Impact for the One Dimensional
//   Bin-Packing Problem (DOI: 10.15439/2020F19)
// - CP for Bin Packing with Multi-Core and GPUs (DOI: 10.4230/LIPIcs.CP.2024.28)

#pragma once

#include <vector>
#include "core/intvar.hpp"
#include "core/acstr.hpp"

class LexChainBit : public Constraint {
   std::vector<var<int>::Ptr> _rows;
   int const _nRows;
   int const _nCols;
   std::vector<int> _firstRow;   // per column: first row that can take it (_nRows if none)
   std::vector<int> _rowLimit;   // per row: largest column it can take
public:
   explicit LexChainBit(std::vector<var<int>::Ptr> const& rows);
   void post() override;
   void propagate() override;
};
