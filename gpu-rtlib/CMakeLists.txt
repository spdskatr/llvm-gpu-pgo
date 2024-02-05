list(APPEND CMAKE_PREFIX_PATH /opt/rocm)
find_package(hip REQUIRED)

# This is so we can find that singular file InstrProfData.inc that we need
include_directories("/opt/rocm/llvm/lib/clang/17.0.0/include")

# CMake for generating bitcode files fails when multiple targets exist
# --offload-arch doesn't matter, the LLVM IR generated is the same for this project
set(BITCODE_NAME rtlib)
add_custom_command(OUTPUT ${BITCODE_NAME}.bc
    COMMAND "/opt/rocm/bin/hipcc"
    ARGS "${CMAKE_CURRENT_SOURCE_DIR}/DeviceLib.cpp" -emit-llvm --cuda-device-only --offload-arch=gfx1100 -fgpu-rdc -o ${BITCODE_NAME}.bc
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/DeviceLib.cpp"
    COMMENT "Generating LLVM bitcode ${BITCODE_NAME}.bc"
    VERBATIM)
add_custom_target(RTBitCode DEPENDS ${BITCODE_NAME}.bc)

add_library(GPURTLib STATIC
    HostLib.cpp
)

add_dependencies(GPURTLib RTBitCode)