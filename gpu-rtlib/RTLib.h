#include <cstdint>
typedef unsigned long uint64_t;
typedef unsigned uint32_t;
typedef unsigned short uint16_t;

// Much of this code is made to be as close as possible to the code in
// compiler-rt/lib/profile/InstrProfilingPlatformOther.c
enum ValueKind {
#define VALUE_PROF_KIND(Enumerator, Value, Descr) Enumerator = Value,
#include "profile/InstrProfData.inc"
};

typedef uintptr_t IntPtrT;
typedef struct __llvm_profile_data {
#define INSTR_PROF_DATA(Type, LLVMType, Name, Initializer) Type Name;
#include "profile/InstrProfData.inc"
} __llvm_profile_data;

typedef struct {
    __llvm_profile_data *DataFirst;
    __llvm_profile_data *DataLast;
    char *NamesFirst;
    char *NamesLast;
    char *CountersFirst;
    char *CountersLast;
} ProfDataLocs;
