#ifndef LLVM_OBFUSCATOR_FLATTER_H
#define LLVM_OBFUSCATOR_FLATTER_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include <string>
#include <algorithm>

#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Comdat.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalIFunc.h"
#include "llvm/IR/GlobalIndirectSymbol.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/ModuleSummaryIndex.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Statepoint.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/TypeFinder.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/UseListOrder.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"

struct Flatter : public llvm::PassInfoMixin<Flatter> {
  // requirements
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &);
  bool runOnModule(llvm::Module &M);

  // plugin
  bool handleInst(llvm::Instruction& inst);
  bool handleInst(llvm::Instruction* inst);
  void transIf(llvm::BranchInst* inst, llvm::BasicBlock* br_bb, llvm::SwitchInst* Switch, llvm::BasicBlock* switch_bb, llvm::Value* Case);
  void printOperands(llvm::Instruction& inst);
  void printInst(llvm::Instruction& inst);
  void printInst(const llvm::Instruction* inst);

  void printBB(llvm::BasicBlock* bb);
  int getLabel(llvm::BasicBlock* bb);

  void flatting(llvm::Function* func);
};

struct LegacyFlatter : public llvm::ModulePass {
  static char ID;
  LegacyFlatter() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;

  Flatter Impl;
};

#endif
