#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/IR/IRBuilder.h"

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Operator.h"

#include "../../common.h"

using namespace llvm;


namespace {
  // Hello2 - The second implementation with getAnalysisUsage implemented.
  struct Hello : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    const DataLayout *TD;
    Hello() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      for (auto &BB : F) {
        for (auto &Inst : BB) {
          APInt offset(64, 0);
          if (isa<GetElementPtrInst>(Inst))
          {
            GetElementPtrInst &gepi = cast<GetElementPtrInst>(Inst);
            GEPOperator &op = cast<GEPOperator>(gepi);

            gepi.accumulateConstantOffset(F.getParent()->getDataLayout(), offset);

            debugOperands(&gepi);
            Type *ty = gepi.getSourceElementType();
            Type *ty2 = gepi.getResultElementType();
            op.print(errs());
            errs() << "\t[SRC] "; 
            ty->print(errs());
            errs() << "\t[RES] ";
            ty2->print(errs());
            errs() << "\t" << offset << "\n\n";
          }
        }
      }

      return false;
    }

    // We don't modify the program, so we preserve all analyses.
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesAll();
      // no longer needed since datalayout merged into module.
      //AU.addRequired<DataLayout>();
    }
  };
}

char Hello::ID = 0;
static RegisterPass<Hello>
Y("Hello", "Hello World Pass (with getAnalysisUsage implemented)");
