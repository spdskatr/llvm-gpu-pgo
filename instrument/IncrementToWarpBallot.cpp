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

/*
Unfortunately I have to implement everything here by hand 
(pseudo-)code for replacement code:
if (Val) {
    unsigned long long mask = __ballot(1);
    if (__lane_id() == __ctzll(mask)) {
        atomicAdd(&deviceCounter, __popcll(mask));
    }
}
NOTE: We assume that Val is either zero or one
*/
static void modifyIncrement(Module *M, CallInst *Call, Value *Val) {
    Type *Int64Ty = Type::getInt64Ty(M->getContext());
    Type *Int32Ty = Type::getInt32Ty(M->getContext());
    Type *MetadataTy = Type::getMetadataTy(M->getContext());
    // Get intrinsics
    Function *CTTZ = Intrinsic::getDeclaration(M, Intrinsic::cttz, Int64Ty);
    Function *CTPOP = Intrinsic::getDeclaration(M, Intrinsic::ctpop, Int64Ty);
    Function *ReadReg = Intrinsic::getDeclaration(M, Intrinsic::read_register, Int64Ty);
    Function *MbcntLo = Intrinsic::getDeclaration(M, Intrinsic::amdgcn_mbcnt_lo);
    Function *MbcntHi = Intrinsic::getDeclaration(M, Intrinsic::amdgcn_mbcnt_hi);

    // Get exec register
    Value *ExecReg = MetadataAsValue::get(M->getContext(), 
        MDNode::get(M->getContext(), {MDString::get(M->getContext(), "exec")}));
    // Get int -1
    Constant *LongZero = ConstantInt::get(Int64Ty, 0);
    Constant *IntZero = ConstantInt::get(Int32Ty, 0);
    Constant *IntNeg1 = ConstantInt::get(Int32Ty, -1);

    // Split up blocks around instruction
    BasicBlock *Before = Call->getParent();
    BasicBlock *Elect = Before->splitBasicBlock(Call, "elect");
    BasicBlock *B = Elect->splitBasicBlock(Call, "instrprof");
    BasicBlock *After = B->splitBasicBlock(Call->getNextNode(), Before->getName() + ".a");

    // BEFORE block
    // Branch to elect block iff val is 1
    Instruction *BeforeTerm = Before->getTerminator();
    IRBuilder<> BeforeB{BeforeTerm};
    BeforeB.CreateCondBr(BeforeB.CreateICmpNE(Val, LongZero), Elect, After);
    BeforeTerm->eraseFromParent();

    // ELECT block
    // Make a conditional branch to the increment instruction for
    // only the elected thread
    Instruction *ElectTerm = Elect->getTerminator();
    IRBuilder<> IRB{ElectTerm};
    // Elect a leader
    Value *Mask = IRB.CreateCall(ReadReg, { ExecReg });
    Value *LaneId = IRB.CreateCall(MbcntHi, { IntNeg1, IRB.CreateCall(MbcntLo, { IntNeg1, IntZero }) });
    Value *TZ = IRB.CreateCall(CTTZ, { Mask, 
        ConstantInt::getBool(M->getContext(), true) });
    Value *IsLeader = IRB.CreateICmpEQ(LaneId, 
        IRB.CreateIntCast(TZ, Int32Ty, false));
    // If leader, branch to profiling block
    IRB.CreateCondBr(IsLeader, B, After);
    // Now that we have our terminator we can erase the original one
    ElectTerm->eraseFromParent();

    // INSTRPROF block
    // Modify the increment intrinsic call
    auto IncrStep = Intrinsic::getDeclaration(M, Intrinsic::instrprof_increment_step);
    IRBuilder<> LeaderB{B->getTerminator()};
    CallInst *Count = LeaderB.CreateCall(CTPOP, { Mask });
    LeaderB.CreateCall(IncrStep, {
        Call->getArgOperand(0), 
        Call->getArgOperand(1), 
        Call->getArgOperand(2),
        Call->getArgOperand(3),
        Count});
    Call->eraseFromParent();
}

PreservedAnalyses IncrementToWarpBallotPass::run(Function &F, FunctionAnalysisManager &AM) {
    if (F.getParent()->getTargetTriple() == "amdgcn-amd-amdhsa") {
        Type *Int64Ty = Type::getInt64Ty(F.getContext());
        SmallVector<std::pair<CallInst *, Value *>, 0> Targets;
        for (Instruction &I : instructions(F)) {
            if (IntrinsicInst *Call = dyn_cast<IntrinsicInst>(&I)) {
                auto ID = Call->getIntrinsicID();
                if (ID == Intrinsic::instrprof_increment) {
                    Targets.emplace_back(Call, ConstantInt::get(Int64Ty, 1));
                } else if (ID == Intrinsic::instrprof_increment_step) {
                    Targets.emplace_back(Call, Call->getArgOperand(4));
                }
            }
        }
        for (auto& [Call, Val] : Targets) {
            modifyIncrement(F.getParent(), Call, Val);
        }
    }
    return PreservedAnalyses::all();
}