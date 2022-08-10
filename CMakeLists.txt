cmake_minimum_required(VERSION 3.5)

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

set(CMAKE_CXX_FLAGS "-Wall -Wsign-compare")
#if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
#   set(CMAKE_CXX_FLAGS_DEBUG "-g -msse4.2")
#   set(CMAKE_CXX_FLAGS_RELEASE "-O3 -msse4.2 -D_NDEBUG -DNDEBUG")  #[[ comment  ]]
#else()
   set(CMAKE_CXX_FLAGS_DEBUG "-g")
   set(CMAKE_CXX_FLAGS_RELEASE "-O3 -D_NDEBUG -DNDEBUG")  #[[ comment  ]]
#endif()

message("FR   : ${CMAKE_CXX_FLAGS_RELEASE}")
message("Build: ${CMAKE_BUILD_TYPE}")

include_directories(.)
add_library(copl
	RuntimeMonitor.cpp
	fail.cpp
	acstr.cpp
	bitset.cpp
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
	mddSeq.cpp
	mddSum.cpp
	mdddelta.cpp
	mddnode.cpp
	mddrelax.cpp
	mddstate.cpp
	regular.cpp
	search.cpp
	solver.cpp
	store.cpp
	table.cpp
	trail.cpp
	trailable.cpp)

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

