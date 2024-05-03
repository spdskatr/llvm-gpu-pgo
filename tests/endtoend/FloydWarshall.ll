; RUN: export FILE="HIP-Examples-Applications/FloydWarshall/FloydWarshall.cpp"
; RUN: %{SETUP} && %{GET} && %{RUN_INSTR} && %{RUN_PROFILE} && %{RUN_MERGE} && %{RUN_OPT} && %{RUN_FINAL}
; CHECK: Output Path Distance Matrix
; CHECK-NEXT: 0 2 3 1 3 2 0 3 1 2 2 2 3 2 2 2 2 3 2 2 0 3
; CHECK: Output Path Matrix
; CHECK-NEXT: 0 220 92 39 207 198 29 207 39 184 39 213 198 214 39 210 198 220 198 254