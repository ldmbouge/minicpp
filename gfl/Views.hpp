#pragma once

#include <cassert>
#include <type_traits>
#include "Types.hpp"
#include "FunQual.hpp"
#ifdef __CUDACC__
#include "Memory.hpp"
#endif

namespace gfl {

template<typename T, typename Ptr>
class VectorView;

template<typename T, typename Ptr = T*>
class ArrayView {
    static_assert(std::is_trivially_copyable_v<T>);
#ifdef __CUDACC__
    static_assert(std::is_same_v<Ptr, T*> or std::is_same_v<Ptr, MirrorPtr<T>>);
#endif

    friend class VectorView<T,Ptr>;

protected:
    Ptr _data;
    i64 _size;

public:
    ArrayView() = delete;
    GFL_HOST_DEVICE ArrayView(Ptr const data, i64 const size) noexcept : _data(data), _size(size) {
        assert(size >= 0);
#ifdef __CUDACC__
        if constexpr (std::is_same_v<Ptr, MirrorPtr<T>>)
            assert(data.valid());
        else
#endif
            assert(data != nullptr);
    }
    GFL_HOST_DEVICE T const* data() const noexcept {
#ifdef __CUDACC__
        if constexpr (std::is_same_v<Ptr, MirrorPtr<T>>)
            return _data.get();
        else
#endif
            return _data;
    }
    GFL_HOST_DEVICE T* data() noexcept { return asMut(asConst(this)->data()); }
    GFL_HOST_DEVICE i64 size() const noexcept { return _size; }
    GFL_HOST_DEVICE bool empty() const noexcept { return _size == 0; }
    GFL_HOST_DEVICE T const* begin() const noexcept { return data(); }
    GFL_HOST_DEVICE T* begin() noexcept { return data(); }
    GFL_HOST_DEVICE T const* end() const noexcept { return data() + _size; }
    GFL_HOST_DEVICE T* end() noexcept { return data() + _size; }
    GFL_HOST_DEVICE T const& at(i64 const idx) const noexcept {
        assert(idx >= 0);
        assert(idx < _size);
        return data()[idx];
    }
    GFL_HOST_DEVICE T& at(i64 const idx) noexcept { return asMut(asConst(this)->at(idx)); }
    GFL_HOST_DEVICE T const& operator[](i64 const idx) const noexcept { return at(idx); }
    GFL_HOST_DEVICE T& operator[](i64 const idx) noexcept { return at(idx); }
    GFL_HOST_DEVICE T const& front() const noexcept {
        assert(_size > 0);
        return data()[0];
    }
    GFL_HOST_DEVICE T& front() noexcept { return asMut(asConst(this)->front()); }
    GFL_HOST_DEVICE T const& back() const noexcept {
        assert(_size > 0);
        return data()[_size - 1];
    }
    GFL_HOST_DEVICE T& back() noexcept { return asMut(asConst(this)->back()); }
    GFL_HOST_DEVICE ArrayView<T> slice(i64 const begin, i64 const end) const noexcept {
        assert(0 <= begin);
        assert(begin <= end);
        assert(end <= _size);
        return ArrayView<T>(asMut(data()) + begin, end - begin);
    }
    GFL_HOST_DEVICE ArrayView<T> slice(i64 const count) const noexcept {
        assert(count != 0);
        if (count >= 0)
            return slice(0, count);
        else
            return slice(_size + count, _size);
    }
};

template<typename T, typename Ptr = T*>
class VectorView {
    ArrayView<T,Ptr> _aView;
    i64 _capacity;

public:
    VectorView() = delete;
    GFL_HOST_DEVICE VectorView(Ptr const data, i64 const capacity) noexcept :
	 _aView(data, 0), _capacity(capacity) { assert(capacity > 0); }
    GFL_HOST_DEVICE T const* data() const noexcept { return _aView.data(); }
    GFL_HOST_DEVICE T* data() noexcept { return _aView.data(); }
    GFL_HOST_DEVICE i64 size() const noexcept { return _aView.size(); }
    GFL_HOST_DEVICE i64 capacity() const noexcept { return _capacity; }
    GFL_HOST_DEVICE bool empty() const noexcept { return _aView.empty(); }
    GFL_HOST_DEVICE bool full() const noexcept { return _aView.size() == _capacity; }
    GFL_HOST_DEVICE T const* begin() const noexcept { return _aView.begin(); }
    GFL_HOST_DEVICE T* begin() noexcept { return _aView.begin(); }
    GFL_HOST_DEVICE T const* end() const noexcept { return _aView.end(); }
    GFL_HOST_DEVICE T* end() noexcept { return _aView.end(); }
    GFL_HOST_DEVICE T const& at(i64 const idx) const noexcept { return _aView.at(idx); }
    GFL_HOST_DEVICE T& at(i64 const idx) noexcept { return _aView.at(idx); }
    GFL_HOST_DEVICE T const& operator[](i64 const idx) const noexcept { return _aView[idx]; }
    GFL_HOST_DEVICE T& operator[](i64 const idx) noexcept { return _aView[idx]; }
    GFL_HOST_DEVICE T const& front() const noexcept { return _aView.front(); }
    GFL_HOST_DEVICE T& front() noexcept { return _aView.front(); }
    GFL_HOST_DEVICE T const& back() const noexcept { return _aView.back(); }
    GFL_HOST_DEVICE T& back() noexcept { return _aView.back(); }
    GFL_HOST_DEVICE ArrayView<T> slice(i64 const begin, i64 const end) const noexcept { return _aView.slice(begin, end); }
    GFL_HOST_DEVICE ArrayView<T> slice(i64 const count) const noexcept { return _aView.slice(count); }
    GFL_HOST_DEVICE void resizeBy(i64 const delta) noexcept {
        i64 const newSize = _aView._size + delta;
        assert(0 <= newSize);
        assert(newSize <= _capacity);
        _aView._size = newSize;
    }
#ifdef __CUDACC__
    GFL_DEVICE i64 resizeByAtomic(i64 const delta) noexcept {
        static_assert(sizeof(i64) == sizeof(llu));
        i64 const oldSize = scast<i64>(atomicAdd(rcast<llu*>(&_aView._size), scast<llu>(delta)));
        assert(0 <= oldSize + delta);
        assert(oldSize + delta <= _capacity);
        return oldSize;
    }
    GFL_DEVICE i64 resizeByAtomicBlock(i64 const delta) noexcept {
        static_assert(sizeof(i64) == sizeof(llu));
        i64 const oldSize = scast<i64>(atomicAdd_block(rcast<llu*>(&_aView._size), scast<llu>(delta)));
        assert(0 <= oldSize + delta);
        assert(oldSize + delta <= _capacity);
        return oldSize;
    }
#endif
    GFL_HOST_DEVICE void resizeTo(i64 const newSize) noexcept { resizeBy(newSize - size()); }
    GFL_HOST_DEVICE void clear() noexcept { resizeTo(0); }
    void append(T const& value) noexcept {
        i64 const oldSize = _aView._size;
        resizeBy(1);
        data()[oldSize] = value;
    }
    void append(ArrayView<T> const& elements) noexcept {
        i64 const oldSize = _aView._size;
        resizeBy(elements.size());
        memcpy(data() + oldSize, elements.data(), scast<usize>(elements.size()) * sizeof(T));
    }
};
}