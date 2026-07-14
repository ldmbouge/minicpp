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
    void* allocate(usize const size) const noexcept {
        checkOrAbort(size != 0, "HeapAllocator: size must not be zero");
        void* memory = std::malloc(size);
        checkOrAbort(memory != nullptr, "HeapAllocator: std::malloc failed");
        return memory;
    }

    void deallocate(void* memory, usize) const noexcept { std::free(memory); }
};

#ifdef __CUDACC__
class DeviceAllocator {
public:
    void* allocate(usize const size) const noexcept {
        checkOrAbort(size != 0, "DeviceAllocator: size must not be zero");
        void* memory = nullptr;
        cudaError_t const status = cudaMalloc(&memory, size);
        checkOrAbort(status == cudaSuccess and memory != nullptr, "DeviceAllocator: cudaMalloc failed");
        return memory;
    }

    void deallocate(void* memory, usize) const noexcept {
        cudaError_t const status = cudaFree(memory);
        checkOrAbort(status == cudaSuccess, "DeviceAllocator: cudaFree failed");
    }
};

class HostAllocator {
public:
    void* allocate(usize const size) const noexcept {
        checkOrAbort(size != 0, "HostAllocator: size must not be zero");
        void* memory = nullptr;
        cudaError_t const status = cudaMallocHost(&memory, size);
        checkOrAbort(status == cudaSuccess and memory != nullptr, "HostAllocator: cudaMallocHost failed");
        return memory;
    }

    void deallocate(void* memory, usize) const noexcept {
        cudaError_t const status = cudaFree(memory);
        checkOrAbort(status == cudaSuccess, "HostAllocator: cudaFree failed");
    }
};

class ManagedAllocator {
public:
    void* allocate(usize const size) const noexcept {
        checkOrAbort(size != 0, "ManagedAllocator: count must not be zero");
        void* memory = nullptr;
        cudaError_t const status = cudaMallocManaged(&memory, size);
        checkOrAbort(status == cudaSuccess and memory != nullptr, "ManagedAllocator: cudaMallocManaged failed");
        return memory;
    }

    void deallocate(void* memory, usize) const noexcept {
        cudaError_t const status = cudaFree(memory);
        checkOrAbort(status == cudaSuccess, "ManagedAllocator: cudaFree failed");
    }
};
#endif

class BumpAllocator {
    // Vectorized instructions: SSE = 16, AVX = 32, AVX-512 = 64
    static constexpr i32 DefaultAlign = 32;
    uptr _begin;
    uptr _current;
    uptr _end;

public:
    BumpAllocator() : _begin(), _current(), _end() {}
    BumpAllocator(BumpAllocator&& other) noexcept
        : _begin(std::exchange(other._begin, 0)),
          _current(std::exchange(other._current, 0)),
          _end(std::exchange(other._end, 0)) {}
    BumpAllocator& operator=(BumpAllocator&& other) noexcept {
        if (this == &other) return *this;
        _begin   = std::exchange(other._begin, 0);
        _current = std::exchange(other._current, 0);
        _end     = std::exchange(other._end, 0);
        return *this;
    }
    BumpAllocator(BumpAllocator const&) = delete;
    BumpAllocator& operator=(BumpAllocator const&) = delete;

    GFL_HOST_DEVICE
    BumpAllocator(void* memory, i64 const size) noexcept :
        _begin(rcast<uptr>(memory)),
        _current(_begin),
        _end(_begin + scast<uptr>(size)) {
        assert(memory != nullptr);
        assert(size > 0);
        assert(_begin <= numeric_limits<uptr>::max() - scast<uptr>(size)); // Overflow
    }

    template<typename Allocator>
    BumpAllocator(Allocator const allocator, i64 const size) noexcept :
        BumpAllocator(allocator.allocate(scast<usize>(DefaultAlign + size)), DefaultAlign + size) {}

    bool fits(usize const size) const noexcept { return roundUp<uptr>(scast<i64>(_current), DefaultAlign) + size <= _end; }

    void* allocate(usize const size) noexcept {
        checkOrAbort(fits(size), "BumpAllocator::allocate: out of memory");
        uptr const current = roundUp<uptr>(scast<i64>(_current), DefaultAlign);
        _current = current + size;
        return rcast<void*>(current);
    }

    template<typename T>
    T* allocate(i64 const count) { return scast<T*>(allocate(sizeof(T) * scast<usize>(count))); }

    void deallocate(void*, usize) const noexcept {}

    GFL_HOST_DEVICE void clear() noexcept { _current = _begin; }
    GFL_HOST_DEVICE void* mem() const noexcept {
        void* mem = rcast<void*>(_begin);
        assert(mem != nullptr);
        return mem;
    }
    GFL_HOST_DEVICE void* freeMem() const noexcept {
        void* mem = rcast<void*>(_current);
        assert(mem != nullptr);
        return mem;
    }
    GFL_HOST_DEVICE i64 freeSize() const noexcept {
        auto const size =  scast<i64>(_end - _current);
        assert(size >= 0);
        return size;
    }
    GFL_HOST_DEVICE i64 usedSize() const noexcept {
        auto const size =  scast<i64>(_current - _begin);
        assert(size >= 0);
        return size;
    }
    GFL_HOST_DEVICE i64 totalSize() const noexcept {
        auto const size =  scast<i64>(_end - _begin);
        assert(size >= 0);
        return size;
    }
    GFL_HOST_DEVICE void * marker() const noexcept { return freeMem(); }
    GFL_HOST_DEVICE void rewindTo(void const* const marker) noexcept {
        uptr const addr = rcast<uptr>(marker);
        assert(_begin <= addr);
        assert(addr <= _current);
        _current = addr;
    }
};
}