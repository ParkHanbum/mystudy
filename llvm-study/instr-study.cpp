#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include "llvm/AsmParser/Parser.h"
#include "llvm/AsmParser/LLParser.h"
#include "llvm/ADT/APInt.h"
#include "gtest/gtest.h"

#include <fstream>
#include <iostream>
#include <sstream>

using namespace llvm;

static std::unique_ptr<Module> makeLLVMModule(LLVMContext &context,
                                              const char *modulestr) {
  SMDiagnostic err;
  return parseAssemblyString(modulestr, err, context);
}

void printGetElementPtrAttributes(const GetElementPtrInst* gep, DataLayout *DL = nullptr) {
  for (unsigned i = 0; i < gep->getNumOperands(); ++i) {
    Value* operand = gep->getOperand(i);
    outs() << "Operand " << i << ": " << *operand << "\n";
  }

  outs() << "Source Element Type: " << *(gep->getSourceElementType()) << "\n";
  outs() << "Result Type: " << *(gep->getResultElementType()) << "\n";

  if (gep->isInBounds())
    outs() << "InBounds: true\n";
  else
    outs() << "InBounds: false\n";

  if (gep->hasIndices()) {
    if (gep->hasAllZeroIndices())
        outs() << "hasAllZeroIndices: true\n";

    if (DL) {
        unsigned BitWidth = DL->getIndexTypeSizeInBits(gep->getType());
        APInt Offset(BitWidth, 0);
        gep->accumulateConstantOffset(*DL, Offset);
        outs() << "accmulateConstantOffset : " << Offset << "\n";
    }
  }
}

void printGetElementPtrAttributes(GetElementPtrInst* gep, DataLayout *DL = nullptr) {
  for (unsigned i = 0; i < gep->getNumOperands(); ++i) {
    Value* operand = gep->getOperand(i);
    outs() << "Operand " << i << ": " << *operand << "\n";
  }

  outs() << "Source Element Type: " << *(gep->getSourceElementType()) << "\n";
  outs() << "Result Type: " << *(gep->getResultElementType()) << "\n";

  if (gep->isInBounds())
    outs() << "InBounds: true\n";
  else
    outs() << "InBounds: false\n";

  if (gep->hasIndices()) {
    if (gep->hasAllZeroIndices())
        outs() << "hasAllZeroIndices: true\n";

    if (DL) {
        unsigned BitWidth = DL->getIndexTypeSizeInBits(gep->getType());
        APInt Offset(BitWidth, 0);
        gep->accumulateConstantOffset(*DL, Offset);
        outs() << "accmulateConstantOffset : " << Offset << "\n";
    }
  }
}


TEST(studyInst, getelementPtr) {
  std::stringstream IR;
  std::ifstream in("instructions/GEP-study.ll");
  IR << in.rdbuf();

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());
  Function* function = M->getFunction("globals_offset_inequal");
  BasicBlock* entryBlock = &function->getEntryBlock();

  for (BasicBlock &BB : *function) {
    for (Instruction &I : BB) {
      if (auto *gep = dyn_cast<GetElementPtrInst>(&I)) {
        printGetElementPtrAttributes(gep);
      }
    }
  }
}

TEST(studyInst, makenotequal) {
  std::stringstream IR;
  std::ifstream in("simplify/compare.ll");
  IR << in.rdbuf();

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());

  Function* function = M->getFunction("globals_offset_inequal");
  BasicBlock* entryBlock = &function->getEntryBlock();

  for (BasicBlock &BB : *function) {
    for (Instruction &I : BB) {
      if (auto *gep = dyn_cast<GetElementPtrInst>(&I)) {
        printGetElementPtrAttributes(gep);
      }
    }
  }
}


TEST(studyInst, GEPInstr) {
  std::stringstream IR;
  std::ifstream in("instructions/GEP-study.ll");
  IR << in.rdbuf();

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());

  Function* function = M->getFunction("study1");
  BasicBlock* entryBlock = &function->getEntryBlock();
  DataLayout DL(M.get());

  for (BasicBlock &BB : *function) {
    for (Instruction &I : BB) {
      if (auto *gep = dyn_cast<GetElementPtrInst>(&I)) {
        printGetElementPtrAttributes(gep, &DL);
      }
    }
  }
}

