#ifndef LLVM_TUTOR_INSTRUMENT_BASIC_H
#define LLVM_TUTOR_INSTRUMENT_BASIC_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

struct Printer : public llvm::PassInfoMixin<Printer> {
  bool handleInst(llvm::Instruction& inst);
  void printOperands(llvm::Instruction& inst);
  void printInst(llvm::Instruction& inst);
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &);
  bool runOnModule(llvm::Module &M);
};

struct LegacyPrinter : public llvm::ModulePass {
  static char ID;
  LegacyPrinter() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;

  Printer Impl;
};

#endif
