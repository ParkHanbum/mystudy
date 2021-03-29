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
  for (Function::iterator BB = func->begin(), E = func->end(); BB != E; ++BB)
  {
    BasicBlock *bb = &*BB;
    errs() << "inside handle BB \n";
    for (BasicBlock::iterator iter = bb->begin(), E = bb->end(); iter != E; ++iter)
    {
      auto *inst = &*iter;
      if (isa<CallInst>(inst) || isa<InvokeInst>(inst))
      {
        auto *call = dyn_cast<CallInst>(inst);
        if (!call->isIndirectCall())
          continue;

        errs() << "start handle instruction \n";
        auto *fnType = call->getFunctionType();
        Value *v = call->getCalledOperand();
        Value *sv = v->stripPointerCasts();

        if (isa<Instruction>(sv)) {
          debugInst(inst);
          backtrace_operands(dyn_cast<Instruction>(sv));
        }

        LLVMContext &context = inst->getFunction()->getContext();
        IRBuilder<> Builder(inst);
        Type *ty = Type::getInt8PtrTy(context);
        Value *CastedCallee = Builder.CreateBitCast(sv, ty);
        Function *func = Intrinsic::getDeclaration(inst->getModule(), Intrinsic::type_test);

        Metadata *MD = MDString::get(context, "fnPtr");
        MDNode *N = MDNode::get(context, MD);
        func->setMetadata("fnptr", N);
        Value *TypeID = MetadataAsValue::get(context, MD);
        Instruction *TypeTest = Builder.CreateCall(func, {CastedCallee, TypeID});
        BasicBlock *TypeTestBB = TypeTest->getParent();

        BasicBlock *ContBB = TypeTestBB->splitBasicBlock(TypeTest, "cont");
        BasicBlock *TrapBB = createBasicBlock(context, "trap"); 
        TrapBB->insertInto(inst->getFunction(), ContBB);

        // remove branch instruction which created by splitBasicBlock calling
        Instruction *temp = dyn_cast<Instruction>(CastedCallee); 
        Instruction *remove = temp->getNextNode();
        ContBB->removePredecessor(remove->getParent());
        remove->eraseFromParent();

        // move @llvm.type.test to right position
        TypeTest->removeFromParent();
        TypeTest->insertAfter(cast<Instruction>(CastedCallee));

        Builder.SetInsertPoint(TypeTestBB);
        Instruction *CondBR = Builder.CreateCondBr(TypeTest, ContBB, TrapBB);
        Builder.SetInsertPoint(TrapBB);
        CallInst *TrapCall = Builder.CreateCall(Intrinsic::getDeclaration(inst->getModule(), Intrinsic::trap));
        TrapCall->setDoesNotReturn();
        TrapCall->setDoesNotThrow();
        Builder.CreateUnreachable();

        errs() << "DEBUGGGGG \n";
        BasicBlock *startHere = inst->getParent();
        iter = startHere->begin();
        E = startHere->end();
        while(1) {
          if (&*BB == startHere)
            break;
          ++BB;
        }

      } // end handling for CallInst InvokeInst
      else if (isa<StoreInst>(inst))
      {
        errs() << "store \n";
        if (inst->getNumOperands() != 2)
          continue;

        Value *op1 = inst->getOperand(0);
        Value *op2 = inst->getOperand(1);
        if (!op1->getType()->isPointerTy() || !op2->getType()->isPointerTy())
          continue;

        if (!isa<Constant>(op1))
          continue;

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
        if (pFunc) {
          debugInst(inst);
	  errs() << "op1 : ";
	  op1->print(errs());
	  errs() << "op2 : ";
	  op2->print(errs());

	  if (isa<Instruction>(op2)) {
	  backtrace_operands(dyn_cast<Instruction>(op2));
	  }
          errs() << "handle Function pointer\n";
        }
      }
    } // end basicblock
  } // end function
}


//-----------------------------------------------------------------------------
// CFI implementation
//-----------------------------------------------------------------------------
bool CFI::runOnModule(Module &M) {
  for (auto &Func : M) {
    handleFunction(&Func);
  }

  /*
     for (auto &Func : M) {
     for (auto &BB : Func) {
     errs() << "\n======================== [bb] =========================\n";
     BB.print(errs(), false);

     for (auto &Ins : BB) {
     debugInst(&Ins);
     }
     }
     }
     */

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
