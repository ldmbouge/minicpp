#pragma once

#include <cstdlib>
#ifdef __CUDACC__
#include <cuda_runtime.h>
#endif
#include <format>
#include <iostream>

#include "Alias.hpp"
#include "FunQual.hpp"

namespace gfl {
[[noreturn]] GFL_HOST_DEVICE inline void abort() {
#ifdef __CUDA_ARCH__
    __trap();
#else
    std::abort();
#endif
}
template<typename... Args> [[noreturn]] void abortWithMsg(std::format_string<Args...> fmt, Args&&... args) {
    std::cerr << "[ERROR] " << std::format(fmt, std::forward<Args>(args)...) << '\n';
    std::cerr.flush();
    abort();
}
template<typename... Args> void checkOrAbort(bool const condition, std::format_string<Args...> fmt, Args&&... args) {
    if (!condition) abortWithMsg(fmt, std::forward<Args>(args)...);
}
GFL_HOST_DEVICE inline void checkOrAbort(bool const condition) {
    if (!condition) abort();
}
#ifdef __CUDACC__
GFL_HOST_DEVICE inline void checkCudaError(cudaError_t const err, char const* file, i32 const line) {
    if (err != cudaSuccess) {
#ifndef __CUDA_ARCH__
        char const* errStr = cudaGetErrorString(err);
        std::cerr << std::format("[ERROR] CUDA API failed: {} ({}) at {}:{}\n", errStr, scast<i32>(err), file, line);
        std::cerr.flush();
#endif
        abort();
    }
}
#endif
}

#ifdef __CUDACC__
#define CHECK_CUDA_ERROR(err) gfl::checkCudaError(err, __FILE__, __LINE__)
#define CHECK_LAST_CUDA_ERROR() CHECK_CUDA_ERROR(cudaGetLastError())
#endif