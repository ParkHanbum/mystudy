#include "ObfuscatorCall.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"

using namespace llvm;


void ObfuscatorCall::printOperands(llvm::Instruction& inst)
{
  for (User::op_iterator start = inst.op_begin(), end = inst.op_end(); start != end; ++start) {
    Value *el = start->get();	
    errs() << "\n\t opcode : " << el->getName() << "\t type : "; 
    el->getType()->print(errs(), false);
  }
}

void ObfuscatorCall::handleCallInst(llvm::Instruction& inst)
{
  errs() << "\n=================================================================\n";
  errs() << "Instruction : " <<  inst.getOpcodeName() << " \t print: ";
  inst.print(errs(), false);
  printOperands(inst);
}

bool ObfuscatorCall::handleInst(llvm::Instruction& inst)
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
  /*
  Value *fncast = new PtrToIntInst(&func, I64, "fncast", &*func.getEntryBlock().getFirstInsertionPt());
  errs() << "\n======================\n";
  fncast->print(errs(), false);
  fncast->getType()->print(errs(), false);
  errs() << "\n======================\n";
  Constant *cont = M.getOrInsertGlobal("TEST", fncast->getType());
  cont = dyn_cast<Constant>(fncast);
  */

  std::string name;
  name.append("TTTT"); name.append(func.getName());
  func.getType()->print(errs(), false);
  GlobalVariable* fnPtr = new GlobalVariable(/*Module=*/M, 
      /*Type=*/ Type::getInt64Ty(func.getContext()),
      /*isConstant=*/ true,
      /*Linkage=*/ GlobalValue::ExternalLinkage,
      /*Initializer=*/ nullptr, // has initializer, specified below
      /*Name=*/StringRef(name));

  IntegerType *I64 = Type::getInt64Ty(func.getContext());

  // i8* bitcast (void ()* @fez to i8*)
  Constant *const_ptr_5 = ConstantExpr::getBitCast(&func, Type::getInt8PtrTy(func.getContext()));

  // (i8* getelementptr (i8, i8* bitcast (i32 ()* @foo to i8*), i64 1) 
  Constant *One = ConstantInt::get(I64, 1);
  Constant *const_ptr_6 = ConstantExpr::getGetElementPtr(Type::getInt8Ty(func.getContext()), const_ptr_5, One);

  // constant i64 ptrtoint (void ()* @fez to i64)
  Constant *const_ptr_7 = ConstantExpr::getPtrToInt(const_ptr_6, I64);

  fnPtr->setInitializer(const_ptr_7);
  return fnPtr;
}


//-----------------------------------------------------------------------------
// ObfuscatorCall implementation
//-----------------------------------------------------------------------------
bool ObfuscatorCall::runOnModule(Module &M) {
  bool Instrumented = false;

  // Function name <--> IR variable that holds the function name
  llvm::StringMap<Constant *> FuncNameMap;
  // Function name <--> Function address
  llvm::StringMap<Constant *> CallCounterMap;

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
    IntegerType *I64 = Type::getInt64Ty(Func.getContext());
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
            gFnAddr->print(errs(), false);
            LoadInst *load = builder.CreateLoad(gFnAddr);
            Value* orig = builder.CreateSub(load, ConstantInt::get(I64, 1));
            Value *tofn = new IntToPtrInst(orig, fn->getType(), "tofncast", &Ins);
            builder.CreateCall(tofn);
          }
        }
      }
    }
  }

  return true;
}

PreservedAnalyses ObfuscatorCall::run(llvm::Module &M,
                                          llvm::ModuleAnalysisManager &) {
  bool Changed = runOnModule(M);

  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}

bool LegacyObfuscatorCall::runOnModule(llvm::Module &M) {
  bool Changed = Impl.runOnModule(M);

  return Changed;
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getObfuscatorCallPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "ObfuscatorCall", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "ObfuscatorCall") {
                    MPM.addPass(ObfuscatorCall());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getObfuscatorCallPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
char LegacyObfuscatorCall::ID = 0;

// Register the pass - required for (among others) opt
static RegisterPass<LegacyObfuscatorCall>
    X(/*PassArg=*/"ObfuscatorCall",
      /*Name=*/"LegacyObfuscatorCall",
      /*CFGOnly=*/false,
      /*is_analysis=*/false);
