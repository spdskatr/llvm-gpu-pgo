#include <llvm/IR/PassManager.h>
#include <llvm/ProfileData/InstrProf.h>
#include "Passes.h"

using namespace llvm;
// Create a runtime hook variable so it doesn't inadvertently end
// up being called by __llvm_profile_register_function. This is a
// bit stupid.
// We don't care if this variable is stripped out. This is just to
// work around the above mentioned bug in LLVM 17 instrprof lowering.
// See: InstrProfiling::emitRegistration()
PreservedAnalyses CreateInstrProfRuntimeHookPass::run(Module &M, ModuleAnalysisManager &AM) {
    auto *Int32Ty = Type::getInt32Ty(M.getContext());
    auto *Var = new GlobalVariable(M, Int32Ty, false,
            GlobalValue::ExternalLinkage, nullptr, getInstrProfRuntimeHookVarName());
    Var->setVisibility(GlobalValue::HiddenVisibility);
    errs() << "Added GPU runtime hook\n";
    return PreservedAnalyses::all();
}

