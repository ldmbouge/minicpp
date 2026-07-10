#pragma once

#include <type_traits>
#include "VectorView.hpp"
#include "Memory.hpp"

namespace gfl
{
    template<typename T, typename Allocator = HeapAllocator<T>>
    class Vector : public VectorView<T> {
        static_assert(std::is_trivially_copyable_v<T>, "Vector<T>: T must be trivially copyable");
        using VectorView<T>::data_;
        using VectorView<T>::capacity_;
    public:
        Vector(Vector const&) = delete;
        Vector& operator=(Vector const&) = delete;
        explicit Vector(i64 const capacity) noexcept : VectorView<T>(capacity, Allocator{}.allocate(scast<usize>(capacity))) {}
        ~Vector() noexcept { Allocator{}.deallocate(data_, scast<usize>(capacity_)); }
#ifdef __CUDACC__
        void prefetchToHost(cudaStream_t const stream = 0) const noexcept {
            static_assert(std::is_same_v<Allocator, ManagedAllocator<T>>, "prefetchToHost: requires ManagedAllocator");
            cudaMemLocation const location = {.type = cudaMemLocationTypeHost, .id = 0};
            cudaError_t const status = cudaMemPrefetchAsync(data_, this->dataMemSize(), location, 0, stream);
            checkOrAbort(status == cudaSuccess, "Vector::prefetchToHost failed");
        }
        void prefetchToDevice(cudaStream_t const stream = 0, i32 const device = 0) const noexcept {
            static_assert(std::is_same_v<Allocator, ManagedAllocator<T>>, "prefetchToDevice: requires ManagedAllocator");
            cudaMemLocation const location = {.type = cudaMemLocationTypeDevice, .id = device};
            cudaError_t const status = cudaMemPrefetchAsync(data_, this->dataMemSize(), location, 0, stream);
            checkOrAbort(status == cudaSuccess, "Vector::prefetchToDevice failed");
        }
#endif
    };
}