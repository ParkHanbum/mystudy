#include "CFI.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"

#include "common.h"

using namespace llvm;


void CFI::handleInst(Instruction *inst)
{
  if (isa<CallInst>(inst) || isa<InvokeInst>(inst))
  {
    auto *call = dyn_cast<CallInst>(inst);
    Function *func = call->getCalledFunction();

    if (func)
      func->print(errs(), false);
  }
}

void CFI::handleBB(BasicBlock *bb)
{
   for (BasicBlock::iterator iter = bb->begin(), E = bb->end(); iter != E; ++iter)
   {
     Instruction *inst = &*iter;
     handleInst(inst);
   }
}

void CFI::handleFunction(Function *func)
{
   for (Function::iterator BB = func->begin(), E = func->end(); BB != E; ++BB)
   {
     BasicBlock *bb = &*BB;
     handleBB(bb);
   }
}

//-----------------------------------------------------------------------------
// CFI implementation
//-----------------------------------------------------------------------------
bool CFI::runOnModule(Module &M) {
  for (auto &Func : M) {
    handleFunction(&Func);
  }

  return true;
}

PreservedAnalyses CFI::run(llvm::Module &M,
    llvm::ModuleAnalysisManager &) {
  bool Changed = runOnModule(M);

  return (Changed ? llvm::PreservedAnalyses::none()
      : llvm::PreservedAnalyses::all());
}

bool LegacyCFI::runOnModule(llvm::Module &M) {
  bool Changed = Impl.runOnModule(M);

  return Changed;
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getCFIPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "CFI", LLVM_VERSION_STRING,
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
          [](StringRef Name, ModulePassManager &MPM,
            ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "CFI") {
          MPM.addPass(CFI());
          return true;
          }
          return false;
          });
    }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getCFIPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
char LegacyCFI::ID = 0;

// Register the pass - required for (among others) opt
static RegisterPass<LegacyCFI>
X(/*PassArg=*/"CFI",
    /*Name=*/"LegacyCFI",
    /*CFGOnly=*/false,
    /*is_analysis=*/false);
