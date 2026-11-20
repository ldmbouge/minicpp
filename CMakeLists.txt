cmake_minimum_required(VERSION 4.2)

project(MiniCPP
        VERSION 1.1
        DESCRIPTION "CP Solver in C++"
        LANGUAGES CXX
)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# Options
option(BUILD_DOCS "Build the documentation (needs Doxygen)" OFF)
option(BUILD_EXAMPLES "Build the examples" OFF)
option(BUILD_PYTHON_BINDINGS "Build the Python bindings" OFF)
option(BUILD_TESTING "Build the unit tests" OFF)
option(ENABLE_GPU "Enable GPU support (CUDA)" OFF)
option(USE_CXX_EXCEPTIONS "Signal failures with C++ exceptions" OFF)
message(STATUS "BUILD_DOCS: ${BUILD_DOCS}")
message(STATUS "BUILD_EXAMPLES: ${BUILD_EXAMPLES}")
message(STATUS "BUILD_PYTHON_BINDINGS: ${BUILD_PYTHON_BINDINGS}")
message(STATUS "BUILD_TESTING: ${BUILD_TESTING}")
message(STATUS "ENABLE_GPU: ${ENABLE_GPU}")
message(STATUS "USE_CXX_EXCEPTIONS: ${USE_CXX_EXCEPTIONS}")

# C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
string(APPEND CMAKE_CXX_FLAGS " -Wall -Wsign-compare -Wno-unused-but-set-variable")

# CUDA
if(ENABLE_GPU)
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES native)
    endif()
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
    string(APPEND CMAKE_CUDA_FLAGS_DEBUG " -g -G")
    string(APPEND CMAKE_CUDA_FLAGS_RELWITHDEBINFO " -lineinfo")
    string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w") # Suppress link warnings
    string(APPEND CMAKE_CUDA_FLAGS " --diag-suppress=68,128,177,550,815,830,997,2417")
endif()

# Dependencies (open-source)
include(FetchContent)
FetchContent_Declare(
        json                                # JSON parsing
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.12.0
        GIT_SHALLOW    TRUE
)
FetchContent_Declare(
        cli11                               # Command-line parsing
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG        v2.7.2
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json cli11)

add_subdirectory(src)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    enable_testing()                        # run with ctest from the build dir
    add_subdirectory(tests)
endif()

if(BUILD_PYTHON_BINDINGS)
    find_package(pybind11 CONFIG REQUIRED)
    add_subdirectory(bindings/python)
endif()

# Documentation
if(BUILD_DOCS)
    FetchContent_Declare(
            doxygen-awesome-css
            GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
            GIT_TAG        v2.5.0
            GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(doxygen-awesome-css)
    FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)
    find_package(Doxygen REQUIRED OPTIONAL_COMPONENTS dot)
    configure_file(docs/Doxyfile.in Doxyfile @ONLY)
    add_custom_target(docs
            COMMAND Doxygen::doxygen ${PROJECT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            VERBATIM
    )
endif()
