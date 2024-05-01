#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/PassManager.h>
#include <llvm/ProfileData/InstrProf.h>
#include <stdatomic.h>
#include "Passes.h"
using namespace llvm;

#define HIP_REGISTER_VAR_NAME "__hipRegisterVar"
#define HIP_MODULE_DTOR_NAME "__hip_module_dtor"
#define HIP_UNREGISTER_FATBIN_NAME "__hipUnregisterFatBinary"
#define HIP_REGISTER_GLOBALS_NAME "__hip_register_globals"
// These are the names we defined. Note that the names may be repeated in
// other places
#define GPUPROF_LOC_NAME "__llvm_gpuprof_loc"
#define GPUPROF_SYNC_NAME "__llvm_gpuprof_sync"

// Insert code within __hip_register_globals:
// __hipRegisterVar(<handle>, __llvm_gpuprof_loc, "__llvm_gpuprof_loc", 
//                  "__llvm_gpuprof_loc", 1, 8, 0, 0)
static void insertRegisterVar(Module &M) {
    auto *RegisterGlobalsF = M.getFunction(HIP_REGISTER_GLOBALS_NAME);
    if (!RegisterGlobalsF) {
        errs() << "Module " << M.getName() << " is probably not a HIP host module, skipping.\n";
    } else {
        auto *RegisterVarF = M.getFunction(HIP_REGISTER_VAR_NAME);

        // (&*p): Dereference the iterator and then reference the result to get the pointer.
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
        errs() << "Inserted a registration call for HIP host module " << M.getName() << "\n";
    }
}

// Before the HIP module gets deregistered, insert a call to dump all of the
// profile data to a file
static void insertDumpAtDeregister(Module &M) {
    FunctionType *FT = FunctionType::get(Type::getVoidTy(M.getContext()), false);
    Function *Sync = Function::Create(FT, llvm::GlobalValue::ExternalLinkage, 0u, GPUPROF_SYNC_NAME, &M);
    Function *TargetFunc = M.getFunction(HIP_MODULE_DTOR_NAME);
    for (Instruction &I : instructions(TargetFunc)) {
        if (CallInst *C = dyn_cast<CallInst>(&I)) {
            Function *Callee = C->getCalledFunction();
            if (Callee && Callee->getName() == HIP_UNREGISTER_FATBIN_NAME) {
                IRBuilder<> IRB{&I};
                IRB.CreateCall(Sync, {});
                errs() << "Inserted a profile sync point at deregister!\n";
                break;
            }
        }
    }
}

static void instrumentHostCode(Module &M) {
    insertRegisterVar(M);
    insertDumpAtDeregister(M);
}

PreservedAnalyses GPURTLibInteropPass::run(Module &M, ModuleAnalysisManager &AM) {
    if (M.getTargetTriple() != "amdgcn-amd-amdhsa") {
        if (getenv("LLVM_GPUPGO_USE")) {
            errs() << "Host-side instrumentation skipped; profile detected.\n";
        } else {
            // Not the device target, so probably the host target. Apply host-side instrumentation
            instrumentHostCode(M);
        }
    }
    return PreservedAnalyses::all();
}