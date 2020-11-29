#include "Flatter.h"

using namespace llvm;

int Flatter::getLabel(BasicBlock* bb)
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
  errs() << "STR : [" << Str << "]";
  if (!Str.empty())
    return std::stoi(Str);
  return 0;
}

void Flatter::printOperands(Instruction& inst)
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

void Flatter::printInst(Instruction& inst)
{
  errs() << "\n=================[Instruction]======================\n";
  errs() << "Instruction : " <<  inst.getOpcodeName() << " " << getLabel(inst.getParent()) << " \t print: ";
  inst.print(errs(), false);
  printOperands(inst);
}

void Flatter::printInst(Instruction* inst)
{
  errs() << "\n=================[Instruction]======================\n";
  errs() << "Instruction : " <<  inst->getOpcodeName() << " \t print: ";
  inst->print(errs(), false);
}

bool Flatter::handleInst(Instruction& inst)
{
  printInst(inst);
  return true;
}

void Flatter::printBB(BasicBlock* bb)
{
  errs() << "\n=================[BasicBlock]======================\n";
  bb->print(errs(), false);
}

void Flatter::flatting(Function *Func)
{
	BasicBlock* entry = &Func->getEntryBlock();
	Instruction* term = entry->getTerminator();
	if (nullptr == term || !isa<BranchInst>(term))
		return;

	IntegerType *I32 = Type::getInt32Ty(Func->getContext());

	//BasicBlock *defaultBB = BasicBlock::Create(Func->getContext(), "", Func);
	//defaultBB->moveAfter(entry);
	BasicBlock *switchBB = BasicBlock::Create(Func->getContext(), "", Func);
	//switchBB->moveAfter(defaultBB);
	switchBB->moveAfter(entry);


	//BasicBlock *zeroBB = BasicBlock::Create(Func->getContext(), "", Func);
	//zeroBB->moveAfter(switchBB);

	/*
	Instruction* first = &*zeroBB->getFirstInsertionPt();
	BranchInst::Create(defaultBB, term);
	if (cast<BranchInst>(term))
	{
		BranchInst *br = cast<BranchInst>(term);
		CmpInst* cond = cast<CmpInst>(br->getCondition());
		cond->moveAfter(first);
	}

	term->moveAfter(first);
	
	IRBuilder<> builder_ent(defaultBB);
	Value *Case = builder_ent.CreateAlloca(I32, nullptr, "CASE");
	*/


	Instruction *Case = new AllocaInst(I32, 0, "CASE", &*entry->getFirstInsertionPt());

	IRBuilder<> builder_sw(switchBB);
	Value *load = builder_sw.CreateLoad(Case);
	SwitchInst *swInst = SwitchInst::Create(load, nullptr, 0, switchBB);

	for (Function::iterator BB = Func->begin(), E = Func->end(); BB != E; ++BB)
	{
		// add case
		BasicBlock* bb = &*BB;
		BasicBlock* end = &*E;
		if (bb == switchBB) continue; // skip make case for itself
		// if (bb == defaultBB) continue;
		if (bb == entry) continue;
		if (bb == end) continue;
		int label = getLabel(bb);
		ConstantInt *case_number = ConstantInt::get(I32, label);
		swInst->addCase(case_number, bb);

		// handle branch , do not need handle conditional branch
		term = bb->getTerminator();
		if (nullptr == term || !isa<BranchInst>(term) || cast<BranchInst>(term)->isConditional())
			continue;

		// handle br only
		BasicBlock* el = term->getSuccessor(0);
		label = getLabel(el);
		case_number = ConstantInt::get(I32, label);
		new StoreInst(case_number, Case, term);
		BranchInst::Create(switchBB, term); 
		
		el->removePredecessor(term->getParent());
		term->eraseFromParent();
	}

	/*
	auto val = ConstantInt::get(I32, getLabel(zeroBB));
	Value *store = builder_ent.CreateStore(val, Case);
	builder_ent.CreateBr(switchBB);
	*/

	BasicBlock *whileBB = BasicBlock::Create(Func->getContext(), "", Func);
	BranchInst::Create(switchBB, whileBB); 
	swInst->setDefaultDest(whileBB);
}



//-----------------------------------------------------------------------------
// Flatter implementation
//-----------------------------------------------------------------------------
bool Flatter::runOnModule(Module &M) {
  bool Instrumented = false;

  // Function name <--> IR variable that holds the call counter
  StringMap<Constant *> CallCounterMap;
  // Function name <--> IR variable that holds the function name
  StringMap<Constant *> FuncNameMap;

  auto &CTX = M.getContext();

	for (Module::iterator Func = M.begin(), E = M.end(); Func != E; ++Func) {
		flatting(&*Func);
	}

	for (auto &Func : M) {
		for (auto &BB : Func) {
			errs() << "\n======================== [bb] =========================\n";
			BB.print(errs(), false);
			errs() << "\n======================== [bb] =========================\n";

			for (auto &Ins : BB) {
				//printInst(Ins);
			}
		}
	}

  return true;
}


PreservedAnalyses Flatter::run(Module &M,
    ModuleAnalysisManager &) {
  bool Changed = runOnModule(M);

  return (Changed ? llvm::PreservedAnalyses::none()
      : llvm::PreservedAnalyses::all());
}

bool LegacyFlatter::runOnModule(Module &M) {
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
