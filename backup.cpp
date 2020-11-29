#include "Flatter.h"

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
  if (!Str.empty())
    return std::stoi(Str);
  return 0;
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

void Flatter::transIf(Instruction* inst, BasicBlock* defaultBB, SwitchInst* Switch, Value* Case)
{
  const Function* func = inst->getFunction();
  IntegerType* I32 = Type::getInt32Ty(func->getContext());

  for (int i = 0; i < inst->getNumSuccessors(); i++) {
    BasicBlock* el = inst->getSuccessor(i);
    int label = getLabel(el);
    ConstantInt* val4 = ConstantInt::get(I32, label);
    Switch->addCase(val4, el);

    BasicBlock* BB = BasicBlock::Create(func->getContext(), "", (Function *)func);
    IRBuilder<> builder(BB);
    auto val = ConstantInt::get(I32, label);
    builder.CreateStore(val, Case);
    BranchInst* temp = BranchInst::Create(defaultBB, BB);
    inst->setSuccessor(i, BB);
  }
}

  BasicBlock *defaultBB = NULL;

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
  BranchInst *BI = NULL;

  Instruction *defaultInst = NULL;
  BasicBlock *originBB = NULL;
  SwitchInst *Switch = NULL;
  bool remove = false;
  for (auto &Func : M) {
    BasicBlock *bb;
    for (auto &BB : Func) {
      bb = &BB;
      if (bb == defaultBB) {
        printBB(&BB);
        continue; // stopgap
      }

      for (Instruction &I : BB) {
        if (remove) {
          Instruction* inst = originBB->getTerminator();
          handleInst(inst);
          //CMP->removeFromParent(); 
          CMP->moveBefore(inst);
          CMP = NULL;
          //BI->removeFromParent();
          //BI->moveBefore(inst);
          //BI = NULL;
          remove = false;
        }
        if (isa<CmpInst>(I)) {
          CMP = dyn_cast<CmpInst>(&I); continue;
        }
        if (CMP && isa<BranchInst>(I) && cast<BranchInst>(I).isConditional()) {
          BI = dyn_cast<BranchInst>(&I);

          // BB 12
          if (defaultBB == NULL) {
            IntegerType *I32 = Type::getInt32Ty(Func.getContext());
            defaultBB = BasicBlock::Create(Func.getContext(), "", &Func);
            originBB = BasicBlock::Create(Func.getContext(), "", &Func);
            IRBuilder<> builder(&BB);
            Value *alloc = builder.CreateAlloca(I32, nullptr, "CASE");
            auto val = ConstantInt::get(I32, 1);
            Value *store = builder.CreateStore(val, alloc);

            IRBuilder<> builderd(defaultBB);
            Value *Case = builderd.CreateLoad(alloc); 

            BranchInst* inst1 = BranchInst::Create(defaultBB, originBB);

            auto *cCMP = CMP->clone();
            cCMP->insertBefore(inst1);
            BranchInst *cBI = dyn_cast<BranchInst>(BI->clone());
            cBI->setCondition(cCMP);
            cBI->insertBefore(inst1);

            if (Switch == NULL)
              Switch = SwitchInst::Create(Case, defaultBB, 0, defaultBB);

            int label = getLabel(originBB);
            ConstantInt *val3 = ConstantInt::get(I32, label);
            Switch->addCase(val, originBB);

            // while(1) 
            BranchInst *inst2 = BranchInst::Create(defaultBB, defaultBB);
            transIf(&I, defaultBB, Switch, alloc);
          }

          BranchInst *br3 = BranchInst::Create(defaultBB, &BB);
          remove = true;
        }
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
