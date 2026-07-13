#pragma once

#include "Views.hpp"
#include "Memory.hpp"

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
#ifdef __CUDACC__
    template<typename P = Ptr, std::enable_if_t<std::is_same_v<P, MirrorPtr<T>>, bool> = true>
    GFL_HOST_DEVICE MirrorPtr<T> mirrorData() const noexcept { return _data; }
#endif
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
    GFL_HOST_DEVICE ArrayView<T> slice() const noexcept { return slice(0, size()); }
};

template<typename T>
class Array {
    ArrayView<T> _aView;
    Array(T* data, i64 const size) noexcept : _aView(data, size) { assert(size > 0); }

public:
    Array() = delete;
    Array(HeapPool& pool, i64 const size) :
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
    DeviceArray(DevicePool& dPool,  i64 const size) :
       DeviceArray(dPool.allocate<T>(size), size) {}
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
class MirrorArray {
    ArrayView<T,MirrorPtr<T>> _aView;
    MirrorArray(MirrorPtr<T> const data, i64 const size) noexcept : _aView(data, size) { assert(size > 0); }
    MirrorArray(T* hostData, T* deviceData, i64 const size) noexcept : _aView({hostData,deviceData}, size) { assert(size > 0); }
public:
    MirrorArray() = delete;
    MirrorArray(MirrorPool& pool, i64 const size) :
       MirrorArray(pool.allocate<T>(size), size) {}
    MirrorArray(HostPool& hPool, DevicePool& dPool, i64 const size) :
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
    void copyToDeviceAsync(cudaStream_t const stream) noexcept {
        cudaError_t const status = cudaMemcpyAsync(_aView.mirrorData().d(), _aView.mirrorData().h(), sizeof(T) * size(), cudaMemcpyHostToDevice, stream);
        checkOrAbort(status == cudaSuccess, "MirrorArray::copyToDeviceAsync: cudaMemcpyAsync failed");
    }
    void copyToHostAsync(cudaStream_t const stream) noexcept {
        cudaError_t const status = cudaMemcpyAsync(_aView.mirrorData().h(), _aView.mirrorData().d(), sizeof(T) * size(), cudaMemcpyDeviceToHost, stream);
        checkOrAbort(status == cudaSuccess, "MirrorArray::copyToHostAsync: cudaMemcpyAsync failed");
    }
};
#endif
}