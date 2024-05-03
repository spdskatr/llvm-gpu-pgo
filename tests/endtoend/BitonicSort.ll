; RUN: export FILE="HIP-Examples-Applications/BitonicSort/BitonicSort.cpp"
; RUN: %{SETUP} && %{GET} && %{RUN_INSTR} && %{RUN_PROFILE} && %{RUN_MERGE} && %{RUN_OPT} && %{RUN_FINAL}
; CHECK: Output
; CHECK-NEXT: 255 255 255 255 255 255 255 255 255 255