#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

struct GPURTLibInteropPass : public llvm::PassInfoMixin<GPURTLibInteropPass> {
    llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

struct GPUInstrPass : public llvm::PassInfoMixin<GPUInstrPass> {
    llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};