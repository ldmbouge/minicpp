#pragma once

#include <cstddef>
#include <new>
#include <vector>

#include "Algorithms.hpp"
#include "Allocators.hpp"
#include "DebugUtils.hpp"
#include "FunQual.hpp"
#include "Macros.hpp"
#include "MathUtils.hpp"
#include "MemUtils.hpp"

namespace gfl {
class ArenaPool {
    uptr _begin{0};
    uptr _current{0};
    uptr _end{0};

public:
    ArenaPool() noexcept = default;
    GFL_HOST_DEVICE ArenaPool(void* const memory, i64 const size) noexcept : _begin(rcast<uptr>(memory)), _current(_begin), _end(_begin + asSize(size)) {
        checkOrAbort(memory != nullptr);
        checkOrAbort(_begin <= highest<uptr>() - asSize(size));
    }
    ArenaPool(ArenaPool&& o) noexcept : _begin(exchange(o._begin, 0)), _current(exchange(o._current, 0)), _end(exchange(o._end, 0)) {}
    ArenaPool& operator=(ArenaPool&& o) noexcept {
        if (this != &o) {
            _begin = exchange(o._begin, 0);
            _current = exchange(o._current, 0);
            _end = exchange(o._end, 0);
        }
        return *this;
    }
    GFL_DISABLE_COPY(ArenaPool);
    GFL_HOST_DEVICE bool fits(i64 const size) const noexcept { return roundUp<uptr>(_current, DefaultAlign) + asSize(size) <= _end; }
    GFL_HOST_DEVICE void* allocate(i64 const size) noexcept {
        checkOrAbort(fits(size));
        uptr const at = roundUp<uptr>(_current, DefaultAlign);
        _current = at + asSize(size);
        return rcast<void*>(at);
    }
    void deallocate(void*, i64) const noexcept {}
    template<typename T, typename... Args> T* make(Args&&... args) { return new (allocate(as<i64>(sizeof(T)))) T(std::forward<Args>(args)...); }
    GFL_HOST_DEVICE void clear() noexcept { _current = _begin; }
    GFL_HOST_DEVICE void* memory() const noexcept { return rcast<void*>(_begin); }
    GFL_HOST_DEVICE void* current() const noexcept { return rcast<void*>(_current); }
    GFL_HOST_DEVICE i64 used() const noexcept { return as<i64>(_current - _begin); }
    GFL_HOST_DEVICE i64 free() const noexcept { return as<i64>(_end - _current); }
    GFL_HOST_DEVICE i64 total() const noexcept { return as<i64>(_end - _begin); }
};
template<typename Allocator> class DynamicPool {
    static constexpr i64 DefaultArenaSize{4ll * 1024ll * 1024ll};
    std::vector<ArenaPool> _arenas;
    void addArena(i64 const size = DefaultArenaSize) { _arenas.emplace_back(Allocator{}.allocate(size), size); }

public:
    explicit DynamicPool() { addArena(); }
    ~DynamicPool() {
        for (auto& a : _arenas) { Allocator{}.deallocate(a.memory(), a.total()); }
    }
    GFL_DISABLE_COPY_AND_MOVE(DynamicPool);
    void* allocate(i64 const size) {
        if (!_arenas.back().fits(size)) addArena(max<i64>(size + DefaultAlign, DefaultArenaSize));
        return _arenas.back().allocate(size);
    }
    template<typename T> T* allocate(i64 const count = 1) {
        i64 const size = as<i64>(sizeof(T)) * count;
        if (!_arenas.back().fits(size)) addArena(max<i64>(size + DefaultAlign, DefaultArenaSize));
        return scast<T*>(_arenas.back().allocate(size));
    }
    void deallocate(void*, i64) noexcept {}
    template<typename T, typename... Args> T* make(Args&&... args) { return new (allocate(as<i64>(sizeof(T)))) T(std::forward<Args>(args)...); }
    i64 used() const noexcept {
        return fold(_arenas, i64{0}, [](i64 const acc, ArenaPool const& p) { return acc + p.used(); });
    }
    i64 free() const noexcept {
        return fold(_arenas, i64{0}, [](i64 const acc, ArenaPool const& p) { return acc + p.free(); });
    }
    i64 total() const noexcept {
        return fold(_arenas, i64{0}, [](i64 const acc, ArenaPool const& p) { return acc + p.total(); });
    }
};

using HeapPool = DynamicPool<HeapAllocator>;
#ifdef __CUDACC__
using DevicePool = DynamicPool<DeviceAllocator>;
using HostPool = DynamicPool<HostAllocator>;
using ManagedPool = DynamicPool<ManagedAllocator>;
#endif
}

inline void* operator new(std::size_t size, gfl::ArenaPool& pool) { return pool.allocate(gfl::as<gfl::i64>(size)); }
inline void* operator new[](std::size_t size, gfl::ArenaPool& pool) { return pool.allocate(gfl::as<gfl::i64>(size)); }
inline void operator delete(void*, gfl::ArenaPool&) noexcept {}
inline void operator delete[](void*, gfl::ArenaPool&) noexcept {}

template<typename Allocator> void* operator new(std::size_t size, gfl::DynamicPool<Allocator>& pool) { return pool.allocate(gfl::as<gfl::i64>(size)); }
template<typename Allocator> void* operator new[](std::size_t size, gfl::DynamicPool<Allocator>& pool) { return pool.allocate(gfl::as<gfl::i64>(size)); }
template<typename Allocator> void operator delete(void*, gfl::DynamicPool<Allocator>&) noexcept {}
template<typename Allocator> void operator delete[](void*, gfl::DynamicPool<Allocator>&) noexcept {}