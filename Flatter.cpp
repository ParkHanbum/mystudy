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

void Flatter::printInst(const Instruction* inst)
{
  errs() << "\n=================[Instruction]======================\n";
  errs() << "Instruction : " <<  inst->getOpcodeName() << " \t print: ";
  inst->print(errs(), false);
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

void Flatter::transIf(BranchInst* inst, BasicBlock* br_bb, SwitchInst* Switch,BasicBlock* switchBB,  Value* Case)
{
  const Function* func = inst->getFunction();
  IntegerType* I32 = Type::getInt32Ty(func->getContext());

	BranchInst::Create(switchBB, inst);


	// move comp, branch inst to new bb
	int label = getLabel(br_bb);
	BasicBlock* bb = BasicBlock::Create(func->getContext(), "", (Function *)func);
	CmpInst* cond = cast<CmpInst>(inst->getCondition());
	BasicBlock::iterator iter = bb->getFirstInsertionPt();
	cond->moveBefore(&*iter);
	inst->moveBefore(&*iter);
	ConstantInt* val4 = ConstantInt::get(I32, label);
	Switch->addCase(val4, bb);
	/*
  for (int i = 0; i < inst->getNumSuccessors(); i++) {
    BasicBlock* el = inst->getSuccessor(i);
    int label = getLabel(el);
    ConstantInt* val4 = ConstantInt::get(I32, label);
    Switch->addCase(val4, el);

    BasicBlock* BB = BasicBlock::Create(func->getContext(), "", (Function *)func);
    IRBuilder<> builder(BB);
    auto val = ConstantInt::get(I32, label);
    builder.CreateStore(val, Case);
    BranchInst* temp = BranchInst::Create(switchBB, BB);
    inst->setSuccessor(i, BB);
		el->removeFromParent();
		//printBB(el);
		//el->moveBefore(bb);
  }
	*/
}


void Flatter::flatting(Function *Func)
{
	BasicBlock *defaultBB = BasicBlock::Create(Func->getContext(), "", Func);
	// ReturnInst *retInst = ReturnInst::Create(Func->getContext(), defaultBB);

	BasicBlock *switchBB = BasicBlock::Create(Func->getContext(), "", Func);
	IRBuilder<> builder(switchBB);
	IntegerType *I32 = Type::getInt32Ty(Func->getContext());
	auto val = ConstantInt::get(I32, 0);
	Value *Case = builder.CreateAlloca(I32, nullptr, "CASE");
	Value *store = builder.CreateStore(val, Case);
	Value *load = builder.CreateLoad(Case);
	SwitchInst *swInst = SwitchInst::Create(load, defaultBB, 0, switchBB);
	for (llvm::Function::iterator BB = Func->begin(), E = Func->end(); BB != E; ++BB)
	{
		Instruction *instr = BB->getTerminator();
		if (nullptr != instr && isa<BranchInst>(instr) && cast<BranchInst>(instr)->isConditional())
		{
			BranchInst *brInst = dyn_cast<BranchInst>(instr);
			transIf(brInst, &*BB, swInst, switchBB, Case);
		}
	}
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

	for (Module::iterator Func = M.begin(), E = M.end(); Func != E; ++Func) {
		flatting(&*Func);
	}

	for (auto &Func : M) {
		for (auto &BB : Func) {
			errs() << "\n======================== [bb] =========================\n";
			BB.print(errs(), false);
			errs() << "\n======================== [bb] =========================\n";

			/*
			for (auto &Ins : BB) {
				handleInst(Ins);
			}
			*/
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
