#pragma once

#include <cstdlib>
#include <new>
#include <utility>
#ifdef __CUDACC__
#include <cuda_runtime.h>
#endif
#include "ArenaAllocator.hpp"
#include "DebugUtils.hpp"
#include "Types.hpp"

namespace gfl {
    template<typename T = u8>
    class HeapAllocator{
    public:
        using value_type = T;
        T* allocate(usize const count) noexcept {
            checkOrAbort(count != 0, "HeapAllocator: count must not be zero");
            void* memory = std::malloc(sizeof(T) * count);
            checkOrAbort(memory != nullptr, "HeapAllocator: std::malloc failed");
            return scast<T*>(memory);
        }
        void deallocate(T* memory, usize) noexcept { std::free(memory); }
    };

#ifdef __CUDACC__
    template<typename T = u8>
    class ManagedAllocator {
    public:
        using value_type = T;
        T* allocate(usize const count) noexcept {
            checkOrAbort(count != 0, "ManagedAllocator: count must not be zero");
            void* memory = nullptr;
            cudaError_t const status = cudaMallocManaged(&memory, sizeof(T) * count);
            checkOrAbort(status == cudaSuccess and memory != nullptr, "ManagedAllocator: cudaMallocManaged failed");
            return scast<T*>(memory);
        }
        void deallocate(T* memory, usize) noexcept {
            cudaError_t const status = cudaFree(memory);
            checkOrAbort(status == cudaSuccess, "ManagedAllocator: cudaFree failed");
        }
    };
    template<typename T = u8>
    class DeviceAllocator {
    public:
        using value_type = T;
        T* allocate(usize const count) noexcept {
            checkOrAbort(count != 0, "DeviceAllocator: count must not be zero");
            void* memory = nullptr;
            cudaError_t const status = cudaMalloc(&memory, sizeof(T) * count);
            checkOrAbort(status == cudaSuccess and memory != nullptr, "DeviceAllocator: cudaMalloc failed");
            return scast<T*>(memory);
        }
        void deallocate(T* memory, usize) noexcept {
            cudaError_t const status = cudaFree(memory);
            checkOrAbort(status == cudaSuccess, "DeviceAllocator: cudaFree failed");
        }
    };
    template<typename T = u8>
    class HostAllocator {
    public:
        using value_type = T;
        T* allocate(usize const count) noexcept {
            checkOrAbort(count != 0, "HostAllocator: count must not be zero");
            void* memory = nullptr;
            cudaError_t const status = cudaMallocHost(&memory, sizeof(T) * count);
            checkOrAbort(status == cudaSuccess and memory != nullptr, "HostAllocator: cudaMalloc failed");
            return scast<T*>(memory);
        }
        void deallocate(T* memory, usize) noexcept {
            cudaError_t const status = cudaFree(memory);
            checkOrAbort(status == cudaSuccess, "HostAllocator: cudaFree failed");
        }
    };

    template<typename Allocator>
    class MemoryPool {
        bool _growing;
        std::vector<ArenaAllocator*> _blocks;
    public:
        MemoryPool() : _growing(true) {};
        MemoryPool(usize const size) noexcept : _growing(false){
            auto b = new ArenaAllocator(Allocator{}.allocate(size), size);
            _blocks.push_back(b);
        }
        void * allocate(usize const size) noexcept {
            if (_growing) {
                auto b = new ArenaAllocator(Allocator{}.allocate(size), size);
                _blocks.push_back(b);
            }
            return _blocks.back()->allocate(size);
        }
        void deallocate(void*, usize) noexcept {}

#ifdef __CUDACC__
        void prefetchToHost(cudaStream_t const stream = 0) const noexcept {
            if (not _growing) {
                cudaMemLocation const location = {.type = cudaMemLocationTypeHost, .id = 0};
                cudaError_t const status = cudaMemPrefetchAsync(_blocks.back()->mem(), _blocks.back()->usedSize(), location, 0, stream);
                checkOrAbort(status == cudaSuccess, "Vector::prefetchToHost failed");
            }

        }
        void prefetchToDevice(cudaStream_t const stream = 0, i32 const device = 0) const noexcept {
            cudaMemLocation const location = {.type = cudaMemLocationTypeDevice, .id = device};
            cudaError_t const status = cudaMemPrefetchAsync(_blocks.back()->mem(), _blocks.back()->usedSize(), location, 0, stream);
            checkOrAbort(status == cudaSuccess, "Vector::prefetchToDevice failed");
        }
#endif
    };

    class MemoryManager {
        MemoryPool<ManagedAllocator<u8>> _managedPool;
        MemoryPool<HostAllocator<u8>> _hostPool;
        MemoryPool<DeviceAllocator<u8>> _devicePool;

    public:
        MemoryManager(usize const managedSize = 4096) : _managedPool(managedSize), _hostPool(), _devicePool() {};
        MemoryPool<ManagedAllocator<>>& managed() noexcept { return _managedPool; }
        MemoryPool<HostAllocator<u8>> &  host() noexcept { return _hostPool; }
        MemoryPool<DeviceAllocator<u8>> &  device() noexcept { return _devicePool; }
    };


#endif
}

// 1. Make operator new a template that accepts any Allocator type
template <typename Allocator>
void* operator new(std::size_t size, gfl::MemoryPool<Allocator>& pool) noexcept {
    return pool.allocate(size);
}

// 2. Make operator delete match it perfectly
template <typename Allocator>
void operator delete(void* ptr, gfl::MemoryPool<Allocator>& pool) noexcept {
}
template <typename Allocator>
void* operator new[](std::size_t size, gfl::MemoryPool<Allocator>& pool) noexcept {
    return pool.allocate(size);
}
template <typename Allocator>
void operator delete[](void* ptr, gfl::MemoryPool<Allocator>& pool) noexcept {
    pool.deallocate(ptr, 0);
}