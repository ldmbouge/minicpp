cmake_minimum_required(VERSION 3.10)

project(miniCPP VERSION 1.0)

configure_file(miniCPP.h.in miniCPP.h)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
option(USE_CXX_EXCEPTIONS "Enable C++ exception support" ON)

set(CMAKE_CXX_FLAGS "-Wall -Wsign-compare")
set(CMAKE_CXX_FLAGS_DEBUG "-g -msse4.2")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -msse4.2 -D_NDEBUG -DNDEBUG")  #[[ comment  ]]

include_directories(.)
add_library(copl RuntimeMonitor.cpp mdd.cpp mddrelax.cpp mdddelta.cpp
	mddAlldiff.cpp mddAmong.cpp mddGcc.cpp mddSeq.cpp mddSum.cpp
	store.cpp bitset.cpp table.cpp literal.cpp explainer.cpp expTrail.cpp
	acstr.cpp domain.cpp mddnode.cpp constraint.cpp     
	intvar.cpp mddstate.cpp trail.cpp mallocWatch.cpp    
	search.cpp trailable.cpp matching.cpp solver.cpp)

file(GLOB files "examples/*.cpp" "examples/mdd/*.cpp")
list(FILTER files EXCLUDE REGEX ".*/examples/mainCont.cpp")
message("${files}")
foreach (file ${files})
	get_filename_component(base ${file} NAME_WE)
	get_filename_component(dir ${file} DIRECTORY)
	add_executable(${base} ${dir}/${base}.cpp)
	target_link_libraries(${base} copl)
	add_dependencies(${base} copl)
endforeach()

file(COPY "examples/data" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
