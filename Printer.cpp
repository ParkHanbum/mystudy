#include "Printer.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"

using namespace llvm;

void Printer::printOperands(llvm::Instruction& inst)
{
  for (User::op_iterator start = inst.op_begin(), end = inst.op_end(); start != end; ++start) {
    Value *el = start->get();	
    errs() << "\n\t opcode : " << el->getName() << "\t type : "; 
    if (el->getType())
      el->getType()->print(errs(), false);
  }
}

void Printer::printInst(llvm::Instruction& inst)
{
  errs() << "\n=================================================================\n";
  errs() << "Instruction : " <<  inst.getOpcodeName() << " \t print: ";
  inst.print(errs(), false);
  printOperands(inst);
}

bool Printer::handleInst(llvm::Instruction& inst)
{

  printInst(inst);
  return true;
}


//-----------------------------------------------------------------------------
// Printer implementation
//-----------------------------------------------------------------------------
bool Printer::runOnModule(Module &M) {
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
        handleInst(Ins);
      }
    }
  }

  return true;
}

PreservedAnalyses Printer::run(llvm::Module &M,
                                          llvm::ModuleAnalysisManager &) {
  bool Changed = runOnModule(M);

  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}

bool LegacyPrinter::runOnModule(llvm::Module &M) {
  bool Changed = Impl.runOnModule(M);

  return Changed;
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getPrinterPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "Printer", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "Printer") {
                    MPM.addPass(Printer());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getPrinterPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
char LegacyPrinter::ID = 0;

// Register the pass - required for (among others) opt
static RegisterPass<LegacyPrinter>
    X(/*PassArg=*/"Printer",
      /*Name=*/"LegacyPrinter",
      /*CFGOnly=*/false,
      /*is_analysis=*/false);
