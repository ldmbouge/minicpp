#pragma once

#include <cassert>

#include "Alias.hpp"
#include "DebugUtils.hpp"
#include "FunQual.hpp"
#include "Macros.hpp"
#include "Pools.hpp"

namespace gfl {
#ifdef __CUDACC__
template<typename T> class MirrorPtr {
    static_assert(std::is_trivially_copyable_v<T> || std::is_void_v<T>);
    T* _host{nullptr};
    T* _device{nullptr};

public:
    using element_type = T;
    using difference_type = i64;
    MirrorPtr() noexcept = default;
    GFL_HOST_DEVICE MirrorPtr(std::nullptr_t) noexcept : _host(nullptr), _device(nullptr) {}
    GFL_HOST_DEVICE MirrorPtr(T* const h, T* const d) noexcept : _host(h), _device(d) {
        assert(h != nullptr);
        assert(d != nullptr);
    }
    GFL_HOST_DEVICE T* h() const noexcept { return _host; }
    GFL_HOST_DEVICE T* d() const noexcept { return _device; }
    GFL_HOST_DEVICE T* get() const noexcept {
#ifdef __CUDA_ARCH__
        return _device;
#else
        return _host;
#endif
    }
    GFL_HOST_DEVICE explicit operator bool() const noexcept { return _host != nullptr && _device != nullptr; }
    GFL_HOST_DEVICE bool operator==(std::nullptr_t) const noexcept { return _host == nullptr || _device == nullptr; }
    GFL_HOST_DEVICE bool operator!=(std::nullptr_t) const noexcept { return !(*this == nullptr); }
    GFL_HOST_DEVICE operator T*() const noexcept { return get(); }
    template<typename U = T> requires(!std::is_void_v<U>)
    GFL_HOST_DEVICE U& operator*() const noexcept {
        return *get();
    }
    template<typename U = T> requires(!std::is_void_v<U>)
    GFL_HOST_DEVICE U* operator->() const noexcept {
        return get();
    }
    template<typename U = T> requires(!std::is_void_v<U>)
    GFL_HOST_DEVICE U& operator[](difference_type const i) const noexcept {
        return get()[i];
    }
    template<typename U = T> requires(!std::is_void_v<U>)
    GFL_HOST_DEVICE MirrorPtr operator+(difference_type const n) const noexcept {
        return {_host + n, _device + n};
    }
    template<typename U = T> requires(!std::is_void_v<U>)
    GFL_HOST_DEVICE MirrorPtr operator-(difference_type const n) const noexcept {
        return {_host - n, _device - n};
    }
    template<typename U = T> requires(!std::is_void_v<U>)
    GFL_HOST_DEVICE MirrorPtr& operator+=(difference_type const n) noexcept {
        _host += n;
        _device += n;
        return *this;
    }
    template<typename U = T> requires(!std::is_void_v<U>)
    GFL_HOST_DEVICE MirrorPtr& operator-=(difference_type const n) noexcept {
        _host -= n;
        _device -= n;
        return *this;
    }
    template<typename U = T> requires(!std::is_void_v<U>)
    GFL_HOST_DEVICE difference_type operator-(MirrorPtr const other) const noexcept {
        assert(*this && other);
        assert((_host - other._host) == (_device - other._device));
        return as<difference_type>(_host - other._host);
    }
    template<typename U> GFL_HOST_DEVICE explicit MirrorPtr(MirrorPtr<U> const other) noexcept : _host(rcast<T*>(other._host)), _device(rcast<T*>(other._device)) {}
};

class MirrorRegion {
    MirrorPtr<void> _begin;
    MirrorPtr<void> _end;

public:
    MirrorRegion() noexcept = default;
    MirrorRegion(MirrorPtr<void> const b, MirrorPtr<void> const e) noexcept : _begin(b), _end(e) { assert(rcast<uptr>(_end.h()) - rcast<uptr>(_begin.h()) == rcast<uptr>(_end.d()) - rcast<uptr>(_begin.d())); }
    i64 size() const noexcept { return as<i64>(rcast<uptr>(_end.h()) - rcast<uptr>(_begin.h())); }
    void copyToDeviceAsync(cudaStream_t const s) const noexcept { CHECK_CUDA_ERROR(cudaMemcpyAsync(_begin.d(), _begin.h(), asSize(size()), cudaMemcpyHostToDevice, s)); }
    void copyToHostAsync(cudaStream_t const s) const noexcept { CHECK_CUDA_ERROR(cudaMemcpyAsync(_begin.h(), _begin.d(), asSize(size()), cudaMemcpyDeviceToHost, s)); }
};

class MirrorPool {
    static constexpr i64 DefaultArenaSize{4ll * 1024ll * 1024ll};
    ArenaPool _hPool;
    ArenaPool _dPool;

public:
    explicit MirrorPool(i64 const size = DefaultArenaSize) : _hPool(HostAllocator{}.allocate(size), size), _dPool(DeviceAllocator{}.allocate(size), size) {}
    ~MirrorPool() {
        HostAllocator{}.deallocate(_hPool.memory(), _hPool.total());
        DeviceAllocator{}.deallocate(_dPool.memory(), _dPool.total());
    }
    GFL_DISABLE_COPY_AND_MOVE(MirrorPool);
    // The two arenas allocate the same sizes in the same order, so host and
    // device offsets stay in lockstep and a MirrorRegion over them is valid
    template<typename T> MirrorPtr<T> allocate(i64 const count = 1) noexcept {
        i64 const size = as<i64>(sizeof(T)) * count;
        return {scast<T*>(_hPool.allocate(size)), scast<T*>(_dPool.allocate(size))};
    }
    void clear() noexcept {
        _hPool.clear();
        _dPool.clear();
    }
    i64 used() const noexcept { return _hPool.used(); }
    i64 free() const noexcept { return _hPool.free(); }
    i64 total() const noexcept { return _hPool.total(); }
    template<typename Block> MirrorRegion record(Block&& b) {
        MirrorPtr<void> const begin(_hPool.current(), _dPool.current());
        b();
        MirrorPtr<void> const end(_hPool.current(), _dPool.current());
        return {begin, end};
    }
    template<typename T, typename... Args> MirrorPtr<T> make(Args&&... args) {
        auto const ptr = allocate<T>();
        new (ptr.h()) T(std::forward<Args>(args)...);
        return ptr;
    }
};
#endif
}