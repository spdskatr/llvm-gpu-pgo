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
#include "profile/InstrProfData.inc"
#define PROF_DATA_START INSTR_PROF_SECT_START(INSTR_PROF_DATA_COMMON)
#define PROF_DATA_STOP INSTR_PROF_SECT_STOP(INSTR_PROF_DATA_COMMON)
#define PROF_NAME_START INSTR_PROF_SECT_START(INSTR_PROF_NAME_COMMON)
#define PROF_NAME_STOP INSTR_PROF_SECT_STOP(INSTR_PROF_NAME_COMMON)
#define PROF_CNTS_START INSTR_PROF_SECT_START(INSTR_PROF_CNTS_COMMON)
#define PROF_CNTS_STOP INSTR_PROF_SECT_STOP(INSTR_PROF_CNTS_COMMON)

#define HIP_ASSERT(x) (assert((x)==hipSuccess))

// Instead of relying on the registration functions, we can directly get the
// size of the sections by using the addresses of special symbols that the
// linker emits for the beginning and end of each section. This is what the
// rest of the source code cryptically refers to as "the linker magic".
extern __device__ __llvm_profile_data PROF_DATA_START;
extern __device__ __llvm_profile_data PROF_DATA_STOP;
extern __device__ char PROF_NAME_START;
extern __device__ char PROF_NAME_STOP;
extern __device__ char PROF_CNTS_START;
extern __device__ char PROF_CNTS_STOP;

// This symbol gets registered by GPURTLibInteropPass and then copied back to
// the CPU.
__device__ ProfDataLocs Loc[1] = {
    &PROF_DATA_START,
    &PROF_DATA_STOP,
    &PROF_NAME_START,
    &PROF_NAME_STOP,
    &PROF_CNTS_START,
    &PROF_CNTS_STOP
};

// We no longer need these - keep them in as a stub.
// These will get optimised out when linked with the device code.
extern "C"
__device__ void __llvm_profile_register_function(void *Data_) {
}

extern "C"
__device__ void __llvm_profile_register_names_function(void *NamesStart_, uint64_t NamesSize) {
}