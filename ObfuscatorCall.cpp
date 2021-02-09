#include <iostream>
#include <random>

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"

#include "ObfuscatorCall.h"
#include "common.h"


using namespace llvm;

// keep the global variable which stored original function address.
llvm::StringMap<Constant *> FuncNameMap;
// keep the offset of how much it moved.
llvm::StringMap<int> FuncOffsetMap;


Constant *CreateGlobalFunctionPtr(Module &M, Function &func)
{
  int offset = getRandomV();
  std::string name;
  name.append("GlobalFunctionPtr_");
  name.append(func.getName());
  GlobalVariable *fnPtr = new GlobalVariable(/*Module=*/M,
                                             /*Type=*/Type::getInt64Ty(func.getContext()),
                                             /*isConstant=*/true,
                                             /*Linkage=*/GlobalValue::ExternalLinkage,
                                             /*Initializer=*/nullptr, // has initializer, specified below
                                             /*Name=*/StringRef(name));

  IntegerType *I64 = Type::getInt64Ty(func.getContext());

  // i8* bitcast (void ()* @fez to i8*)
  Constant *const_ptr_5 = ConstantExpr::getBitCast(&func, Type::getInt8PtrTy(func.getContext()));

  // (i8* getelementptr (i8, i8* bitcast (i32 ()* @foo to i8*), i64 1)
  Constant *One = ConstantInt::get(I64, offset);
  Constant *const_ptr_6 = ConstantExpr::getGetElementPtr(Type::getInt8Ty(func.getContext()), const_ptr_5, One);

  // constant i64 ptrtoint (void ()* @fez to i64)
  Constant *const_ptr_7 = ConstantExpr::getPtrToInt(const_ptr_6, I64);

  fnPtr->setInitializer(const_ptr_7);

  FuncNameMap[func.getName()] = fnPtr;
  FuncOffsetMap[func.getName()] = offset; 
  return fnPtr;
}

bool ObfuscatorCall::runOnModule(Module &M)
{
  auto &CTX = M.getContext();
  IntegerType *I64 = Type::getInt64Ty(M.getContext());

  for (auto &Func : M)
  {
    if (Func.isDeclaration())
      continue;

    Constant *var = CreateGlobalFunctionPtr(M, Func);
  }

  for (auto &Func : M)
  {
    for (auto &BB : Func)
    {
      for (auto &Ins : BB)
      {
        if (isa<CallInst>(Ins) || isa<InvokeInst>(Ins))
        {
          IRBuilder<> builder(&Ins);
          debugInst(&Ins);

          CallInst *cinst = dyn_cast<CallInst>(&Ins);

          Function *fn = cinst->getCalledFunction();
          if (fn)
          {
            Constant *gFnAddr = FuncNameMap[fn->getName()];
            LoadInst *load = builder.CreateLoad(gFnAddr);
            int offset = FuncOffsetMap[fn->getName()];
            Value* orig = builder.CreateSub(load, ConstantInt::get(I64, offset));
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
                                      llvm::ModuleAnalysisManager &)
{
  bool Changed = runOnModule(M);

  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}


bool LegacyObfuscatorCall::runOnModule(llvm::Module &M)
{
  bool Changed = Impl.runOnModule(M);

  return Changed;
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getObfuscatorCallPluginInfo()
{
  return {LLVM_PLUGIN_API_VERSION, "ObfuscatorCall", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "ObfuscatorCall")
                  {
                    MPM.addPass(ObfuscatorCall());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
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
