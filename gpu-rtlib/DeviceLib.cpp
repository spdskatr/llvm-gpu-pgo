#include <stdlib.h>
#define __HIP_PLATFORM_AMD__
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "hip/hip_runtime_api.h"
#include "hip/hip_runtime.h"
#include "hip/driver_types.h"
#include "RTLib.h"

#define HIP_ASSERT(x) (assert((x)==hipSuccess))

static __device__ const void *getMinAddr(const void *A1, const void *A2) {
  return A1 < A2 ? A1 : A2;
}

static __device__ const void *getMaxAddr(const void *A1, const void *A2) {
  return A1 > A2 ? A1 : A2;
}

__device__ ProfDataLocs __llvm_gpuprof_loc[1] = {{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }};

// TODO: We should also probably emit a reference to this variable on the host
// side to get the linker to link the compiler-rt init module
// See: llvm/ProfileData/InstrProf.h
__device__ int __llvm_profile_runtime = 0;

extern "C"
__device__ void __llvm_profile_register_function(void *Data_) {
    // We assume that counters are have size uint64
    // (i.e. __llvm_profile_counter_entry_size() == sizeof(uint64_t))
    // This is probably true for any future llvm version that rocm supports
    const __llvm_profile_data *Data = (__llvm_profile_data *)Data_;
    if (!__llvm_gpuprof_loc[0].DataFirst) {
        __llvm_gpuprof_loc[0].DataFirst = Data;
        __llvm_gpuprof_loc[0].DataLast = Data + 1;
        __llvm_gpuprof_loc[0].CountersFirst = (char *)((size_t)Data_ + (size_t)Data->CounterPtr);
        __llvm_gpuprof_loc[0].CountersLast =
            __llvm_gpuprof_loc[0].CountersFirst + Data->NumCounters * sizeof(uint64_t);
    } else {
        __llvm_gpuprof_loc[0].DataFirst = (const __llvm_profile_data *)getMinAddr(__llvm_gpuprof_loc[0].DataFirst, Data);
        __llvm_gpuprof_loc[0].CountersFirst = (char *)getMinAddr(
            __llvm_gpuprof_loc[0].CountersFirst, (char *)((size_t)Data_ + (size_t)Data->CounterPtr));

        __llvm_gpuprof_loc[0].DataLast = (const __llvm_profile_data *)getMaxAddr(__llvm_gpuprof_loc[0].DataLast, Data + 1);
        __llvm_gpuprof_loc[0].CountersLast = (char *)getMaxAddr(
            __llvm_gpuprof_loc[0].CountersLast,
            (char *)((size_t)Data_ + (size_t)Data->CounterPtr +
                Data->NumCounters * sizeof(uint64_t)));
    }
}

extern "C"
__device__ void __llvm_profile_register_names_function(void *NamesStart, uint64_t NamesSize) {
    if (!__llvm_gpuprof_loc[0].NamesFirst) {
        __llvm_gpuprof_loc[0].NamesFirst = (const char *)NamesStart;
        __llvm_gpuprof_loc[0].NamesLast = (const char *)NamesStart + NamesSize;
        return;
    }
    __llvm_gpuprof_loc[0].NamesFirst = (const char *)getMinAddr(__llvm_gpuprof_loc[0].NamesFirst, NamesStart);
    __llvm_gpuprof_loc[0].NamesLast =
        (const char *)getMaxAddr(__llvm_gpuprof_loc[0].NamesLast, (const char *)NamesStart + NamesSize);
}