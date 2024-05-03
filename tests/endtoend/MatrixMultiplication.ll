; RUN: export FILE="HIP-Examples-Applications/MatrixMultiplication/MatrixMultiplication.cpp"
; RUN: %{SETUP} && %{GET} && %{RUN_INSTR} && %{RUN_PROFILE} && %{RUN_MERGE} && %{RUN_OPT} && %{RUN_FINAL}
; CHECK: Output
; CHECK-NEXT: 1769.65 1684.62 1921.35 2008.79 1814.25 1765.03 1785.98 1858.77 1699.89 1819.99 1805.21 