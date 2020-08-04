//========================================================================
// FILE:
//    DynamicCallCounter.cpp
//
// DESCRIPTION:
//    Counts dynamic function calls in a module. `Dynamic` in this context means
//    runtime function calls (as opposed to static, i.e. compile time). Note
//    that runtime calls can only be analysed while the underlying module is
//    executing. In order to count them one has to instrument the input
//    module.
//
//    This pass adds/injects code that will count function calls at
//    runtime and prints the results when the module exits. More specifically:
//      1. For every function F _defined_ in M:
//          * defines a global variable, `i32 CounterFor_F`, initialised with 0
//          * adds instructions at the beginning of F that increment `CounterFor_F`
//            every time F executes
//      2. At the end of the module (after `main`), calls `printf_wrapper` that
//         prints the global call counters injected by this pass (e.g.
//         `CounterFor_F`). The definition of `printf_wrapper` is also inserted by
//         DynamicCallCounter.
//
//    To illustrate, the following code will be injected at the beginning of
//    function F (defined in the input module):
//    ```IR
//      %1 = load i32, i32* @CounterFor_F
//      %2 = add i32 1, %1
//      store i32 %2, i32* @CounterFor_F
//    ```
//    The following definition of `CounterFor_F` is also added:
//    ```IR
//      @CounterFor_foo = common global i32 0, align 4
//    ```
//
//    This pass will only count calls to functions _defined_ in the input
//    module. Functions that are only _declared_ (and defined elsewhere) are not
//    counted.
//
// USAGE:
//    1. Legacy pass manager:
//      $ opt -load <BUILD_DIR>/lib/libDynamicCallCounter.so `\`
//        --legacy-dynamic-cc <bitcode-file> -o instrumented.bin
//      $ lli instrumented.bin
//    2. New pass manager:
//      $ opt -load-pass-plugin <BUILD_DIR>/lib/libDynamicCallCounter.so `\`
//        -passes=-"dynamic-cc" <bitcode-file> -o instrumentend.bin
//      $ lli instrumented.bin
//
// License: MIT
//========================================================================
#include "ObfuscatorCall.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"

using namespace llvm;

#define DEBUG_TYPE "dynamic-cc"

Constant *CreateGlobalCounter(Module &M, StringRef GlobalVarName) {
  auto &CTX = M.getContext();

  // This will insert a declaration into M
  Constant *NewGlobalVar =
      M.getOrInsertGlobal(GlobalVarName, IntegerType::getInt32Ty(CTX));

  // This will change the declaration into definition (and initialise to 0)
  GlobalVariable *NewGV = M.getNamedGlobal(GlobalVarName);
  NewGV->setLinkage(GlobalValue::CommonLinkage);
  NewGV->setAlignment(MaybeAlign(4));
  NewGV->setInitializer(llvm::ConstantInt::get(CTX, APInt(32, 0)));

  return NewGlobalVar;
}

//-----------------------------------------------------------------------------
// DynamicCallCounter implementation
//-----------------------------------------------------------------------------
bool DynamicCallCounter::runOnModule(Module &M) {
  bool Instrumented = false;

  // Function name <--> IR variable that holds the call counter
  llvm::StringMap<Constant *> CallCounterMap;
  // Function name <--> IR variable that holds the function name
  llvm::StringMap<Constant *> FuncNameMap;

  auto &CTX = M.getContext();

  for (auto &Func : M) {
    std::string test = Func.getName();
    test += "A";
    Func.setName(test);
    /*
    for (auto &BB : Func) 
    {
      for (auto &Ins : BB) {
        auto ICS = ImmutableCallSite(&Ins);
        if (nullptr == ICS.getInstruction()) {
          continue;
        }
        // Check whether the called function is directly invoked
        auto DirectInvoc =
          dyn_cast<Function>(ICS.getCalledValue()->stripPointerCasts());
        if (nullptr == DirectInvoc) {
          continue;
        }
      }
    }
    */
  }

  return true;
}

PreservedAnalyses DynamicCallCounter::run(llvm::Module &M,
                                          llvm::ModuleAnalysisManager &) {
  bool Changed = runOnModule(M);

  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}

bool LegacyDynamicCallCounter::runOnModule(llvm::Module &M) {
  bool Changed = Impl.runOnModule(M);

  return Changed;
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getDynamicCallCounterPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "ObfuscatorCall", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "ObfuscatorCall") {
                    MPM.addPass(DynamicCallCounter());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getDynamicCallCounterPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
char LegacyDynamicCallCounter::ID = 0;

// Register the pass - required for (among others) opt
static RegisterPass<LegacyDynamicCallCounter>
    X(/*PassArg=*/"ObfuscatorCall",
      /*Name=*/"LegacyDynamicCallCounter",
      /*CFGOnly=*/false,
      /*is_analysis=*/false);
