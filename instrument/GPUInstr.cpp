#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ProfileData/InstrProf.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation/PGOInstrumentation.h"
#include "llvm/Transforms/Instrumentation/InstrProfiling.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include <llvm/Support/Casting.h>

#define HIP_REGISTER_VAR_NAME "__hipRegisterVar"
#define HIP_REGISTER_GLOBALS_NAME "__hip_register_globals"
#define GPUPROF_LOC_NAME "__llvm_gpuprof_loc"
#define HIP_LAUNCH_KERNEL_NAME "hipLaunchKernel"
#define GPUPROF_SYNC_NAME "__llvm_gpuprof_sync"

using namespace llvm;

namespace {

struct GPUInstrPass : public PassInfoMixin<GPUInstrPass> {
    // Insert code within __hip_register_globals:
    // __hipRegisterVar(<handle>, __llvm_gpuprof_loc, "__llvm_gpuprof_loc", 
    //                  "__llvm_gpuprof_loc", 1, 8, 0, 0)
    void insertRegisterVar(Module &M) {
        auto *RegisterVarF = M.getFunction(HIP_REGISTER_VAR_NAME);
        auto *RegisterGlobalsF = M.getFunction(HIP_REGISTER_GLOBALS_NAME);

        IRBuilder<> IRB{&*RegisterGlobalsF->getEntryBlock().getFirstInsertionPt()};
        auto *IntTy = IRB.getInt32Ty();
        auto *SizeTy = IRB.getInt64Ty();
        auto *PtrTy = Type::getInt64PtrTy(M.getContext());

        // Add global shadow variable for the loc name
        auto *LocNameVar = IRB.CreateGlobalString(GPUPROF_LOC_NAME);
        auto *LocVar = new GlobalVariable(
            M, PtrTy, false, llvm::GlobalValue::ExternalLinkage, 
            nullptr, GPUPROF_LOC_NAME);
        LocVar->setExternallyInitialized(true);

        // The first argument of __hipRegisterVar stores the GPUBinaryHandle.
        // See: clang/lib/CodeGen/CGCUDANV.cpp
        auto *GPUHandle = RegisterGlobalsF->getArg(0);
        ArrayRef<Value *> args{{ 
            GPUHandle, 
            LocVar, 
            LocNameVar, 
            LocNameVar, 
            ConstantInt::get(IntTy, 1),
            ConstantInt::get(SizeTy, 8),
            ConstantInt::get(IntTy, 0),
            ConstantInt::get(IntTy, 0) }};
        IRB.CreateCall(RegisterVarF, args);
    }

    void insertGPUProfDump(Module &M) {
        for (Function &F : M) {
            for (BasicBlock &B : F) {
                for (Instruction &I : B) {
                    if (CallInst *C = dyn_cast<CallInst>(&I)) {
                        Function *Callee = C->getCalledFunction();
                        if (Callee && Callee->getName() == HIP_LAUNCH_KERNEL_NAME) {
                            IRBuilder IRB{C->getInsertionPointAfterDef()};
                            FunctionType *FT = FunctionType::get(IRB.getVoidTy(), false);
                            Function *F = Function::Create(FT, llvm::GlobalValue::ExternalLinkage, 0u, GPUPROF_SYNC_NAME, &M);
                            IRB.CreateCall(F, {});
                            errs() << "Inserted a call within function " << F << "\n";
                        }
                    }
                }
            }
        }
    }

    PreservedAnalyses instrumentHostCode(Module &M, ModuleAnalysisManager &AM) {
        insertRegisterVar(M);
        insertGPUProfDump(M);
        return PreservedAnalyses::all();
    }

    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        PreservedAnalyses pa = PreservedAnalyses::all();
        if (M.getTargetTriple() == "amdgcn-amd-amdhsa") {
            errs() << "Running PGO instrumentation on module " << M.getModuleIdentifier() 
                << " with device target " << M.getTargetTriple() << "\n";

            // Insert intrinsics
            // NOTE: LLVM was modified to change bitcasts to addrspace casts
            PGOInstrumentationGen g;
            pa.intersect(g.run(M, AM));

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

        } else {
            // Otherwise, we assume host target
            errs() << "Running PGO instrumentation on module " << M.getModuleIdentifier() << " with host target " << M.getTargetTriple() << "\n";

            pa.intersect(instrumentHostCode(M, AM));
        }
        // Run verifier
        VerifierPass v;
        pa.intersect(v.run(M, AM));

        errs() << "PGO instrumentation completed and verified!\n";
        return pa;
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
