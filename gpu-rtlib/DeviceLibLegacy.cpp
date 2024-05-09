// The old version of DeviceLib, without the linker magic.
// File retained for benchmarking purposes.
#include <stdlib.h>
#define __HIP_PLATFORM_AMD__
#include "RTLib.h"
#include "hip/driver_types.h"
#include "hip/hip_runtime.h"
#include "hip/hip_runtime_api.h"
#include <assert.h>
#include <stdint.h>

#define Loc __llvm_gpuprof_loc
#define HIP_ASSERT(x) (assert((x) == hipSuccess))

template <typename T> static __device__ T *getMinAddr(T *A1, T *A2) {
  return A1 < A2 ? A1 : A2;
}

template <typename T> static __device__ T *getMaxAddr(T *A1, T *A2) {
  return A1 > A2 ? A1 : A2;
}

__device__ ProfDataLocs Loc;

extern "C" __device__ void __llvm_profile_register_function(void *Data_) {
  // We assume that counters are have size uint64
  // (i.e. __llvm_profile_counter_entry_size() == sizeof(uint64_t))
  // This is probably true for any future llvm version that rocm supports
  __llvm_profile_data *Data = static_cast<__llvm_profile_data *>(Data_);
  if (!Loc.DataFirst) {
    Loc.DataFirst = Data;
    Loc.DataLast = Data + 1;
    Loc.CountersFirst = static_cast<char *>(Data_) + Data->CounterPtr;
    Loc.CountersLast = Loc.CountersFirst + Data->NumCounters * sizeof(uint64_t);
  } else {
    Loc.DataFirst = getMinAddr(Loc.DataFirst, Data);
    Loc.CountersFirst = getMinAddr(
        Loc.CountersFirst, static_cast<char *>(Data_) + Data->CounterPtr);

    Loc.DataLast = getMaxAddr(Loc.DataLast, Data + 1);
    Loc.CountersLast = getMaxAddr(
        Loc.CountersLast, static_cast<char *>(Data_) + Data->CounterPtr +
                              Data->NumCounters * sizeof(uint64_t));
  }
}

extern "C" __device__ void
__llvm_profile_register_names_function(void *NamesStart_, uint64_t NamesSize) {
  char *NamesStart = static_cast<char *>(NamesStart_);
  if (!Loc.NamesFirst) {
    Loc.NamesFirst = NamesStart;
    Loc.NamesLast = NamesStart + NamesSize;
    return;
  }
  Loc.NamesFirst = getMinAddr(Loc.NamesFirst, NamesStart);
  Loc.NamesLast = getMaxAddr(Loc.NamesLast, NamesStart + NamesSize);
}