; RUN: $LLVM_ROOT/bin/opt \
; RUN:  -load-pass-plugin=$GPUPGO_ROOT/build/instrument/GPUInstrPass.so \ 
; RUN:  -S -passes=instr-warp-ballot,simplifycfg %s \
; RUN:    | $LLVM_ROOT/bin/FileCheck %s
target triple = "amdgcn-amd-amdhsa"

define void @test(i64 %0) {
entry:
; CHECK: icmp
; CHECK-SAME: %0
; CHECK: elect:
  ; CHECK-NEXT: read_register
  ; CHECK-NEXT: mbcnt.lo
  ; CHECK-NEXT: mbcnt.hi
  ; CHECK-NEXT: cttz
  ; CHECK: icmp
; CHECK: instrprof:
  ; CHECK: ctpop
  ; CHECK: increment.step
  call void @llvm.instrprof.increment.step(ptr addrspacecast (ptr addrspace(1) 
        @counter to ptr), i64 0, i32 1, i32 0, i64 %0)
  ret void
}
declare void @llvm.instrprof.increment.step(ptr, i64, i32, i32, i64)
@counter = private addrspace(1) constant [4 x i8] c"test"
