add_library(GPURTLib SHARED
    RTlib.cpp
)
set(CMAKE_CXX_COMPILER "/opt/rocm/bin/hipcc")
set(CMAKE_CXX_LINKER   "/opt/rocm/bin/hipcc")