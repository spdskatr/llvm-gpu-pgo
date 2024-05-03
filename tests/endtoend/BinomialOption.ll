; RUN: export FILE="HIP-Examples-Applications/BinomialOption/BinomialOption.cpp"
; RUN: %{SETUP} && %{GET} && %{RUN_INSTR} && %{RUN_PROFILE} && %{RUN_MERGE} && %{RUN_OPT} && %{RUN_FINAL}
; CHECK: Output
; CHECK-NEXT: 6.57809 0.920039 5.39851 5.70162 8.26097 0.291796 0.649763 5.12428 0.45692 2.12934