#ifndef __ARENAALLOCATOR_H
#define __ARENAALLOCATOR_H

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "Align.hpp"
#include "Types.hpp"
#include "FunQual.hpp"
#include "Backend.hpp"
#include "MathUtils.hpp"
#include "Memory.hpp"

namespace gfl
{
    class ArenaAllocator
    {
    protected:
        uptr const begin {0};
        uptr current {0};
        uptr const end {0};

    public:
        ArenaAllocator() = default;

        ArenaAllocator(ArenaAllocator const &) = delete;
        ArenaAllocator(ArenaAllocator &&) = delete;
        ArenaAllocator& operator=(ArenaAllocator const &) = delete;
        ArenaAllocator& operator=(ArenaAllocator &&) = delete;

        GFL_HOST_DEVICE
        ArenaAllocator(i64 const size, u8 * memory) noexcept :
            begin(rcast<uptr>(memory)),
            current(begin),
            end(begin + scast<uptr>(size))
        {
            assert(memory != nullptr);
            assert(size > 0);
            assert(begin <= numeric_limits<uptr>::max() - scast<uptr>(size)); // Overflow
        }

        template<typename T>
        GFL_HOST_DEVICE
        T * allocate(i64 const count, i32 const align) noexcept
        {
            uptr const size = sizeof(T) * scast<uptr>(count);
            uptr const memory = roundUp<uptr>(current, align);
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
            return rcast<T*>(memory);
        }

        template<typename T>
        GFL_HOST_DEVICE
        T * allocate(i64 const count = 1) noexcept
        {
            i32 const align = max<i32>(alignof(T), DefaultAlign);
            return allocate<T>(count, align);
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
//   class Region {
//     gfl::u8* _startCPU;
//     gfl::u8* _endCPU;
//     gfl::u8* _startGPU;
//     gfl::u8* _endGPU;
//     Region(gfl::u8* sC,gfl::u8* eC,gfl::u8* sG,gfl::u8* eG) : _startCPU(sC),_endCPU(eC),_startGPU(sG),_endGPU(eG) {}
//   public:
//     Region() : _startCPU(nullptr),_endCPU(nullptr),_startGPU(nullptr),_endGPU(nullptr) {}
//     friend class MPool;
//     void copyToDevice(cudaStream_t cus) {
//       cudaMemcpyAsync(_startGPU,_startCPU,_endCPU-_startCPU,cudaMemcpyHostToDevice,cus);
//       CHECK_LAST_CUDA_ERROR();
//     }
//     void copyToHost(cudaStream_t cus) {
//       cudaMemcpyAsync(_startCPU,_startGPU,_endCPU-_startCPU,cudaMemcpyDeviceToHost,cus);
//       CHECK_LAST_CUDA_ERROR();
//     }
//   };
//   class MPool {
//   public:
//     typedef MPool* Ptr;
//     MPool(const i64 sz) : cpu(sz,cudaReserveHost(sz)),
// 			  gpu(sz,cudaReserveDevice(sz)) {}
//     ~MPool() {
//       cudaReleaseDevice(gpu.mem());
//       cudaReleaseHost(cpu.mem());
//     }
//     ArenaAllocator cpu;
//     ArenaAllocator gpu;
//     template <typename Block> Region record(Block b) {
//       auto cpuBEG = cpu.freeMem();
//       auto gpuBEG = gpu.freeMem();
//       b();
//       return Region { cpuBEG,cpu.freeMem(),gpuBEG,gpu.freeMem()};
//     }
//   };
// }
//
// // Placement new overloads for ArenaAllocator
// inline void * operator new(std::size_t const size, gfl::ArenaAllocator& allocator) {
//     return allocator.allocate<gfl::u8>(gfl::scast<gfl::i64>(size), gfl::DefaultAlign);
// }
//
// inline void * operator new[](std::size_t const size, gfl::ArenaAllocator& allocator) {
//     return allocator.allocate<gfl::u8>(gfl::scast<gfl::i64>(size), gfl::DefaultAlign);
// }
//
// // Required matching deletes (no-op, arena owns the memory)
// inline void operator delete(void*, gfl::ArenaAllocator&) noexcept {}
// inline void operator delete[](void*, gfl::ArenaAllocator&) noexcept {}

#endif
