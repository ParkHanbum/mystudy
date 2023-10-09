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

#include "llvm/IR/Function.h"
#include "llvm/InitializePasses.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"


using namespace llvm;

#define DEBUG_TYPE "loop-opt-tutorial"

#define LLVM_DEBUG(X)                                                              \
  do {                                                                             \
    dbgs() << "[" << DEBUG_TYPE << "]" << __FUNCTION__ << ":" << __LINE__ << "\t"; \
    X;                                                                             \
  } while (false)

static bool we_are_not_prepared(Loop &L) {
  bool Changed = false;

  LLVM_DEBUG(dbgs() << "Entering LoopOptTutorialPass::run\n");
  LLVM_DEBUG(dbgs() << "Loop: ";  dbgs() << "\n");

  return Changed;
}

PreservedAnalyses LoopOptTutorialPass::run(Loop &L, LoopAnalysisManager &LAM,
                                           LoopStandardAnalysisResults &AR,
                                           LPMUpdater &) {
  bool Changed = false;

  Changed = we_are_not_prepared(L);
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