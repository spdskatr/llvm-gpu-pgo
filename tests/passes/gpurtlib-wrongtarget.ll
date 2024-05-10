; RUN: $LLVM_ROOT/bin/opt \
; RUN:  -load-pass-plugin=$GPUPGO_ROOT/build/instrument/GPUInstrPass.so \ 
; RUN:  -S -passes=instr-augment-host %s \
; RUN:    | $LLVM_ROOT/bin/FileCheck %s
target triple = "amdgcn-amd-amdhsa"

; CHECK-NOT: @__llvm_gpuprof_loc
; CHECK-NOT: @__llvm_gpuprof_sync()

define internal void @__hip_register_globals(ptr %0) {
  ; CHECK-NOT: call void @__hipRegisterVar
  ret void
}

declare void @__hipUnregisterFatBinary(ptr) local_unnamed_addr
define internal void @__hip_module_dtor() {
  tail call void @__hipUnregisterFatBinary(ptr null)
  ret void
}

declare void @__hipRegisterVar(ptr, ptr, ptr, ptr, i32, i64, i32, i32) local_unnamed_addr