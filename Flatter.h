#ifndef LLVM_OBFUSCATOR_FLATTER_H
#define LLVM_OBFUSCATOR_FLATTER_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

struct Flatter : public llvm::PassInfoMixin<Flatter> {
  // requirements
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &);
  bool runOnModule(llvm::Module &M);

  // plugin
  bool handleInst(llvm::Instruction& inst);
  void printOperands(llvm::Instruction& inst);
  void printInst(llvm::Instruction& inst);
};

struct LegacyFlatter : public llvm::ModulePass {
  static char ID;
  LegacyFlatter() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;

  Flatter Impl;
};

#endif
