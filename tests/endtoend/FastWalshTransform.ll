; RUN: export FILE="HIP-Examples-Applications/FastWalshTransform/FastWalshTransform.cpp"
; RUN: %{SETUP} && %{GET} && %{RUN_INSTR} && %{RUN_PROFILE} && %{RUN_MERGE} && %{RUN_OPT} && %{RUN_FINAL}
; CHECK: Output
; CHECK-NEXT: 15.3732 201.81 51.9855 89.2322 92.572 34.4675 96.2478 66.3863 11.345 225.168 161.374 96.5491 81.8505 211.932 108.827 124.578 202.418 244.549 182.722