cmake_minimum_required(VERSION 3.28)

project(miniCPP VERSION 1.1 LANGUAGES CXX)

# Build type
################################################################################
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Options
################################################################################
option(USE_CXX_EXCEPTIONS "Signal failures with C++ exceptions instead of setjmp/longjmp" OFF)
option(ENABLE_GPU "Enable GPU support (CUDA)" OFF)
option(BUILD_EXAMPLES "Build the examples" ON)
option(BUILD_TESTING "Build the unit tests" ON)
set(BUILD_PYTHON_BINDINGS AUTO CACHE STRING "Build the experimental Python bindings (ON, OFF or AUTO)")
set_property(CACHE BUILD_PYTHON_BINDINGS PROPERTY STRINGS AUTO ON OFF)
message(STATUS "USE_CXX_EXCEPTIONS: ${USE_CXX_EXCEPTIONS}")
message(STATUS "ENABLE_GPU: ${ENABLE_GPU}")

# C++
################################################################################
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Executables at the top of the build tree, next to the copied data/ folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

# CUDA
################################################################################
if(ENABLE_GPU)
    # Default to the local GPU unless set with -DCMAKE_CUDA_ARCHITECTURES=...
    # or the CUDAARCHS environment variable. Must come before enable_language.
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES AND NOT DEFINED ENV{CUDAARCHS})
        set(CMAKE_CUDA_ARCHITECTURES native)
    endif()
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)

    set(CMAKE_CUDA_STANDARD 20)             # handle.hpp uses C++20 requires-clauses (nvcc >= 12)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

    string(APPEND CMAKE_CUDA_FLAGS_DEBUG " -g -G")
    string(APPEND CMAKE_CUDA_FLAGS_RELWITHDEBINFO " -lineinfo")
    # In CMAKE_CUDA_FLAGS so it also reaches the device-link step
    string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w")
endif()

# Dependencies (open-source, header-only)
################################################################################
include(FetchContent)
FetchContent_Declare(
        json                                # JSON parsing
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.12.0
        GIT_SHALLOW    TRUE
)
FetchContent_Declare(
        cli11                               # command-line parsing
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG        v2.6.2
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json cli11)

# Warnings (after the dependencies, so they only apply to this project)
################################################################################
add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wsign-compare>
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-but-set-variable>
        $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:AppleClang,Clang>>:-Wno-vla-cxx-extension>
        $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=68,128,177,550,815,830,997,2417>
)

# Targets
################################################################################
add_subdirectory(src)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    enable_testing()                        # run with ctest from the build dir
    add_subdirectory(tests)
endif()

# Instances: link data/ into the build tree so that the executables and the
# scripts in benchmarks/ can open "data/..." when run from there. Falls back
# to a copy where symlinks are not available.
set(DATA_LINK ${PROJECT_BINARY_DIR}/data)
if(IS_DIRECTORY ${DATA_LINK} AND NOT IS_SYMLINK ${DATA_LINK})
    file(REMOVE_RECURSE ${DATA_LINK})       # stale copy from an older build
endif()
file(CREATE_LINK ${PROJECT_SOURCE_DIR}/data ${DATA_LINK} SYMBOLIC RESULT DATA_LINK_RESULT)
if(NOT DATA_LINK_RESULT EQUAL 0)
    file(COPY ${PROJECT_SOURCE_DIR}/data DESTINATION ${PROJECT_BINARY_DIR})
endif()

if(BUILD_PYTHON_BINDINGS STREQUAL "AUTO")
    find_package(pybind11 CONFIG QUIET)
    if(pybind11_FOUND)
        add_subdirectory(bindings/python)
    else()
        message(STATUS "pybind11 not found: skipping the Python bindings")
    endif()
elseif(BUILD_PYTHON_BINDINGS)
    find_package(pybind11 CONFIG REQUIRED)
    add_subdirectory(bindings/python)
endif()

# Documentation
################################################################################
find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(DOXYGEN_FOUND)
    FetchContent_Declare(
            doxygen-awesome-css
            GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
            GIT_TAG        v2.3.4
            GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(doxygen-awesome-css)
    FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)

    # Graphs only when Graphviz is installed
    if(DOXYGEN_DOT_FOUND)
        set(DOXYGEN_HAVE_DOT YES)
    else()
        set(DOXYGEN_HAVE_DOT NO)
    endif()

    # Settings live in docs/Doxyfile.in; HTML goes to <build>/docs/html
    configure_file(docs/Doxyfile.in Doxyfile @ONLY)
    add_custom_target(docs
            COMMAND Doxygen::doxygen ${PROJECT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
    )
else()
    message(STATUS "Doxygen not found: the docs target is not available")
endif()
