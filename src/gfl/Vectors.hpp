#pragma once

#include "Arrays.hpp"
#include "Memory.hpp"

namespace gfl {

// ============================================================================
// VectorView - non-owning growable view (size <= capacity) over a region.
// Ptr is T* (plain) or MirrorPtr<T> (host+device mirror, CUDA only).
// ============================================================================
template<typename T, typename Ptr = T*>
class VectorView {
    ArrayView<T,Ptr> _aView;
    i64 _capacity;
public:
    using value_type = T;

    // --- Construction (starts empty) ---
    VectorView() = delete;
    GFL_HOST_DEVICE VectorView(Ptr const data, i64 const capacity) noexcept
        : _aView(data, 0), _capacity(capacity) { assert(capacity > 0); }

    // --- Data access ---
    GFL_HOST_DEVICE T const* data() const noexcept { return _aView.data(); }
    GFL_HOST_DEVICE T* data() noexcept { return _aView.data(); }
#ifdef __CUDACC__
    template<typename P = Ptr, std::enable_if_t<std::is_same_v<P, MirrorPtr<T>>, bool> = true>
    GFL_HOST_DEVICE MirrorPtr<T> mirrorData() const noexcept { return _aView.mirrorData(); }
#endif

    // --- Size / capacity ---
    GFL_HOST_DEVICE i64 size() const noexcept { return _aView.size(); }
    GFL_HOST_DEVICE i64 capacity() const noexcept { return _capacity; }
    GFL_HOST_DEVICE bool empty() const noexcept { return _aView.empty(); }
    GFL_HOST_DEVICE bool full() const noexcept { return _aView.size() == _capacity; }

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

    // --- Resize ---
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

    // --- Append ---
    GFL_HOST_DEVICE void append(T const& value) noexcept {
        i64 const oldSize = _aView._size;
        resizeBy(1);
        data()[oldSize] = value;
    }
    template<typename Container>
    GFL_HOST_DEVICE void append(Container const& c) noexcept {
        static_assert(std::is_same_v<typename Container::value_type, T>);
        if (not c.empty()) {
            i64 const oldSize = _aView._size;
            resizeBy(scast<i64>(c.size()));
            memcpy(data() + oldSize, c.data(), sizeof(T) * scast<usize>(c.size()));
        }
    }
    GFL_HOST_DEVICE void appendAtomic(T const& value) noexcept {
        i64 const oldSize = resizeByAtomic(1);
        data()[oldSize] = value;
    }
    template<typename Container>
    GFL_HOST_DEVICE void appendAtomic(Container const& c) noexcept {
        static_assert(std::is_same_v<typename Container::value_type, T>);
        if (not c.empty()) {
            i64 const oldSize = resizeByAtomic(scast<i64>(c.size()));
            memcpy(data() + oldSize, c.data(), sizeof(T) * scast<usize>(c.size()));
        }
    }
    GFL_DEVICE void appendAtomicBlock(T const& value) noexcept {
        i64 const oldSize = resizeByAtomicBlock(1);
        data()[oldSize] = value;
    }
    template<typename Container>
    GFL_DEVICE void appendAtomicBlock(Container const& c) noexcept {
        static_assert(std::is_same_v<typename Container::value_type, T>);
        if (not c.empty()) {
            i64 const oldSize = resizeByAtomicBlock(scast<i64>(c.size()));
            memcpy(data() + oldSize, c.data(), sizeof(T) * scast<usize>(c.size()));
        }
    }

    // --- Fill ---
    GFL_HOST_DEVICE void fill(T const& value) noexcept { view().fill(value); }
    GFL_HOST_DEVICE void zero() noexcept { view().zero(); }
    template<typename Container>
    void loadFrom(Container const& c) noexcept {
        static_assert(std::is_same_v<typename Container::value_type, T>);
        assert(scast<i64>(c.size()) <= _capacity);
        resizeTo(scast<i64>(c.size()));
        std::memcpy(data(), c.data(), sizeof(T) * scast<usize>(c.size()));
    }
    template<typename Container>
    void dumpTo(Container& c) const noexcept {
        static_assert(std::is_same_v<typename Container::value_type, T>);
        assert(size() <= scast<i64>(c.size()));
        std::memcpy(c.data(), data(), sizeof(T) * scast<usize>(size()));
    }
};

// ============================================================================
// Vector - host-only owning growable vector (pool-allocated).
// ============================================================================
template<typename T>
class Vector {
    VectorView<T> _vView;
    Vector(T* const data, i64 const capacity) noexcept : _vView(data,capacity) { assert(capacity > 0); }
public:
    using value_type = T;

