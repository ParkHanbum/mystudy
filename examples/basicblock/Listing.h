#ifndef _BASIC_BLOCK_H_
#define _BASIC_BLOCK_H_

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

#include "../../common.h"

struct Listing : public llvm::PassInfoMixin<Listing> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &);
  bool runOnModule(llvm::Module &M);
};

struct LegacyListing : public llvm::ModulePass {
  static char ID;
  LegacyListing() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;

  Listing Impl;
};

#endif
