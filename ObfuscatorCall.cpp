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
            gFnAddr->print(errs(), false);
	    errs() << "\n";
	    gFnAddr->getType()->print(errs(), false);
            errs() << "\n=====================================\n";
            /*
            LoadInst *load = builder.CreateLoad(gFnAddr, true);
            builder.CreateCall(load);
            */

	    /*
	    %0 = load i64, i64* @test2, align 8
	    %sub = sub nsw i64 %0, 1
	    %1 = inttoptr i64 %sub to i32 (...)*
	    %call = call i32 (...) %1()
	    */
	    LoadInst *load = builder.CreateLoad(gFnAddr);
	    Value *tofn = new IntToPtrInst(load, fn->getType(), "tofncast", &Ins);
            builder.CreateCall(tofn);

            // NEW
	    /*
            PointerType *pt = Type::getInt32PtrTy(Func.getContext());
            IntegerType *I64 = Type::getInt64Ty(Func.getContext());
            Value *test = builder.CreateAlloca(I64, nullptr, "test");
            Value *fncast = new PtrToIntInst(gFnAddr, I64, "fncast", &Ins);
            builder.CreateStore(fncast, test);
            LoadInst *ld = builder.CreateLoad(test, fncast);
            Value *tofn = new IntToPtrInst(ld, fn->getType(), "tofncast", &Ins);

            builder.CreateCall(tofn);
	    */

            /*
            IntegerType *I32 = Type::getInt32Ty(Func.getContext());

            Value* store = builder.CreateAlloca(I32, nullptr, "A");
            Value *Cast = new PtrToIntInst(gFnAddr, I32, "globalfn_cast", &Ins);
            LoadInst *load2 = builder.CreateLoad(Cast);
            builder.CreateCall(load2);

            builder.CreateStore(Cast, store);
            LoadInst *load2 = builder.CreateLoad(store);
            builder.CreateCall(load2);
            */

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
