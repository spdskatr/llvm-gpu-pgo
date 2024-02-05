typedef unsigned long uint64_t;
typedef unsigned uint32_t;
typedef unsigned short uint16_t;

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
