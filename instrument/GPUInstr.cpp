#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation/PGOInstrumentation.h"
#include "llvm/Transforms/Instrumentation/InstrProfiling.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

namespace {

struct GPUInstrPass : public PassInfoMixin<GPUInstrPass> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        if (M.getTargetTriple() == "amdgcn-amd-amdhsa") {
            errs() << "Running PGO instrumentation on module " << M.getModuleIdentifier() << " targeting " << M.getTargetTriple() << "\n";
            PGOInstrumentationGen g;
            PreservedAnalyses pa1 = g.run(M, AM);

            InstrProfiling p{InstrProfOptions {
                .NoRedZone = false,
                .DoCounterPromotion = false,
                .Atomic = true,
                .UseBFIInPromotion = false,
                .InstrProfileOutput = "amdgpu.profraw"
            }};
            PreservedAnalyses pa2 = p.run(M, AM);
            //for (auto &F : M) {
            //    errs() << "Ran PGOInstrumentationGen on " << F.getName() << "\n";
            //}
            VerifierPass v;
            PreservedAnalyses pa3 = v.run(M, AM);
            pa3.intersect(pa2);
            pa3.intersect(pa1);
            errs() << "PGO instrumentation completed and verified!\n";
            return pa3;
        }
        
        
        return PreservedAnalyses::all();
    };
};

}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "GPU PGO instrumentation pass",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(GPUInstrPass());
                });
        }
    };
}
