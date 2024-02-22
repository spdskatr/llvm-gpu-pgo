#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Casting.h>
#include <utility>
#include "Passes.h"

#define GPURTLIB_ELECT_NAME "__llvm_gpuprof_elect"

using namespace llvm;

// Insert the __elect function defined in the device rtlib that will be used
// for optimising the counter increments.
void insertElectDecl(Module &M) {
    auto *FunctionTy = FunctionType::get(Type::getInt32Ty(M.getContext()), false);
    auto *F = Function::Create(FunctionTy, llvm::GlobalValue::ExternalLinkage, 
            GPURTLIB_ELECT_NAME, M);
    F->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
}

PreservedAnalyses IncrementToWarpBallotPass::run(Module &M, ModuleAnalysisManager &AM) {
    if (M.getTargetTriple() == "amdgcn-amd-amdhsa") {
        insertElectDecl(M);
        Function *ElectF = M.getFunction("__llvm_gpuprof_elect");
        Type *Int64Ty = Type::getInt64Ty(M.getContext());
        for (Function &F : M) {
            SmallVector<std::pair<IntrinsicInst *, Value *>, 0> Targets;
            for (Instruction &I : instructions(F)) {
                if (IntrinsicInst *Call = dyn_cast<IntrinsicInst>(&I)) {
                    auto ID = Call->getIntrinsicID();
                    if (ID == Intrinsic::instrprof_increment) {
                        Targets.emplace_back(Call, ConstantInt::get(Int64Ty, 1));
                    } else if (ID == Intrinsic::instrprof_increment_step) {
                        Targets.emplace_back(Call, Call->getArgOperand(4));
                    }
                }
            }
            errs() << "Located " << Targets.size() << " targets for function " << F.getName() << "\n";
        }
    }
    return PreservedAnalyses::all();
}