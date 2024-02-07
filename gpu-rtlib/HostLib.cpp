#include <cstdio>
#include <cstdlib>
#include <cassert>
#define __HIP_PLATFORM_AMD__
#include "hip/hip_runtime_api.h"
#include "RTLib.h"

#define HIP_ASSERT(x) (assert((x)==hipSuccess))
ProfDataLocs *init_loc(void);

// Define the host symbol that gets registered with DeviceLib symbol by the
// GPUInstrPass.
ProfDataLocs *__llvm_gpuprof_loc = init_loc();

// Device addresses for profiling data.
ProfDataLocs DeviceLoc;
// Host addresses for profiling data.
ProfDataLocs HostLoc;

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

void dumpRes(ProfDataLocs& res) {
    printf("DataFirst: %p\n", res.DataFirst);
    printf("DataLast: %p\n", res.DataLast);
    printf("NamesFirst: %p\n", res.NamesFirst);
    printf("NamesLast: %p\n", res.NamesLast);
    printf("CountersFirst: %p\n", res.CountersFirst);
    printf("CountersLast: %p\n", res.CountersLast);
}

template <typename T>
size_t distanceInBytes(T* start, T* end) {
    return (end - start) * sizeof(T);
}

void fetchLocs() {
    HIP_ASSERT(hipMemcpyFromSymbol(&DeviceLoc, __llvm_gpuprof_loc,
        sizeof(ProfDataLocs), 0, hipMemcpyDeviceToHost));
    // Also allocate memory for HostLoc
    size_t DataSize = DeviceLoc.DataLast - DeviceLoc.DataFirst;
    size_t NamesSize = DeviceLoc.NamesLast - DeviceLoc.NamesFirst;
    size_t CountersSize = DeviceLoc.CountersLast - DeviceLoc.CountersFirst;

    HostLoc.DataFirst = (__llvm_profile_data *)malloc(DataSize * sizeof(__llvm_profile_data));
    HostLoc.DataLast = HostLoc.DataFirst + DataSize;
    HostLoc.NamesFirst = (const char *)malloc(NamesSize * sizeof(char));
    HostLoc.NamesLast = HostLoc.NamesFirst + DataSize;
    HostLoc.CountersFirst = (char *)malloc(CountersSize * sizeof(char));
    HostLoc.CountersLast = HostLoc.CountersFirst + CountersSize;
}

void fetchData() {
    assert(HostLoc.DataFirst && "Locs were not fetched!");
    memcpyArbitraryDeviceToHost(
        (void *)HostLoc.DataFirst, (void *)DeviceLoc.DataFirst,
        distanceInBytes(HostLoc.DataFirst, HostLoc.DataLast));
    memcpyArbitraryDeviceToHost(
        (void *)HostLoc.NamesFirst, (void *)DeviceLoc.NamesFirst,
        distanceInBytes(HostLoc.NamesFirst, HostLoc.NamesLast));
    memcpyArbitraryDeviceToHost(
        (void *)HostLoc.CountersFirst, (void *)DeviceLoc.CountersFirst,
        distanceInBytes(HostLoc.CountersFirst, HostLoc.CountersLast));
}

extern "C"
void __llvm_gpuprof_sync(void) {
    HIP_ASSERT(hipDeviceSynchronize());
    if (!DeviceLoc.DataFirst)
        fetchLocs();
    fetchData();
    dumpRes(DeviceLoc);
    printf("counters:");
    for (char *c = HostLoc.CountersFirst; c < HostLoc.CountersLast; c++) {
        printf(" %02x", (unsigned char)*c);
    }
    printf("\n");
}

ProfDataLocs *init_loc(void) {
    // We can put initialisation code here, we just can't use atexit because
    // that will be too late
    return nullptr;
}