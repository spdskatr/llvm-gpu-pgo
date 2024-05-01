#pragma once
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

struct IncrementToWarpBallotPass : public llvm::PassInfoMixin<IncrementToWarpBallotPass> {
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
};

struct CreateInstrProfRuntimeHookPass : public llvm::PassInfoMixin<CreateInstrProfRuntimeHookPass> {
    llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

struct GPURTLibInteropPass : public llvm::PassInfoMixin<GPURTLibInteropPass> {
    llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

struct GPUInstrPass : public llvm::PassInfoMixin<GPUInstrPass> {
    llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};