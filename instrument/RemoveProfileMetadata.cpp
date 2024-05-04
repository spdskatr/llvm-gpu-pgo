#include "Passes.h"
#include <llvm/Analysis/ProfileSummaryInfo.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/BlockFrequency.h>
#include <llvm/Analysis/BranchProbabilityInfo.h>
using namespace llvm;

// Scrubs all profile metadata from the function and the ProfileSummary from the module containing it.
// Useful for debugging purposes.
PreservedAnalyses RemoveProfileMetadataPass::run(Function &F, FunctionAnalysisManager &AM) {
    F.getParent()->setModuleFlag(llvm::Module::Override, "ProfileSummary", NULL);
    F.eraseMetadata(LLVMContext::MD_prof);
    for (Instruction &I : instructions(F)) {
        I.setMetadata(LLVMContext::MD_prof, NULL);
        I.setMetadata(LLVMContext::MD_irr_loop, NULL);
    }
    PreservedAnalyses pa = PreservedAnalyses::all();
    pa.abandon(ProfileSummaryAnalysis::ID());
    pa.abandon(BlockFrequencyAnalysis::ID());
    pa.abandon(BranchProbabilityAnalysis::ID());
    return pa;
}