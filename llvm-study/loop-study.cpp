//===- LoopInfoTest.cpp - LoopInfo unit tests -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopNestAnalysis.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/ScalarEvolutionExpander.h"
#include "llvm/Transforms/Utils/SimplifyIndVar.h"
#include "gtest/gtest.h"

#include <fstream>
#include <iostream>

using namespace llvm;

#define LLVM_DEBUG(X) \
  do {                \
    X;                \
  } while (false)

// Flatten
Loop *InnerLoop, *OuterLoop;
PHINode *InnerInductionPHI = nullptr;
PHINode *OuterInductionPHI = nullptr;
Value *InnerTripCount = nullptr;
Value *OuterTripCount = nullptr;

SmallPtrSet<Value *, 4> LinearIVUses; // Contains the linear expressions
                                      // of the form i*M+j that will be
                                      // replaced.

BinaryOperator *InnerIncrement = nullptr; // Uses of induction variables in
BinaryOperator *OuterIncrement = nullptr; // loop control statements that
BranchInst *InnerBranch = nullptr;        // are safe to ignore.
BranchInst *OuterBranch = nullptr;        // The instruction that needs to be
                                          // updated with new tripcount.
SmallPtrSet<PHINode *, 4> InnerPHIsToTransform;
bool Widened = false;
PHINode *NarrowInnerInductionPHI = nullptr; // Holds the old/narrow induction
PHINode *NarrowOuterInductionPHI = nullptr; // phis, i.e. the Phis before IV
                                            // has been apllied. Used to skip
                                            // checks on phi nodes.

/// Build the loop info for the function and run the Test.
static void
runWithLoopInfo(Module &M, StringRef FuncName,
                function_ref<void(Function &F, LoopInfo &LI)> Test) {
  auto *F = M.getFunction(FuncName);
  // Compute the dominator tree and the loop info for the function.
  DominatorTree DT(*F);
  LoopInfo LI(DT);
  Test(*F, LI);
}

static std::unique_ptr<Module> makeLLVMModule(LLVMContext &Context,
                                              const char *ModuleStr) {
  SMDiagnostic Err;
  return parseAssemblyString(ModuleStr, Err, Context);
}

// Examine getUniqueExitBlocks/getUniqueNonLatchExitBlocks functions.
TEST(LoopInfoTest, LoopUniqueExitBlocks) {
  const char *ModuleStr =
      "target datalayout = \"e-m:o-i64:64-f80:128-n8:16:32:64-S128\"\n"
      "define void @foo(i32 %n, i1 %cond) {\n"
      "entry:\n"
      "  br label %for.cond\n"
      "for.cond:\n"
      "  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]\n"
      "  %cmp = icmp slt i32 %i.0, %n\n"
      "  br i1 %cond, label %for.inc, label %for.end1\n"
      "for.inc:\n"
      "  %inc = add nsw i32 %i.0, 1\n"
      "  br i1 %cmp, label %for.cond, label %for.end2, !llvm.loop !0\n"
      "for.end1:\n"
      "  br label %for.end\n"
      "for.end2:\n"
      "  br label %for.end\n"
      "for.end:\n"
      "  ret void\n"
      "}\n"
      "!0 = distinct !{!0, !1}\n"
      "!1 = !{!\"llvm.loop.distribute.enable\", i1 true}\n";

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, ModuleStr);

  runWithLoopInfo(*M, "foo", [&](Function &F, LoopInfo &LI) {
    Function::iterator FI = F.begin();
    // First basic block is entry - skip it.
    BasicBlock *Header = &*(++FI);
    assert(Header->getName() == "for.cond");
    Loop *L = LI.getLoopFor(Header);

    SmallVector<BasicBlock *, 2> Exits;
    // This loop has 2 unique exits.
    L->getUniqueExitBlocks(Exits);
    for (BasicBlock *bb : Exits)
      bb->print(errs());

    EXPECT_TRUE(Exits.size() == 2);
    // And one unique non latch exit.
    Exits.clear();
    L->getUniqueNonLatchExitBlocks(Exits);
    for (BasicBlock *bb : Exits)
      bb->print(errs());

    EXPECT_TRUE(Exits.size() == 1);
  });
}

