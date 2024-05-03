; RUN: $LLVM_ROOT/bin/opt \
; RUN:  -load-pass-plugin=$GPUPGO_ROOT/build/instrument/GPUInstrPass.so \ 
; RUN:  -S -passes=instr-warp-ballot,simplifycfg %s \
; RUN:    | $LLVM_ROOT/bin/FileCheck %s
target triple = "amdgcn-amd-amdhsa"

; COM: There should be no elect block after CFG simplification
; CHECK-NOT: elect
define void @test() {
entry:
  ; CHECK: read_register
  ; CHECK: mbcnt.lo
  ; CHECK: mbcnt.hi
  ; CHECK: cttz
  ; CHECK: icmp
; CHECK: instrprof:
  ; CHECK: ctpop
  ; CHECK: increment.step
  call void @llvm.instrprof.increment(ptr addrspacecast (ptr addrspace(1) 
        @counter to ptr), i64 0, i32 1, i32 0)
  ret void
}
declare void @llvm.instrprof.increment(ptr, i64, i32, i32)
@counter = private addrspace(1) constant [4 x i8] c"test"
