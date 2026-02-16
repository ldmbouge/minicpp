cmake_minimum_required(VERSION 3.28)

project(miniCPP VERSION 1.1)

configure_file(miniCPP.h.in miniCPP.h)

# ------------------------------------------------------------------------
# Let's go for Debug by default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
# ------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(USE_CXX_EXCEPTIONS "Enable C++ exception support" ON)

# Dependencies
# 3rd party open-source library to (header only!)
# 1. Parse JSON Files
# 2. Formatting of outputs (Will be in the C++ 26 version)
# In use whether we use the GPU or not.
# 
include(FetchContent)
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json
        GIT_TAG        master
)
#FetchContent_Declare(
#        fmt
#        GIT_REPOSITORY https://github.com/fmtlib/fmt
#        GIT_TAG        master
#)
FetchContent_MakeAvailable(json)
#FetchContent_MakeAvailable(fmt)


# experimental
#set(CXX /opt/homebrew/Cellar/llvm/18.1.8/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

set(CMAKE_CXX_FLAGS "-fPIC -Wall -Wsign-compare -Wno-vla-cxx-extension -Wno-unused-but-set-variable")
#if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
#   set(CMAKE_CXX_FLAGS_DEBUG "-g -msse4.2")
#   set(CMAKE_CXX_FLAGS_RELEASE "-O3 -msse4.2 -D_NDEBUG -DNDEBUG")  #[[ comment  ]]
#else()
   set(CMAKE_CXX_FLAGS_DEBUG "-g")
   set(CMAKE_CXX_FLAGS_RELEASE "-O3 -D_NDEBUG -DNDEBUG")  #  -target wasm64 -nostdlib -Wl,--no-entry -Wl,--export-all
#endif()

message("FR   : ${CMAKE_CXX_FLAGS_RELEASE}")
message("Build: ${CMAKE_BUILD_TYPE}")

# GPU support
option(GPU "Enable GPU support" OFF)
message(STATUS "GPU support: ${GPU}")
if(GPU)
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)

    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_ARCHITECTURES "native")
    set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

    string(APPEND CMAKE_CUDA_FLAGS_DEBUG " -g -G")
    string(APPEND CMAKE_CUDA_FLAGS_RELWITHDEBINFO " -lineinfo")

    # Suppress warnings
    string(APPEND CMAKE_CUDA_FLAGS " --diag-suppress 177")
    string(APPEND CMAKE_CUDA_FLAGS " --diag-suppress 550")
    string(APPEND CMAKE_CUDA_FLAGS " --diag-suppress 815")
    string(APPEND CMAKE_CUDA_FLAGS " --diag-suppress 830")
    string(APPEND CMAKE_CUDA_FLAGS " --diag-suppress 997")
    string(APPEND CMAKE_CUDA_FLAGS " --diag-suppress 68")
    string(APPEND CMAKE_CUDA_FLAGS " --diag-suppress 128")
    string(APPEND CMAKE_CUDA_FLAGS " --diag-suppress 2417")
endif()

set(GPU_CONSTR_SRC
    gpu_constraints/cumulative.cu
)

set(GLB_CONSTR_SRC
  global_constraints/cumulative.cpp
)

set(GPU_EXAMPLES
  gpu_examples/rcpsp.cu
)

# Auxiliary functions
function(add_executables DIR FILTER LIBS)
    string(TOUPPER "${DIR}" DIR_UPPER)
    set(DIR_MSG "${DIR_UPPER}: ")
    file(GLOB DIR_SRC "${PROJECT_SOURCE_DIR}/${DIR}/${FILTER}")
    foreach (EXE_SRC ${DIR_SRC})
        get_filename_component(EXE_NAME ${EXE_SRC} NAME_WE)
        set(DIR_MSG "${DIR_MSG} ${EXE_NAME}")
        add_executable(${EXE_NAME} ${EXE_SRC})
        foreach (LIB ${LIBS})
            target_link_libraries(${EXE_NAME} ${LIB})
        endforeach()
    endforeach()
    message(STATUS ${DIR_MSG})
