# GFL -- GPU Friendly Library
# Header-only, dependency-free. CUDA usage is guarded by __CUDACC__, so the headers compile both as plain C++ and through nvcc.
add_library(gfl INTERFACE)
add_library(gfl::gfl ALIAS gfl)
target_include_directories(gfl INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>)
target_compile_features(gfl INTERFACE cxx_std_20)