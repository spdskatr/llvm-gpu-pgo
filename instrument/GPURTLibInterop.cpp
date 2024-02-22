#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/PassManager.h>
#include <llvm/ProfileData/InstrProf.h>
#include "Passes.h"
using namespace llvm;

#define HIP_REGISTER_VAR_NAME "__hipRegisterVar"
#define HIP_REGISTER_GLOBALS_NAME "__hip_register_globals"
#define HIP_LAUNCH_KERNEL_NAME "hipLaunchKernel"
// These are the names we defined. Note that the names may be repeated in
// other places
#define GPUPROF_LOC_NAME "__llvm_gpuprof_loc"
#define GPUPROF_SYNC_NAME "__llvm_gpuprof_sync"

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

// After each kernel call, create a call to __llvm_gpuprof_sync()
void insertGPUProfDump(Module &M) {
    for (Function &F : M) {
        for (Instruction &I : instructions(F)) {
            if (CallInst *C = dyn_cast<CallInst>(&I)) {
                Function *Callee = C->getCalledFunction();
                if (Callee && Callee->getName() == HIP_LAUNCH_KERNEL_NAME) {
                    IRBuilder IRB{C->getInsertionPointAfterDef()};
                    FunctionType *FT = FunctionType::get(IRB.getVoidTy(), false);
                    Function *F = Function::Create(FT, llvm::GlobalValue::ExternalLinkage, 0u, GPUPROF_SYNC_NAME, &M);
                    IRB.CreateCall(F, {});
                    errs() << "Inserted a call for kernel call " << C->getArgOperand(0)->getName() << "\n";
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

PreservedAnalyses GPURTLibInteropPass::run(Module &M, ModuleAnalysisManager &AM) {
    PreservedAnalyses pa = PreservedAnalyses::all();
    if (M.getTargetTriple() != "amdgcn-amd-amdhsa") {
        // We assume host target
        pa.intersect(instrumentHostCode(M, AM));
        errs() << "Added host hooks for getting profdata\n";
    }
    return pa;
}