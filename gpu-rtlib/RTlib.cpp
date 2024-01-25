#include <cstdlib>
#define __HIP_PLATFORM_AMD__
#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include <stdlib.h>
#include<iostream>
//#include "hip/hip_runtime_api.h"
#include "hip/hip_runtime.h"
#include "hip/driver_types.h"

#define HIP_ASSERT(x) (assert((x)==hipSuccess))

__device__ void __llvm_profile_register_names_function(void *s) {
    printf("registered names %p\n", s);
}

__global__ void __llvm_profile_register_function(void *s) {
    printf("registered %p\n", s);
}

void test(void) {
    printf("Hello world!\n");
}