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
          if (isa<GetElementPtrInst>(Inst))
          {
            GetElementPtrInst &gepi = cast<GetElementPtrInst>(Inst);
            GEPOperator &op = cast<GEPOperator>(gepi);
            op.print(errs());
            errs() << "\n";
          }
        }
      }
      APInt offset;

      return false;
    }

    // We don't modify the program, so we preserve all analyses.
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesAll();
      //-
      //AU.addRequired<DataLayout>();
    }
  };
}

char Hello::ID = 0;
static RegisterPass<Hello>
Y("Hello", "Hello World Pass (with getAnalysisUsage implemented)");
