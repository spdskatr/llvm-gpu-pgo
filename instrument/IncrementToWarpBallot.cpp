#include <llvm/ADT/StringRef.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Casting.h>
#include "Passes.h"

using namespace llvm;


PreservedAnalyses IncrementToWarpBallotPass::run(Function &F, FunctionAnalysisManager &AM) {
    for (BasicBlock &B : F) {
        for (Instruction &I : B) {
            // TODO: Replace increment with a warp ballot so only one thread
            // does the increment
        }
    }
    return PreservedAnalyses::all();
}