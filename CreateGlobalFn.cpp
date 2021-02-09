#include "ObfuscatorCall.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"

using namespace llvm;

void DynamicCallCounter::printOperands(llvm::Instruction& inst)
{
  for (User::op_iterator start = inst.op_begin(), end = inst.op_end(); start != end; ++start) {
    Value *el = start->get();	
    errs() << "\n\t opcode : " << el->getName() << "\t type : "; 
    el->getType()->print(errs(), false);
  }
}

void DynamicCallCounter::handleCallInst(llvm::Instruction& inst)
{
  errs() << "\n=================================================================\n";
  errs() << "Instruction : " <<  inst.getOpcodeName() << " \t print: ";
  inst.print(errs(), false);
  printOperands(inst);
}

bool DynamicCallCounter::handleInst(llvm::Instruction& inst)
{
  for (User::op_iterator start = inst.op_begin(), end = inst.op_end(); start != end; ++start) {
    Value *el = start->get();	
  }

  if(isa<CallInst>(inst) || isa<InvokeInst>(inst)) {
    handleCallInst(inst);
    return true;
  }

  return false;
}

/*
Constant *CreateGlobalFunction(Module &M, Function &func) {
  auto &CTX = M.getContext();
  Type* type = func.getReturnType();

  std::string name;
  name.append("TTTT");
  name.append(func.getName());
  // This will insert a declaration into M
  Constant *NewGlobalVar =
      M.getOrInsertGlobal(name, type);

  // This will change the declaration into definition (and initialise to 0)
  GlobalVariable *NewGV = M.getNamedGlobal(name);
  errs() << NewGV << '\n';
  NewGV->print(errs(),false);
  NewGV->setLinkage(GlobalValue::CommonLinkage);
  NewGV->setAlignment(8);
  // NewGV->setInitializer();

  type->print(errs(), false);
  errs() << '\n' << func.getName() << '\n';
  NewGlobalVar->print(errs(), false);
  errs() << '\n';
  return NewGlobalVar;
}*/

Constant *CreateGlobalFunctionPtr(Module &M, Function &func) {
  std::string name;
  name.append("TTTT"); name.append(func.getName());

  func.getType()->print(errs(), false);
  GlobalVariable* fnPtr = new GlobalVariable(/*Module=*/M, 
      /*Type=*/ func.getType(),
      /*isConstant=*/ false,
      /*Linkage=*/ GlobalValue::ExternalLinkage,
      /*Initializer=*/ &func, // has initializer, specified below
      /*Name=*/StringRef(name));
  fnPtr->setAlignment((8));

  return fnPtr;

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
    if (Func.isDeclaration())
      continue;

    // IRBuilder<> builder(&*Func.getEntryBlock().getFirstInsertionPt());
    Constant *var = CreateGlobalFunctionPtr(M, Func);
    FuncNameMap[Func.getName()] = var;
    //LoadInst *load = builder.CreateLoad(var);
  }

  for (auto &Func : M) {
    for (auto &BB : Func) {
      for (auto &Ins : BB) {
        if (handleInst(Ins)) {
          errs() << "\n=====================================\n";
          IRBuilder<> builder(&Ins);
          CallInst *cinst = dyn_cast<CallInst>(&Ins);
          cinst->print(errs(), false);
          errs() << "\n=====================================\n";

          Function *fn = cinst->getCalledFunction();
          if (fn) {
            Constant *gFnAddr = FuncNameMap[fn->getName()];
            gFnAddr->print(errs(), false); gFnAddr->getType()->print(errs(), false);
            errs() << "\n=====================================\n";

            // Value* store = builder.CreateAlloca(gFnAddr->getType(), nullptr, "A");
            LoadInst *load = builder.CreateLoad(gFnAddr);
            builder.CreateCall(load);


            //builder.CreateStore(gFnAddr, store);
            //builder.CreateCall(gFnAddr);

          }
        }
      }
    }
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
