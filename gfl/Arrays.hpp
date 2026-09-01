#pragma once

#include <algorithm>
#include <cstring>
#include "Memory.hpp"

namespace gfl {

template<typename T, typename Ptr>
class VectorView;

// ============================================================================
// ArrayView - non-owning view over a fixed-size region. Ptr is T* (plain) or
// MirrorPtr<T> (host+device mirror, CUDA only).
// ============================================================================
template<typename T, typename Ptr = T*>
class ArrayView {
    static_assert(std::is_trivially_copyable_v<T>);
#ifdef __CUDACC__
    static_assert(std::is_same_v<Ptr, T*> or std::is_same_v<Ptr, MirrorPtr<T>>);
#endif
    Ptr _data;
protected:
    i64 _size;
    friend class VectorView<T,Ptr>;
public:
    using value_type = T;
    // --- Construction ---
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

    // --- Data access ---
    GFL_HOST_DEVICE T const* data() const noexcept {
#ifdef __CUDACC__
        if constexpr (std::is_same_v<Ptr, MirrorPtr<T>>)
            return _data.get();
        else
#endif
            return _data;
    }
    GFL_HOST_DEVICE T* data() noexcept { return asMut(asConst(this)->data()); }
#ifdef __CUDACC__
    template<typename P = Ptr, std::enable_if_t<std::is_same_v<P, MirrorPtr<T>>, bool> = true>
    GFL_HOST_DEVICE MirrorPtr<T> mirrorData() const noexcept { return _data; }
#endif

    // --- Size ---
    GFL_HOST_DEVICE i64 size() const noexcept { return _size; }
    GFL_HOST_DEVICE bool empty() const noexcept { return _size == 0; }

    // --- Iterators ---
    GFL_HOST_DEVICE T const* begin() const noexcept { return data(); }
    GFL_HOST_DEVICE T* begin() noexcept { return data(); }
    GFL_HOST_DEVICE T const* end() const noexcept { return data() + _size; }
    GFL_HOST_DEVICE T* end() noexcept { return data() + _size; }

    // --- Element access ---
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

    // --- Views ---
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
    GFL_HOST_DEVICE ArrayView<T> view() const noexcept { return slice(0, size()); }

    // --- Bulk operations ---
    void fill(T const& value) noexcept { std::fill(begin(), end(), value); }
    void zero() noexcept { std::memset(data(), 0, sizeof(T) * scast<usize>(_size)); }
    template<typename Container>
    void loadFrom(Container const& c) noexcept {
        static_assert(std::is_same_v<typename Container::value_type, T>);
        assert(scast<i64>(c.size()) == size());
        std::memcpy(data(), c.data(), sizeof(T) * scast<usize>(size()));
    }
    template<typename Container>
    void dumpTo(Container& c) const noexcept {
        static_assert(std::is_same_v<typename Container::value_type, T>);
        assert(size() <= scast<i64>(c.size()));
        std::memcpy(c.data(), data(), sizeof(T) * scast<usize>(size()));
    }
};

// ============================================================================
// Array - host-only owning array (pool-allocated).
// ============================================================================
template<typename T>
class Array {
    ArrayView<T> _aView;
    Array(T* data, i64 const size) noexcept : _aView(data, size) { assert(size > 0); }
public:
    using value_type = T;

    // --- Construction ---
    Array() = delete;
    Array(HeapPool& pool, i64 const size) : Array(pool.allocate<T>(size), size) {}

    // --- Data access ---
    T const* data() const noexcept { return _aView.data(); }
    T* data() noexcept { return _aView.data(); }

    // --- Size ---
    i64 size() const noexcept { return _aView.size(); }

    // --- Iterators ---
    T const* begin() const noexcept { return _aView.begin(); }
    T* begin() noexcept { return _aView.begin(); }
    T const* end() const noexcept { return _aView.end(); }
    T* end() noexcept { return _aView.end(); }

    // --- Element access ---
    T const& at(i64 const idx) const noexcept { return _aView.at(idx); }
    T& at(i64 const idx) noexcept { return _aView.at(idx); }
    T const& operator[](i64 const idx) const noexcept { return _aView[idx]; }
    T& operator[](i64 const idx) noexcept { return _aView[idx]; }
    T const& front() const noexcept { return _aView.front(); }
    T& front() noexcept { return _aView.front(); }
    T const& back() const noexcept { return _aView.back(); }
    T& back() noexcept { return _aView.back(); }