#define BlockT BasicBlock
#define LoopT Loop
BlockT *getLoopPredecessor(Loop *L) {
  BlockT *Out = nullptr;
  BlockT *bb = L->getHeader();
  for (const auto Pred : children<Inverse<BlockT *>>(bb)) {
    if (!L->contains(Pred)) {
      if (Out && Out != Pred)
        return nullptr;
      Out = Pred;
    }
  }

  Out->print(errs());
  return Out;
}

/// getLoopLatch - If there is a single latch block for this loop, return it.
/// A latch block is a block that contains a branch back to the header.
BlockT *getLoopLatch(Loop *L) {
  BlockT *Header = L->getHeader();
  BlockT *Latch = nullptr;
  for (const auto Pred : children<Inverse<BlockT *>>(Header)) {
    if (L->contains(Pred)) {
      if (Latch)
        return nullptr;
      Latch = Pred;
    }
  }

  Latch->print(errs());
  return Latch;
}

bool hasDedicatedExits(Loop *L) {
  // Each predecessor of each exit block of a normal loop is contained
  // within the loop.
  SmallVector<BlockT *, 4> UniqueExitBlocks;
  L->getUniqueExitBlocks(UniqueExitBlocks);
  for (BlockT *EB : UniqueExitBlocks)
    for (BlockT *Predecessor : children<Inverse<BlockT *>>(EB))
      if (!L->contains(Predecessor))
        return false;
  // All the requirements are met.
  return true;
}

static bool isLoopSimplifyForm(Loop *L) {
  return getLoopPredecessor(L) && getLoopLatch(L) && hasDedicatedExits(L);
}

/// Get the latch condition instruction.
ICmpInst *getLatchCmpInst(Loop *L) {
  if (BasicBlock *Latch = L->getLoopLatch())
    if (BranchInst *BI = dyn_cast_or_null<BranchInst>(Latch->getTerminator()))
      if (BI->isConditional())
        return dyn_cast<ICmpInst>(BI->getCondition());

  return nullptr;
}

PHINode *getInductionVariable(Loop *L, ScalarEvolution &SE) {
  BasicBlock *Header = L->getHeader();
  ICmpInst *CmpInst = getLatchCmpInst(L);
  if (!CmpInst)
    return nullptr;

  Value *LatchCmpOp0 = CmpInst->getOperand(0);
  Value *LatchCmpOp1 = CmpInst->getOperand(1);
  errs() << "getInductionVariable : \n";
  errs() << "CmpInst : ";
  CmpInst->print(errs());
  errs() << "\nLatchCmpOp0 : ";
  LatchCmpOp0->print(errs());
  errs() << "\nLatchCmpOp1 : ";
  LatchCmpOp1->print(errs());

  for (PHINode &IndVar : Header->phis()) {
    InductionDescriptor IndDesc;
    if (!InductionDescriptor::isInductionPHI(&IndVar, L, &SE, IndDesc))
      continue;

    BasicBlock *Latch = getLoopLatch(L);
    Value *StepInst = IndVar.getIncomingValueForBlock(Latch);
    errs() << "PHINode IndVar : ";
    IndVar.print(errs());
    errs() << "\nPHINode IndVar getIncomingValueForBlock : ";
    StepInst->print(errs());
    errs() << "\n";

    // case 1:
    // IndVar = phi[{InitialValue, preheader}, {StepInst, latch}]
    // StepInst = IndVar + step
    // cmp = StepInst < FinalValue
    if (StepInst == LatchCmpOp0 || StepInst == LatchCmpOp1)
      return &IndVar;

    // case 2:
    // IndVar = phi[{InitialValue, preheader}, {StepInst, latch}]
    // StepInst = IndVar + step
    // cmp = IndVar < FinalValue
    if (&IndVar == LatchCmpOp0 || &IndVar == LatchCmpOp1)
      return &IndVar;
  }

  return nullptr;
}

bool getInductionDescriptor(Loop *L, ScalarEvolution &SE,
                            InductionDescriptor &IndDesc) {
  PHINode *IndVar = getInductionVariable(L, SE);
  errs() << "getInductionDescriptor : ";
  IndVar->print(errs());

  if (IndVar)
    return InductionDescriptor::isInductionPHI(IndVar, L, &SE, IndDesc);

  return false;
}

