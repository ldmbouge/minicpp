#pragma once

#include "Views.hpp"
#include "Memory.hpp"

namespace gfl {
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
#ifdef __CUDACC__
    template<typename P = Ptr, std::enable_if_t<std::is_same_v<P, MirrorPtr<T>>, bool> = true>
    GFL_HOST_DEVICE MirrorPtr<T> mirrorData() const noexcept { return _aView.mirrorData(); }
#endif
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
    GFL_HOST_DEVICE ArrayView<T> slice() const noexcept { return _aView.slice(); }
    GFL_HOST_DEVICE void resizeBy(i64 const delta) noexcept {
        i64 const newSize = _aView._size + delta;
        assert(0 <= newSize);
        assert(newSize <= _capacity);
        _aView._size = newSize;
    }
    GFL_HOST_DEVICE i64 resizeByAtomic(i64 const delta) noexcept {
#ifdef __CUDA_ARCH__
        static_assert(sizeof(i64) == sizeof(llu));
        i64 const oldSize = scast<i64>(atomicAdd(rcast<llu*>(&_aView._size), scast<llu>(delta)));
#else
        i64 const oldSize = __atomic_fetch_add(&_aView._size, delta, __ATOMIC_RELAXED);
#endif
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
    GFL_HOST_DEVICE void resizeTo(i64 const newSize) noexcept {
        assert(0 <= newSize);
        assert(newSize <= _capacity);
        _aView._size = newSize;
    }
    GFL_HOST_DEVICE void clear() noexcept { resizeTo(0); }
    GFL_HOST_DEVICE void append(T const& value) noexcept {
        i64 const oldSize = _aView._size;
        resizeBy(1);
        data()[oldSize] = value;
    }
    GFL_HOST_DEVICE void appendAtomic(T const& value) noexcept {
        i64 const oldSize = resizeByAtomic(1);
        data()[oldSize] = value;
    }
    GFL_HOST_DEVICE void appendAtomicBlock(T const& value) noexcept {
        i64 const oldSize = resizeByAtomic(1);
        data()[oldSize] = value;
    }
    GFL_HOST_DEVICE void append(ArrayView<T> const& elements) noexcept {
        if (not elements.empty()) {
            i64 const oldSize = _aView._size;
            resizeBy(elements.size());
            memcpy(data() + oldSize, elements.data(), scast<usize>(elements.size()) * sizeof(T));
        }
    }
    GFL_HOST_DEVICE void appendAtomic(ArrayView<T> const& elements) noexcept {
        if (not elements.empty()) {
            i64 const oldSize = resizeByAtomic(elements.size());
            memcpy(data() + oldSize, elements.data(), scast<usize>(elements.size()) * sizeof(T));
        }
    }
    GFL_DEVICE void appendAtomicBlock(ArrayView<T> const& elements) noexcept {
        if (not elements.empty()) {
            i64 const oldSize = resizeByAtomicBlock(elements.size());
            memcpy(data() + oldSize, elements.data(), scast<usize>(elements.size()) * sizeof(T));
        }
    }
};

template<typename T>
class Vector {
    VectorView<T> _vView;
    Vector(T* const data, i64 const capacity) noexcept : _vView(data,capacity) { assert(capacity > 0); }
public:
    Vector() = delete;
    Vector(HeapPool& pool,  i64 const capacity) :
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
    ArrayView<T> slice() const noexcept { return _vView.slice(); }
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
    DeviceVector(DevicePool& dPool,  i64 const capacity) :
        DeviceVector(dPool.allocate<T>(capacity), capacity) {}
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
    GFL_DEVICE ArrayView<T> slice() const noexcept { return _vView.slice(); }
    GFL_HOST_DEVICE void resizeBy(i64 const delta) noexcept { _vView.resizeBy(delta); }
    GFL_DEVICE i64 resizeByAtomic(i64 const delta) noexcept { return _vView.resizeByAtomic(delta); }
    GFL_DEVICE i64 resizeByAtomicBlock(i64 const delta) noexcept { return _vView.resizeByAtomicBlock(delta); }
    GFL_HOST_DEVICE void resizeTo(i64 const size) noexcept { _vView.resizeTo(size); }
    GFL_HOST_DEVICE void clear() noexcept { _vView.clear(); }
    GFL_DEVICE void append(T const& value) noexcept { _vView.append(value); }
    GFL_DEVICE void append(ArrayView<T> const& elements) noexcept { _vView.append(elements); }
    GFL_DEVICE void appendAtomic(T const& value) noexcept { _vView.appendAtomic(value); }
    GFL_DEVICE void appendAtomic(ArrayView<T> const& elements) noexcept { _vView.appendAtomic(elements); }
    GFL_DEVICE void appendAtomicBlock(ArrayView<T> const& elements) noexcept { _vView.appendAtomicBlock(elements); }
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
    MirrorVector(HostPool & hPool, DevicePool& dPool, i64 const capacity) :
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
    GFL_HOST_DEVICE ArrayView<T> slice() const noexcept { return _vView.slice(); }
    GFL_HOST_DEVICE void resizeBy(i64 const delta) noexcept { _vView.resizeBy(delta); }
    GFL_HOST_DEVICE i64 resizeByAtomic(i64 const delta) noexcept { return _vView.resizeByAtomic(delta); }
    GFL_DEVICE i64 resizeByAtomicBlock(i64 const delta) noexcept { return _vView.resizeByAtomicBlock(delta); }
    GFL_HOST_DEVICE void resizeTo(i64 const size) noexcept { _vView.resizeTo(size); }
    GFL_HOST_DEVICE void clear() noexcept { _vView.clear(); }
    GFL_HOST_DEVICE void append(T const& value) noexcept { _vView.append(value); }
    GFL_HOST_DEVICE void append(ArrayView<T> const& elements) noexcept { _vView.append(elements); }
    GFL_HOST_DEVICE void appendAtomic(T const& value) noexcept { _vView.appendAtomic(value); }
    GFL_HOST_DEVICE void appendAtomic(ArrayView<T> const& elements) noexcept { _vView.appendAtomic(elements); }
    GFL_DEVICE void appendAtomicBlock(ArrayView<T> const& elements) noexcept { _vView.appendAtomicBlock(elements); }

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