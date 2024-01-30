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
} InstrProfilingLoc;

static __device__ const void *getMinAddr(const void *A1, const void *A2) {
  return A1 < A2 ? A1 : A2;
}

static __device__ const void *getMaxAddr(const void *A1, const void *A2) {
  return A1 > A2 ? A1 : A2;
}

extern "C"
__device__ InstrProfilingLoc Loc{};

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
    if (!Loc.DataFirst) {
        Loc.DataFirst = Data;
        Loc.DataLast = Data + 1;
        Loc.CountersFirst = (uintptr_t)Data_ + (char *)Data->CounterPtr;
        Loc.CountersLast =
            Loc.CountersFirst + Data->NumCounters * sizeof(uint64_t);
        return;
    }

    Loc.DataFirst = (const __llvm_profile_data *)getMinAddr(Loc.DataFirst, Data);
    Loc.CountersFirst = (char *)getMinAddr(
        Loc.CountersFirst, (uintptr_t)Data_ + (char *)Data->CounterPtr);

    Loc.DataLast = (const __llvm_profile_data *)getMaxAddr(Loc.DataLast, Data + 1);
    Loc.CountersLast = (char *)getMaxAddr(
        Loc.CountersLast,
        (uintptr_t)Data_ + (char *)Data->CounterPtr +
            Data->NumCounters * sizeof(uint64_t));
}

extern "C"
__device__ void __llvm_profile_register_names_function(void *NamesStart, uint64_t NamesSize) {
    if (!Loc.NamesFirst) {
        Loc.NamesFirst = (const char *)NamesStart;
        Loc.NamesLast = (const char *)NamesStart + NamesSize;
        return;
    }
    Loc.NamesFirst = (const char *)getMinAddr(Loc.NamesFirst, NamesStart);
    Loc.NamesLast =
        (const char *)getMaxAddr(Loc.NamesLast, (const char *)NamesStart + NamesSize);
}