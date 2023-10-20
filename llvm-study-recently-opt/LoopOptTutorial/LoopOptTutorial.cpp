//===-------- LoopOptTutorial.cpp - Loop Opt Tutorial Pass ------*- C++ -*-===//
//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// This file contains a small loop pass to used to illustrate several aspects
/// writing a loop optimization. It was developed as part of the "Writing a Loop
/// Optimization" tutorial, presented at LLVM Devepeloper's Conference, 2019.
//===----------------------------------------------------------------------===

#include "LoopOptTutorial.h"
#include "llvm/Analysis/IVDescriptors.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopPass.h"

#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

#define DEBUG_TYPE "loop-opt-tutorial"

#define LLVM_DEBUG(X)                                                              \
  do {                                                                             \
    dbgs() << "[" << DEBUG_TYPE << "]" << __FUNCTION__ << ":" << __LINE__ << "\t"; \
    X;                                                                             \
  } while (false)

#define LLVM_DEBUGM(X)                                                                    \
  do {                                                                                    \
    dbgs() << "[" << DEBUG_TYPE << "]" << __FUNCTION__ << ":" << __LINE__ << "\t" X "\n"; \
  } while (false)


static void autopsy_loop(Loop *L, ScalarEvolution *SE) {
  BasicBlock *Header = L->getHeader();
  LLVM_DEBUGM("[LoopHeader] : ";); Header->dump();
  BasicBlock *latch = L->getLoopLatch();
  LLVM_DEBUGM("[LoopLatch] : ";); latch->dump();
  BranchInst *BI = dyn_cast_or_null<BranchInst>(latch->getTerminator());
  LLVM_DEBUGM("[LoopLatch Inst] : ";); BI->dump();

  if (BI->isConditional()) {
    ICmpInst *cmp = dyn_cast<ICmpInst>(BI->getCondition());
    Value *Op0 = cmp->getOperand(0);
    Value *Op1 = cmp->getOperand(1);
    Op0->dump();
    Op1->dump();
  }

  for (PHINode &PHI : Header->phis()) {
    InductionDescriptor ID;
    if (!InductionDescriptor::isInductionPHI(&PHI, L, SE, ID))
      continue;

    LLVM_DEBUGM("PHI IndVar : ";); PHI.dump();
    Value *StepInst = PHI.getIncomingValueForBlock(latch);
    if (StepInst)
      LLVM_DEBUGM("PHI IndVar getIncomingValueForBlock : "; StepInst->dump(););
  }
}

/// Clones a loop \p OrigLoop.  Returns the loop and the blocks in \p
/// Blocks.
/// Updates LoopInfo assuming the loop is dominated by block \p LoopDomBB.
/// Insert the new blocks before block specified in \p Before.
static Loop *myCloneLoopWithPreheader(BasicBlock *Before, BasicBlock *LoopDomBB,
                                      Loop *OrigLoop, ValueToValueMapTy &VMap,
                                      const Twine &NameSuffix, LoopInfo *LI,
                                      DominatorTree *DT,
                                      SmallVectorImpl<BasicBlock *> &Blocks);

