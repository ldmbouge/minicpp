#pragma once

#include <algorithm>
#include <cassert>
#include <type_traits>

#include "Alias.hpp"
#include "ArrayView.hpp"
#include "Mirror.hpp"

namespace gfl {
// A dense, row-major rows x cols matrix backed by ONE contiguous allocation
// (unlike an ArrayView<ArrayView<T>>, whose rows are separately allocated and
// so aren't guaranteed to sit anywhere near each other in memory).
// operator[] returns a row as an ArrayView<T>, so existing A[i][j] call sites
// work unchanged after switching a model from ArrayView<ArrayView<T>> to
// MatrixView<T>.
template<typename T, typename Ptr = T*> class MatrixView {
    static_assert(std::is_trivially_copyable_v<T>);
    Ptr _data{nullptr};
    i64 _rows{0};
    i64 _cols{0};

public:
    using value_type = T;
    using pointer = Ptr;
    MatrixView() noexcept = default;
    GFL_HOST_DEVICE MatrixView(Ptr const data, i64 const rows, i64 const cols) noexcept : _data(data), _rows(rows), _cols(cols) {
        assert(rows >= 0 && cols >= 0);
        assert(data != nullptr || rows * cols == 0);
    }
    template<typename Pool> MatrixView(Pool& pool, i64 const rows, i64 const cols) noexcept : MatrixView(pool.template allocate<T>(rows * cols), rows, cols) {}

    GFL_HOST_DEVICE i64 rows() const noexcept { return _rows; }
    GFL_HOST_DEVICE i64 cols() const noexcept { return _cols; }
    GFL_HOST_DEVICE i64 size() const noexcept { return _rows * _cols; }
    GFL_HOST_DEVICE T const* data() const noexcept { return _data; }
    GFL_HOST_DEVICE T* data() noexcept { return _data; }

    GFL_HOST_DEVICE T const& at(i64 const i, i64 const j) const noexcept {
        assert(0 <= i && i < _rows);
        assert(0 <= j && j < _cols);
        return _data[i * _cols + j];
    }
    GFL_HOST_DEVICE T& at(i64 const i, i64 const j) noexcept {
        assert(0 <= i && i < _rows);
        assert(0 <= j && j < _cols);
        return _data[i * _cols + j];
    }
    GFL_HOST_DEVICE T const& operator()(i64 const i, i64 const j) const noexcept { return at(i, j); }
    GFL_HOST_DEVICE T& operator()(i64 const i, i64 const j) noexcept { return at(i, j); }

    GFL_HOST_DEVICE ArrayView<T, Ptr> row(i64 const i) const noexcept {
        assert(0 <= i && i < _rows);
        return ArrayView<T, Ptr>(_data + i * _cols, _cols);
    }
    GFL_HOST_DEVICE ArrayView<T, Ptr> operator[](i64 const i) const noexcept { return row(i); }

    void fill(T const& value) noexcept { std::fill(_data, _data + size(), value); }
};
#ifdef __CUDACC__
template<typename T> using MirrorMatrixView = MatrixView<T, MirrorPtr<T>>;
#endif
}
