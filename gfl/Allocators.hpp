#pragma once

#include <cstdlib>
#ifdef __CUDACC__
#include <cuda_runtime.h>
#endif

#include "Alias.hpp"
#include "DebugUtils.hpp"

namespace gfl {
class HeapAllocator {
public:
    void* allocate(i64 const size) const noexcept {
        checkOrAbort(size != 0, "HeapAllocator: size must not be zero");
        void* memory = std::malloc(asSize(size));
        checkOrAbort(memory != nullptr, "HeapAllocator: std::malloc failed");
        return memory;
    }
    template<typename T> T* allocate(i64 const count = 1) const noexcept { return scast<T*>(allocate(as<i64>(sizeof(T)) * count)); }
    void deallocate(void* memory, i64) const noexcept { std::free(memory); }
};

#ifdef __CUDACC__
class DeviceAllocator {
public:
    void* allocate(i64 const size = 1) const noexcept {
        checkOrAbort(size != 0, "DeviceAllocator: size must not be zero");
        void* memory = nullptr;
        checkOrAbort(cudaMalloc(&memory, asSize(size)) == cudaSuccess && memory != nullptr, "DeviceAllocator: cudaMalloc failed");
        return memory;
    }
    template<typename T> T* allocate(i64 const count = 1) const noexcept { return scast<T*>(allocate(as<i64>(sizeof(T)) * count)); }
    void deallocate(void* memory, i64) const noexcept { checkOrAbort(cudaFree(memory) == cudaSuccess, "DeviceAllocator: cudaFree failed"); }
};

class HostAllocator {
public:
    void* allocate(i64 const size) const noexcept {
        checkOrAbort(size != 0, "HostAllocator: size must not be zero");
        void* memory = nullptr;
        checkOrAbort(cudaMallocHost(&memory, asSize(size)) == cudaSuccess && memory != nullptr, "HostAllocator: cudaMallocHost failed");
        return memory;
    }
    template<typename T> T* allocate(i64 const count = 1) const noexcept { return scast<T*>(allocate(as<i64>(sizeof(T)) * count)); }
    void deallocate(void* memory, i64) const noexcept { checkOrAbort(cudaFreeHost(memory) == cudaSuccess, "HostAllocator: cudaFreeHost failed"); }
};

class ManagedAllocator {
public:
    void* allocate(i64 const size) const noexcept {
        checkOrAbort(size != 0, "ManagedAllocator: size must not be zero");
        void* memory = nullptr;
        checkOrAbort(cudaMallocManaged(&memory, asSize(size)) == cudaSuccess && memory != nullptr, "ManagedAllocator: cudaMallocManaged failed");
        return memory;
    }
    template<typename T> T* allocate(i64 const count = 1) const noexcept { return scast<T*>(allocate(as<i64>(sizeof(T)) * count)); }
    void deallocate(void* memory, i64) const noexcept { checkOrAbort(cudaFree(memory) == cudaSuccess, "ManagedAllocator: cudaFree failed"); }
};
#endif
}