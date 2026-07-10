#pragma once

#include <type_traits>
#include <cstring>
#include "SizedArray.hpp"
#include "Memory.hpp"

namespace gfl
{

    template<typename T, template<typename> class Allocator = HeapAllocator>
    class Vector {
        static_assert(std::is_trivially_copyable_v<T>, "Vector<T>: T must be trivially copyable");
        i64 _capacity;
        SizedArray<T,Allocator> _sa;
    public:
        Vector(Vector const&) noexcept = default;
        Vector& operator=(Vector const&) noexcept = default;
        explicit Vector(i64 const capacity) noexcept : _capacity(capacity), _sa(_capacity) {}
        GFL_HOST_DEVICE void clear() noexcept { _sa.resizeBy(-_sa.size());}
        GFL_HOST_DEVICE i64 size() const noexcept { return _sa.size(); }
        GFL_HOST_DEVICE i64 capacity() const noexcept { return _capacity; }
        GFL_HOST_DEVICE T* data() noexcept { return _sa.data(); }
        GFL_HOST_DEVICE T const* data() const noexcept { return _sa.data(); }
        GFL_HOST_DEVICE T& at(i64 const i) noexcept { return _sa.at(i); }
        GFL_HOST_DEVICE T const& at(i64 const i) const noexcept { return _sa.at(i); }
        GFL_HOST_DEVICE T& operator[](i64 const i) noexcept { return _sa[i]; }
        GFL_HOST_DEVICE T const& operator[](i64 const i) const noexcept { return _sa[i]; }
        GFL_HOST_DEVICE void pushBackAtomic(ArrayView<T> const & items) noexcept {
            i64 const n = items.size();
            assert(_sa.size() + n <= _capacity);
            i64 const oldSize = _sa.resizeByAtomic(items.size());
            memcpy(&_sa[oldSize], items.data(), items.dataMemSize());
        }

#ifdef __CUDACC__
        void prefetchToHost(cudaStream_t const stream) const noexcept { _sa.prefetchToHost(stream); }
        void prefetchToDevice(cudaStream_t const stream, i32 const device) const noexcept { _sa.prefetchToDevice(stream,device);}
#endif
    };
}