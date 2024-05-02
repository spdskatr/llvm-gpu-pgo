#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/CommandLine.h>
#include "Passes.h"

using namespace llvm;

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() { return {
    .APIVersion = LLVM_PLUGIN_API_VERSION,
    .PluginName = "GPU PGO instrumentation pass",
    .PluginVersion = "v0.1",
    .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
        PB.registerPipelineParsingCallback([](StringRef Name, FunctionPassManager& FPM, ArrayRef<PassBuilder::PipelineElement>) {
            if (Name == "instr-warp-ballot") {
                FPM.addPass(IncrementToWarpBallotPass{});
                return true;
            }
            return false;
        });
        PB.registerPipelineParsingCallback([](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>) {
            if (Name == "instr-create-hook") {
                MPM.addPass(CreateInstrProfRuntimeHookPass{});
                return true;
            }
            if (Name == "instr-gen-gpu") {
                MPM.addPass(GPUInstrPass{});
                return true;
            }
            return false;
        });

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