endfunction()

include_directories(.)

add_library(copl
	RuntimeMonitor.cpp
	fail.cpp
	acstr.cpp
	bitset.cpp
        commandList.cpp
	constraint.cpp
	domain.cpp
	fz_constraints/bool_array.cpp
	fz_constraints/bool_bin.cpp
	fz_constraints/bool_lin.cpp
	fz_constraints/bool_misc.cpp
	fz_constraints/flatzinc.cpp
	fz_constraints/int_array.cpp
	fz_constraints/int_bin.cpp
	fz_constraints/int_lin.cpp
	fz_constraints/int_tern.cpp
	fz_parser/flatzinc.cpp
	fz_parser/lexer.yy.cpp
	fz_parser/parser.tab.cpp
	fz_parser/registry.cpp
	intvar.cpp
	lex.cpp
	mallocWatch.cpp
	matching.cpp
	mdd.cpp
	mddAlldiff.cpp
	mddAmong.cpp
	mddGcc.cpp
	mddAbs.cpp
	mddMaximumCutObjective.cpp
    mddPrecedence.cpp
	mddGoc.cpp
	mddSeq.cpp
	mddSum.cpp
    mddTSPSum.cpp
	mdddelta.cpp
	mddnode.cpp
	mddrelax.cpp
	mddstate.cpp
	regular.cpp
	search.cpp
	solver.cpp
	store.cpp
	table.cpp
    tracer.cpp
	trail.cpp
	trailable.cpp
    ${GLB_CONSTR_SRC}
)

if (GPU)
    target_sources(copl PRIVATE ${GPU_CONSTR_SRC})
    target_link_libraries(copl PRIVATE CUDA::cudart)
    target_include_directories(copl PRIVATE {CUDAToolkit_INCLUDE_DIRS})

    add_executables("gpu_examples" "*.cu" "copl;nlohmann_json::nlohmann_json")
endif()

add_library(fzl
		fz_constraints/bool_array.cpp
		fz_constraints/bool_bin.cpp
		fz_constraints/bool_lin.cpp
		fz_constraints/bool_misc.cpp
		fz_constraints/flatzinc.cpp
		fz_constraints/int_array.cpp
		fz_constraints/int_bin.cpp
		fz_constraints/int_lin.cpp
		fz_constraints/int_tern.cpp
		fz_parser/flatzinc.cpp
		fz_parser/lexer.yy.cpp
		fz_parser/parser.tab.cpp
		fz_parser/registry.cpp
		)

#This is the full list of exapmles for miniCPP		
################################################################################
file(GLOB files "examples/*.cpp" "examples/mdd/*.cpp" "examples/tutorial/*.cpp")
#file(GLOB files "examples/tutorial/*.cpp" "examples/mdd/amongNurse.cpp" "examples/mdd/amongNurse2.cpp")
list(FILTER files EXCLUDE REGEX ".*/examples/mainCont.cpp")
list(FILTER files EXCLUDE REGEX ".*/examples/fz.cpp")
message("${files}")

foreach (file ${files})
	get_filename_component(base ${file} NAME_WE)
	get_filename_component(dir ${file} DIRECTORY)
	message("BASE IS: [${base}]")
	add_executable(${base} ${dir}/${base}.cpp)
	target_link_libraries(${base} copl)
	add_dependencies(${base} copl)
endforeach()

# This is to compile the minizinc interface to miniCPP
################################################################################
add_executable(fzn-minicpp examples/fz.cpp)
target_link_libraries(fzn-minicpp copl fzl)

# Data files for the benchmarks
################################################################################
file(COPY "examples/data" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Python3 bindings (Experimental)
################################################################################
#[[ add_subdirectory(pybind11)  ]]

find_package(pybind11 REQUIRED)
pybind11_add_module(minicpp pybridge.cpp)
target_link_libraries(minicpp PRIVATE copl)
add_dependencies(minicpp copl)

