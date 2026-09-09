#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <span>
#include <type_traits>
#include "Mirror.hpp"

namespace gfl {
template<typename T, typename Ptr = T*> class ArrayView {
    static_assert(std::is_trivially_copyable_v<T>);
    Ptr _data{nullptr};
    i64 _size{0};

public:
    using value_type = T;
    using pointer = Ptr;
    ArrayView() noexcept = default;
    GFL_HOST_DEVICE ArrayView(Ptr const data, i64 const size) noexcept : _data(data), _size(size) {
        assert(size >= 0);
        assert(data != nullptr);
    }
    GFL_HOST_DEVICE ArrayView(Ptr const begin, Ptr const end) noexcept : ArrayView(begin, gfl::as<i64>(end - begin)) {}
    template<typename Pool> ArrayView(Pool& pool, i64 const count) noexcept : ArrayView(pool.template allocate<T>(count), count) {}
    GFL_HOST_DEVICE T const* data() const noexcept { return _data; }
    GFL_HOST_DEVICE T* data() noexcept { return _data; }
    GFL_HOST_DEVICE i64 size() const noexcept { return _size; }
    GFL_HOST_DEVICE i64 sizeBytes() const noexcept { return gfl::as<i64>(sizeof(T)) * _size; }
    GFL_HOST_DEVICE T const* begin() const noexcept { return data(); }
    GFL_HOST_DEVICE T* begin() noexcept { return data(); }
    GFL_HOST_DEVICE T const* end() const noexcept { return data() + _size; }
    GFL_HOST_DEVICE T* end() noexcept { return data() + _size; }
    GFL_HOST_DEVICE T const& at(i64 const i) const noexcept {
        assert(i >= 0);
        assert(i < _size);
        return _data[i];
    }
    GFL_HOST_DEVICE T& at(i64 const i) noexcept {
        assert(i >= 0);
        assert(i < _size);
        return _data[i];
    }
    GFL_HOST_DEVICE T const& operator[](i64 const i) const noexcept { return at(i); }
    GFL_HOST_DEVICE T& operator[](i64 const i) noexcept { return at(i); }
    GFL_HOST_DEVICE T const& front() const noexcept {
        assert(_size > 0);
        return _data[0];
    }
    GFL_HOST_DEVICE T& front() noexcept {
        assert(_size > 0);
        return _data[0];
    }
    GFL_HOST_DEVICE T const& back() const noexcept {
        assert(_size > 0);
        return _data[_size - 1];
    }
    GFL_HOST_DEVICE T& back() noexcept {
        assert(_size > 0);
        return _data[_size - 1];
    }
    GFL_HOST_DEVICE ArrayView<T, Ptr> slice(i64 const begin, i64 const end) const noexcept {
        assert(0 <= begin && 0 <= end);
        assert(begin <= _size && end <= _size);
        if (begin >= end) return {_data + _size, i64{0}};
        return {_data + begin, end - begin};
    }
    GFL_HOST_DEVICE void swap(ArrayView& o) noexcept {
        ArrayView const tmp = *this;
        *this = o;
        o = tmp;
    }
    GFL_HOST_DEVICE ArrayView<T, Ptr> view() const noexcept { return slice(0, _size); }
    void fill(T const& value) noexcept { std::fill(begin(), end(), value); }
    void loadFrom(std::initializer_list<T> const il) noexcept {
        assert(gfl::as<i64>(il.size()) == _size);
        std::memcpy(data(), il.begin(), sizeof(T) * gfl::asSize(_size));
    }
    void loadFrom(std::span<T const> const src) noexcept {
        assert(gfl::as<i64>(src.size()) == _size);
        std::memcpy(data(), src.data(), sizeof(T) * gfl::asSize(_size));
    }
    void dumpTo(std::span<T> const dst) const noexcept {
        assert(_size <= gfl::as<i64>(dst.size()));
        std::memcpy(dst.data(), data(), sizeof(T) * gfl::asSize(_size));
    }
    template<typename U> GFL_HOST_DEVICE ArrayView<U> as() const noexcept requires std::is_same_v<T, u8> && std::is_same_v<Ptr, u8*>
    {
        return {rcast<U*>(_data), gfl::as<i64>(sizeBytes() / sizeof(U))};
    }
    operator std::span<T>() noexcept requires std::is_same_v<Ptr, T*>
    {
        return {data(), gfl::asSize(_size)};
    }
    operator std::span<T const>() const noexcept requires std::is_same_v<Ptr, T*>
    {
        return {data(), gfl::asSize(_size)};
    }
    GFL_DEVICE void print() const noexcept {
        printf("[");
        for (auto i = 0; i < _size; ++i) {
            if (i > 0) printf(" ");
            T const& v = at(i);
            if constexpr (std::is_floating_point_v<T>)
                printf("%.2f", gfl::as<f64>(v));
            else if constexpr (std::is_signed_v<T>)
                printf("%lld", gfl::as<lld>(v));
            else if constexpr (isUnsigned<T>)
                printf("%llu", gfl::as<llu>(v));
            else
                v.print();
        }
        printf("]\n");
    }
};
#ifdef __CUDACC__
template<typename T> using MirrorArrayView = ArrayView<T, MirrorPtr<T>>;
#endif
}