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
using namespace std;

#define SPACE ' '

void debugType(Type *ty)
{

}

void debugOperands(Instruction *inst, int depth=0)
{
  for (User::op_iterator start = inst->op_begin(), end = inst->op_end(); start != end; ++start)
  {
    Value *el = start->get();
    if (el->hasName()) {
      errs() << string(depth+1, SPACE) << el->getName() << " type : ";
      // el->print(errs(), true);
    }
    if (el->getType()) {
      auto *ty = el->getType();
      errs() << string(depth+2, SPACE) << ty->getNumContainedTypes();
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
      errs() << SPACE;
      ty->print(errs(), true, false);

    }
    errs() << "\n";
  }
}

inline void debugInst(llvm::Value *value, int depth=0, bool print_op=false)
{
  if (isa<Instruction>(value)) {
    Instruction *inst = dyn_cast<Instruction>(value);

    errs() << string(depth, SPACE) << inst->getOpcodeName() << SPACE;
    if (isa<GetElementPtrInst>(value)) {
        errs() << "GEP : " << dyn_cast<GetElementPtrInst>(value)->getAddressSpace() << "\n";
        errs() << "GEP : " << dyn_cast<GetElementPtrInst>(value)->getPointerAddressSpace() << "\n";
    }
    inst->print(errs());
    errs() << "\n";
    if (print_op)
      debugOperands(inst, depth);
  }

  else if (isa<Constant>(value)) {
    Constant *cont = dyn_cast<Constant>(value);
    errs() << string(depth, SPACE);
    cont->print(errs());
    errs() << "\n";
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

inline void backtrace_operands(Instruction *inst, int depth=0)
{
  debugInst(inst, depth);
  for(User::op_iterator iter = inst->op_begin(), E = inst->op_end(); iter != E; ++iter)
  {
    Value *op = (&*iter)->get();

    if (isa<Instruction>(op))
      backtrace_operands(dyn_cast<Instruction>(op), depth+1);
    else {
      errs() << string(depth+1, SPACE);
      op->print(errs());
      errs() << '\n';
    }
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
