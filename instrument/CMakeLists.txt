add_llvm_pass_plugin(GPUInstrPass
    AMDGPUAtomicLowering.cpp
    CreateInstrProfRuntimeHook.cpp
    GPUInstr.cpp
    GPURTLibInterop.cpp
    ModuleInfo.cpp
)
