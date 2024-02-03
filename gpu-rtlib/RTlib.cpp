#include <stdlib.h>
#define __HIP_PLATFORM_AMD__
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "hip/hip_runtime_api.h"
#include "hip/hip_runtime.h"
#include "hip/driver_types.h"

#define HIP_ASSERT(x) (assert((x)==hipSuccess))

// Much of this code is made to be as close as possible to the code in
// compiler-rt/lib/profile/InstrProfilingPlatformOther.c
enum ValueKind {
#define VALUE_PROF_KIND(Enumerator, Value, Descr) Enumerator = Value,
#include "profile/InstrProfData.inc"
};

typedef void *IntPtrT;
typedef struct __llvm_profile_data {
#define INSTR_PROF_DATA(Type, LLVMType, Name, Initializer) Type Name;
#include "profile/InstrProfData.inc"
} __llvm_profile_data;

typedef struct {
    const __llvm_profile_data *DataFirst;
    const __llvm_profile_data *DataLast;
    const char *NamesFirst;
    const char *NamesLast;
    char *CountersFirst;
    char *CountersLast;
} ProfDataLocs;

static __device__ const void *getMinAddr(const void *A1, const void *A2) {
  return A1 < A2 ? A1 : A2;
}

static __device__ const void *getMaxAddr(const void *A1, const void *A2) {
  return A1 > A2 ? A1 : A2;
}

__device__ ProfDataLocs Locs{};

extern "C"
__device__ ProfDataLocs *__llvm_gpuprof_loc;

// TODO: We should also probably emit a reference to this variable on the host
// side to get the linker to link the compiler-rt init module
// See: llvm/ProfileData/InstrProf.h
extern "C"
__device__ int __llvm_profile_runtime = 0;

extern "C"
__device__ void __llvm_profile_register_function(void *Data_) {
    // We assume that counters are have size uint64
    // (i.e. __llvm_profile_counter_entry_size() == sizeof(uint64_t))
    // This is probably true for any future llvm version that rocm supports
    const __llvm_profile_data *Data = (__llvm_profile_data *)Data_;
    if (!Locs.DataFirst) {
        Locs.DataFirst = Data;
        Locs.DataLast = Data + 1;
        Locs.CountersFirst = (uintptr_t)Data_ + (char *)Data->CounterPtr;
        Locs.CountersLast =
            Locs.CountersFirst + Data->NumCounters * sizeof(uint64_t);
        return;
    }

    Locs.DataFirst = (const __llvm_profile_data *)getMinAddr(Locs.DataFirst, Data);
    Locs.CountersFirst = (char *)getMinAddr(
        Locs.CountersFirst, (uintptr_t)Data_ + (char *)Data->CounterPtr);

    Locs.DataLast = (const __llvm_profile_data *)getMaxAddr(Locs.DataLast, Data + 1);
    Locs.CountersLast = (char *)getMaxAddr(
        Locs.CountersLast,
        (uintptr_t)Data_ + (char *)Data->CounterPtr +
            Data->NumCounters * sizeof(uint64_t));
}

extern "C"
__device__ void __llvm_profile_register_names_function(void *NamesStart, uint64_t NamesSize) {
    __llvm_gpuprof_loc = &Locs;
    if (!Locs.NamesFirst) {
        Locs.NamesFirst = (const char *)NamesStart;
        Locs.NamesLast = (const char *)NamesStart + NamesSize;
        return;
    }
    Locs.NamesFirst = (const char *)getMinAddr(Locs.NamesFirst, NamesStart);
    Locs.NamesLast =
        (const char *)getMaxAddr(Locs.NamesLast, (const char *)NamesStart + NamesSize);
}