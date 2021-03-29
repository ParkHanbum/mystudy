#include "Skeleton.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"

#include "common.h"

using namespace llvm;


//-----------------------------------------------------------------------------
// Skeleton implementation
//-----------------------------------------------------------------------------
bool Skeleton::runOnModule(Module &M) {
  bool Instrumented = false;

  // Function name <--> IR variable that holds the call counter
  llvm::StringMap<Constant *> CallCounterMap;
  // Function name <--> IR variable that holds the function name
  llvm::StringMap<Constant *> FuncNameMap;

  auto &CTX = M.getContext();


  for (auto &Func : M) {
    for (auto &BB : Func) {
      errs() << "\n======================== [bb] =========================\n";
      BB.print(errs(), false);
      errs() << "\n======================== [bb] =========================\n";

      for (auto &Ins : BB) {
        debugInst(&Ins);
      }
    }
  }

  return true;
}

PreservedAnalyses Skeleton::run(llvm::Module &M,
    llvm::ModuleAnalysisManager &) {
  bool Changed = runOnModule(M);

  return (Changed ? llvm::PreservedAnalyses::none()
      : llvm::PreservedAnalyses::all());
}

bool LegacySkeleton::runOnModule(llvm::Module &M) {
  bool Changed = Impl.runOnModule(M);

  return Changed;
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getSkeletonPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "Skeleton", LLVM_VERSION_STRING,
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
          [](StringRef Name, ModulePassManager &MPM,
            ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "Skeleton") {
          MPM.addPass(Skeleton());
          return true;
          }
          return false;
          });
    }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getSkeletonPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
char LegacySkeleton::ID = 0;

// Register the pass - required for (among others) opt
static RegisterPass<LegacySkeleton>
X(/*PassArg=*/"Skeleton",
    /*Name=*/"LegacySkeleton",
    /*CFGOnly=*/false,
    /*is_analysis=*/false);
