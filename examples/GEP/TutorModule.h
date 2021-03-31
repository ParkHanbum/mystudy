#ifndef _BASIC_BLOCK_H_
#define _BASIC_BLOCK_H_

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

#include "../../common.h"

struct TutorModule : public llvm::PassInfoMixin<TutorModule> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &);
  bool runOnModule(llvm::Module &M);
};

struct LegacyTutorModule : public llvm::ModulePass {
  static char ID;
  LegacyTutorModule() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;

  TutorModule Impl;
};

#endif
