#pragma once

#include <type_traits>
#include "Types.hpp"
#include "DebugUtils.hpp"
#include "MathUtils.hpp"
#include "Memory.hpp"

namespace gfl {
    template<typename T, template<typename> class Allocator = HeapAllocator>
    class SizedArray
    {
        static_assert(std::is_trivially_copyable_v<T>, "SizedArray: T must be trivially copyable");
        u8* _block;
        GFL_HOST_DEVICE constexpr i32 dataOffset() const noexcept{ return roundUp<i32>(sizeof(i64),max<i32>(alignof(T),alignof(i64))); }
        GFL_HOST_DEVICE constexpr i64 memSize(i64 const size) const noexcept {return dataOffset() + size * scast<i64>(sizeof(T));}
        GFL_HOST_DEVICE i64* _size() const noexcept { return rcast<i64*>(_block); }
    public:
        explicit SizedArray(i64 const size) noexcept {
            checkOrAbort(size > 0, "SizedArray::SizedArray: size must be positive");
            _block = Allocator<u8>{}.allocate(scast<usize>(memSize(size)));
            checkOrAbort(_block != nullptr, "SizedArray: allocation failed");
            *rcast<i64*>(_block) = size;
        }

        // ~SizedArray() noexcept {
        //     checkOrAbort(_block != nullptr, "SizedArray::~SizedArray: null pointer");
        //     Allocator<u8>{}.deallocate(_block, scast<usize>(memSize(size())));
        //     _block = nullptr;
        // }
        GFL_HOST_DEVICE i64 size() const noexcept { return *_size(); }
        GFL_HOST_DEVICE T* data() const noexcept { return rcast<T*>(_block + dataOffset()); }
        GFL_HOST_DEVICE T& at(i64 const i) noexcept {
            assert(i >= 0);
            assert(i < size());
            return data()[i];
        }
        GFL_HOST_DEVICE T const & at(i64 const i) const noexcept {
            assert(i >= 0);
            assert(i < size());
            return data()[i];
        }
        GFL_HOST_DEVICE T& operator[](i64 const i) noexcept { return at(i); }
        GFL_HOST_DEVICE T const& operator[](i64 const i) const noexcept { return at(i); }
        GFL_HOST_DEVICE i64 resizeBy(i64 const delta) noexcept {
            assert(size() + delta >= 0);
            *_size() += delta;
            return size();
        }
        GFL_HOST_DEVICE i64 resizeByAtomic(i64 const delta) noexcept {
            assert(size() + delta >= 0);
            static_assert(sizeof(i64) == sizeof(llu), "SizedArray::resizeByAtomic: i64 must be 64-bit");
#ifdef __CUDA_ARCH__
            i64 const oldSize = scast<i64>(atomicAdd(rcast<llu*>(_size()), scast<llu>(delta)));
#else
            i64 const oldSize = __atomic_fetch_add(_size(), delta, __ATOMIC_RELAXED);
#endif
            return oldSize;
        }

#ifdef __CUDACC__
        void prefetchToDevice(cudaStream_t const stream, i32 const device) const noexcept {
            static_assert(std::is_same_v<Allocator<u8>, ManagedAllocator<u8>>, "SizedArray::prefetchToDevice: ManagedAllocator required");
            cudaMemLocation const location = {.type = cudaMemLocationTypeDevice, .id = device};
            cudaError_t const status = cudaMemPrefetchAsync(_block, scast<usize>(memSize(size())), location, 0, stream);
            checkOrAbort(status == cudaSuccess, "SizedArray::prefetchToDevice failed");
        }
        void prefetchToHost(cudaStream_t const stream) const noexcept {
            static_assert(std::is_same_v<Allocator<u8>, ManagedAllocator<u8>>, "SizedArray::prefetchToHost: ManagedAllocator required");
            cudaMemLocation const location = {.type = cudaMemLocationTypeHost, .id = 0};
            cudaError_t const status = cudaMemPrefetchAsync(_block, scast<usize>(memSize(size())), location, 0, stream);
            checkOrAbort(status == cudaSuccess, "SizedArray::prefetchToHost failed");
        }
#endif
    };
}