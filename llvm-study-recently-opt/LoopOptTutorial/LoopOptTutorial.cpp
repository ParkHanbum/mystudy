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

bool LoopSplit::run(Loop &L) const {
  // return splitLoop(L);
  // TODO:  filter non-candidates & diagnose them
  return splitLoopInHalf(L);
}

bool LoopSplit::splitLoop(Loop &L) const {
  assert(L.isLoopSimplifyForm() && "Expecting a loop in simplify form");
  assert(L.isSafeToClone() && "Loop is not safe to be cloned");

  // Clone the original loop.
  BasicBlock *Preheader = L.getLoopPreheader();
  BasicBlock *Pred = Preheader->getSinglePredecessor();

  assert(Pred && "Preheader does not have a single predecessor");

  ValueToValueMapTy VMap;
  LLVM_DEBUG(dbgs() << "InsertPoint: " << Preheader->getName() << "\n");
  Loop *ClonedLoop = cloneLoop(L, *Preheader, *Pred, VMap);
  LLVM_DEBUG(dbgs() << "Created " << ClonedLoop << ":" << *ClonedLoop << "\n");

  Preheader = ClonedLoop->getLoopPreheader();

  return true;
}

bool LoopSplit::splitLoopInHalf(Loop &L) const {
  assert(L.isLoopSimplifyForm() && "Expecting a loop in simplify form");
  assert(L.isSafeToClone() && "Loop is not safe to be cloned");
  assert(L.getSubLoops().empty() && "Expecting a innermost loop");

  // Split the loop preheader to create an insertion point for the cloned loop.
  BasicBlock *Preheader = L.getLoopPreheader();
  BasicBlock *Pred = Preheader->getSinglePredecessor();

  assert(Pred && "Preheader does not have a single predecessor");
  LLVM_DEBUG(dumpLoopFunction("Before splitting preheader:\n", L););
  BasicBlock *InsertBefore = SplitBlock(Preheader, Preheader->getTerminator(), &DT, &LI);
  LLVM_DEBUG(dumpLoopFunction("After splitting preheader:\n", L););

  LLVM_DEBUG(dbgs() << "InsertPoint: " << Preheader->getName() << "\n");
  Loop *ClonedLoop = cloneLoopInHalf(L, *InsertBefore, *Pred);
  LLVM_DEBUG(dumpLoopFunction("After cloning the loop:\n", L););
  Preheader = ClonedLoop->getLoopPreheader();

  return true;
}

Loop *LoopSplit::cloneLoopInHalf(Loop &L, BasicBlock &InsertBefore, BasicBlock &Pred) const {
  // Clone the original loop, insert the clone before the "InsertBefore" BB.
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

Loop *LoopSplit::cloneLoop(Loop &L, BasicBlock &Preheader, BasicBlock &Pred,
                           ValueToValueMapTy &VMap) const {
  assert(L.getSubLoops().empty() && "Expecting a innermost loop");

  BasicBlock *ExitBlock = L.getExitBlock();
  assert(ExitBlock && "Expecting outermost loop to have a valid exit block");

  // Clone the original loop and remap instructions in the cloned loop.
  SmallVector<BasicBlock *, 4> ClonedLoopBlocks;

  LLVM_DEBUG(dbgs() << "Preheader :\n"; Preheader.dump(); dbgs() << "\n");
  LLVM_DEBUG(dbgs() << "Pred :\n"; Pred.dump(); dbgs() << "\n");
  LLVM_DEBUG(dbgs() << "ExitBlock :\n"; ExitBlock->dump(); dbgs() << "\n");
  Loop *NewLoop = myCloneLoopWithPreheader(&Preheader, &Pred, &L,
                                           VMap, "", &LI, &DT, ClonedLoopBlocks);
  LLVM_DEBUG(dbgs() << "[origin Loop] \n"; L.dump(); dbgs() << "[cloned Loop] \n";
             NewLoop->dump(); dbgs() << "\n");
  VMap[ExitBlock] = &Preheader;

  LLVM_DEBUGM("[Cloned Blocks]");
  for (auto bb : ClonedLoopBlocks)
    LLVM_DEBUG(bb->dump(); dbgs() << "\n");

  remapInstructionsInBlocks(ClonedLoopBlocks, VMap);
  LLVM_DEBUGM("[Remapped Cloned Blocks]");
  for (auto bb : ClonedLoopBlocks)
    LLVM_DEBUG(bb->dump(); dbgs() << "\n");

  LLVM_DEBUG(dbgs() << "Pred Terminator : "; Pred.dump(); dbgs() << "\n");
  Pred.getTerminator()->replaceUsesOfWith(&Preheader,
                                          NewLoop->getLoopPreheader());

  return NewLoop;
}

static bool isCandidate(const Loop &L) {
  if (!L.isLoopSimplifyForm()) {
    LLVM_DEBUGM("[Reject Reason] Require loops with preheaders and dedicated exits");
    return false;
  }

  if (!L.isSafeToClone()) {
    LLVM_DEBUGM("[Reject Reason] Since we use cloning to split the loop, it has to be safe to clone");
    return false;
  }

  if (!L.getExitingBlock()) {
    LLVM_DEBUGM("[Reject Reason] If the loop has multiple exiting blocks, do not split");
    return false;
  }

  if (!L.getExitBlock()) {
    LLVM_DEBUGM("[Reject Reason] If loop has multiple exit blocks, do not split.");
    return false;
  }

  if (!L.getSubLoops().empty()) {
    LLVM_DEBUGM("Only split innermost loops. Thus, if the loop has any children, it cannot be split.");
    return false;
  }

  return true;
}

static bool doLoopOptimization(Loop &L, LoopInfo &LI, DominatorTree &DT) {
  bool Changed = false;

  LLVM_DEBUG(dbgs() << "Entering LoopOptTutorialPass::run\n");
  LLVM_DEBUG(dbgs() << "Loop: "; dbgs() << "\n");
  Changed = isCandidate(L);
  if (Changed != false) {
    LLVM_DEBUGM("This loop is right candidate");
    Changed = LoopSplit(LI, DT).run(L);
  }

  return Changed;
}

PreservedAnalyses LoopOptTutorialPass::run(Loop &L, LoopAnalysisManager &LAM,
                                           LoopStandardAnalysisResults &AR,
                                           LPMUpdater &) {
  bool Changed = false;

  Changed = doLoopOptimization(L, AR.LI, AR.DT);
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