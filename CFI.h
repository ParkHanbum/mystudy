#ifndef LLVM_TUTOR_INSTRUMENT_BASIC_H
#define LLVM_TUTOR_INSTRUMENT_BASIC_H

#include "common.h"

struct CFI : public llvm::PassInfoMixin<CFI> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &);
  bool runOnModule(llvm::Module &M);
  void handleFunction(llvm::Function *func);
  void handleBB(llvm::BasicBlock *bb);
  void handleInst(llvm::Instruction *inst);
};

struct LegacyCFI : public llvm::ModulePass {
  static char ID;
  LegacyCFI() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;

  CFI Impl;
};

#endif