    // --- Construction ---
    Vector() = delete;
    Vector(HeapPool& pool, i64 const capacity) : Vector(pool.allocate<T>(capacity), capacity) {}

    // --- Data access ---
    T const* data() const noexcept { return _vView.data(); }
    T* data() noexcept { return _vView.data(); }

    // --- Size / capacity ---
    i64 size() const noexcept { return _vView.size(); }
    i64 capacity() const noexcept { return _vView.capacity(); }
    bool empty() const noexcept { return _vView.empty(); }
    bool full() const noexcept { return _vView.full(); }

    // --- Iterators ---
    T const* begin() const noexcept { return _vView.begin(); }
    T* begin() noexcept { return _vView.begin(); }
    T const* end() const noexcept { return _vView.end(); }
    T* end() noexcept { return _vView.end(); }

    // --- Element access ---
    T const& at(i64 const idx) const noexcept { return _vView.at(idx); }
    T& at(i64 const idx) noexcept { return _vView.at(idx); }
    T const& operator[](i64 const idx) const noexcept { return _vView[idx]; }
    T& operator[](i64 const idx) noexcept { return _vView[idx]; }
    T const& front() const noexcept { return _vView.front(); }
    T& front() noexcept { return _vView.front(); }
    T const& back() const noexcept { return _vView.back(); }
    T& back() noexcept { return _vView.back(); }

    // --- Views ---
    ArrayView<T> slice(i64 const begin, i64 const end) const noexcept { return _vView.slice(begin, end); }
    ArrayView<T> slice(i64 const count) const noexcept { return _vView.slice(count); }
    ArrayView<T> view() const noexcept { return _vView.view(); }

    // --- Resize ---
    void resizeBy(i64 const delta) noexcept { _vView.resizeBy(delta); }
    void resizeTo(i64 const size) noexcept { _vView.resizeTo(size); }
    void clear() noexcept { _vView.clear(); }

    // --- Append ---
    void append(T const& value) noexcept { _vView.append(value); }
    template<typename Container>
    void append(Container const& c) noexcept { _vView.append(c); }

    // --- Fill ---
    void fill(T const& value) noexcept { _vView.fill(value); }
    void zero() noexcept { _vView.zero(); }
    template<typename Container>
    void loadFrom(Container const& c) noexcept { _vView.loadFrom(c); }
    template<typename Container>
    void dumpTo(Container& c) const noexcept { _vView.dumpTo(c); }
};

#ifdef __CUDACC__
// ============================================================================
// DeviceVector - device-resident owning growable vector. Element access and
// atomic appends are device-only; metadata is host-readable.
// ============================================================================
template<typename T>
class DeviceVector {
    VectorView<T> _vView;
    DeviceVector(T* const data, i64 const capacity) noexcept : _vView(data,capacity) { assert(capacity > 0); }
    T* devicePtr() const noexcept { return const_cast<T*>(_vView.data()); }
public:
    using value_type = T;

    // --- Construction ---
    DeviceVector() = delete;
    DeviceVector(DevicePool& dPool, i64 const capacity) : DeviceVector(dPool.allocate<T>(capacity), capacity) {}

    // --- Data access ---
    GFL_HOST_DEVICE T const* data() const noexcept { return _vView.data(); }
    GFL_DEVICE T* data() noexcept { return _vView.data(); }

    // --- Size / capacity ---
    GFL_HOST_DEVICE i64 size() const noexcept { return _vView.size(); }
    GFL_HOST_DEVICE i64 capacity() const noexcept { return _vView.capacity(); }
    GFL_HOST_DEVICE bool empty() const noexcept { return _vView.empty(); }
    GFL_HOST_DEVICE bool full() const noexcept { return _vView.full(); }

    // --- Iterators (device only) ---
    GFL_DEVICE T const* begin() const noexcept { return _vView.begin(); }
    GFL_DEVICE T* begin() noexcept { return _vView.begin(); }
    GFL_DEVICE T const* end() const noexcept { return _vView.end(); }
    GFL_DEVICE T* end() noexcept { return _vView.end(); }

