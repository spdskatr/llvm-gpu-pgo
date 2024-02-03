#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
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
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation/PGOInstrumentation.h"
#include "llvm/Transforms/Instrumentation/InstrProfiling.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"

#define HIP_REGISTER_VAR_NAME "__hipRegisterVar"
#define HIP_REGISTER_GLOBALS_NAME "__hip_register_globals"
#define GPUPROF_LOC_NAME "__llvm_gpuprof_loc"
#define PROFDATALOCS_SIZE (6 * sizeof(void *))

using namespace llvm;

namespace {

struct GPUInstrPass : public PassInfoMixin<GPUInstrPass> {
    PreservedAnalyses instrumentHostCode(Module &M, ModuleAnalysisManager &AM) {
        auto *RegisterVarF = M.getFunction(HIP_REGISTER_VAR_NAME);
        auto *RegisterGlobalsF = M.getFunction(HIP_REGISTER_GLOBALS_NAME);

        IRBuilder<> IRB{&*RegisterGlobalsF->getEntryBlock().getFirstInsertionPt()};
        auto *IntTy = IRB.getInt32Ty();
        auto *SizeTy = IRB.getInt64Ty();
        auto *PtrTy = Type::getInt64PtrTy(M.getContext());

        // Add global shadow variable for the loc name
        auto *LocNameVar = IRB.CreateGlobalString(GPUPROF_LOC_NAME);
        // TODO: Possibly change this to external linkage?
        auto *LocVar = new GlobalVariable(
            M, PtrTy, false, llvm::GlobalValue::WeakAnyLinkage, 
            nullptr, GPUPROF_LOC_NAME);

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
