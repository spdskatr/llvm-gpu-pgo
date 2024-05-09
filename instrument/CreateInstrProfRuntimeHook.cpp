#include "Passes.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/PassManager.h>
#include <llvm/ProfileData/InstrProf.h>

using namespace llvm;
// Create a runtime hook variable so it doesn't inadvertently end
// up being called by __llvm_profile_register_function. This is a
// bit stupid.
// We don't care if this variable is stripped out. This is just to
// work around the above mentioned bug in LLVM 17 instrprof lowering.
// See: InstrProfiling::emitRegistration()
PreservedAnalyses
CreateInstrProfRuntimeHookPass::run(Module &M, ModuleAnalysisManager &AM) {
  if (M.getTargetTriple() == "amdgcn-amd-amdhsa") {
    auto *Int32Ty = Type::getInt32Ty(M.getContext());
    auto *Var =
        new GlobalVariable(M, Int32Ty, false, GlobalValue::ExternalLinkage,
                           nullptr, getInstrProfRuntimeHookVarName());
    Var->setVisibility(GlobalValue::HiddenVisibility);
    errs() << "Added GPU runtime hook\n";
    PreservedAnalyses PA;
    PA.preserveSet(AllAnalysesOn<Function>::ID());
    return PA;
  }
  return PreservedAnalyses::all();
}
