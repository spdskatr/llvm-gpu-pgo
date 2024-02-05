#include <cstdio>
#include <cstdlib>
#include <cassert>
#define __HIP_PLATFORM_AMD__
#include "hip/hip_runtime_api.h"
#include "RTLib.h"

#define HIP_ASSERT(x) (assert((x)==hipSuccess))
ProfDataLocs *init_loc(void);

ProfDataLocs *__llvm_gpuprof_loc = init_loc();

void dump_res(ProfDataLocs& res) {
    printf("DataFirst: %p\n", res.DataFirst);
}

extern "C"
void __llvm_gpuprof_sync(void) {
    HIP_ASSERT(hipDeviceSynchronize());
    ProfDataLocs *res = (ProfDataLocs *)malloc(sizeof(ProfDataLocs));
    HIP_ASSERT(hipMemcpyFromSymbol(res, __llvm_gpuprof_loc,
        sizeof(ProfDataLocs *), 0, hipMemcpyDeviceToHost));
    dump_res(*res);
    free(res);
}

ProfDataLocs *init_loc(void) {
    // We can put initialisation code here, we just can't use atexit because
    // that will be too late
    return nullptr;
}