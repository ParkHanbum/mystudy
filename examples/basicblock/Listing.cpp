#include "Listing.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"


using namespace llvm;


//-----------------------------------------------------------------------------
// Listing implementation
//-----------------------------------------------------------------------------
bool Listing::runOnModule(Module &M) {
  bool Instrumented = false;

  // Function name <--> IR variable that holds the call counter
  llvm::StringMap<Constant *> CallCounterMap;
  // Function name <--> IR variable that holds the function name
  llvm::StringMap<Constant *> FuncNameMap;

  auto &CTX = M.getContext();

  for (auto &Func : M) {
    errs() << "Listing BasicBlock by using List";
    for (Function::iterator iter = Func.begin(), E = Func.end(); iter != E; ++iter)
    {
      BasicBlock *bb = &*iter;
      errs() << "\n======================== [bb] =========================\n";
      bb->print(errs(), false);
      errs() << "\n======================== [bb] =========================\n";
    }


    errs() << "Listing BasicBl ck by using iterator";
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

PreservedAnalyses Listing::run(llvm::Module &M,
    llvm::ModuleAnalysisManager &) {
  bool Changed = runOnModule(M);

  return (Changed ? llvm::PreservedAnalyses::none()
      : llvm::PreservedAnalyses::all());
}

bool LegacyListing::runOnModule(llvm::Module &M) {
  bool Changed = Impl.runOnModule(M);

  return Changed;
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getListingPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "Listing", LLVM_VERSION_STRING,
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
          [](StringRef Name, ModulePassManager &MPM,
            ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "Listing") {
          MPM.addPass(Listing());
          return true;
          }
          return false;
          });
    }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getListingPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
char LegacyListing::ID = 0;

// Register the pass - required for (among others) opt
static RegisterPass<LegacyListing>
X(/*PassArg=*/"Listing",
    /*Name=*/"LegacyListing",
    /*CFGOnly=*/false,
    /*is_analysis=*/false);
