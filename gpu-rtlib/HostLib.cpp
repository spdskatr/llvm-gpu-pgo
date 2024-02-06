#include <cstdio>
#include <cstdlib>
#include <cassert>
#define __HIP_PLATFORM_AMD__
#include "hip/hip_runtime_api.h"
#include "RTLib.h"

#define HIP_ASSERT(x) (assert((x)==hipSuccess))
ProfDataLocs *init_loc(void);

ProfDataLocs *__llvm_gpuprof_loc = init_loc();

// Some GPUs don't like you directly copying from arbitrary memory locations
// back into the host. We create a proxy buffer on the device and perform two
// memcpys instead. 
void memcpyArbitraryDeviceToHost(void *hostPtr, const void *devPtr, size_t size) {
    void *proxyPtr;
    HIP_ASSERT(hipMalloc(&proxyPtr, size));
    HIP_ASSERT(hipMemcpy(proxyPtr, devPtr, size, hipMemcpyDeviceToDevice));
    HIP_ASSERT(hipMemcpy(hostPtr, proxyPtr, size, hipMemcpyDeviceToHost));
    HIP_ASSERT(hipFree(proxyPtr));
}

void dump_res(ProfDataLocs& res) {
    printf("DataFirst: %p\n", res.DataFirst);
    printf("DataLast: %p\n", res.DataLast);
    printf("NamesFirst: %p\n", res.NamesFirst);
    printf("NamesLast: %p\n", res.NamesLast);
    printf("CountersFirst: %p\n", res.CountersFirst);
    printf("CountersLast: %p\n", res.CountersLast);
    size_t countersSize = (res.CountersLast - res.CountersFirst) * sizeof(char);
    char *counters = (char *)malloc(countersSize);
    if (!counters) {
        fprintf(stderr, "wtf!!!\n");
        return;
    }

    memcpyArbitraryDeviceToHost(counters, res.CountersFirst, countersSize);

    printf("counters:");
    for (size_t i = 0; i < countersSize; i++) {
        printf(" %02x", (unsigned char)counters[i]);
    }
    printf("\n");
    free(counters);
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