    // --- Element access (device only) ---
    GFL_DEVICE T const& at(i64 const idx) const noexcept { return _vView.at(idx); }
    GFL_DEVICE T& at(i64 const idx) noexcept { return _vView.at(idx); }
    GFL_DEVICE T const& operator[](i64 const idx) const noexcept { return _vView[idx]; }
    GFL_DEVICE T& operator[](i64 const idx) noexcept { return _vView[idx]; }
    GFL_DEVICE T const& front() const noexcept { return _vView.front(); }
    GFL_DEVICE T& front() noexcept { return _vView.front(); }
    GFL_DEVICE T const& back() const noexcept { return _vView.back(); }
    GFL_DEVICE T& back() noexcept { return _vView.back(); }

    // --- Views ---
    GFL_DEVICE ArrayView<T> slice(i64 const begin, i64 const end) const noexcept { return _vView.slice(begin, end); }
    GFL_DEVICE ArrayView<T> slice(i64 const count) const noexcept { return _vView.slice(count); }
    GFL_DEVICE ArrayView<T> view() const noexcept { return _vView.view(); }

    // --- Resize ---
    GFL_HOST_DEVICE void resizeBy(i64 const delta) noexcept { _vView.resizeBy(delta); }
    GFL_DEVICE i64 resizeByAtomic(i64 const delta) noexcept { return _vView.resizeByAtomic(delta); }
    GFL_DEVICE i64 resizeByAtomicBlock(i64 const delta) noexcept { return _vView.resizeByAtomicBlock(delta); }
    GFL_HOST_DEVICE void resizeTo(i64 const size) noexcept { _vView.resizeTo(size); }
    GFL_HOST_DEVICE void clear() noexcept { _vView.clear(); }

    // --- Append (device only) ---
    GFL_DEVICE void append(T const& value) noexcept { _vView.append(value); }
    template<typename Container>
    GFL_DEVICE void append(Container const& c) noexcept { _vView.append(c); }
    GFL_DEVICE void appendAtomic(T const& value) noexcept { _vView.appendAtomic(value); }
    template<typename Container>
    GFL_DEVICE void appendAtomic(Container const& c) noexcept { _vView.appendAtomic(c); }
    GFL_DEVICE void appendAtomicBlock(T const& value) noexcept { _vView.appendAtomicBlock(value); }
    template<typename Container>
    GFL_DEVICE void appendAtomicBlock(Container const& c) noexcept { _vView.appendAtomicBlock(c); }

    // --- Transfer ---
    template<typename Container>
    void dumpToDeviceAsync(cudaStream_t const stream, Container const& c) noexcept { _vView.dumpToDeviceAsync(stream, c); }
    template<typename Container>
    void loadFromDeviceAsync(cudaStream_t const stream, Container& c) noexcept { _vView.loadFromDeviceAsync(stream, c); }
};

// ============================================================================
// MirrorVector - host+device mirror owning growable vector. Transfers explicitly.
// ============================================================================
template<typename T>
class MirrorVector {
    VectorView<T,MirrorPtr<T>> _vView;
    MirrorVector(MirrorPtr<T> const data, i64 const capacity) noexcept : _vView(data, capacity) { assert(capacity > 0); }
    MirrorVector(T* hostData, T* deviceData, i64 const capacity) noexcept : _vView({hostData,deviceData}, capacity) { assert(capacity > 0); }
public:
    using value_type = T;

    // --- Construction ---
    MirrorVector() = delete;
    MirrorVector(MirrorPool& pool, i64 const capacity) : MirrorVector(pool.allocate<T>(capacity), capacity) {}
    MirrorVector(HostPool& hPool, DevicePool& dPool, i64 const capacity) :
        MirrorVector(hPool.allocate<T>(capacity), dPool.allocate<T>(capacity), capacity) {}

    // --- Data access ---
    GFL_HOST_DEVICE T const* data() const noexcept { return _vView.data(); }
    GFL_HOST_DEVICE T* data() noexcept { return _vView.data(); }

    // --- Size / capacity ---
    GFL_HOST_DEVICE i64 size() const noexcept { return _vView.size(); }
    GFL_HOST_DEVICE i64 capacity() const noexcept { return _vView.capacity(); }
    GFL_HOST_DEVICE bool empty() const noexcept { return _vView.empty(); }
    GFL_HOST_DEVICE bool full() const noexcept { return _vView.full(); }

