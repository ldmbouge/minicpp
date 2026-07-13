#pragma once

#include "Views.hpp"
#include "Memory.hpp"

namespace gfl {
template<typename T>
class Vector {
    VectorView<T> _vView;
    Vector(T* const data, i64 const capacity) noexcept : _vView(data,capacity) { assert(capacity > 0); }
public:
    Vector() = delete;
    Vector(MemoryPool<HeapAllocator>& pool,  i64 const capacity) :
        Vector(pool.allocate<T>(capacity), capacity) {}
    T const* data() const noexcept { return _vView.data(); }
    T* data() noexcept { return _vView.data(); }
    i64 size() const noexcept { return _vView.size(); }
    i64 capacity() const noexcept { return _vView.capacity(); }
    bool empty() const noexcept { return _vView.empty(); }
    bool full() const noexcept { return _vView.full(); }
    T const* begin() const noexcept { return _vView.begin(); }
    T* begin() noexcept { return _vView.begin(); }
    T const* end() const noexcept { return _vView.end(); }
    T* end() noexcept { return _vView.end(); }
    T const& at(i64 const idx) const noexcept { return _vView.at(idx); }
    T& at(i64 const idx) noexcept { return _vView.at(idx); }
    T const& operator[](i64 const idx) const noexcept { return _vView[idx]; }
    T& operator[](i64 const idx) noexcept { return _vView[idx]; }
    T const& front() const noexcept { return _vView.front(); }
    T& front() noexcept { return _vView.front(); }
    T const& back() const noexcept { return _vView.back(); }
    T& back() noexcept { return _vView.back(); }
    ArrayView<T> slice(i64 const begin, i64 const end) const noexcept { return _vView.slice(begin, end); }
    ArrayView<T> slice(i64 const count) const noexcept { return _vView.slice(count); }
    void resizeBy(i64 const delta) noexcept { _vView.resizeBy(delta); }
    void resizeTo(i64 const size) noexcept { _vView.resizeTo(size); }
    void clear() noexcept { _vView.clear(); }
    void append(T const& value) noexcept { _vView.append(value); }
    void append(ArrayView<T> const& elements) noexcept { _vView.append(elements); }
};

#ifdef __CUDACC__
template<typename T>
class DeviceVector {
    VectorView<T> _vView;
    DeviceVector(T* const data, i64 const capacity) noexcept : _vView(data,capacity) { assert(capacity > 0); }
public:
    DeviceVector() = delete;
    DeviceVector(MemoryPool<DeviceAllocator>& pool,  i64 const capacity) :
        DeviceVector(pool.allocate<T>(capacity), capacity) {}
    GFL_HOST_DEVICE T const* data() const noexcept { return _vView.data(); }
    GFL_DEVICE T* data() noexcept { return _vView.data(); }
    GFL_HOST_DEVICE i64 size() const noexcept { return _vView.size(); }
    GFL_HOST_DEVICE i64 capacity() const noexcept { return _vView.capacity(); }
    GFL_HOST_DEVICE bool empty() const noexcept { return _vView.empty(); }
    GFL_HOST_DEVICE bool full() const noexcept { return _vView.full(); }
    GFL_DEVICE T const* begin() const noexcept { return _vView.begin(); }
    GFL_DEVICE T* begin() noexcept { return _vView.begin(); }
    GFL_DEVICE T const* end() const noexcept { return _vView.end(); }
    GFL_DEVICE T* end() noexcept { return _vView.end(); }
    GFL_DEVICE T const& at(i64 const idx) const noexcept { return _vView.at(idx); }
    GFL_DEVICE T& at(i64 const idx) noexcept { return _vView.at(idx); }
    GFL_DEVICE T const& operator[](i64 const idx) const noexcept { return _vView[idx]; }
    GFL_DEVICE T& operator[](i64 const idx) noexcept { return _vView[idx]; }
    GFL_DEVICE T const& front() const noexcept { return _vView.front(); }
    GFL_DEVICE T& front() noexcept { return _vView.front(); }
    GFL_DEVICE T const& back() const noexcept { return _vView.back(); }
    GFL_DEVICE T& back() noexcept { return _vView.back(); }
    GFL_DEVICE ArrayView<T> slice(i64 const begin, i64 const end) const noexcept { return _vView.slice(begin, end); }
    GFL_DEVICE ArrayView<T> slice(i64 const count) const noexcept { return _vView.slice(count); }
    GFL_DEVICE void resizeBy(i64 const delta) noexcept { _vView.resizeBy(delta); }
    GFL_DEVICE i64 resizeByAtomic(i64 const delta) noexcept { return _vView.resizeByAtomic(delta); }
    GFL_DEVICE i64 resizeByAtomicBlock(i64 const delta) noexcept { return _vView.resizeByAtomicBlock(delta); }
    GFL_DEVICE void resizeTo(i64 const size) noexcept { _vView.resizeTo(size); }
    GFL_HOST_DEVICE void clear() noexcept { _vView.clear(); }
    void copyToDeviceAsync(cudaStream_t const stream, ArrayView<T> const & elements) noexcept {
        assert(elements.size() <= size());
        cudaError_t const status = cudaMemcpyAsync(data(), elements.data(), sizeof(T) * elements.size(), cudaMemcpyHostToDevice, stream);
        checkOrAbort(status == cudaSuccess, "MirrorValue::copyToDevice: cudaMemcpyAsync failed");
    }
    void copyToHostAsync(cudaStream_t const stream, ArrayView<T> const & elements) noexcept {
        assert(size() <= elements.size());
        cudaError_t const status = cudaMemcpyAsync(elements.data(),data(), sizeof(T) * elements.size(), cudaMemcpyDeviceToHost, stream);
        checkOrAbort(status == cudaSuccess, "MirrorValue::copyToHost: cudaMemcpyAsync failed");
    }

};

template<typename T>
class MirrorVector {
    VectorView<T,MirrorPtr<T>> _vView;
    MirrorVector(MirrorPtr<T> const data, i64 const capacity) noexcept : _vView(data, capacity) { assert(capacity > 0); }
    MirrorVector(T* hostData, T* deviceData, i64 const capacity) noexcept : _vView({hostData,deviceData}, capacity) { assert(capacity > 0); }
public:
    MirrorVector() = delete;
    MirrorVector(MirrorPool& pool,  i64 const capacity) :
        MirrorVector(pool.allocate<T>(capacity), capacity) {}
    MirrorVector(MemoryPool<HostAllocator>& hPool, MemoryPool<HostAllocator>& dPool, i64 const capacity) :
        MirrorVector(hPool.allocate<T>(capacity), dPool.allocate<T>(capacity), capacity) {}
    GFL_HOST_DEVICE T const* data() const noexcept { return _vView.data(); }
    GFL_HOST_DEVICE T* data() noexcept { return _vView.data(); }
    GFL_HOST_DEVICE i64 size() const noexcept { return _vView.size(); }
    GFL_HOST_DEVICE i64 capacity() const noexcept { return _vView.capacity(); }
    GFL_HOST_DEVICE bool empty() const noexcept { return _vView.empty(); }
    GFL_HOST_DEVICE bool full() const noexcept { return _vView.full(); }
    GFL_HOST_DEVICE T const* begin() const noexcept { return _vView.begin(); }
    GFL_HOST_DEVICE T* begin() noexcept { return _vView.begin(); }
    GFL_HOST_DEVICE T const* end() const noexcept { return _vView.end(); }
    GFL_HOST_DEVICE T* end() noexcept { return _vView.end(); }
    GFL_HOST_DEVICE T const& at(i64 const idx) const noexcept { return _vView.at(idx); }
    GFL_HOST_DEVICE T& at(i64 const idx) noexcept { return _vView.at(idx); }
    GFL_HOST_DEVICE T const& operator[](i64 const idx) const noexcept { return _vView[idx]; }
    GFL_HOST_DEVICE T& operator[](i64 const idx) noexcept { return _vView[idx]; }
    GFL_HOST_DEVICE T const& front() const noexcept { return _vView.front(); }
    GFL_HOST_DEVICE T& front() noexcept { return _vView.front(); }
    GFL_HOST_DEVICE T const& back() const noexcept { return _vView.back(); }
    GFL_HOST_DEVICE T& back() noexcept { return _vView.back(); }
    GFL_HOST_DEVICE ArrayView<T> slice(i64 const begin, i64 const end) const noexcept { return _vView.slice(begin, end); }
    GFL_HOST_DEVICE ArrayView<T> slice(i64 const count) const noexcept { return _vView.slice(count); }
    GFL_HOST_DEVICE void resizeBy(i64 const delta) noexcept { _vView.resizeBy(delta); }
    GFL_DEVICE i64 resizeByAtomic(i64 const delta) noexcept { return _vView.resizeByAtomic(delta); }
    GFL_DEVICE i64 resizeByAtomicBlock(i64 const delta) noexcept { return _vView.resizeByAtomicBlock(delta); }
    GFL_HOST_DEVICE void resizeTo(i64 const size) noexcept { _vView.resizeTo(size); }
    GFL_HOST_DEVICE void clear() noexcept { _vView.clear(); }
    void copyToDeviceAsync(cudaStream_t const stream) noexcept {
        cudaError_t const status = cudaMemcpyAsync(_vView.mirrorData().d(), _vView.mirrorData().h(), sizeof(T) * size(), cudaMemcpyHostToDevice, stream);
        checkOrAbort(status == cudaSuccess, "MirrorVector::copyToDeviceAsync: cudaMemcpyAsync failed");
    }
    void copyToHostAsync(cudaStream_t const stream, ArrayView<T> const & elements) noexcept {
        cudaError_t const status = cudaMemcpyAsync(_vView.mirrorData().h(), _vView.mirrorData().d(), sizeof(T) * size(), cudaMemcpyDeviceToHost, stream);
        checkOrAbort(status == cudaSuccess, "MirrorVector::copyToHostAsync: cudaMemcpyAsync failed");
    }
};
#endif
}