#pragma once

#include <cassert>
#include <vector>
#include "FunQual.hpp"
#include "Allocators.hpp"

namespace gfl {

template<typename Allocator>
class MemoryPool {
    struct Block {
        void* memory;
        usize size;
    };
    std::vector<Block> _blocks;
public:
    ~MemoryPool() {
        for (auto b : _blocks)
            Allocator{}.deallocate(b.memory, b.size);
    }
    void* allocate(usize const size) noexcept {
        _blocks.push_back(Block{Allocator{}.allocate(size), size});
        return _blocks.back().memory;
    }
    void deallocate(void*, usize) noexcept {}
    template<typename T>
    T* allocate(usize const count) noexcept { return scast<T*>(allocate(sizeof(T)* count));}
};

#ifdef __CUDACC__
template<typename T>
class MirrorPtr {
    T* _host;
    T* _device;

public:
    MirrorPtr() = delete;
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
    GFL_HOST_DEVICE T& operator*() const noexcept { return *get(); }
    GFL_HOST_DEVICE T* operator->() const noexcept { return get(); }
    GFL_HOST_DEVICE bool valid() const noexcept { return _host != nullptr and _device != nullptr; }
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
    template<typename T>
    MirrorPtr<T> allocate(i64 const count) noexcept {
        return {_hostPool.allocate<T>(count), _devicePool.allocate<T>(count)};
    }
    template<typename T, typename... Args>
    MirrorPtr<T> make(Args&&... args) noexcept {
        static_assert(std::is_trivially_copyable_v<T>);
        MirrorPtr<T> mptr = allocate<T>(1);
        new (mptr.h()) T(std::forward<Args>(args)...);
        return mptr;
    }
    void copyToDeviceAsync(cudaStream_t const stream) noexcept {
        assert(_hostPool.usedSize() <= _devicePool.usedSize());
        cudaError_t const status = cudaMemcpyAsync(_devicePool.mem(), _hostPool.mem(), _hostPool.usedSize(), cudaMemcpyHostToDevice, stream);
        checkOrAbort(status == cudaSuccess, "MirrorValue::copyToDevice: cudaMemcpyAsync failed");
    }
    void copyToHostAsync(cudaStream_t const stream) noexcept {
        assert(_devicePool.usedSize() <= _hostPool.usedSize());
        cudaError_t const status = cudaMemcpyAsync(_hostPool.mem(), _devicePool.mem(), _hostPool.usedSize(), cudaMemcpyDeviceToHost, stream);
        checkOrAbort(status == cudaSuccess, "MirrorValue::copyToHost: cudaMemcpyAsync failed");
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