bool isCanonical(Loop *L, ScalarEvolution &SE) {
  InductionDescriptor IndDesc;
  if (!getInductionDescriptor(L, SE, IndDesc))
    return false;

  ConstantInt *Init = dyn_cast_or_null<ConstantInt>(IndDesc.getStartValue());
  if (!Init || !Init->isZero())
    return false;

  if (IndDesc.getInductionOpcode() != Instruction::Add)
    return false;

  ConstantInt *Step = IndDesc.getConstIntStepValue();
  if (!Step || !Step->isOne())
    return false;

  return true;
}

static bool findLoopComponents(
    Loop *L, SmallPtrSetImpl<Instruction *> &IterationInstructions,
    PHINode *&InductionPHI, Value *&TripCount, BinaryOperator *&Increment,
    BranchInst *&BackBranch, ScalarEvolution *SE, bool IsWidened) {
  if (!isLoopSimplifyForm(L)) {
    errs() << "Loop is not normal form\n";
    return false;
  }

  // Currently, to simplify the implementation, the Loop induction variable must
  // start at zero and increment with a step size of one.
  if (!isCanonical(L, *SE)) {
    errs() << "Loop is not canonical\n";
    return false;
  }

  // There must be exactly one exiting block, and it must be the same at the
  // latch.
  BasicBlock *Latch = L->getLoopLatch();
  if (L->getExitingBlock() != Latch) {
    dbgs() << "Exiting and latch block are different\n";
    return false;
  }

  // Find the induction PHI. If there is no induction PHI, we can't do the
  // transformation. TODO: could other variables trigger this? Do we have to
  // search for the best one?
  InductionPHI = L->getInductionVariable(*SE);
  if (!InductionPHI) {
    dbgs() << "Could not find induction PHI\n";
    return false;
  }
  dbgs() << "\n"
         << "Found induction PHI: ";
  InductionPHI->dump();

  bool ContinueOnTrue = L->contains(Latch->getTerminator()->getSuccessor(0));
  auto IsValidPredicate = [&](ICmpInst::Predicate Pred) {
    if (ContinueOnTrue)
      return Pred == CmpInst::ICMP_NE || Pred == CmpInst::ICMP_ULT;
    else
      return Pred == CmpInst::ICMP_EQ;
  };

  // Find Compare and make sure it is valid. getLatchCmpInst checks that the
  // back branch of the latch is conditional.
  ICmpInst *Compare = L->getLatchCmpInst();
  if (!Compare || !IsValidPredicate(Compare->getUnsignedPredicate()) ||
      Compare->hasNUsesOrMore(2)) {
    dbgs() << "Could not find valid comparison\n";
    return false;
  }
  BackBranch = cast<BranchInst>(Latch->getTerminator());
  IterationInstructions.insert(BackBranch);
  dbgs() << "Found back branch: ";
  BackBranch->dump();
  IterationInstructions.insert(Compare);
  dbgs() << "Found comparison: ";
  Compare->dump();

  // Find increment and trip count.
  // There are exactly 2 incoming values to the induction phi; one from the
  // pre-header and one from the latch. The incoming latch value is the
  // increment variable.
  Increment =
      cast<BinaryOperator>(InductionPHI->getIncomingValueForBlock(Latch));
  if (Increment->hasNUsesOrMore(3)) {
    dbgs() << "Could not find valid increment\n";
    return false;
  }
  // The trip count is the RHS of the compare. If this doesn't match the trip
  // count computed by SCEV then this is because the trip count variable
  // has been widened so the types don't match, or because it is a constant and
  // another transformation has changed the compare (e.g. icmp ult %inc,
  // tripcount -> icmp ult %j, tripcount-1), or both.
  Value *RHS = Compare->getOperand(1);

  return true;
}

