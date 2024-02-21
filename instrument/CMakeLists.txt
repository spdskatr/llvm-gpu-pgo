add_llvm_pass_plugin(GPUInstrPass
    IncrementToWarpBallot.cpp
    CreateInstrProfRuntimeHook.cpp
    GPUInstr.cpp
    GPURTLibInterop.cpp
    ModuleInfo.cpp
)
