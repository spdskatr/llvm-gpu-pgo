#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#define __HIP_PLATFORM_AMD__
#include "hip/hip_runtime_api.h"
#include "RTLib.h"

#ifdef __LLVM_GPUPROF_DEBUG
#define DEBUG_PRINTF(x...) fprintf(stderr, x)
#else
#define DEBUG_PRINTF(x...)
#endif

#define HostLoc __llvm_prf_override_locs

#define HIP_ASSERT(x) (assert((x)==hipSuccess))
ProfDataLocs *init_loc(void);

// Compiler-rt hooks to init and write the profile data.
int __llvm_profile_runtime;
extern "C" void __llvm_profile_initialize_file(void);
extern "C" int __llvm_profile_write_file(void);

// Define the host symbol that gets registered with DeviceLib symbol by the
// GPUInstrPass.
ProfDataLocs *__llvm_gpuprof_loc = init_loc();

// Device addresses for profiling data. Registered with device side by GPUInstrPass.
ProfDataLocs DeviceLoc;
// Proxy device addresses for profiling data.
// Some GPUs don't like you directly copying from arbitrary memory locations
// back into the host. We create a proxy buffer on the device and perform two
// memcpys instead. 
ProfDataLocs ProxyLoc;
// Host addresses for profiling data.
ProfDataLocs HostLoc;

static void memcpyArbitraryDeviceToHost(void *hostPtr, void *proxyPtr, const void *devPtr, size_t size) {
    HIP_ASSERT(hipMemcpy(proxyPtr, devPtr, size, hipMemcpyDeviceToDevice));
    HIP_ASSERT(hipMemcpy(hostPtr, proxyPtr, size, hipMemcpyDeviceToHost));
}

static void debugRes(ProfDataLocs& res) {
    DEBUG_PRINTF("DataFirst: %p\n", res.DataFirst);
    DEBUG_PRINTF("DataLast: %p\n", res.DataLast);
    DEBUG_PRINTF("NamesFirst: %p\n", res.NamesFirst);
    DEBUG_PRINTF("NamesLast: %p\n", res.NamesLast);
    DEBUG_PRINTF("CountersFirst: %p\n", res.CountersFirst);
    DEBUG_PRINTF("CountersLast: %p\n", res.CountersLast);
}

static void debugLog() {
    // Show where the data got copied from/to
    DEBUG_PRINTF("Device loc:\n");
    debugRes(DeviceLoc);
    DEBUG_PRINTF("Host loc:\n");
    debugRes(HostLoc);

    // Show some data
    DEBUG_PRINTF("counters:");
    for (char *c = HostLoc.CountersFirst; c < HostLoc.CountersLast; c++) {
        DEBUG_PRINTF(" %02x", (unsigned char)*c);
    }
    DEBUG_PRINTF("\n");
}

template <typename T>
static size_t distanceInBytes(T* start, T* end) {
    return (end - start) * sizeof(T);
}

static void fixRelativePositions() {
    // We have to change the relative pointers for the profdata since all the
    // memory locations have changed
    size_t HostCounterRel = HostLoc.CountersFirst - reinterpret_cast<char *>(HostLoc.DataFirst);
    size_t DeviceCounterRel = DeviceLoc.CountersFirst - reinterpret_cast<char *>(DeviceLoc.DataFirst);
    size_t RelativePtrChange = HostCounterRel - DeviceCounterRel;
    for (auto *Data = HostLoc.DataFirst; Data < HostLoc.DataLast; Data++) {
        // Unfortunately .CounterPtr has type const uintptr_t, but luckily we
        // can cast and it works fine because the memory is on the heap
        uintptr_t *Rel = const_cast<uintptr_t *>(&Data->CounterPtr);
        *Rel = Data->CounterPtr + RelativePtrChange;
    }
}

static void allocateLocs() {
    size_t DataSize = DeviceLoc.DataLast - DeviceLoc.DataFirst;
    size_t NamesSize = DeviceLoc.NamesLast - DeviceLoc.NamesFirst;
    size_t CountersSize = DeviceLoc.CountersLast - DeviceLoc.CountersFirst;

    HostLoc.DataFirst = static_cast<__llvm_profile_data *>(malloc(DataSize * sizeof(__llvm_profile_data)));
    HostLoc.DataLast = HostLoc.DataFirst + DataSize;
    HostLoc.NamesFirst = static_cast<char *>(malloc(NamesSize * sizeof(char)));
    HostLoc.NamesLast = HostLoc.NamesFirst + NamesSize;
    HostLoc.CountersFirst = static_cast<char *>(malloc(CountersSize * sizeof(char)));
    HostLoc.CountersLast = HostLoc.CountersFirst + CountersSize;

    HIP_ASSERT(hipMalloc(&ProxyLoc.DataFirst, DataSize * sizeof(__llvm_profile_data)));
    ProxyLoc.DataLast = ProxyLoc.DataFirst + DataSize;
    HIP_ASSERT(hipMalloc(&ProxyLoc.NamesFirst, NamesSize * sizeof(char)));
    ProxyLoc.NamesLast = ProxyLoc.NamesFirst + NamesSize;
    HIP_ASSERT(hipMalloc(&ProxyLoc.CountersFirst, CountersSize * sizeof(char)));
    ProxyLoc.CountersLast = ProxyLoc.CountersFirst + CountersSize;
}

static void fetchData() {
    assert(HostLoc.DataFirst && "Locs were not fetched!");
    memcpyArbitraryDeviceToHost(
        HostLoc.DataFirst, ProxyLoc.DataFirst, DeviceLoc.DataFirst,
        distanceInBytes(HostLoc.DataFirst, HostLoc.DataLast));
    memcpyArbitraryDeviceToHost(
        HostLoc.NamesFirst, ProxyLoc.NamesFirst, DeviceLoc.NamesFirst,
        distanceInBytes(HostLoc.NamesFirst, HostLoc.NamesLast));
    memcpyArbitraryDeviceToHost(
        HostLoc.CountersFirst, ProxyLoc.CountersFirst, DeviceLoc.CountersFirst,
        distanceInBytes(HostLoc.CountersFirst, HostLoc.CountersLast));

    fixRelativePositions();
}

// Sync hook run after each kernel call
extern "C"
void __llvm_gpuprof_sync(void) {
    HIP_ASSERT(hipDeviceSynchronize());
    if (!DeviceLoc.DataFirst) {
        // Fetch symbol data to DeviceLoc
        HIP_ASSERT(hipMemcpyFromSymbol(&DeviceLoc, __llvm_gpuprof_loc,
            sizeof(ProfDataLocs), 0, hipMemcpyDeviceToHost));
        allocateLocs();
        // Now that we have profdata, we should initialise compiler-rt now
        __llvm_profile_initialize_file();
    }
    fetchData();
    debugLog();
}

void dump_data_to_file() {
    assert(!__llvm_profile_write_file());
} 

ProfDataLocs *init_loc() {
    // We can put initialisation code here, this will always run after the main
    // hip module gets initialised
    // Note: We can't atexit and then read profiledata here, by that time the
    // HIP module would have been deregistered already
    atexit(dump_data_to_file);
    return nullptr;
}