static bool DoFlattenLoopPair(DominatorTree *DT, LoopInfo *LI,
                              ScalarEvolution *SE, AssumptionCache *AC,
                              const TargetTransformInfo *TTI, LPMUpdater *U,
                              MemorySSAUpdater *MSSAU) {
  Function *F = OuterLoop->getHeader()->getParent();
  Value *NewTripCount = BinaryOperator::CreateMul(
      InnerTripCount, OuterTripCount, "flatten.tripcount",
      OuterLoop->getLoopPreheader()->getTerminator());
  dbgs() << "Created new trip count in preheader: ";
  NewTripCount->dump();

  // Fix up PHI nodes that take values from the inner loop back-edge, which
  // we are about to remove.
  InnerInductionPHI->removeIncomingValue(InnerLoop->getLoopLatch());

  // The old Phi will be optimised away later, but for now we can't leave
  // leave it in an invalid state, so are updating them too.
  for (PHINode *PHI : InnerPHIsToTransform)
    PHI->removeIncomingValue(InnerLoop->getLoopLatch());

  // Modify the trip count of the outer loop to be the product of the two
  // trip counts.
  cast<User>(OuterBranch->getCondition())->setOperand(1, NewTripCount);

  // Replace the inner loop backedge with an unconditional branch to the exit.
  BasicBlock *InnerExitBlock = InnerLoop->getExitBlock();
  BasicBlock *InnerExitingBlock = InnerLoop->getExitingBlock();
  InnerExitingBlock->getTerminator()->eraseFromParent();
  BranchInst::Create(InnerExitBlock, InnerExitingBlock);

  // Update the DomTree and MemorySSA.
  DT->deleteEdge(InnerExitingBlock, InnerLoop->getHeader());
  if (MSSAU)
    MSSAU->removeEdge(InnerExitingBlock, InnerLoop->getHeader());

  // Replace all uses of the polynomial calculated from the two induction
  // variables with the one new one.
  IRBuilder<> Builder(OuterInductionPHI->getParent()->getTerminator());
  for (Value *V : LinearIVUses) {
    Value *OuterValue = OuterInductionPHI;
    if (Widened)
      OuterValue = Builder.CreateTrunc(OuterInductionPHI, V->getType(),
                                       "flatten.trunciv");

    dbgs() << "Replacing: ";
    V->dump();
    dbgs() << "with:      ";
    OuterValue->dump();
    V->replaceAllUsesWith(OuterValue);
  }

  // Tell LoopInfo, SCEV and the pass manager that the inner loop has been
  // deleted, and any information that have about the outer loop invalidated.
  SE->forgetLoop(OuterLoop);
  SE->forgetLoop(InnerLoop);
  if (U)
    U->markLoopAsDeleted(*InnerLoop, InnerLoop->getName());
  LI->erase(InnerLoop);

  return true;
}

// lets find out how used loop related functions in loopflatten
TEST(LoopInfoTest, LoopFlatten) {
  std::stringstream IR;
  std::ifstream in("Loop/loop-flatten-test1.ll");
  IR << in.rdbuf();

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());

  auto *F = M->getFunction("test1");
  TargetLibraryInfoImpl TLII;
  TargetLibraryInfo TLI(TLII);
  AssumptionCache AC(*F);
  DominatorTree DT(*F);
  LoopInfo LI(DT);
  ScalarEvolution SE(*F, TLI, AC, DT, LI);
  AAResults AA(TLI);
  DataLayout DL(F->getParent());
  TargetTransformInfo TTI(DL);

  MemorySSA MSSA(*F, &AA, &DT);
  MemorySSAUpdater MSSAU(&MSSA);

  errs() << "============ START DIGGING TO LOOPFLATTEN =============\n";
  SmallPtrSet<Instruction *, 8> IterationInstructions;

  // lets into flatten starting
  errs() << "LOOP NEST ITERATING : \n";
  std::unique_ptr<LoopNest> LN;
  for (Loop *L : LI) {
    LN = LoopNest::getLoopNest(*L, SE);
  }
  errs() << *LN << "\n";

  for (Loop *loop : LN->getLoops()) {
    OuterLoop = loop->getParentLoop();
    if (!OuterLoop)
      continue;
    InnerLoop = loop;
  }
  errs() << "OuterLoop : ";
  OuterLoop->print(errs());
  if (!findLoopComponents(InnerLoop, IterationInstructions,
                          InnerInductionPHI, InnerTripCount,
                          InnerIncrement, InnerBranch, &SE, Widened))
    FAIL() << "Could not found Loop Componenets\n";
  errs() << "InnerLoop : ";
  InnerLoop->print(errs());
  if (!findLoopComponents(OuterLoop, IterationInstructions,
                          OuterInductionPHI, OuterTripCount,
                          OuterIncrement, OuterBranch, &SE, Widened))
    FAIL() << "Could not found Loop Componenets\n";

  // DoFlattenLoopPair(&DT, &LI, &SE, &AC, &TTI, nullptr, &MSSAU);
  errs() << "\n=======================FIN=============================\n";
}