/// Clones a loop \p OrigLoop.  Returns the loop and the blocks in \p
/// Blocks.
/// Updates LoopInfo assuming the loop is dominated by block \p LoopDomBB.
/// Insert the new blocks before block specified in \p Before.
static Loop *myCloneLoopWithPreheader(BasicBlock *Before, BasicBlock *LoopDomBB,
                                      Loop *OrigLoop, ValueToValueMapTy &VMap,
                                      const Twine &NameSuffix, LoopInfo *LI,
                                      DominatorTree *DT,
                                      SmallVectorImpl<BasicBlock *> &Blocks) {
  Function *F = OrigLoop->getHeader()->getParent();
  Loop *ParentLoop = OrigLoop->getParentLoop();
  DenseMap<Loop *, Loop *> LMap;

  Loop *NewLoop = LI->AllocateLoop();
  LMap[OrigLoop] = NewLoop;
  if (ParentLoop)
    ParentLoop->addChildLoop(NewLoop);
  else
    LI->addTopLevelLoop(NewLoop);

  BasicBlock *OrigPH = OrigLoop->getLoopPreheader();
  assert(OrigPH && "No preheader");
  BasicBlock *NewPH = CloneBasicBlock(OrigPH, VMap, NameSuffix, F);
  // To rename the loop PHIs.
  VMap[OrigPH] = NewPH;
  Blocks.push_back(NewPH);

  // Update LoopInfo.
  if (ParentLoop)
    ParentLoop->addBasicBlockToLoop(NewPH, *LI);

  // Update DominatorTree.
  DT->addNewBlock(NewPH, LoopDomBB);

  for (Loop *CurLoop : OrigLoop->getLoopsInPreorder()) {
    Loop *&NewLoop = LMap[CurLoop];
    if (!NewLoop) {
      NewLoop = LI->AllocateLoop();

      // Establish the parent/child relationship.
      Loop *OrigParent = CurLoop->getParentLoop();
      assert(OrigParent && "Could not find the original parent loop");
      Loop *NewParentLoop = LMap[OrigParent];
      assert(NewParentLoop && "Could not find the new parent loop");

      NewParentLoop->addChildLoop(NewLoop);
    }
  }

  for (BasicBlock *BB : OrigLoop->getBlocks()) {
    Loop *CurLoop = LI->getLoopFor(BB);
    Loop *&NewLoop = LMap[CurLoop];
    assert(NewLoop && "Expecting new loop to be allocated");

    BasicBlock *NewBB = CloneBasicBlock(BB, VMap, NameSuffix, F);
    VMap[BB] = NewBB;

    // Update LoopInfo.
    NewLoop->addBasicBlockToLoop(NewBB, *LI);

    // Add DominatorTree node. After seeing all blocks, update to correct
    // IDom.
    DT->addNewBlock(NewBB, NewPH);

    Blocks.push_back(NewBB);
  }

  for (BasicBlock *BB : OrigLoop->getBlocks()) {
    // Update loop headers.
    Loop *CurLoop = LI->getLoopFor(BB);
    if (BB == CurLoop->getHeader())
      LMap[CurLoop]->moveToHeader(cast<BasicBlock>(VMap[BB]));

    // Update DominatorTree.
    BasicBlock *IDomBB = DT->getNode(BB)->getIDom()->getBlock();
    DT->changeImmediateDominator(cast<BasicBlock>(VMap[BB]),
                                 cast<BasicBlock>(VMap[IDomBB]));
  }

  // Move them physically from the end of the block list.
  F->splice(Before->getIterator(), F, NewPH->getIterator());
  F->splice(Before->getIterator(), F, NewLoop->getHeader()->getIterator(),
            F->end());

  return NewLoop;
}



//===----------------------------------------------------------------------===//
// LoopSplit implementation
//

void LoopSplit::dumpLoopFunction(const StringRef Msg, const Loop &L) const {
  const Function &F = *L.getHeader()->getParent();
  dbgs() << Msg;
  F.dump();
}

bool LoopSplit::isCandidate(const Loop &L) const {
  // Require loops with preheaders and dedicated exits
  if (!L.isLoopSimplifyForm())
    return false;
  // Since we use cloning to split the loop, it has to be safe to clone
  if (!L.isSafeToClone())
    return false;
  // If the loop has multiple exiting blocks, do not split
  if (!L.getExitingBlock())
    return false;
  // If loop has multiple exit blocks, do not split.
  if (!L.getExitBlock())
    return false;
  // Only split innermost loops. Thus, if the loop has any children, it cannot
  // be split.
  //auto Children = L.getSubLoops();
  if (!L.getSubLoops().empty())
    return false;
  return true;
}

bool LoopSplit::run(Loop &L) const {
  LLVM_DEBUG(dbgs() << "Entering \n");

  // First analyze the loop and prune invalid candidates.
  if (!isCandidate(L))
    return false;

  if (!isCandidate(L)) {
    LLVM_DEBUG(dbgs() << "Loop " << L.getName()
                      << " is not a candidate for splitting.\n");
    return false;
  }

  // Attempt to split the loop and report the result.
  if (!splitLoop(L))
    autopsy_loop(&L, &SE);

  return true;
}

