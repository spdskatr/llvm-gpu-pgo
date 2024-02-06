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
    printf("DataLast: %p\n", res.DataLast);
    printf("NamesFirst: %p\n", res.NamesFirst);
    printf("NamesLast: %p\n", res.NamesLast);
    printf("CountersFirst: %p\n", res.CountersFirst);
    printf("CountersLast: %p\n", res.CountersLast);
}

extern "C"
void __llvm_gpuprof_sync(void) {
    HIP_ASSERT(hipDeviceSynchronize());
    ProfDataLocs hostLoc = {};

    HIP_ASSERT(hipMemcpyFromSymbol(&hostLoc, __llvm_gpuprof_loc,
        sizeof(ProfDataLocs), 0, hipMemcpyDeviceToHost));
    dump_res(hostLoc);
}

ProfDataLocs *init_loc(void) {
    // We can put initialisation code here, we just can't use atexit because
    // that will be too late
    return nullptr;
}