void printICmpInstArgs(Instruction* instruction) {
  if (ICmpInst* icmp = dyn_cast<ICmpInst>(instruction)) {
    outs() << "Operand 0: " << *(icmp->getOperand(0)) << "\n";
    outs() << "Operand 1: " << *(icmp->getOperand(1)) << "\n";
    outs() << "Predicate: " << icmp->getPredicateName(icmp->getPredicate()).str() << "\n";
  }
}

TEST(studyInst, ICMP) {
  std::stringstream IR;
  std::ifstream in("instructions/GEP-study.ll");
  IR << in.rdbuf();

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());

  Function* function = M->getFunction("globals_offset_inequal");
  BasicBlock* entryBlock = &function->getEntryBlock();

  for (BasicBlock &BB : *function) {
    for (Instruction &I : BB) {
      if (auto const *icmp = dyn_cast<ICmpInst>(&I))
        printICmpInstArgs(&I);
    }
  }
}

void processGlobalVar(GlobalVariable& globalVar) {
  // Global Variable 정보 출력
  outs() << "Name: " << globalVar.getName() << "\n";
  outs() << "Type: " << *(globalVar.getType()) << "\n";
  outs() << "Linkage: " << globalVar.getLinkage() << "\n";
  outs() << "Thread Local Mode: " << globalVar.getThreadLocalMode() << "\n";
  outs() << "Is Constant: " << globalVar.isConstant() << "\n";
  outs() << "Is Thread Local: " << globalVar.isThreadLocal() << "\n";
  outs() << "Alignment: " << globalVar.getAlignment() << "\n";
  outs() << "Address Space: " << globalVar.getType()->getAddressSpace() << "\n";
  // 추가적인 정보를 출력할 수 있습니다.
  globalVar.dump();
}

TEST(studyInst, GlobalValue) {
  std::stringstream IR;
  std::ifstream in("instructions/GEP-study.ll");
  IR << in.rdbuf();

  std::vector<GlobalVariable*> globalVars;

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());

  for (auto& global : M->globals()) {
    processGlobalVar(global);
    globalVars.push_back(&global);
  }

  std::cout << globalVars.size();
  GlobalVariable* globalA = globalVars.back();
  globalVars.pop_back();
  GlobalVariable* globalB = globalVars.back();
  globalA->dump(); globalB->dump();
}

TEST(studyInst, temp) {
  std::stringstream IR;
  std::ifstream in("instructions/GEP-study.ll");
  IR << in.rdbuf();

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());
  Function* function = M->getFunction("globals_offset_inequal");
  DataLayout DL(M.get());
  BasicBlock* entryBlock = &function->getEntryBlock();

  for (BasicBlock &BB : *function) {
    for (Instruction &I : BB) {
      if (auto const *icmp = dyn_cast<ICmpInst>(&I)) {
        Value *op0 = icmp->getOperand(0);
        Value *op1 = icmp->getOperand(1);
        const auto *gep1 = dyn_cast<GEPOperator>(op0);
        const auto *gep2 = dyn_cast<GEPOperator>(op1);
        if (auto *gep = dyn_cast<GetElementPtrInst>(gep1))
          printGetElementPtrAttributes(gep);

        if (auto *gep = dyn_cast<GetElementPtrInst>(gep2))
          printGetElementPtrAttributes(gep);

        unsigned gep1_ops = gep1->getNumOperands();
        unsigned gep2_ops = gep2->getNumOperands();

        if (gep1_ops == gep2_ops && gep1_ops > 1) {
          llvm::outs() << "op : " << gep1_ops << " op: " << gep2_ops << "\n";

          unsigned BitWidth = DL.getIndexTypeSizeInBits(gep1->getType());
          APInt TmpOffset1(BitWidth, 0);
          APInt TmpOffset2(BitWidth, 0);
          gep1->accumulateConstantOffset(DL, TmpOffset1);
          gep2->accumulateConstantOffset(DL, TmpOffset2);

          llvm::outs() << "1 : " << TmpOffset1 << " 2 : " << TmpOffset2 << "\n";
          for (unsigned i = 1; i != gep1_ops; ++i) {
            ConstantInt *gep1_opi, *gep2_opi;
            gep1_opi = dyn_cast<ConstantInt>(gep1->getOperand(i));
            gep2_opi = dyn_cast<ConstantInt>(gep2->getOperand(i));

            if (gep1_opi && gep2_opi)
              gep1_opi->dump();
            if (gep1_opi == gep2_opi)
              std::cout << "same \n";
          }
        }
      }
    }
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
