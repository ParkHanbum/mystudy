#ifndef _SKELETON_BASIC_H_
#define _SKELETON_BASIC_H_

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

#include "common.h"

struct Skeleton : public llvm::PassInfoMixin<Skeleton> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &);
  bool runOnModule(llvm::Module &M);
};

struct LegacySkeleton : public llvm::ModulePass {
  static char ID;
  LegacySkeleton() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;

  Skeleton Impl;
};

#endif
