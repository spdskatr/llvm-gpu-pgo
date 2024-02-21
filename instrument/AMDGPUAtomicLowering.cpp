#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Casting.h>
#include "Passes.h"

using namespace llvm;

PreservedAnalyses AMDGPUAtomicLoweringPass::run(Module &M, ModuleAnalysisManager &AM) {
    for (Function &F : M) {
        for (BasicBlock &B : F) {
            for (Instruction &I : B) {
                if (AtomicRMWInst *rmw = dyn_cast<AtomicRMWInst>(&I)) {
                    // TODO: do something with the atomics
                    rmw->dump();
                }
            }
        }
    }
    return PreservedAnalyses::all();
}