#pragma once
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

struct IncrementToWarpBallotPass
    : public llvm::PassInfoMixin<IncrementToWarpBallotPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};

struct CreateInstrProfRuntimeHookPass
    : public llvm::PassInfoMixin<CreateInstrProfRuntimeHookPass> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

class GPURTLibInteropPass : public llvm::PassInfoMixin<GPURTLibInteropPass> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

class GPUInstrPass : public llvm::PassInfoMixin<GPUInstrPass> {
  std::string UseProfilePath;

public:
  GPUInstrPass(std::string UseProfilePath = "") : UseProfilePath(UseProfilePath) {}
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
};

class RemoveProfileMetadataPass
    : public llvm::PassInfoMixin<RemoveProfileMetadataPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);
};