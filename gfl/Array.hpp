#pragma once

#include <type_traits>
#include "ArrayView.hpp"
#include "Memory.hpp"

namespace gfl
{
    template<typename T, typename MemoryPool>
    class Array : public ArrayView<T> {
        static_assert(std::is_trivially_copyable_v<T>, "Array<T>: T must be trivially copyable");
        using ArrayView<T>::size_;
        using ArrayView<T>::data_;
    public:
        Array(Array const&) = delete;
        Array& operator=(Array const&) = delete;
        explicit Array(i64 const size, MemoryPool & p) noexcept : ArrayView<T>(size, new (p) T[size]) {}
        ~Array() noexcept {}
#ifdef __CUDACC__
        void prefetchToHost(cudaStream_t const stream = 0) const noexcept {
             cudaMemLocation const location = {.type = cudaMemLocationTypeHost, .id = 0};
            cudaError_t const status = cudaMemPrefetchAsync(data_, this->dataMemSize(), location, 0, stream);
            checkOrAbort(status == cudaSuccess, "Vector::prefetchToHost failed");
        }
        void prefetchToDevice(cudaStream_t const stream = 0, i32 const device = 0) const noexcept {
            cudaMemLocation const location = {.type = cudaMemLocationTypeDevice, .id = device};
            cudaError_t const status = cudaMemPrefetchAsync(data_, this->dataMemSize(), location, 0, stream);
            checkOrAbort(status == cudaSuccess, "Vector::prefetchToDevice failed");
        }
#endif
    };
}