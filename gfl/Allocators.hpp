#pragma once

#include <cstdlib>
#include <cassert>
#ifdef __CUDACC__
#include <cuda_runtime.h>
#endif
#include "DebugUtils.hpp"
#include "MathUtils.hpp"
#include "Types.hpp"
#include "Backend.hpp"

namespace gfl {

class HeapAllocator {
public:
    void* allocate(usize const size) noexcept {
        checkOrAbort(size != 0, "HeapAllocator: size must not be zero");
        void* memory = std::malloc(size);
        checkOrAbort(memory != nullptr, "HeapAllocator: std::malloc failed");
        return memory;
    }
    void deallocate(void* memory, usize) noexcept { std::free(memory); }
};

#ifdef __CUDACC__
class DeviceAllocator {
public:
    void * allocate(usize const size) noexcept {
        checkOrAbort(size != 0, "DeviceAllocator: size must not be zero");
        void* memory = nullptr;
        cudaError_t const status = cudaMalloc(&memory, size);
        checkOrAbort(status == cudaSuccess and memory != nullptr, "DeviceAllocator: cudaMalloc failed");
        return memory;
    }
    void deallocate(void* memory, usize) noexcept {
        cudaError_t const status = cudaFree(memory);
        checkOrAbort(status == cudaSuccess, "DeviceAllocator: cudaFree failed");
    }
};

class HostAllocator {
public:
    void * allocate(usize const size) noexcept {
        checkOrAbort(size != 0, "HostAllocator: size must not be zero");
        void* memory = nullptr;
        cudaError_t const status = cudaMallocHost(&memory, size);
        checkOrAbort(status == cudaSuccess and memory != nullptr, "HostAllocator: cudaMallocHost failed");
        return memory;
    }
    void deallocate(void* memory, usize) noexcept {
        cudaError_t const status = cudaFree(memory);
        checkOrAbort(status == cudaSuccess, "HostAllocator: cudaFree failed");
    }
};
class ManagedAllocator {
public:
    void* allocate(usize const size) noexcept {
        checkOrAbort(size != 0, "ManagedAllocator: count must not be zero");
        void* memory = nullptr;
        cudaError_t const status = cudaMallocManaged(&memory, size);
        checkOrAbort(status == cudaSuccess and memory != nullptr, "ManagedAllocator: cudaMallocManaged failed");
        return memory;
    }
    void deallocate(void* memory, usize) noexcept {
        cudaError_t const status = cudaFree(memory);
        checkOrAbort(status == cudaSuccess, "ManagedAllocator: cudaFree failed");
    }
};
#endif

class BumpAllocator {
    // Vectorized instructions: SSE = 16, AVX = 32, AVX-512 = 64
    static constexpr i32 DefaultAlign {32};
    uptr const begin {0};
    uptr current {0};
    uptr const end {0};

    public:
        BumpAllocator() = delete;
        BumpAllocator(BumpAllocator const &) = delete;
        BumpAllocator(BumpAllocator &&) = delete;
        BumpAllocator& operator=(BumpAllocator const &) = delete;
        BumpAllocator& operator=(BumpAllocator &&) = delete;

        GFL_HOST_DEVICE
        BumpAllocator(void * memory, i64 const size) noexcept :
            begin(rcast<uptr>(memory)),
            current(begin),
            end(begin + scast<uptr>(size))
        {
            assert(memory != nullptr);
            assert(size > 0);
            assert(begin <= numeric_limits<uptr>::max() - scast<uptr>(size)); // Overflow
        }

        void * allocate(usize const size) noexcept
        {
            uptr const memory = roundUp<uptr>(current, DefaultAlign);
            uptr const newCurrent = memory + size;
            if (newCurrent > end)
            {
                constexpr char const * errorMsg = "ArenaAllocator::allocate failed: out of memory\n";
#ifdef __CUDA_ARCH__
                printf("%s", errorMsg);
#else
                std::fprintf(stderr, "%s", errorMsg);
                std::fflush(stderr);
#endif
                abort();
            }
            current = newCurrent;
            return rcast<void*>(memory);
        }

        template<typename T>
        T * allocate(i64 const count) {
            return scast<T*>(allocate(sizeof(T) * count));
        }

        GFL_HOST_DEVICE
        void clear() noexcept { current = begin; }
        GFL_HOST_DEVICE u8 const * getMarker() const noexcept { return rcast<u8*>(current); }
        GFL_HOST_DEVICE
        void rewindTo(u8 const * const marker) noexcept
        {
            uptr const addr = rcast<uptr>(marker);
            assert(begin <= addr);
            assert(addr <= current);
            current = addr;
        }

        GFL_HOST_DEVICE u8* mem() const noexcept { return rcast<u8*>(begin); }
        GFL_HOST_DEVICE u8* freeMem() const noexcept { return rcast<u8*>(current); }
        GFL_HOST_DEVICE i64 freeSize() const noexcept { return scast<i64>(end - current); }
        GFL_HOST_DEVICE i64 usedSize() const noexcept { return scast<i64>(current - begin); }
        GFL_HOST_DEVICE i64 totalSize() const noexcept { return scast<i64>(end - begin); }
    };
}