    // --- Views ---
    ArrayView<T> slice(i64 const begin, i64 const end) const noexcept { return _aView.slice(begin, end); }
    ArrayView<T> slice(i64 const count) const noexcept { return _aView.slice(count); }
    ArrayView<T> view() const noexcept { return _aView.view(); }

    // --- Bulk operations ---
    void fill(T const& value) noexcept { _aView.fill(value); }
    void zero() noexcept { _aView.zero(); }
    template<typename Container>
    void loadFrom(Container const& c) noexcept { _aView.loadFrom(c); }
    template<typename Container>
    void dumpTo(Container& c) const noexcept { _aView.dumpTo(c); }
};

#ifdef __CUDACC__
// ============================================================================
// DeviceArray - device-resident array. Element access is device-only.
// Metadata is host-readable. Element transfers via copyTo*Async.
// ============================================================================
template<typename T>
class DeviceArray {
    ArrayView<T> _aView;
    DeviceArray(T* data, i64 const size) noexcept : _aView(data, size) { assert(size > 0); }
public:
    using value_type = T;

    // --- Construction ---
    DeviceArray() = delete;
    DeviceArray(DevicePool& dPool, i64 const size) : DeviceArray(dPool.allocate<T>(size), size) {}

    // --- Data access ---
    GFL_HOST_DEVICE T const* data() const noexcept { return _aView.data(); }
    GFL_DEVICE T* data() noexcept { return _aView.data(); }

    // --- Size ---
    GFL_HOST_DEVICE i64 size() const noexcept { return _aView.size(); }

    // --- Iterators (device only) ---
    GFL_DEVICE T const* begin() const noexcept { return _aView.begin(); }
    GFL_DEVICE T* begin() noexcept { return _aView.begin(); }
    GFL_DEVICE T const* end() const noexcept { return _aView.end(); }
    GFL_DEVICE T* end() noexcept { return _aView.end(); }

    // --- Element access (device only) ---
    GFL_DEVICE T const& at(i64 const idx) const noexcept { return _aView.at(idx); }
    GFL_DEVICE T& at(i64 const idx) noexcept { return _aView.at(idx); }
    GFL_DEVICE T const& operator[](i64 const idx) const noexcept { return _aView[idx]; }
    GFL_DEVICE T& operator[](i64 const idx) noexcept { return _aView[idx]; }
    GFL_DEVICE T const& front() const noexcept { return _aView.front(); }
    GFL_DEVICE T& front() noexcept { return _aView.front(); }
    GFL_DEVICE T const& back() const noexcept { return _aView.back(); }
    GFL_DEVICE T& back() noexcept { return _aView.back(); }

    // --- Views ---
    GFL_DEVICE ArrayView<T> slice(i64 const begin, i64 const end) const noexcept { return _aView.slice(begin, end); }
    GFL_DEVICE ArrayView<T> slice(i64 const count) const noexcept { return _aView.slice(count); }
    ArrayView<T> view() const noexcept { return _aView.view(); }

    // --- Transfer ---
    template<typename Container>
    void dumpToDeviceAsync(cudaStream_t const stream, Container const& c) noexcept {
        static_assert(std::is_same_v<typename Container::value_type, T>);
        assert(scast<i64>(c.size()) == size());
        cudaError_t const status = cudaMemcpyAsync(asMut(data()), c.data(), sizeof(T) * scast<usize>(size()), cudaMemcpyHostToDevice, stream);
        checkOrAbort(status == cudaSuccess, "DeviceArray::copyToDeviceAsync: cudaMemcpyAsync failed");
    }
    template<typename Container>
    void loadFromDeviceAsync(cudaStream_t const stream, Container& c) noexcept {
        static_assert(std::is_same_v<typename Container::value_type, T>);
        assert(size() == scast<i64>(c.size()));
        cudaError_t const status = cudaMemcpyAsync(c.data(), asMut(data()), sizeof(T) * scast<usize>(size()), cudaMemcpyDeviceToHost, stream);
        checkOrAbort(status == cudaSuccess, "DeviceArray::copyToHostAsync: cudaMemcpyAsync failed");
    }
};

// ============================================================================
// MirrorArray - host+device mirror array. Transfers explicitly.
// ============================================================================
template<typename T>
class MirrorArray {
    ArrayView<T,MirrorPtr<T>> _aView;
    MirrorArray(MirrorPtr<T> const data, i64 const size) noexcept : _aView(data, size) { assert(size > 0); }
    MirrorArray(T* hostData, T* deviceData, i64 const size) noexcept : _aView({hostData,deviceData}, size) { assert(size > 0); }
public:
    using value_type = T;

