# One executable per source file, named after the file
file(GLOB EXAMPLE_SRC CONFIGURE_DEPENDS
        basic/*.cpp
        scheduling/*.cpp
        mdd/*.cpp
        tutorial/*.cpp
)

# Depends on a cont.hpp that no longer exists
list(FILTER EXAMPLE_SRC EXCLUDE REGEX "/basic/mainCont\\.cpp$")

if(ENABLE_GPU)
    file(GLOB GPU_EXAMPLE_SRC CONFIGURE_DEPENDS gpu/*.cu)
    list(APPEND EXAMPLE_SRC ${GPU_EXAMPLE_SRC})
endif()

foreach(src IN LISTS EXAMPLE_SRC)
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE copl nlohmann_json::nlohmann_json CLI11::CLI11)
endforeach()

list(LENGTH EXAMPLE_SRC EXAMPLE_COUNT)
message(STATUS "Examples: ${EXAMPLE_COUNT} targets")
