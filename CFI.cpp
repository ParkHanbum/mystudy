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
}

void CFI::handleBB(BasicBlock *bb)
{
}

void CFI::handleFunction(Function *func)
{
  BasicBlock *bb;

  for (Function::iterator BB = func->begin(), E = func->end(); BB != E; ++BB)
  {
    if (bb == &*BB)
      continue;
    
    bb = &*BB;
    Instruction *inst;

    for (BasicBlock::iterator iter = bb->begin(), E = bb->end(); iter != E; ++iter)
    {
      if (inst == &*iter)
        continue;
      inst = &*iter;
      if (isa<CallInst>(inst) || isa<InvokeInst>(inst))
      {
        auto *call = dyn_cast<CallInst>(inst);

        if (!call->isIndirectCall())
          return;

        auto *fnType = call->getFunctionType();

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
        BasicBlock *TypeTestBB = TypeTest->getParent();

        BasicBlock *ContBB = TypeTestBB->splitBasicBlock(TypeTest, "cont");
        BasicBlock *TrapBB = createBasicBlock(context, "trap"); 
        TrapBB->insertInto(inst->getFunction(), ContBB);

        // remove branch instruction which created by splitBasicBlock calling
        errs() << "DEBUGGGGGG";
        Instruction *temp = dyn_cast<Instruction>(CastedCallee); 
        Instruction *remove = temp->getNextNode();
        remove->removeFromParent();

        // move @llvm.type.test to right position
        debugInst(TypeTest);
        TypeTest->removeFromParent();
        TypeTest->insertAfter(cast<Instruction>(CastedCallee));

        Builder.SetInsertPoint(TypeTestBB);
        Instruction *CondBR = Builder.CreateCondBr(TypeTest, ContBB, TrapBB);
        Builder.SetInsertPoint(TrapBB);
        CallInst *TrapCall = Builder.CreateCall(Intrinsic::getDeclaration(inst->getModule(), Intrinsic::trap));
        TrapCall->setDoesNotReturn();
        TrapCall->setDoesNotThrow();
        Builder.CreateUnreachable();

        return;
      }
    }
  }
}

//-----------------------------------------------------------------------------
// CFI implementation
//-----------------------------------------------------------------------------
bool CFI::runOnModule(Module &M) {
  for (auto &Func : M) {
    handleFunction(&Func);
  }

  for (auto &Func : M) {
    for (auto &BB : Func) {
      errs() << "\n======================== [bb] =========================\n";
      BB.print(errs(), false);

      for (auto &Ins : BB) {
        debugInst(&Ins);
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
