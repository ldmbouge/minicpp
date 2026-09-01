#pragma once

#include <cassert>
#include <vector>
#include "FunQual.hpp"
#include "Allocators.hpp"

namespace gfl {

template<typename Allocator>
class MemoryPool {
    static constexpr i64 DefaultBlockSize = 4 * 1024 * 1024ll;
    i64 const _blockSize;
    std::vector<BumpAllocator*> _blocks;
public:
    MemoryPool(i64 const blockSize = DefaultBlockSize) : _blockSize(blockSize), _blocks() {
        _blocks.emplace_back(new BumpAllocator(Allocator{}, _blockSize));
    }
    ~MemoryPool() {
        for (auto b : _blocks) {
            Allocator{}.deallocate(b->mem(), b->totalSize());
            delete b;
        }
    }
    void* allocate(usize const size) noexcept {
        if (not _blocks.back()->fits(size)) {
            i64 const blockSize = gfl::max<i64>(scast<i64>(size), _blockSize);
            _blocks.emplace_back(new BumpAllocator(Allocator{}, blockSize));
        }
        assert(_blocks.back()->fits(size));
        return _blocks.back()->allocate(size);
    }
    template<typename T>
    T* allocate(usize const count) noexcept { return scast<T*>(allocate(sizeof(T) * count)); }

    void deallocate(void*, usize) noexcept {}
};
using HeapPool = MemoryPool<HeapAllocator>;

#ifdef __CUDACC__
using DevicePool = MemoryPool<DeviceAllocator>;
using HostPool = MemoryPool<HostAllocator>;

template<typename T>
class MirrorPtr {
    static_assert(std::is_trivially_copyable_v<T> or std::is_void_v<T>);
    T* _host;
    T* _device;

public:
    MirrorPtr() : _host(nullptr), _device(nullptr) {};
    MirrorPtr(T* host, T* device) noexcept : _host(host), _device(device) {
        assert(host != nullptr);
        assert(device != nullptr);
    }
    T* h() const noexcept { return _host; }
    GFL_HOST_DEVICE T* d() const noexcept { return _device; }
    GFL_HOST_DEVICE T* get() const noexcept {
#ifdef __CUDA_ARCH__
        return _device;
#else
        return _host;
#endif
    }
    template <typename U = T, typename = std::enable_if_t<not std::is_void_v<U>>>
    GFL_HOST_DEVICE U& operator*() const noexcept { return *get();}
    GFL_HOST_DEVICE T* operator->() const noexcept { return get(); }
    GFL_HOST_DEVICE bool valid() const noexcept { return _host != nullptr and _device != nullptr; }
};

class MirrorRegion {
    MirrorPtr<void> _begin;
    MirrorPtr<void> _end;
public:
    MirrorRegion() = default;
    MirrorRegion(MirrorPtr<void> begin, MirrorPtr<void> end) noexcept : _begin(begin), _end(end) {}

    MirrorRegion(MirrorRegion&&) noexcept = default;
    MirrorRegion& operator=(MirrorRegion&&) noexcept = default;

    MirrorRegion(const MirrorRegion&) = delete;
    MirrorRegion& operator=(const MirrorRegion&) = delete;

    i64 size() const noexcept { return rcast<uptr>(_end.h()) - rcast<uptr>(_begin.h()); }
    void copyToDeviceAsync(cudaStream_t const stream) noexcept {
        cudaError_t const status = cudaMemcpyAsync(_begin.d(), _begin.h(), size(), cudaMemcpyHostToDevice, stream);
        checkOrAbort(status == cudaSuccess, "MirrorRegion::copyToDevice: cudaMemcpyAsync failed");
    }
    void copyToHostAsync(cudaStream_t const stream) noexcept {
        cudaError_t const status = cudaMemcpyAsync(_begin.h(), _begin.d(), size(), cudaMemcpyDeviceToHost, stream);
        checkOrAbort(status == cudaSuccess, "MirrorRegion::copyToHost: cudaMemcpyAsync failed");
    }
};

class MirrorPool {
    static constexpr usize DefaultSize = 4096;
    BumpAllocator _hostPool;
    BumpAllocator _devicePool;
public:
    MirrorPool(usize const size = DefaultSize) :
        _hostPool(HostAllocator{}.allocate(size), size),
        _devicePool(DeviceAllocator{}.allocate(size), size)
    {}
    ~MirrorPool() {
        HostAllocator{}.deallocate(_hostPool.mem(), _hostPool.totalSize());
        DeviceAllocator{}.deallocate(_devicePool.mem(), _devicePool.totalSize());
    }
    MirrorPool(MirrorPool&& other) noexcept = delete;
    MirrorPool& operator=(MirrorPool&& other) noexcept = delete;
    MirrorPool(MirrorPool const&) = delete;
    MirrorPool& operator=(MirrorPool const&) = delete;

    template<typename T>
    MirrorPtr<T> allocate(i64 const count) noexcept {
        return {_hostPool.allocate<T>(count), _devicePool.allocate<T>(count)};
    }

    template <typename Block>
    MirrorRegion record(Block b) {
        MirrorPtr<void> begin(_hostPool.freeMem(), _devicePool.freeMem());
        b();
        MirrorPtr<void> end(_hostPool.freeMem(), _devicePool.freeMem());
        return {begin, end};
    }

    template<typename T, typename... Args>
    MirrorPtr<T> make(Args&&... args) noexcept {
        static_assert(std::is_trivially_copyable_v<T>);
        MirrorPtr<T> mptr = allocate<T>(1);
        new (mptr.h()) T(std::forward<Args>(args)...);
        return mptr;
    }
};

#endif
}

template<typename Allocator>
void* operator new(std::size_t size, gfl::MemoryPool<Allocator>& pool) noexcept { return pool.allocate(size); }

template<typename Allocator>
void operator delete(void*, gfl::MemoryPool<Allocator>&) noexcept {}

template<typename Allocator>
void* operator new[](std::size_t size, gfl::MemoryPool<Allocator>& pool) noexcept { return pool.allocate(size); }

template<typename Allocator>
void operator delete[](void*, gfl::MemoryPool<Allocator>&) noexcept {}