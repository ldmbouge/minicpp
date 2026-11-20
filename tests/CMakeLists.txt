# Unit tests: one executable per source file, registered with CTest.
# A test passes when its executable exits with 0 (failed asserts abort).
file(GLOB TEST_SRC CONFIGURE_DEPENDS *.cpp)

foreach(src IN LISTS TEST_SRC)
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE copl)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES TIMEOUT 120 LABELS unit)
endforeach()

list(LENGTH TEST_SRC TEST_COUNT)
message(STATUS "Tests: ${TEST_COUNT} targets")
