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

void debugType(Type *ty)
{

}

void debugOperands(Instruction *inst)
{
  for (User::op_iterator start = inst->op_begin(), end = inst->op_end(); start != end; ++start)
  {
    Value *el = start->get();
    if (el->hasName()) {
      errs() << "\t" << el->getName() << " type : ";
      // el->print(errs(), true);
    }
    if (el->getType()) {
      auto *ty = el->getType();
      errs() << ty->getNumContainedTypes();
      switch(ty->getTypeID()) {
        case Type::FunctionTyID:
          errs() << "FunctionTy";
          break;
        case Type::PointerTyID:
          errs() << "PointerTy:"<< dyn_cast<PointerType>(ty)->getAddressSpace();
          break;
        case Type::StructTyID:
          errs() << "StructTy";
          break;
        case Type::IntegerTyID:
          errs() << "IntegerTy";
          break;
        default:
          errs() << ty->getTypeID();
          break;
      }
      errs() << "  ";
      ty->print(errs(), true, false);

    }
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

inline void debugBB(llvm::BasicBlock *bb)
{
  errs() << "==============[bb : " + bb->getName() + "]===============\n";

  for (BasicBlock::iterator iter = bb->begin(), E = bb->end(); iter != E; ++iter)
  {
    Instruction *inst = &*iter;
    debugInst(inst);
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