    // --- Iterators ---
    GFL_HOST_DEVICE T const* begin() const noexcept { return _vView.begin(); }
    GFL_HOST_DEVICE T* begin() noexcept { return _vView.begin(); }
    GFL_HOST_DEVICE T const* end() const noexcept { return _vView.end(); }
    GFL_HOST_DEVICE T* end() noexcept { return _vView.end(); }

    // --- Element access ---
    GFL_HOST_DEVICE T const& at(i64 const idx) const noexcept { return _vView.at(idx); }
    GFL_HOST_DEVICE T& at(i64 const idx) noexcept { return _vView.at(idx); }
    GFL_HOST_DEVICE T const& operator[](i64 const idx) const noexcept { return _vView[idx]; }
    GFL_HOST_DEVICE T& operator[](i64 const idx) noexcept { return _vView[idx]; }
    GFL_HOST_DEVICE T const& front() const noexcept { return _vView.front(); }
    GFL_HOST_DEVICE T& front() noexcept { return _vView.front(); }
    GFL_HOST_DEVICE T const& back() const noexcept { return _vView.back(); }
    GFL_HOST_DEVICE T& back() noexcept { return _vView.back(); }

    // --- Views ---
    GFL_HOST_DEVICE ArrayView<T> slice(i64 const begin, i64 const end) const noexcept { return _vView.slice(begin, end); }
    GFL_HOST_DEVICE ArrayView<T> slice(i64 const count) const noexcept { return _vView.slice(count); }
    GFL_HOST_DEVICE ArrayView<T> view() const noexcept { return _vView.view(); }

    // --- Resize ---
    GFL_HOST_DEVICE void resizeBy(i64 const delta) noexcept { _vView.resizeBy(delta); }
    GFL_DEVICE i64 resizeByAtomic(i64 const delta) noexcept { return _vView.resizeByAtomic(delta); }
    GFL_DEVICE i64 resizeByAtomicBlock(i64 const delta) noexcept { return _vView.resizeByAtomicBlock(delta); }
    GFL_HOST_DEVICE void resizeTo(i64 const size) noexcept { _vView.resizeTo(size); }
    GFL_HOST_DEVICE void clear() noexcept { _vView.clear(); }

    // --- Append ---
    GFL_HOST_DEVICE void append(T const& value) noexcept { _vView.append(value); }
    template<typename Container>
    GFL_HOST_DEVICE void append(Container const& c) noexcept { _vView.append(c); }
    GFL_DEVICE void appendAtomic(T const& value) noexcept { _vView.appendAtomic(value); }
    template<typename Container>
    GFL_DEVICE void appendAtomic(Container const& c) noexcept { _vView.appendAtomic(c); }
    GFL_DEVICE void appendAtomicBlock(T const& value) noexcept { _vView.appendAtomicBlock(value); }
    template<typename Container>
    GFL_DEVICE void appendAtomicBlock(Container const& c) noexcept { _vView.appendAtomicBlock(c); }

    // --- Host fill / load ---
    void fill(T const& value) noexcept { _vView.fill(value); }
    void zero() noexcept { _vView.zero(); }
    template<typename Container>
    void loadFrom(Container const& c) noexcept { _vView.loadFrom(c); }
    template<typename Container>
    void dumpTo(Container& c) const noexcept { _vView.dumpTo(c);}

    // --- Transfer ---
    void copyToDeviceAsync(cudaStream_t const stream) noexcept {
        cudaError_t const status = cudaMemcpyAsync(_vView.mirrorData().d(), _vView.mirrorData().h(), sizeof(T) * scast<usize>(size()), cudaMemcpyHostToDevice, stream);
        checkOrAbort(status == cudaSuccess, "MirrorVector::copyToDeviceAsync: cudaMemcpyAsync failed");
    }
    void copyToHostAsync(cudaStream_t const stream) noexcept {
        cudaError_t const status = cudaMemcpyAsync(_vView.mirrorData().h(), _vView.mirrorData().d(), sizeof(T) * scast<usize>(size()), cudaMemcpyDeviceToHost, stream);
        checkOrAbort(status == cudaSuccess, "MirrorVector::copyToHostAsync: cudaMemcpyAsync failed");
    }
};
#endif
}