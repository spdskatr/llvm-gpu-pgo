#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ProfileData/InstrProf.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation/PGOInstrumentation.h"
#include "llvm/Transforms/Instrumentation/InstrProfiling.h"
#include "llvm/Transforms/Scalar/SROA.h"
#include "llvm/Transforms/Scalar/EarlyCSE.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/InlineAdvisor.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/Utils/SimplifyCFGOptions.h>
#include "Passes.h"

using namespace llvm;

PreservedAnalyses GPUInstrPass::run(Module &M, ModuleAnalysisManager &AM) {
    // Only run PGO instrumentation if the target is GPU
    if (M.getTargetTriple() == "amdgcn-amd-amdhsa") {
        errs() << "Running PGO instrumentation on module " << M.getModuleIdentifier() 
            << " with device target " << M.getTargetTriple() << "\n";

        // Prepare pipeline
        ModulePassManager MPM;
        // Preparation
        // This replicates the setup of addPGOInstrPasses before the
        // PGOInstrumentationGen pass.
        InlineParams IP;
        ModuleInlinerWrapperPass MIWP{IP, true, 
            InlineContext{
                .LTOPhase = ThinOrFullLTOPhase::ThinLTOPreLink, 
                .Pass = llvm::InlinePass::EarlyInliner
            }};
        FunctionPassManager FPM;
        FPM.addPass(SROAPass{SROAOptions::ModifyCFG});
        FPM.addPass(EarlyCSEPass{});
        FPM.addPass(SimplifyCFGPass{
            SimplifyCFGOptions{}.convertSwitchRangeToICmp(true)});
        FPM.addPass(InstCombinePass{});
        MIWP.getPM().addPass(createCGSCCToFunctionPassAdaptor(std::move(FPM), true));
        MPM.addPass(std::move(MIWP));

        // Insert intrinsics
        // NOTE: LLVM was modified to change bitcasts to addrspace casts
        MPM.addPass(PGOInstrumentationGen{});
        // Lower intrinsics
        // NOTE: LLVM was modified to change bitcasts to addrspace casts
        MPM.addPass(InstrProfiling{InstrProfOptions {
            .NoRedZone = false,
            // TODO: See if we can set this to true?
            .DoCounterPromotion = false,
            .Atomic = true,
            .UseBFIInPromotion = false
        }});
        // Run verifier
        MPM.addPass(VerifierPass{});

        // Execute pipeline
        PreservedAnalyses pa = MPM.run(M, AM);
        errs() << "PGO instrumentation completed and verified!\n";
        return pa;
    }
    return PreservedAnalyses::all();
};


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
        PB.printPassNames(errs());
    }
};}
