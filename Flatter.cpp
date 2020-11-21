#include "Flatter.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Comdat.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalIFunc.h"
#include "llvm/IR/GlobalIndirectSymbol.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/ModuleSummaryIndex.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Statepoint.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/TypeFinder.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/UseListOrder.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"


using namespace llvm;

int Flatter::getLabel(llvm::BasicBlock* bb)
{
  std::string Str;

  if (bb->getName().empty()) {
    raw_string_ostream OS(Str);
    bb->printAsOperand(OS, false);
  }
  else {
    Str = bb->getName();
  }

  Str.erase(std::remove(Str.begin(), Str.end(), '%'), Str.end());
  errs() << "STR : " << Str;
  return std::stoi(Str);
}

void Flatter::printOperands(llvm::Instruction& inst)
{
  for (User::op_iterator start = inst.op_begin(), end = inst.op_end(); start != end; ++start) {
    Value *el = start->get();	
    std::string Str;

    if(el->getName().empty()) {
      raw_string_ostream OS(Str);
      el->printAsOperand(OS, false);
    } else {
      Str = el->getName();
    }

    errs() << "\n\t operand : " << Str << "\t type : "; 
    if (el->getType())
      el->getType()->print(errs(), false);
  }
}

void Flatter::printInst(llvm::Instruction& inst)
{
  errs() << "\n=================[Instruction]======================\n";
  errs() << "Instruction : " <<  inst.getOpcodeName() << " \t print: ";
  inst.print(errs(), false);
  printOperands(inst);
}

bool Flatter::handleInst(llvm::Instruction* inst)
{
  errs() << "\n=================[Instruction]======================\n";
  errs() << "Instruction : " <<  inst->getOpcodeName() << " \t print: ";
  inst->print(errs(), false);
  return true;
}

bool Flatter::handleInst(llvm::Instruction& inst)
{
  printInst(inst);
  return true;
}

void Flatter::printBB(llvm::BasicBlock* bb)
{
  errs() << "\n=================[BasicBlock]======================\n";
  bb->print(errs(), false);
}

//-----------------------------------------------------------------------------
// Flatter implementation
//-----------------------------------------------------------------------------
bool Flatter::runOnModule(Module &M) {
  bool Instrumented = false;

  // Function name <--> IR variable that holds the call counter
  llvm::StringMap<Constant *> CallCounterMap;
  // Function name <--> IR variable that holds the function name
  llvm::StringMap<Constant *> FuncNameMap;

  auto &CTX = M.getContext();

  Instruction *CMP = NULL;
  Instruction *defaultInst = NULL;
  BasicBlock *defaultBB = NULL;
  SwitchInst *Switch = NULL;
  for (auto &Func : M) {
    for (auto &BB : Func) {
      for (Instruction &I : BB) {
        if (isa<CmpInst>(I)) {
          CMP = dyn_cast<CmpInst>(&I); continue;
        }
        if (CMP && isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
          BranchInst *BI = dyn_cast<BranchInst>(&I);

          if (defaultBB == NULL) {
            defaultBB = BasicBlock::Create(Func.getContext(), "Flatting", &Func);
          }

          IRBuilder<> builder(&BB);
          IntegerType *I32 = Type::getInt32Ty(Func.getContext());
          Value *Cast = builder.CreateAlloca(I32, nullptr, "CASE");

          if (Switch == NULL)
            Switch = SwitchInst::Create(Cast, defaultBB, 0, &BB);

          for (int i = 0; i < I.getNumSuccessors(); i++) {
            BasicBlock *el = I.getSuccessor(i);
            int label = getLabel(el);

            ConstantInt *val = ConstantInt::get(I32, label);

            Switch->addCase(val, el);
          }

          
          // PS1->eraseFromParent();
          // PS2->eraseFromParent();
          // BI->moveBefore(defaultInst);
          // CMP->moveBefore(BI);
          // BI->eraseFromParent();
        }

        CMP = NULL;
      }
      if (defaultBB != NULL) {
        BranchInst *br = BranchInst::Create(&BB, defaultBB);
        defaultBB = NULL;
      }
      printBB(&BB);
    }
  }

  return true;
}

PreservedAnalyses Flatter::run(llvm::Module &M,
    llvm::ModuleAnalysisManager &) {
  bool Changed = runOnModule(M);

  return (Changed ? llvm::PreservedAnalyses::none()
      : llvm::PreservedAnalyses::all());
}

bool LegacyFlatter::runOnModule(llvm::Module &M) {
  bool Changed = Impl.runOnModule(M);

  return Changed;
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getFlatterPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "Flatter", LLVM_VERSION_STRING,
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
          [](StringRef Name, ModulePassManager &MPM,
            ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "Flatter") {
          MPM.addPass(Flatter());
          return true;
          }
          return false;
          });
    }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getFlatterPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
char LegacyFlatter::ID = 0;

// Register the pass - required for (among others) opt
static RegisterPass<LegacyFlatter>
X(/*PassArg=*/"Flatter",
    /*Name=*/"LegacyFlatter",
    /*CFGOnly=*/false,
    /*is_analysis=*/false);
