#ifndef LLVM_OBFUSCATOR_COMMON_H
#define LLVM_OBFUSCATOR_COMMON_H

#include <iostream>
#include <random>

#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"
#include "llvm/IR/Value.h"


using namespace llvm;

void debugOperands(Instruction *inst)
{
  for (User::op_iterator start = inst->op_begin(), end = inst->op_end(); start != end; ++start)
  {
    Value *el = start->get();
    errs() << "\t" << el->getName() << "\t type : ";
    if (el->getType())
    	el->getType()->print(errs(), false);
    errs() << "\n";
  }
}

inline void debugInst(llvm::Value *value)
{
  if (isa<Instruction>(value)) {
    Instruction *inst = dyn_cast<Instruction>(value);
    errs() << "=======================================================\n";
    errs() << "" << inst->getOpcodeName() << "\t";
    inst->print(errs(), false);
    errs() << "\n";
    debugOperands(inst);
    errs() << "=======================================================\n";
  }

  else if (isa<Constant>(value)) {
    Constant *cont = dyn_cast<Constant>(value);
    errs() << "=======================================================\n";
    cont->print(errs(), false);
    errs() << "\n";
    errs() << "=======================================================\n";
  }
}

int getRandomV()
{
  std::random_device rd;
  std::mt19937 gen(rd());
  // half of maximum int value.
  std::uniform_int_distribution<int> dist(1073741823, 2147483646);
  return dist(gen);
}


#endif
