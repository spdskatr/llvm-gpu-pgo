// RUN: $HIP_PATH/bin/hipcc %s -fpass-plugin=$GPUPGO_ROOT/build/instrument/GPUInstrPass.so \
// RUN:   -L$GPUPGO_ROOT/build/gpu-rtlib -lGPURTLib -lclang_rt.profile \
// RUN:   -Xoffload-linkeramdgcn-amd-amdhsa $GPUPGO_ROOT/build/gpu-rtlib/rtlib.bc \
// RUN:   -Xoffload-linkeramdgcn-amd-amdhsa -plugin-opt=-amdgpu-early-inline-all=true \
// RUN:   -Xoffload-linkeramdgcn-amd-amdhsa -plugin-opt=-amdgpu-function-calls=false \
// RUN:   -fgpu-rdc -o %t
//
// RUN: LLVM_PROFILE_FILE=%t.profraw %t
//
// RUN: $LLVM_ROOT/bin/llvm-profdata show %t.profraw | FileCheck %s
// CHECK: Maximum function count: 1073741824

#define __HIP_PLATFORM_AMD__
#include "hip/hip_runtime.h"
#include <assert.h>

#define HIP_ASSERT(x) (assert((x) == hipSuccess))

#define WIDTH 32768
#define HEIGHT 32768

#define NUM (WIDTH * HEIGHT)

#define THREADS_PER_BLOCK_X 16
#define THREADS_PER_BLOCK_Y 16
#define THREADS_PER_BLOCK_Z 1

__global__ void dummy_kernel() {}

using namespace std;

int main() {

  hipDeviceProp_t devProp;
  HIP_ASSERT(hipGetDeviceProperties(&devProp, 0));

  hipLaunchKernelGGL(
      dummy_kernel,
      dim3(WIDTH / THREADS_PER_BLOCK_X, HEIGHT / THREADS_PER_BLOCK_Y),
      dim3(THREADS_PER_BLOCK_X, THREADS_PER_BLOCK_Y), 0, 0);

  return 0;
}
