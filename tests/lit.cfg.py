import lit.formats
import os

config.name = "LLVM GPU PGO"

config.test_exec_root = "."
config.test_source_root = config.test_exec_root
config.test_format = lit.formats.ShTest("0")
config.suffixes = {".ll", ".cpp"}

config.environment["HIP_PATH"] = os.environ.get("HIP_PATH") or "/opt/rocm"
config.environment["LLVM_ROOT"] = os.environ.get("LLVM_ROOT") or os.environ.get("HOME") + "/llvm17/build"
config.environment["GPUPGO_ROOT"] = os.environ.get("GPUPGO_ROOT") or os.environ.get("HOME") + "/llvm-gpu-pgo"
config.environment["HIP_CLANG_PATH"] = os.path.join(config.environment["LLVM_ROOT"], "bin")
