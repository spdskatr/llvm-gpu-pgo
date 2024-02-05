#include "RTLib.h"
#include <cstdio>
#include <cstdlib>

ProfDataLocs *__init_loc(void);

ProfDataLocs *__llvm_gpuprof_loc = __init_loc();

void __dump_loc(void) {
    printf("Location: %p\n", __llvm_gpuprof_loc);
}

ProfDataLocs *__init_loc(void) {
    atexit(__dump_loc);
    return nullptr;
}