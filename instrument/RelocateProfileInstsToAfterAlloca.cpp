#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Casting.h>
#include "Passes.h"
using namespace llvm;

PreservedAnalyses RelocateProfileInstsToAfterAllocaPass::run(Module &M, ModuleAnalysisManager &AM) {
    for (Function &F : M) {
        if (!F.empty()) {
            BasicBlock &EntryBlock = F.front();
            if (IntrinsicInst *Intr = dyn_cast<IntrinsicInst>(&EntryBlock.front())) {
                if (Intr->getIntrinsicID() == Intrinsic::instrprof_increment) {
                    Intr->removeFromParent();
                    Intr->insertBefore(&*EntryBlock.getFirstNonPHIOrDbgOrAlloca());
                }
            }
        }
    }
    return PreservedAnalyses::all();
}