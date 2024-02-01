#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ProfileData/InstrProf.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation/PGOInstrumentation.h"
#include "llvm/Transforms/Instrumentation/InstrProfiling.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

namespace {

struct GPUInstrPass : public PassInfoMixin<GPUInstrPass> {
    PreservedAnalyses instrumentDeviceCode(Module &M, ModuleAnalysisManager &AM) {
        Function *InitFunction = M.getFunction(getInstrProfRegFuncsName());
        for (Function& F : M) {
            if (F.getCallingConv() == CallingConv::AMDGPU_KERNEL) {
                // Instrument each kernel with call to initialisation...
                // TODO: Make this idempotent
                for (Instruction& I : F.getEntryBlock()) {
                    if (I.isTerminator()) {
                        IRBuilder<> IRB{&I};
                        IRB.CreateCall(InitFunction, {});
                        break;
                    }
                }
            }
        }
        return PreservedAnalyses::all();
    }

    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        if (M.getTargetTriple() == "amdgcn-amd-amdhsa") {
            errs() << "Running PGO instrumentation on module " << M.getModuleIdentifier() 
                << " with device target " << M.getTargetTriple() << "\n";

            // Insert intrinsics
            // NOTE: LLVM was modified to change bitcasts to addrspace casts
            PGOInstrumentationGen g;
            PreservedAnalyses pa = g.run(M, AM);

            // Lower intrinsics
            // NOTE: LLVM was modified to change bitcasts to addrspace casts
            // and also not emit the profiler initialisation function
            InstrProfiling p{InstrProfOptions {
                .NoRedZone = false,
                .DoCounterPromotion = false,
                .Atomic = true,
                .UseBFIInPromotion = false,
                .InstrProfileOutput = "amdgpu.profraw"
            }};
            pa.intersect(p.run(M, AM));

            // Run own pass
            // Temporarily disabled.
            //pa.intersect(instrumentDeviceCode(M, AM));

            // Run verifier
            VerifierPass v;
            pa.intersect(v.run(M, AM));

            errs() << "PGO instrumentation completed and verified!\n";
            return pa;
        } else {
            // Otherwise, we assume host target
            errs() << "Running PGO instrumentation on module " << M.getModuleIdentifier() << " with host target " << M.getTargetTriple() << "\n";

            return PreservedAnalyses::all();
        }
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
