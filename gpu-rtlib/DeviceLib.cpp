#include <stdlib.h>
#define __HIP_PLATFORM_AMD__
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "hip/hip_runtime_api.h"
#include "hip/hip_runtime.h"
#include "hip/driver_types.h"
#include "RTLib.h"

#define Loc __llvm_gpuprof_loc
#define HIP_ASSERT(x) (assert((x)==hipSuccess))

template <typename T>
static __device__ T *getMinAddr(T *A1, T *A2) {
  return A1 < A2 ? A1 : A2;
}

template <typename T>
static __device__ T *getMaxAddr(T *A1, T *A2) {
  return A1 > A2 ? A1 : A2;
}

__device__ ProfDataLocs Loc[1];

extern "C"
__device__ void __llvm_profile_register_function(void *Data_) {
    // We assume that counters are have size uint64
    // (i.e. __llvm_profile_counter_entry_size() == sizeof(uint64_t))
    // This is probably true for any future llvm version that rocm supports
    __llvm_profile_data *Data = static_cast<__llvm_profile_data *>(Data_);
    if (!Loc[0].DataFirst) {
        Loc[0].DataFirst = Data;
        Loc[0].DataLast = Data + 1;
        Loc[0].CountersFirst = static_cast<char *>(Data_) + Data->CounterPtr;
        Loc[0].CountersLast =
            Loc[0].CountersFirst + Data->NumCounters * sizeof(uint64_t);
    } else {
        Loc[0].DataFirst = getMinAddr(Loc[0].DataFirst, Data);
        Loc[0].CountersFirst = getMinAddr(
            Loc[0].CountersFirst, static_cast<char *>(Data_) + Data->CounterPtr);

        Loc[0].DataLast = getMaxAddr(Loc[0].DataLast, Data + 1);
        Loc[0].CountersLast = getMaxAddr(
            Loc[0].CountersLast,
            static_cast<char *>(Data_) + Data->CounterPtr + Data->NumCounters * sizeof(uint64_t));
    }
}

extern "C"
__device__ void __llvm_profile_register_names_function(void *NamesStart_, uint64_t NamesSize) {
    char *NamesStart = static_cast<char *>(NamesStart_);
    if (!Loc[0].NamesFirst) {
        Loc[0].NamesFirst = NamesStart;
        Loc[0].NamesLast = NamesStart + NamesSize;
        return;
    }
    Loc[0].NamesFirst = getMinAddr(Loc[0].NamesFirst, NamesStart);
    Loc[0].NamesLast =
        getMaxAddr(Loc[0].NamesLast, NamesStart + NamesSize);
}