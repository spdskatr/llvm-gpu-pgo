; RUN: export FILE="HIP-Examples-Applications/HelloWorld/HelloWorld.cpp"
; RUN: %{SETUP} && %{GET} && %{RUN_INSTR} && %{RUN_PROFILE} && %{RUN_MERGE} && %{RUN_OPT} && %{RUN_FINAL}
; CHECK: output string:
; CHECK: HelloWorld