TEST(LoopInfoTest, HowToFindTripCount) {
  const char *ModuleStr =
      "target datalayout = \"e-m:o-i64:64-f80:128-n8:16:32:64-S128\"\n"
      "define void @foo(i32 %n, i1 %cond) {\n"
      "entry:\n"
      "  br label %for.cond\n"
      "for.cond:\n"
      "  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]\n"
      "  %cmp = icmp slt i32 %i.0, %n\n"
      "  br i1 %cond, label %for.inc, label %for.end1\n"
      "for.inc:\n"
      "  %inc = add nsw i32 %i.0, 1\n"
      "  br i1 %cmp, label %for.cond, label %for.end2, !llvm.loop !0\n"
      "for.end1:\n"
      "  br label %for.end\n"
      "for.end2:\n"
      "  br label %for.end\n"
      "for.end:\n"
      "  ret void\n"
      "}\n"
      "!0 = distinct !{!0, !1}\n"
      "!1 = !{!\"llvm.loop.distribute.enable\", i1 true}\n";

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, ModuleStr);
  auto *F = M->getFunction("foo");
  TargetLibraryInfoImpl TLII;
  TargetLibraryInfo TLI(TLII);
  AssumptionCache AC(*F);
  DominatorTree DT(*F);
  LoopInfo LI(DT);
  ScalarEvolution SE(*F, TLI, AC, DT, LI);
  AAResults AA(TLI);
  DataLayout DL(F->getParent());
  TargetTransformInfo TTI(DL);
  MemorySSA MSSA(*F, &AA, &DT);
  MemorySSAUpdater MSSAU(&MSSA);

  Function::iterator FI = F->begin();
  // First basic block is entry - skip it.
  BasicBlock *Header = &*(++FI);
  assert(Header->getName() == "for.cond");
  Loop *L = LI.getLoopFor(Header);

  // Find the induction PHI. If there is no induction PHI, we can't do the
  // transformation. TODO: could other variables trigger this? Do we have to
  // search for the best one?
  PHINode *InductionPHI = L->getInductionVariable(SE);
  if (!InductionPHI) {
    dbgs() << "Could not find induction PHI\n";
  }
  dbgs() << "\n"
         << "Found induction PHI: ";
  InductionPHI->dump();

  BasicBlock *Latch = L->getLoopLatch();
  bool ContinueOnTrue = L->contains(Latch->getTerminator()->getSuccessor(0));
  auto IsValidPredicate = [&](ICmpInst::Predicate Pred) {
    if (ContinueOnTrue)
      return Pred == CmpInst::ICMP_NE || Pred == CmpInst::ICMP_ULT;
    else
      return Pred == CmpInst::ICMP_EQ;
  };

  ICmpInst *Compare = L->getLatchCmpInst();
  if (!Compare || !IsValidPredicate(Compare->getUnsignedPredicate()) ||
      Compare->hasNUsesOrMore(2)) {
    LLVM_DEBUG(dbgs() << "Could not find valid comparison\n");
    FAIL();
  }

  // Find increment and trip count.
  // There are exactly 2 incoming values to the induction phi; one from the
  // pre-header and one from the latch. The incoming latch value is the
  // increment variable.
  BinaryOperator *Increment =
      cast<BinaryOperator>(InductionPHI->getIncomingValueForBlock(Latch));
  if (Increment->hasNUsesOrMore(3)) {
    LLVM_DEBUG(dbgs() << "Could not find valid increment\n");
    FAIL();
  }

  // The trip count is the RHS of the compare. If this doesn't match the trip
  // count computed by SCEV then this is because the trip count variable
  // has been widened so the types don't match, or because it is a constant and
  // another transformation has changed the compare (e.g. icmp ult %inc,
  // tripcount -> icmp ult %j, tripcount-1), or both.
  Value *RHS = Compare->getOperand(1);

  dbgs() << "ComPare : ";
  Compare->dump();
  dbgs() << "TripCount : ";
  RHS->dump();
  dbgs() << "FIN";
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}