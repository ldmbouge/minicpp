#pragma once

#include "Views.hpp"
#include "Memory.hpp"

namespace gfl {
template<typename T>
class Array {
    ArrayView<T> _aView;
    Array(T* data, i64 const size) noexcept : _aView(data, size) { assert(size > 0); }

public:
    Array() = delete;
    Array(MemoryPool<HeapAllocator>& pool,  i64 const size) :
        Array(pool.allocate<T>(size), size) {}
    T const* data() const noexcept { return _aView.data(); }
    T* data() noexcept { return _aView.data(); }
    i64 size() const noexcept { return _aView.size(); }
    T const* begin() const noexcept { return _aView.begin(); }
    T* begin() noexcept { return _aView.begin(); }
    T const* end() const noexcept { return _aView.end(); }
    T* end() noexcept { return _aView.end(); }
    T const& at(i64 const idx) const noexcept { return _aView.at(idx); }
    T& at(i64 const idx) noexcept { return _aView.at(idx); }
    T const& operator[](i64 const idx) const noexcept { return _aView[idx]; }
    T& operator[](i64 const idx) noexcept { return _aView[idx]; }
    T const& front() const noexcept { return _aView.front(); }
    T& front() noexcept { return _aView.front(); }
    T const& back() const noexcept { return _aView.back(); }
    T& back() noexcept { return _aView.back(); }
    ArrayView<T> slice(i64 const begin, i64 const end) const noexcept { return _aView.slice(begin, end); }
    ArrayView<T> slice(i64 const count) const noexcept { return _aView.slice(count); }
};

#ifdef __CUDACC__
template<typename T>
class DeviceArray {
    ArrayView<T> _aView;
    DeviceArray(T* data, i64 const size) noexcept : _aView(data, size) { assert(size > 0); }
public:
    DeviceArray() = delete;
    DeviceArray(MemoryPool<DeviceAllocator>& pool,  i64 const size) :
       DeviceArray(pool.allocate<T>(size), size) {}
    GFL_HOST_DEVICE T const* data() const noexcept { return _aView.data(); }
    GFL_DEVICE T* data() noexcept { return _aView.data(); }
    GFL_HOST_DEVICE i64 size() const noexcept { return _aView.size(); }
    GFL_DEVICE T const* begin() const noexcept { return _aView.begin(); }
    GFL_DEVICE T* begin() noexcept { return _aView.begin(); }
    GFL_DEVICE T const* end() const noexcept { return _aView.end(); }
    GFL_DEVICE T* end() noexcept { return _aView.end(); }
    GFL_DEVICE T const& at(i64 const idx) const noexcept { return _aView.at(idx); }
    GFL_DEVICE T& at(i64 const idx) noexcept { return _aView.at(idx); }
    GFL_DEVICE T const& operator[](i64 const idx) const noexcept { return _aView[idx]; }
    GFL_DEVICE T& operator[](i64 const idx) noexcept { return _aView[idx]; }
    GFL_DEVICE T const& front() const noexcept { return _aView.front(); }
    GFL_DEVICE T& front() noexcept { return _aView.front(); }
    GFL_DEVICE T const& back() const noexcept { return _aView.back(); }
    GFL_DEVICE T& back() noexcept { return _aView.back(); }
    GFL_DEVICE ArrayView<T> slice(i64 const begin, i64 const end) const noexcept { return _aView.slice(begin, end); }
    GFL_DEVICE ArrayView<T> slice(i64 const count) const noexcept { return _aView.slice(count); }
};
template<typename T>
class MirrorArray {
    ArrayView<T,MirrorPtr<T>> _aView;
    MirrorArray(MirrorPtr<T> const data, i64 const size) noexcept : _aView(data, size) { assert(size > 0); }
    MirrorArray(T* hostData, T* deviceData, i64 const size) noexcept : _aView({hostData,deviceData}, size) { assert(size > 0); }
public:
    MirrorArray() = delete;
    MirrorArray(MirrorPool& pool, i64 const size) :
       MirrorArray(pool.allocate<T>(size), size) {}
    MirrorArray(MemoryPool<HostAllocator>& hPool, MemoryPool<HostAllocator>& dPool, i64 const size) :
        MirrorArray(hPool.allocate<T>(size), dPool.allocate<T>(size), size) {}
    GFL_HOST_DEVICE T const* data() const noexcept { return _aView.data(); }
    GFL_HOST_DEVICE T* data() noexcept { return _aView.data(); }
    GFL_HOST_DEVICE i64 size() const noexcept { return _aView.size(); }
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
};
#endif
}