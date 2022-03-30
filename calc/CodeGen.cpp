#include "CodeGen.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
class ToIRVisitor : public ASTVisitor {
  Module *M;
  IRBuilder<> Builder;
  Type *VoidTy;
  Type *Int32Ty;
  Type *Int8PtrTy;
  Type *Int8PtrPtrTy;
  Constant *Int32Zero;

  IntegerType *I32;

  Value *V;
  Instruction *inst;
  BasicBlock *entry;
  Function *MainFn;
  StringMap<Value *> nameMap;

public:
  ToIRVisitor(Module *M) : M(M), Builder(M->getContext()) {
    VoidTy = Type::getVoidTy(M->getContext());
    Int32Ty = Type::getInt32Ty(M->getContext());
    Int8PtrTy = Type::getInt8PtrTy(M->getContext());
    I32 = Type::getInt32Ty(M->getContext());
    Int8PtrPtrTy = Int8PtrTy->getPointerTo();
    Int32Zero = ConstantInt::get(Int32Ty, 0, true);
  }

  void run(AST *Tree) {
    FunctionType *MainFty = FunctionType::get(
        Int32Ty, {Int32Ty, Int8PtrPtrTy}, false);
    Function *MainFn = Function::Create(
        MainFty, GlobalValue::ExternalLinkage, "main", M);
    entry = BasicBlock::Create(M->getContext(),
                               "entry", MainFn);

    Builder.SetInsertPoint(entry);

    Tree->accept(*this);
    Builder.CreateRet(V);
    // Builder.CreateRet(Int32Zero);
  }

  virtual void visit(Factor &Node) override {
    errs() << "visit FACTOR";
    int intval;
    Node.getVal().getAsInteger(10, intval);
    V = ConstantInt::get(Int32Ty, intval, true);
    inst = Builder.CreateAlloca(Int32Ty, nullptr, "");
    Builder.CreateStore(V, inst);
  };

  virtual void visit(BinaryOp &Node) override {
    errs() << "visit BINOP";
    Node.getLeft()->accept(*this);
    Value *Left = V;
    Value *l = inst;
    Node.getRight()->accept(*this);
    Value *Right = V;
    Value *r = inst;

    switch (Node.getOperator()) {
    case BinaryOp::Plus:
      V = Builder.CreateNSWAdd(Builder.CreateLoad(l), Builder.CreateLoad(r));
      break;
    case BinaryOp::Minus:
      V = Builder.CreateNSWSub(Builder.CreateLoad(l), Builder.CreateLoad(r));
      break;
    }

    errs() << "\n";
    V->print(errs(), false);
  };
};
} // namespace

void CodeGen::compile(AST *Tree) {
  LLVMContext Ctx;
  Module *M = new Module("calc.expr", Ctx);
  ToIRVisitor ToIR(M);
  ToIR.run(Tree);
  M->print(outs(), nullptr);
}
