# Python bindings (experimental)
pybind11_add_module(minicpp pybridge.cpp)
target_link_libraries(minicpp PRIVATE copl)
