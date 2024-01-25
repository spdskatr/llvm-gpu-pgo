list(APPEND CMAKE_PREFIX_PATH /opt/rocm)
find_package(hip REQUIRED)

# Use HIPCC.
set(CMAKE_CXX_COMPILER "/opt/rocm/bin/hipcc")
set(CMAKE_CXX_LINKER   "/opt/rocm/bin/hipcc")

add_compile_options(-fgpu-rdc)

set(BITCODE_NAME rtlib)
# set(FILES RTlib.cpp)

file(GLOB_RECURSE FILES "${CMAKE_SOURCE_DIR}/gpu-rtlib/*.cpp")

add_custom_command(OUTPUT ${BITCODE_NAME}.bc
    COMMAND ${CMAKE_CXX_COMPILER}
    ARGS ${FILES} -emit-llvm --cuda-device-only -o ${BITCODE_NAME}.bc
    DEPENDS ${FILES}
    COMMENT "Generating LLVM bitcode ${BITCODE_NAME}.bc"
    VERBATIM)

add_custom_target(RTBitCode DEPENDS ${BITCODE_NAME}.bc)

add_library(GPURTLib SHARED
    ${FILES}
)

add_dependencies(GPURTLib RTBitCode)