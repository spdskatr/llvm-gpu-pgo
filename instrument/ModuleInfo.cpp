#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include "Passes.h"

using namespace llvm;

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() { return {
    .APIVersion = LLVM_PLUGIN_API_VERSION,
    .PluginName = "GPU PGO instrumentation pass",
    .PluginVersion = "v0.1",
    .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
        PB.registerPipelineStartEPCallback([](ModulePassManager &MPM, OptimizationLevel OL) {
            MPM.addPass(GPURTLibInteropPass{});
        });
        // Insert PGO instrumentation as close to the original place as
        // possible, which is during the module simplification pipeline.
        PB.registerPipelineEarlySimplificationEPCallback([](ModulePassManager &MPM, OptimizationLevel OL) {
            MPM.addPass(GPUInstrPass{});
        });
    }
};}