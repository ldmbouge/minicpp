#pragma once

#include <cstdlib>
#include <new>
#include <utility>
#ifdef __CUDACC__
#include <cuda_runtime.h>
#endif
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
#endif

    template<typename T, template<typename> class Allocator = HeapAllocator, typename... Args>
    T* make(Args&&... args) noexcept{
        return ::new (Allocator<T>{}.allocate(1)) T(std::forward<Args>(args)...);
    }
    template<typename T, template<typename> class Allocator = HeapAllocator>
    void destroy(T* ptr) noexcept {
        checkOrAbort(ptr != nullptr, "destroy: null pointer");
        ptr->~T();
        Allocator<T>{}.deallocate(ptr, 1);
    }
}