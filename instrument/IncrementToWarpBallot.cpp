#include <cassert>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IntrinsicsAMDGPU.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Casting.h>
#include <utility>
#include "Passes.h"

using namespace llvm;

// Unfortunately I have to implement everything here by hand
// (pseudo-)code for replacement code:
//  unsigned long long mask = __ballot(1);
//  if (__lane_id() == __ctzll(mask)) {
//      atomicAdd(&deviceCounter, __popcll(mask));
//  }
static void modifyIncrement(Module &M, CallInst *Call, Value *Val) {
    Type *Int64Ty = Type::getInt64Ty(M.getContext());
    Type *Int32Ty = Type::getInt32Ty(M.getContext());
    Type *MetadataTy = Type::getMetadataTy(M.getContext());
    // Get intrinsics
    Function *CTTZ = Intrinsic::getDeclaration(&M, Intrinsic::cttz, Int64Ty);
    Function *CTPOP = Intrinsic::getDeclaration(&M, Intrinsic::ctpop, Int64Ty);
    Function *ReadReg = Intrinsic::getDeclaration(&M, Intrinsic::read_register, Int64Ty);
    Function *MbcntLo = Intrinsic::getDeclaration(&M, Intrinsic::amdgcn_mbcnt_lo);
    Function *MbcntHi = Intrinsic::getDeclaration(&M, Intrinsic::amdgcn_mbcnt_hi);

    // Get exec register
    Value *ExecReg = MetadataAsValue::get(M.getContext(), 
        MDNode::get(M.getContext(), {MDString::get(M.getContext(), "exec")}));
    // Get int -1
    Constant *IntZero = ConstantInt::get(Int32Ty, 0);
    Constant *IntNeg1 = ConstantInt::get(Int32Ty, -1);

    // Split up blocks around instruction
    BasicBlock *Before = Call->getParent();
    BasicBlock *B = Before->splitBasicBlock(Call, "instrprof");
    BasicBlock *After = B->splitBasicBlock(Call->getNextNode(), Before->getName() + ".a");

    // Make a conditional branch to the increment instruction for
    // only the elected thread
    Instruction *BeforeTerm = Before->getTerminator();
    IRBuilder<> IRB{BeforeTerm};
    // Elect a leader
    Value *Mask = IRB.CreateCall(ReadReg, { ExecReg });
    Value *LaneId = IRB.CreateCall(MbcntHi, { IntNeg1, IRB.CreateCall(MbcntLo, { IntNeg1, IntZero }) });
    Value *TZ = IRB.CreateCall(CTTZ, { Mask, 
        ConstantInt::getBool(M.getContext(), true) });
    Value *IsLeader = IRB.CreateICmpEQ(LaneId, 
        IRB.CreateIntCast(TZ, Int32Ty, false));
    // If leader, branch to profiling block
    IRB.CreateCondBr(IsLeader, B, After);
    // Now that we have our terminator we can erase the original one
    BeforeTerm->eraseFromParent();

    // Modify the increment intrinsic call
    auto IncrStep = Intrinsic::getDeclaration(&M, Intrinsic::instrprof_increment_step);
    IRBuilder<> LeaderB{B->getTerminator()};
    CallInst *Count = LeaderB.CreateCall(CTPOP, { Mask });
    Value *NewVal = LeaderB.CreateMul(Val, Count);
    LeaderB.CreateCall(IncrStep, {
        Call->getArgOperand(0), 
        Call->getArgOperand(1), 
        Call->getArgOperand(2),
        Call->getArgOperand(3),
        NewVal});
    Call->eraseFromParent();
}

PreservedAnalyses IncrementToWarpBallotPass::run(Module &M, ModuleAnalysisManager &AM) {
    if (M.getTargetTriple() == "amdgcn-amd-amdhsa") {
        Type *Int64Ty = Type::getInt64Ty(M.getContext());
        for (Function &F : M) {
            SmallVector<std::pair<CallInst *, Value *>, 0> Targets;
            for (Instruction &I : instructions(F)) {
                if (IntrinsicInst *Call = dyn_cast<IntrinsicInst>(&I)) {
                    auto ID = Call->getIntrinsicID();
                    if (ID == Intrinsic::instrprof_increment) {
                        Targets.emplace_back(Call, ConstantInt::get(Int64Ty, 1));
                    // TODO: Right now step increments aren't handled correctly
                    // since it doesn't take into account that threads could
                    // have different values. Will probably need some shfls
                    //} else if (ID == Intrinsic::instrprof_increment_step) {
                    //    Targets.emplace_back(Call, Call->getArgOperand(4));
                    }
                }
            }
            for (auto& [Call, Val] : Targets) {
                modifyIncrement(M, Call, Val);
            }
        }
    }
    return PreservedAnalyses::all();
}