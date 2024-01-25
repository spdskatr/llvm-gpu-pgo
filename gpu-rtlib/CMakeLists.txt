list(APPEND CMAKE_PREFIX_PATH /opt/rocm)
find_package(hip REQUIRED)

# Use HIPCC.
set(CMAKE_CXX_COMPILER "/opt/rocm/bin/hipcc")
set(CMAKE_CXX_LINKER   "/opt/rocm/bin/hipcc")

add_compile_options(-fgpu-rdc)

add_library(GPURTLib SHARED
    RTlib.cpp
)