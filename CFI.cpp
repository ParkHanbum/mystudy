#include "CFI.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"

using namespace llvm;

void CFI::printOperands(llvm::Instruction& inst)
{
	for (User::op_iterator start = inst.op_begin(), end = inst.op_end(); start != end; ++start) {
		Value *el = start->get();	
		errs() << "\n\t opcode : " << el->getName() << "\t type : "; 
		if (el->getType())
			el->getType()->print(errs(), false);
	}
}

void CFI::printInst(llvm::Instruction& inst)
{
	errs() << "\n=================================================================\n";
	errs() << "Instruction : " <<  inst.getOpcodeName() << " \t print: ";
	inst.print(errs(), false);
	printOperands(inst);
}

bool CFI::handleInst(llvm::Instruction& inst)
{

	printInst(inst);
	return true;
}


//-----------------------------------------------------------------------------
// CFI implementation
//-----------------------------------------------------------------------------
bool CFI::runOnModule(Module &M) {
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