bool LoopSplit::splitLoop(Loop &L) const {
  assert(L.isLoopSimplifyForm() && "Expecting a loop in simplify form");
  assert(L.isSafeToClone() && "Loop is not safe to be cloned");
  assert(L.getSubLoops().empty() && "Expecting a innermost loop");

  Instruction *Split =
      computeSplitPoint(L, L.getLoopPreheader()->getTerminator());

  // Split the loop preheader to create an insertion point for the cloned loop.
  BasicBlock *Preheader = L.getLoopPreheader();

  LLVM_DEBUG(dumpLoopFunction("Before splitting preheader:\n", L););
  BasicBlock *InsertBefore = SplitBlock(Preheader, Preheader->getTerminator(), &DT, &LI);
  LLVM_DEBUG(dumpLoopFunction("After splitting preheader:\n", L););

  LLVM_DEBUG(dbgs() << "InsertPoint: " << Preheader->getName() << "\n");
  Loop *ClonedLoop = cloneLoop(L, *InsertBefore, *Preheader);
  LLVM_DEBUG(dumpLoopFunction("After cloning the loop:\n", L););
  Preheader = ClonedLoop->getLoopPreheader();

  // Modify the upper bound of the cloned loop.
  ICmpInst *LatchCmpInst = getLatchCmpInst(*ClonedLoop);
  assert(LatchCmpInst && "Unable to find the latch comparison instruction");
  LatchCmpInst->setOperand(1, Split);

  // Modify the lower bound of the original loop.
  PHINode *IndVar = L.getInductionVariable(SE);
  assert(IndVar && "Unable to find the induction variable PHI node");
  IndVar->setIncomingValueForBlock(L.getLoopPreheader(), Split);

  return true;
}

Loop *LoopSplit::cloneLoop(Loop &L, BasicBlock &InsertBefore, BasicBlock &Pred) const {
  // Clone the original loop, insert the clone before the "InsertBefore" BB.
  Function &F = *L.getHeader()->getParent();
  SmallVector<BasicBlock *, 4> ClonedLoopBlocks;

  ValueToValueMapTy VMap;
  LLVM_DEBUG(dbgs() << "Preheader :\n"; InsertBefore.dump(); dbgs() << "\n");
  LLVM_DEBUG(dbgs() << "Pred :\n"; Pred.dump(); dbgs() << "\n");
  Loop *NewLoop = myCloneLoopWithPreheader(&InsertBefore, &Pred, &L, VMap,
                                           "ClonedLoop", &LI, &DT, ClonedLoopBlocks);
  assert(NewLoop && "Run ot of memory");
  LLVM_DEBUG(dbgs() << "Create new loop: " << NewLoop->getName() << "\n";
             dumpLoopFunction("After cloning loop:\n", L););

  // Update instructions referencing the original loop basic blocks to
  // reference the corresponding block in the cloned loop.
  VMap[L.getExitBlock()] = &InsertBefore;
  remapInstructionsInBlocks(ClonedLoopBlocks, VMap);
  LLVM_DEBUG(dumpLoopFunction("After instruction remapping:\n", L););

  // Make the predecessor of original loop jump to the cloned loop.
  LLVM_DEBUG(dbgs() << "replace cloned loop preheader direct to NewLoop\n");
  Pred.getTerminator()->replaceUsesOfWith(&InsertBefore,
                                          NewLoop->getLoopPreheader());

  return NewLoop;
}

Instruction *LoopSplit::computeSplitPoint(const Loop &L,
                                          Instruction *InsertBefore) const {
  std::optional<Loop::LoopBounds> Bounds = L.getBounds(SE);
  assert(Bounds.has_value() && "Unable to retrieve the loop bounds");

  Value &IVInitialVal = Bounds->getInitialIVValue();
  Value &IVFinalVal = Bounds->getFinalIVValue();
  auto *Sub =
    BinaryOperator::Create(Instruction::Sub, &IVFinalVal, &IVInitialVal, "", InsertBefore);

  return BinaryOperator::Create(Instruction::UDiv, Sub,
                                ConstantInt::get(IVFinalVal.getType(), 2),
                                "", InsertBefore);
}

ICmpInst *LoopSplit::getLatchCmpInst(const Loop &L) const {
  if (BasicBlock *Latch = L.getLoopLatch())
    if (BranchInst *BI = dyn_cast_or_null<BranchInst>(Latch->getTerminator()))
      if (BI->isConditional())
        return dyn_cast<ICmpInst>(BI->getCondition());

  return nullptr;
}

PreservedAnalyses LoopOptTutorialPass::run(Loop &L, LoopAnalysisManager &LAM,
                                           LoopStandardAnalysisResults &AR,
                                           LPMUpdater &) {
  bool Changed = false;

  Changed = LoopSplit(AR.LI, AR.DT, AR.SE).run(L);

  if (!Changed)
    return PreservedAnalyses::all();

  return getLoopPassPreservedAnalyses();
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION, "LoopOptTutorial", "v0.1",
      [](PassBuilder &PB) {
        PB.registerPipelineParsingCallback(
            [](StringRef Name, LoopPassManager &LPM,
               ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "loop-opt-tutorial") {
                LPM.addPass(LoopOptTutorialPass());
                return true;
              }
              return false;
            });
      }};
}