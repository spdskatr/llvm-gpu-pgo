#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Pass.h>
#include <llvm/ProfileData/InstrProf.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Transforms/Instrumentation/PGOInstrumentation.h>
#include <llvm/Transforms/Instrumentation/InstrProfiling.h>
#include <llvm/Transforms/Scalar/SROA.h>
#include <llvm/Transforms/Scalar/EarlyCSE.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Analysis/ProfileSummaryInfo.h>
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
        IP.DefaultThreshold = 0;
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
        MIWP.getPM().addPass(
            createCGSCCToFunctionPassAdaptor(std::move(FPM), false));
        MPM.addPass(std::move(MIWP));
        MPM.addPass(GlobalDCEPass{});

        if (char *Filename = getenv("LLVM_GPUPGO_USE")) {
            errs() << "Using " << Filename << " as profile file for PGOInstrumentationUse\n";
            MPM.addPass(PGOInstrumentationUse{Filename});
            MPM.addPass(RequireAnalysisPass<ProfileSummaryAnalysis, Module>{});
        } else {
            // Insert intrinsics
            // NOTE: LLVM was modified to change bitcasts to addrspace casts
            MPM.addPass(PGOInstrumentationGen{});
            // Make increments more efficient
            //MPM.addPass(IncrementToWarpBallotPass{});
            // Deactivate the LLVM 17 bug
            MPM.addPass(CreateInstrProfRuntimeHookPass{});
            // Lower intrinsics
            // NOTE: LLVM was modified to change bitcasts to addrspace casts
            MPM.addPass(InstrProfiling{InstrProfOptions {
                .NoRedZone = false,
                .DoCounterPromotion = false,
                .Atomic = true,
                .UseBFIInPromotion = false
            }});
        }
        // Run verifier
        MPM.addPass(VerifierPass{});

        // Execute pipeline
        PreservedAnalyses pa = MPM.run(M, AM);

        // TODO: Remove this
        for (Function &F : M) for (BasicBlock &B : F) for (Instruction &I : B) {
            if (IntrinsicInst *Call = dyn_cast<IntrinsicInst>(&I)) {
                if (Call->getIntrinsicID() == Intrinsic::stacksave) {
                    errs() << "Stacksave found!\n";
                    I.dump();
                    errs() << "Block:\n";
                    B.dump();
                    errs() << "Function:\n";
                    F.dump();
                }
            }
        }
        errs() << "Stacksave where\n";

        errs() << "PGO pass completed!\n";
        return pa;
    }
    return PreservedAnalyses::all();
};

