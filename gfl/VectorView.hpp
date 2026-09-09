#pragma once

#include <cassert>
#include <cstring>
#include <span>
#include <type_traits>

#include "ArrayView.hpp"

namespace gfl {

template<typename T, typename Ptr = T*> class VectorView {
    Ptr _data{nullptr};
    i64 _size{0};
    i64 _capacity{0};

public:
    using value_type = T;
    VectorView() = default;
    GFL_HOST_DEVICE VectorView(Ptr const data, i64 const capacity) noexcept : _data(data), _size(0), _capacity(capacity) {
        assert(capacity >= 0);
        assert(data != nullptr);
    }
    template<typename Pool> VectorView(Pool& pool, i64 const capacity) noexcept : VectorView(pool.template allocate<T>(capacity), capacity) {}
    GFL_HOST_DEVICE ArrayView<T, Ptr> view() const noexcept { return {_data, _size}; }
    GFL_HOST_DEVICE operator ArrayView<T, Ptr>() const noexcept { return view(); }
    GFL_HOST_DEVICE T const* data() const noexcept { return _data; }
    GFL_HOST_DEVICE T* data() noexcept { return _data; }
    GFL_HOST_DEVICE T const* begin() const noexcept { return _data; }
    GFL_HOST_DEVICE T* begin() noexcept { return _data; }
    GFL_HOST_DEVICE T const* end() const noexcept { return _data + _size; }
    GFL_HOST_DEVICE T* end() noexcept { return _data + _size; }
    GFL_HOST_DEVICE i64 size() const noexcept { return _size; }
    GFL_HOST_DEVICE i64 capacity() const noexcept { return _capacity; }
    GFL_HOST_DEVICE bool empty() const noexcept { return _size == 0; }
    GFL_HOST_DEVICE bool full() const noexcept { return _size == _capacity; }
    GFL_HOST_DEVICE i64 sizeBytes() const noexcept { return as<i64>(sizeof(T)) * _size; }
    GFL_HOST_DEVICE T const& at(i64 const i) const noexcept { return view().at(i); }
    GFL_HOST_DEVICE T& at(i64 const i) noexcept { return view().at(i); }
    GFL_HOST_DEVICE T const& operator[](i64 const i) const noexcept { return at(i); }
    GFL_HOST_DEVICE T& operator[](i64 const i) noexcept { return at(i); }
    GFL_HOST_DEVICE T const& front() const noexcept { return view().front(); }
    GFL_HOST_DEVICE T& front() noexcept { return at(0); }
    GFL_HOST_DEVICE T const& back() const noexcept { return view().back(); }
    GFL_HOST_DEVICE T& back() noexcept { return view().back(); }
    GFL_HOST_DEVICE ArrayView<T, Ptr> slice(i64 const begin, i64 const end) const noexcept {
        if (begin >= end) return {_data + _capacity, i64{0}};
        return view().slice(begin, end);
    }
    GFL_HOST_DEVICE i64 resizeTo(i64 const size) noexcept {
        assert(0 <= size && size <= _capacity);
        auto const oldSize = _size;
        _size = size;
        return oldSize;
    }
    GFL_HOST_DEVICE i64 resizeBy(i64 const delta) noexcept { return resizeTo(_size + delta); }
    GFL_HOST_DEVICE void clear() noexcept { _size = 0; }
    GFL_HOST_DEVICE i64 resizeByAtomic(i64 const delta) noexcept {
        static_assert(sizeof(i64) == sizeof(llu));
#ifdef __CUDA_ARCH__
        i64 const oldSize = as<i64>(atomicAdd(rcast<llu*>(&_size), as<llu>(delta)));
#else
        i64 const oldSize = __atomic_fetch_add(&_size, delta, __ATOMIC_RELAXED);
#endif
        assert(0 <= oldSize + delta);
        assert(oldSize + delta <= _capacity);
        return oldSize;
    }
    // Block-scoped atomics are a CUDA intrinsic with no host counterpart, and
    // GFL_DEVICE is empty off-device — so the host compiler would otherwise
    // parse a body naming a function it has never heard of.
#ifdef __CUDACC__
    GFL_DEVICE i64 resizeByAtomicBlock(i64 const delta) noexcept {
        static_assert(sizeof(i64) == sizeof(llu));
        i64 const oldSize = as<i64>(atomicAdd_block(rcast<llu*>(&_size), as<llu>(delta)));
        assert(0 <= oldSize + delta);
        assert(oldSize + delta <= _capacity);
        return oldSize;
    }
#endif
    GFL_HOST_DEVICE void swap(VectorView& o) noexcept {
        VectorView const tmp = *this;
        *this = o;
        o = tmp;
    }
    GFL_HOST_DEVICE void pushBack(T const& value) noexcept {
        i64 const oldSize = _size;
        resizeTo(oldSize + 1);
        _data[oldSize] = value;
    }
    void pushBack(std::span<T const> const src) noexcept {
        if (src.empty()) return;
        i64 const oldSize = _size;
        resizeTo(oldSize + as<i64>(src.size()));
        std::memcpy(_data + oldSize, src.data(), sizeof(T) * src.size());
    }
    GFL_HOST_DEVICE void pushBackAtomic(T const& value) noexcept { _data[resizeByAtomic(1)] = value; }
    // Claims room for the whole batch in one atomic, so a thread's elements
    // land contiguously instead of interleaved with those of other threads
    GFL_HOST_DEVICE void pushBackAtomic(ArrayView<T, Ptr> const src) noexcept {
        if (src.size() == 0) return;
        i64 const at = resizeByAtomic(src.size());
        for (i64 i = 0; i < src.size(); ++i) _data[at + i] = src[i];
    }
#ifdef __CUDACC__
    GFL_DEVICE void pushBackAtomicBlock(T const& value) noexcept { _data[resizeByAtomicBlock(1)] = value; }
#endif
    template<typename... Args> GFL_HOST_DEVICE T& emplaceBack(Args&&... args) noexcept {
        assert(_size < _capacity);
        T* const p = _data + _size;
        T* const result = new (p) T(gfl::forward<Args>(args)...);
        ++_size;
        return *result;
    }
    template<typename... Args> GFL_DEVICE T& emplaceBackAtomic(Args&&... args) noexcept {
        i64 const i = resizeByAtomic(1);
        T* const result = new (_data + i) T(gfl::forward<Args>(args)...);
        return *result;
    }
#ifdef __CUDACC__
    template<typename... Args> GFL_DEVICE T& emplaceBackAtomicBlock(Args&&... args) noexcept {
        i64 const i = resizeByAtomicBlock(1);
        T* const result = new (_data + i) T(gfl::forward<Args>(args)...);
        return *result;
    }
#endif
    void fill(T const& value) noexcept { view().fill(value); }
    void loadFrom(std::span<T const> const src) noexcept {
        assert(as<i64>(src.size()) <= _capacity);
        resizeTo(as<i64>(src.size()));
        view().loadFrom(src);
    }
    void dumpTo(std::span<T> const dst) const noexcept { view().dumpTo(dst); }
    operator std::span<T>() noexcept requires std::is_same_v<Ptr, T*>
    {
        return view();
    }
    operator std::span<T const>() const noexcept requires std::is_same_v<Ptr, T*>
    {
        return view();
    }
    GFL_DEVICE void print() const noexcept { view().print(); }
};
#ifdef __CUDACC__
template<typename T> using MirrorVectorView = VectorView<T, MirrorPtr<T>>;
#endif
} // namespace gfl
