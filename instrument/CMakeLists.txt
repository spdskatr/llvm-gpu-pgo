add_llvm_pass_plugin(GPUInstrPass
    AMDGPUAtomicLowering.cpp
    GPUInstr.cpp
    GPURTLibInterop.cpp
    ModuleInfo.cpp
)
