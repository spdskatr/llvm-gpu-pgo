; RUN: $LLVM_ROOT/bin/opt \
; RUN:  -load-pass-plugin=$GPUPGO_ROOT/build/instrument/GPUInstrPass.so \ 
; RUN:  -S -passes=instr-create-hook %s \
; RUN:    | $LLVM_ROOT/bin/FileCheck %s
target triple = "x86_64-unknown-linux"

; CHECK-NOT: __llvm_profile_runtime