    // --- Construction ---
    MirrorArray() = delete;
    MirrorArray(MirrorPool& pool, i64 const size) : MirrorArray(pool.allocate<T>(size), size) {}
    MirrorArray(HostPool& hPool, DevicePool& dPool, i64 const size) :
        MirrorArray(hPool.allocate<T>(size), dPool.allocate<T>(size), size) {}

    // --- Data access ---
    GFL_HOST_DEVICE T const* data() const noexcept { return _aView.data(); }
    GFL_HOST_DEVICE T* data() noexcept { return _aView.data(); }

    // --- Size ---
    GFL_HOST_DEVICE i64 size() const noexcept { return _aView.size(); }

    // --- Iterators ---
    GFL_HOST_DEVICE T const* begin() const noexcept { return _aView.begin(); }
    GFL_HOST_DEVICE T* begin() noexcept { return _aView.begin(); }
    GFL_HOST_DEVICE T const* end() const noexcept { return _aView.end(); }
    GFL_HOST_DEVICE T* end() noexcept { return _aView.end(); }

    // --- Element access ---
    GFL_HOST_DEVICE T const& at(i64 const idx) const noexcept { return _aView.at(idx); }
    GFL_HOST_DEVICE T& at(i64 const idx) noexcept { return _aView.at(idx); }
    GFL_HOST_DEVICE T const& operator[](i64 const idx) const noexcept { return _aView[idx]; }
    GFL_HOST_DEVICE T& operator[](i64 const idx) noexcept { return _aView[idx]; }
    GFL_HOST_DEVICE T const& front() const noexcept { return _aView.front(); }
    GFL_HOST_DEVICE T& front() noexcept { return _aView.front(); }
    GFL_HOST_DEVICE T const& back() const noexcept { return _aView.back(); }
    GFL_HOST_DEVICE T& back() noexcept { return _aView.back(); }

    // --- Views ---
    GFL_HOST_DEVICE ArrayView<T> slice(i64 const begin, i64 const end) const noexcept { return _aView.slice(begin, end); }
    GFL_HOST_DEVICE ArrayView<T> slice(i64 const count) const noexcept { return _aView.slice(count); }
    GFL_HOST_DEVICE ArrayView<T> view() const noexcept { return _aView.view(); }

    // --- Host fill / load ---
    void fill(T const& value) noexcept { _aView.fill(value); }
    void zero() noexcept { _aView.zero(); }
    template<typename Container>
    void loadFrom(Container const& c) noexcept { _aView.loadFrom(c); }
    template<typename Container>
    void dumpTo(Container& c) const noexcept { _aView.dumpTo(c); }

    // --- Transfer ---
    void copyToDeviceAsync(cudaStream_t const stream) noexcept {
        cudaError_t const status = cudaMemcpyAsync(_aView.mirrorData().d(), _aView.mirrorData().h(), sizeof(T) * scast<usize>(size()), cudaMemcpyHostToDevice, stream);
        checkOrAbort(status == cudaSuccess, "MirrorArray::copyToDeviceAsync: cudaMemcpyAsync failed");
    }
    void copyToHostAsync(cudaStream_t const stream) noexcept {
        cudaError_t const status = cudaMemcpyAsync(_aView.mirrorData().h(), _aView.mirrorData().d(), sizeof(T) * scast<usize>(size()), cudaMemcpyDeviceToHost, stream);
        checkOrAbort(status == cudaSuccess, "MirrorArray::copyToHostAsync: cudaMemcpyAsync failed");
    }
};
#endif
}