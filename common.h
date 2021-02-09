#ifndef LLVM_OBFUSCATOR_COMMON_H
#define LLVM_OBFUSCATOR_COMMON_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"

using namespace llvm;

void debugOperands(Instruction *inst)
{
  for (User::op_iterator start = inst->op_begin(), end = inst->op_end(); start != end; ++start)
  {
    Value *el = start->get();
    errs() << "\n\t opcode : " << el->getName() << "\t type : ";
    el->getType()->print(errs(), false);
    errs() << "\n";
  }
}

inline void debugInst(llvm::Value *value)
{
  if (isa<Instruction>(value)) {
    Instruction *inst = dyn_cast<Instruction>(value);
    errs() << "=======================================================\n";
    errs() << "Instruction : " << inst->getOpcodeName() << " \t print: ";
    inst->print(errs(), false);
    debugOperands(inst);
    errs() << "=======================================================\n";
  }

  else if (isa<Constant>(value)) {
    Constant *cont = dyn_cast<Constant>(value);
    errs() << "=======================================================\n";
    cont->print(errs(), false);
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
