#include "CFI.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/Alignment.h"

#include "common.h"

using namespace llvm;

/// createBasicBlock - Create an LLVM basic block.
llvm::BasicBlock *createBasicBlock(
    llvm::LLVMContext &context,
    const Twine &name = "",
    llvm::Function *parent = nullptr,
    llvm::BasicBlock *before = nullptr) {
  return llvm::BasicBlock::Create(context, name, parent, before);
}

void CFI::handleInst(Instruction *inst)
{
  if (isa<CallInst>(inst) || isa<InvokeInst>(inst))
  {
    auto *call = dyn_cast<CallInst>(inst);
    
    if (!call->isIndirectCall())
      return;
    
    debugInst(call);
    auto *fnType = call->getFunctionType();

    if (fnType)
      fnType->print(errs(), true);


    Value *v = call->getCalledValue();
    Value *sv = v->stripPointerCasts();

    LLVMContext &context = inst->getFunction()->getContext();
    IRBuilder<> Builder(inst);
    Type *ty = Type::getInt8PtrTy(context);
    Value *CastedCallee = Builder.CreateBitCast(sv, ty);
    Function *func = Intrinsic::getDeclaration(inst->getModule(), Intrinsic::type_test);

    Metadata *MD = MDString::get(context, "fnPtr");
    Value *TypeID = MetadataAsValue::get(context, MD);
    Instruction *TypeTest = Builder.CreateCall(func, {CastedCallee, TypeID});

    BasicBlock *head = TypeTest->getParent();
    BasicBlock *tail = head->splitBasicBlock(TypeTest, "cont");

    // BasicBlock *Cont = createBasicBlock(context, "cont");
    BasicBlock *Trap = createBasicBlock(context, "trap", inst->getFunction(), tail);
    Trap->moveAfter(head);

    Builder.CreateCondBr(TypeTest, tail, Trap);
    Builder.SetInsertPoint(Trap);
    CallInst *TrapCall = Builder.CreateCall(Intrinsic::getDeclaration(inst->getModule(), Intrinsic::trap));
    TrapCall->setDoesNotReturn();
    TrapCall->setDoesNotThrow();
    Builder.CreateUnreachable();
  }
  else if (isa<StoreInst>(inst))
  {
    errs() << "store \n";
    debugInst(inst);

   if (inst->getNumOperands() != 2)
     return;

   Value *op1 = inst->getOperand(0);
   Value *op2 = inst->getOperand(1);
   if (!op1->getType()->isPointerTy() || !op2->getType()->isPointerTy())
     return;

   if (!isa<Constant>(op1))
     return;

   Function *pFunc = NULL;
   // extract function pointer from op1 if it is ConstantExpr.
   if (isa<ConstantExpr>(op1)) {
     errs() << "constantExpr";
     auto *expr = dyn_cast<ConstantExpr>(op1);
     if (isa<Function>(expr->getOperand(0)))
       pFunc = dyn_cast<Function>(expr->getOperand(0));
   } else if (isa<Function>(op1)) {
     errs() << "function";
     pFunc = dyn_cast<Function>(op1);
   }

   // handle function pointer
   if (pFunc)
     errs() << "handle Function pointer\n";
  }
}

void CFI::handleBB(BasicBlock *bb)
{
   for (BasicBlock::iterator iter = bb->begin(), E = bb->end(); iter != E; ++iter)
   {
     Instruction *inst = &*iter;
     handleInst(inst);
   }
}

void CFI::handleFunction(Function *func)
{
   for (Function::iterator BB = func->begin(), E = func->end(); BB != E; ++BB)
   {
     BasicBlock *bb = &*BB;
     handleBB(bb);
   }
}

//-----------------------------------------------------------------------------
// CFI implementation
//-----------------------------------------------------------------------------
bool CFI::runOnModule(Module &M) {
  for (auto &Func : M) {
    handleFunction(&Func);
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
