#ifndef LLVM_TUTOR_INSTRUMENT_BASIC_H
#define LLVM_TUTOR_INSTRUMENT_BASIC_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "common.h"

struct ObfuscatorCall : public llvm::PassInfoMixin<ObfuscatorCall> {
  bool handleInst(llvm::Instruction& inst);
  void printOperands(llvm::Instruction& inst);
  void handleCallInst(llvm::Instruction& inst);
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &);
  bool runOnModule(llvm::Module &M);
};

struct LegacyObfuscatorCall : public llvm::ModulePass {
  static char ID;
  LegacyObfuscatorCall() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;

  ObfuscatorCall Impl;
};

#endif
