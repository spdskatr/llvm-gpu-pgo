add_compile_options(-g)

add_llvm_pass_plugin(GPUInstrPass
    IncrementToWarpBallot.cpp
    CreateInstrProfRuntimeHook.cpp
    GPUInstr.cpp
    GPURTLibInterop.cpp
    RemoveProfileMetadata.cpp
    ModuleInfo.cpp
)
