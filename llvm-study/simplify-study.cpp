#include "llvm/ADT/None.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AssumeBundleQueries.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopNestAnalysis.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/InstSimplifyPass.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/ScalarEvolutionExpander.h"
#include "llvm/Transforms/Utils/SimplifyIndVar.h"
#include "gtest/gtest.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/CmpInstAnalysis.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/InstSimplifyFolder.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/OverflowInstAnalysis.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Analysis/VectorUtils.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/KnownBits.h"

#include <fstream>
#include <iostream>

using namespace llvm;
using namespace llvm::PatternMatch;

#define DEBUG_TYPE "simplify-study"

enum { RecursionLimit = 3 };

STATISTIC(NumReassoc, "Number of reassociations");

#define LLVM_DEBUG(X)                                                              \
  do {                                                                             \
    dbgs() << "[" << DEBUG_TYPE << "]" << __FUNCTION__ << ":" << __LINE__ << "\t"; \
    X;                                                                             \
  } while (false)

static Value *_simplifyAddInst(Value *Op0, Value *Op1, bool IsNSW, bool IsNUW,
                               const SimplifyQuery &Q, unsigned);

static Value *_simplifyXorInst(Value *Op0, Value *Op1, const SimplifyQuery &Q, unsigned);

/// Given operands for a CastInst, fold the result or return null.
static Value *_simplifyCastInst(unsigned CastOpc, Value *Op, Type *Ty,
                                const SimplifyQuery &Q, unsigned);

static APInt stripAndComputeConstantOffsets(const DataLayout &DL, Value *&V,
                                            bool AllowNonInbounds = false) {
  assert(V->getType()->isPtrOrPtrVectorTy());

  APInt Offset = APInt::getZero(DL.getIndexTypeSizeInBits(V->getType()));
  V = V->stripAndAccumulateConstantOffsets(DL, Offset, AllowNonInbounds);
  // As that strip may trace through `addrspacecast`, need to sext or trunc
  // the offset calculated.
  return Offset.sextOrTrunc(DL.getIndexTypeSizeInBits(V->getType()));
}

/// Given a bitwise logic op, check if the operands are add/sub with a common
/// source value and inverted constant (identity: C - X -> ~(X + ~C)).
static Value *_simplifyLogicOfAddSub(Value *Op0, Value *Op1,
                                     Instruction::BinaryOps Opcode) {
  assert(Op0->getType() == Op1->getType() && "Mismatched binop types");
  assert(BinaryOperator::isBitwiseLogicOp(Opcode) && "Expected logic op");
  LLVM_DEBUG(dbgs() << "_simplifyLogicOfAddSub opcode : " << Instruction::getOpcodeName(Opcode) << "\n");
  LLVM_DEBUG(dbgs() << "OP0 : ";Op0->dump());
  LLVM_DEBUG(dbgs() << "OP1 : ";Op1->dump());
  Value *X;
  Constant *C1, *C2;

  //LLVM_DEBUG(dbgs() << "??? : "; m_Add(m_Value(X), m_Constant(C1)).dump());

  if ((match(Op0, m_Add(m_Value(X), m_Constant(C1))) &&
       match(Op1, m_Sub(m_Constant(C2), m_Specific(X)))) ||
      (match(Op1, m_Add(m_Value(X), m_Constant(C1))) &&
       match(Op0, m_Sub(m_Constant(C2), m_Specific(X))))) {
    LLVM_DEBUG(dbgs() << "match inside : \n");
    if (ConstantExpr::getNot(C1) == C2) {
      LLVM_DEBUG(dbgs() << "ConstantExpr : \n");
      // (X + C) & (~C - X) --> (X + C) & ~(X + C) --> 0
      // (X + C) | (~C - X) --> (X + C) | ~(X + C) --> -1
      // (X + C) ^ (~C - X) --> (X + C) ^ ~(X + C) --> -1
      Type *Ty = Op0->getType();
      return Opcode == Instruction::And ? ConstantInt::getNullValue(Ty)
                                        : ConstantInt::getAllOnesValue(Ty);
    }
  }


  LLVM_DEBUG(dbgs() << "PASS! \n");
  return nullptr;
}

static Constant *foldOrCommuteConstant(Instruction::BinaryOps Opcode,
                                       Value *&Op0, Value *&Op1,
                                       const SimplifyQuery &Q) {
  if (auto *CLHS = dyn_cast<Constant>(Op0)) {
    if (auto *CRHS = dyn_cast<Constant>(Op1)) {
      switch (Opcode) {
      default:
        break;
      case Instruction::FAdd:
      case Instruction::FSub:
      case Instruction::FMul:
      case Instruction::FDiv:
      case Instruction::FRem:
        if (Q.CxtI != nullptr)
          return ConstantFoldFPInstOperands(Opcode, CLHS, CRHS, Q.DL, Q.CxtI);
      }
      return ConstantFoldBinaryOpOperands(Opcode, CLHS, CRHS, Q.DL);
    }

    // Canonicalize the constant to the RHS if this is a commutative operation.
    if (Instruction::isCommutative(Opcode))
      std::swap(Op0, Op1);
  }
  return nullptr;
}

/// Given operands for a BinaryOperator, see if we can fold the result.
/// If not, this returns null.
static Value *simplifyBinOp(unsigned Opcode, Value *LHS, Value *RHS,
                            const SimplifyQuery &Q, unsigned MaxRecurse) {
  switch (Opcode) {
  case Instruction::Add:
    return ::simplifyAddInst(LHS, RHS, false, false, Q);
  case Instruction::Xor:
    return ::simplifyXorInst(LHS, RHS, Q);
  case Instruction::Sub:
    return ::simplifySubInst(LHS, RHS, false, false, Q);

  /*
  case Instruction::Mul:
    return simplifyMulInst(LHS, RHS, Q, MaxRecurse);
  case Instruction::SDiv:
    return simplifySDivInst(LHS, RHS, Q, MaxRecurse);
  case Instruction::UDiv:
    return simplifyUDivInst(LHS, RHS, Q, MaxRecurse);
  case Instruction::SRem:
    return simplifySRemInst(LHS, RHS, Q, MaxRecurse);
  case Instruction::URem:
    return simplifyURemInst(LHS, RHS, Q, MaxRecurse);
  case Instruction::Shl:
    return simplifyShlInst(LHS, RHS, false, false, Q, MaxRecurse);
  case Instruction::LShr:
    return simplifyLShrInst(LHS, RHS, false, Q, MaxRecurse);
  case Instruction::AShr:
    return simplifyAShrInst(LHS, RHS, false, Q, MaxRecurse);
  case Instruction::And:
    return simplifyAndInst(LHS, RHS, Q, MaxRecurse);
  case Instruction::Or:
    return simplifyOrInst(LHS, RHS, Q, MaxRecurse);
  case Instruction::FAdd:
    return simplifyFAddInst(LHS, RHS, FastMathFlags(), Q, MaxRecurse);
  case Instruction::FSub:
    return simplifyFSubInst(LHS, RHS, FastMathFlags(), Q, MaxRecurse);
  case Instruction::FMul:
    return simplifyFMulInst(LHS, RHS, FastMathFlags(), Q, MaxRecurse);
  case Instruction::FDiv:
    return simplifyFDivInst(LHS, RHS, FastMathFlags(), Q, MaxRecurse);
  case Instruction::FRem:
    return simplifyFRemInst(LHS, RHS, FastMathFlags(), Q, MaxRecurse);
  */
  default:
    llvm_unreachable("Unexpected opcode");
  }
}



/// Generic simplifications for associative binary operations.
/// Returns the simpler value, or null if none was found.
static Value *simplifyAssociativeBinOp(Instruction::BinaryOps Opcode,
                                       Value *LHS, Value *RHS,
                                       const SimplifyQuery &Q,
                                       unsigned MaxRecurse) {
  assert(Instruction::isAssociative(Opcode) && "Not an associative operation!");
  LLVM_DEBUG(dbgs() << "enter simplifyAssociativeBinOp : "
                    << "Op : " << Instruction::getOpcodeName(Opcode) << "\n");
  LLVM_DEBUG(dbgs() << "LHS : "; LHS->dump());
  LLVM_DEBUG(dbgs() << "RHS : "; RHS->dump());

  // Recursion is always used, so bail out at once if we already hit the limit.
  if (!MaxRecurse--)
    return nullptr;

  BinaryOperator *Op0 = dyn_cast<BinaryOperator>(LHS);
  BinaryOperator *Op1 = dyn_cast<BinaryOperator>(RHS);

  // Transform: "(A op B) op C" ==> "A op (B op C)" if it simplifies completely.
  if (Op0 && Op0->getOpcode() == Opcode) {
    Value *A = Op0->getOperand(0);
    Value *B = Op0->getOperand(1);
    Value *C = RHS;
    LLVM_DEBUG(dbgs() << "check : ";A->dump();B->dump();C->dump());
    // Does "B op C" simplify?
    if (Value *V = simplifyBinOp(Opcode, B, C, Q, MaxRecurse)) {
      LLVM_DEBUG(dbgs() << "(B op C) simplifyBinOp : ";V->dump());
      // It does!  Return "A op V" if it simplifies or is already available.
      // If V equals B then "A op V" is just the LHS.
      if (V == B) {
        LLVM_DEBUG(dbgs() << "V == B : ";V->dump());
        return LHS;
      }
      // Otherwise return "A op V" if it simplifies.
      if (Value *W = simplifyBinOp(Opcode, A, V, Q, MaxRecurse)) {
        LLVM_DEBUG(dbgs() << "(A op V) simplifyBinOp : ";W->dump());
        ++NumReassoc;
        return W;
      }
    }
  }

  // Transform: "A op (B op C)" ==> "(A op B) op C" if it simplifies completely.
  if (Op1 && Op1->getOpcode() == Opcode) {
    Value *A = LHS;
    Value *B = Op1->getOperand(0);
    Value *C = Op1->getOperand(1);
    LLVM_DEBUG(dbgs() << "check : ";A->dump();B->dump();C->dump());
    // Does "A op B" simplify?
    if (Value *V = simplifyBinOp(Opcode, A, B, Q, MaxRecurse)) {
      // It does!  Return "V op C" if it simplifies or is already available.
      // If V equals B then "V op C" is just the RHS.
      if (V == B)
        return RHS;
      // Otherwise return "V op C" if it simplifies.
      if (Value *W = simplifyBinOp(Opcode, V, C, Q, MaxRecurse)) {
        ++NumReassoc;
        return W;
      }
    }
  }

  // The remaining transforms require commutativity as well as associativity.
  if (!Instruction::isCommutative(Opcode))
    return nullptr;

  // Transform: "(A op B) op C" ==> "(C op A) op B" if it simplifies completely.
  if (Op0 && Op0->getOpcode() == Opcode) {
    Value *A = Op0->getOperand(0);
    Value *B = Op0->getOperand(1);
    Value *C = RHS;
    LLVM_DEBUG(dbgs() << "check : ";A->dump();B->dump();C->dump());
    // Does "C op A" simplify?
    if (Value *V = simplifyBinOp(Opcode, C, A, Q, MaxRecurse)) {
      // It does!  Return "V op B" if it simplifies or is already available.
      // If V equals A then "V op B" is just the LHS.
      if (V == A)
        return LHS;
      // Otherwise return "V op B" if it simplifies.
      if (Value *W = simplifyBinOp(Opcode, V, B, Q, MaxRecurse)) {
        ++NumReassoc;
        return W;
      }
    }
  }

  // Transform: "A op (B op C)" ==> "B op (C op A)" if it simplifies completely.
  if (Op1 && Op1->getOpcode() == Opcode) {
    Value *A = LHS;
    Value *B = Op1->getOperand(0);
    Value *C = Op1->getOperand(1);
    LLVM_DEBUG(dbgs() << "check : ";A->dump();B->dump();C->dump());
    // Does "C op A" simplify?
    if (Value *V = simplifyBinOp(Opcode, C, A, Q, MaxRecurse)) {
      // It does!  Return "B op V" if it simplifies or is already available.
      // If V equals C then "B op V" is just the RHS.
      if (V == C)
        return RHS;
      // Otherwise return "B op V" if it simplifies.
      if (Value *W = simplifyBinOp(Opcode, B, V, Q, MaxRecurse)) {
        ++NumReassoc;
        return W;
      }
    }
  }

  LLVM_DEBUG(dbgs() << "exit simplifyAssociativeBinOp\n");
  return nullptr;
}




/// Compute the constant difference between two pointer values.
/// If the difference is not a constant, returns zero.
static Constant *computePointerDifference(const DataLayout &DL, Value *LHS,
                                          Value *RHS) {
  APInt LHSOffset = stripAndComputeConstantOffsets(DL, LHS);
  APInt RHSOffset = stripAndComputeConstantOffsets(DL, RHS);

  // If LHS and RHS are not related via constant offsets to the same base
  // value, there is nothing we can do here.
  if (LHS != RHS)
    return nullptr;

  // Otherwise, the difference of LHS - RHS can be computed as:
  //    LHS - RHS
  //  = (LHSOffset + Base) - (RHSOffset + Base)
  //  = LHSOffset - RHSOffset
  Constant *Res = ConstantInt::get(LHS->getContext(), LHSOffset - RHSOffset);
  if (auto *VecTy = dyn_cast<VectorType>(LHS->getType()))
    Res = ConstantVector::getSplat(VecTy->getElementCount(), Res);
  return Res;
}


static Value *_simplifyCastInst(unsigned CastOpc, Value *Op, Type *Ty,
                               const SimplifyQuery &Q, unsigned MaxRecurse) {
  if (auto *C = dyn_cast<Constant>(Op))
    return ConstantFoldCastOperand(CastOpc, C, Ty, Q.DL);

  if (auto *CI = dyn_cast<CastInst>(Op)) {
    auto *Src = CI->getOperand(0);
    Type *SrcTy = Src->getType();
    Type *MidTy = CI->getType();
    Type *DstTy = Ty;
    if (Src->getType() == Ty) {
      auto FirstOp = static_cast<Instruction::CastOps>(CI->getOpcode());
      auto SecondOp = static_cast<Instruction::CastOps>(CastOpc);
      Type *SrcIntPtrTy =
          SrcTy->isPtrOrPtrVectorTy() ? Q.DL.getIntPtrType(SrcTy) : nullptr;
      Type *MidIntPtrTy =
          MidTy->isPtrOrPtrVectorTy() ? Q.DL.getIntPtrType(MidTy) : nullptr;
      Type *DstIntPtrTy =
          DstTy->isPtrOrPtrVectorTy() ? Q.DL.getIntPtrType(DstTy) : nullptr;
      if (CastInst::isEliminableCastPair(FirstOp, SecondOp, SrcTy, MidTy, DstTy,
                                         SrcIntPtrTy, MidIntPtrTy,
                                         DstIntPtrTy) == Instruction::BitCast)
        return Src;
    }
  }

  // bitcast x -> x
  if (CastOpc == Instruction::BitCast)
    if (Op->getType() == Ty)
      return Op;

  return nullptr;
}


/// Given operands for a Sub, see if we can fold the result.
/// If not, this returns null.
static Value *_simplifySubInst(Value *Op0, Value *Op1, bool isNSW, bool isNUW,
                              const SimplifyQuery &Q, unsigned MaxRecurse) {
  if (Constant *C = foldOrCommuteConstant(Instruction::Sub, Op0, Op1, Q))
    return C;

  // X - poison -> poison
  // poison - X -> poison
  if (isa<PoisonValue>(Op0) || isa<PoisonValue>(Op1))
    return PoisonValue::get(Op0->getType());

  // X - undef -> undef
  // undef - X -> undef
  if (Q.isUndefValue(Op0) || Q.isUndefValue(Op1))
    return UndefValue::get(Op0->getType());

  // X - 0 -> X
  if (match(Op1, m_Zero()))
    return Op0;

  // X - X -> 0
  if (Op0 == Op1)
    return Constant::getNullValue(Op0->getType());

  // Is this a negation?
  if (match(Op0, m_Zero())) {
    // 0 - X -> 0 if the sub is NUW.
    if (isNUW)
      return Constant::getNullValue(Op0->getType());

    KnownBits Known = llvm::computeKnownBits(Op1, Q.DL, 0, Q.AC, Q.CxtI, Q.DT);
    if (Known.Zero.isMaxSignedValue()) {
      // Op1 is either 0 or the minimum signed value. If the sub is NSW, then
      // Op1 must be 0 because negating the minimum signed value is undefined.
      if (isNSW)
        return Constant::getNullValue(Op0->getType());

      // 0 - X -> X if X is 0 or the minimum signed value.
      return Op1;
    }
  }

  // (X + Y) - Z -> X + (Y - Z) or Y + (X - Z) if everything simplifies.
  // For example, (X + Y) - Y -> X; (Y + X) - Y -> X
  Value *X = nullptr, *Y = nullptr, *Z = Op1;
  if (MaxRecurse && match(Op0, m_Add(m_Value(X), m_Value(Y)))) { // (X + Y) - Z
    // See if "V === Y - Z" simplifies.
    if (Value *V = simplifyBinOp(Instruction::Sub, Y, Z, Q, MaxRecurse - 1))
      // It does!  Now see if "X + V" simplifies.
      if (Value *W = simplifyBinOp(Instruction::Add, X, V, Q, MaxRecurse - 1)) {
        // It does, we successfully reassociated!
        ++NumReassoc;
        return W;
      }
    // See if "V === X - Z" simplifies.
    if (Value *V = simplifyBinOp(Instruction::Sub, X, Z, Q, MaxRecurse - 1))
      // It does!  Now see if "Y + V" simplifies.
      if (Value *W = simplifyBinOp(Instruction::Add, Y, V, Q, MaxRecurse - 1)) {
        // It does, we successfully reassociated!
        ++NumReassoc;
        return W;
      }
  }

  // X - (Y + Z) -> (X - Y) - Z or (X - Z) - Y if everything simplifies.
  // For example, X - (X + 1) -> -1
  X = Op0;
  if (MaxRecurse && match(Op1, m_Add(m_Value(Y), m_Value(Z)))) { // X - (Y + Z)
    // See if "V === X - Y" simplifies.
    if (Value *V = simplifyBinOp(Instruction::Sub, X, Y, Q, MaxRecurse - 1))
      // It does!  Now see if "V - Z" simplifies.
      if (Value *W = simplifyBinOp(Instruction::Sub, V, Z, Q, MaxRecurse - 1)) {
        // It does, we successfully reassociated!
        ++NumReassoc;
        return W;
      }
    // See if "V === X - Z" simplifies.
    if (Value *V = simplifyBinOp(Instruction::Sub, X, Z, Q, MaxRecurse - 1))
      // It does!  Now see if "V - Y" simplifies.
      if (Value *W = simplifyBinOp(Instruction::Sub, V, Y, Q, MaxRecurse - 1)) {
        // It does, we successfully reassociated!
        ++NumReassoc;
        return W;
      }
  }

  // Z - (X - Y) -> (Z - X) + Y if everything simplifies.
  // For example, X - (X - Y) -> Y.
  Z = Op0;
  if (MaxRecurse && match(Op1, m_Sub(m_Value(X), m_Value(Y)))) // Z - (X - Y)
    // See if "V === Z - X" simplifies.
    if (Value *V = simplifyBinOp(Instruction::Sub, Z, X, Q, MaxRecurse - 1))
      // It does!  Now see if "V + Y" simplifies.
      if (Value *W = simplifyBinOp(Instruction::Add, V, Y, Q, MaxRecurse - 1)) {
        // It does, we successfully reassociated!
        ++NumReassoc;
        return W;
      }

  // trunc(X) - trunc(Y) -> trunc(X - Y) if everything simplifies.
  if (MaxRecurse && match(Op0, m_Trunc(m_Value(X))) &&
      match(Op1, m_Trunc(m_Value(Y))))
    if (X->getType() == Y->getType())
      // See if "V === X - Y" simplifies.
      if (Value *V = simplifyBinOp(Instruction::Sub, X, Y, Q, MaxRecurse - 1))
        // It does!  Now see if "trunc V" simplifies.
        if (Value *W = _simplifyCastInst(Instruction::Trunc, V, Op0->getType(), Q, MaxRecurse - 1))
          // It does, return the simplified "trunc V".
          return W;

  // Variations on GEP(base, I, ...) - GEP(base, i, ...) -> GEP(null, I-i, ...).
  if (match(Op0, m_PtrToInt(m_Value(X))) && match(Op1, m_PtrToInt(m_Value(Y))))
    if (Constant *Result = computePointerDifference(Q.DL, X, Y))
      return ConstantExpr::getIntegerCast(Result, Op0->getType(), true);

  // i1 sub -> xor.
  if (MaxRecurse && Op0->getType()->isIntOrIntVectorTy(1))
    if (Value *V = _simplifyXorInst(Op0, Op1, Q, MaxRecurse - 1))
      return V;

  // Threading Sub over selects and phi nodes is pointless, so don't bother.
  // Threading over the select in "A - select(cond, B, C)" means evaluating
  // "A-B" and "A-C" and seeing if they are equal; but they are equal if and
  // only if B and C are equal.  If B and C are equal then (since we assume
  // that operands have already been simplified) "select(cond, B, C)" should
  // have been simplified to the common value of B and C already.  Analysing
  // "A-B" and "A-C" thus gains nothing, but costs compile time.  Similarly
  // for threading over phi nodes.

  return nullptr;
}


/// Given operands for a Xor, see if we can fold the result.
/// If not, this returns null.
static Value *_simplifyXorInst(Value *Op0, Value *Op1, const SimplifyQuery &Q, unsigned MaxRecurse) {
  LLVM_DEBUG(dbgs() << "Enter simplifyXorInst : ";
             dbgs() << "Op0 : "; Op0->dump(); dbgs() << "Op1 : "; Op1->dump(););
  if (Constant *C = foldOrCommuteConstant(Instruction::Xor, Op0, Op1, Q)) {
    LLVM_DEBUG(dbgs() << "foldOrCommuteConstant : "; C->dump());
    return C;
  }

  // X ^ poison -> poison
  if (isa<PoisonValue>(Op1)) {
    LLVM_DEBUG(dbgs() << "PoisonValue>(Op1) : "; Op1->dump());
    return Op1;
  }

  // A ^ undef -> undef
  if (Q.isUndefValue(Op1)) {
    LLVM_DEBUG(dbgs() << "Q.isUndefValue(Op1) : "; Op1->dump());
    return Op1;
  }

  // A ^ 0 = A
  if (match(Op1, m_Zero())) {
    LLVM_DEBUG(dbgs() << "match(Op1, m_Zero() : "; Op1->dump());
    return Op0;
  }

  // A ^ A = 0
  if (Op0 == Op1) {
    LLVM_DEBUG(dbgs() << "isKnownNegation(Op0, Op1) : "; Op0->dump());
    return Constant::getNullValue(Op0->getType());
  }

  // A ^ ~A  =  ~A ^ A  =  -1
  if (match(Op0, m_Not(m_Specific(Op1))) || match(Op1, m_Not(m_Specific(Op0)))) {
    LLVM_DEBUG(dbgs() << "OP0 : "; Op0->dump());
    LLVM_DEBUG(dbgs() << "OP1 : "; Op1->dump());
    return Constant::getAllOnesValue(Op0->getType());
  }

  auto foldAndOrNot = [](Value *X, Value *Y) -> Value * {
    Value *A, *B;
    // (~A & B) ^ (A | B) --> A -- There are 8 commuted variants.
    if (match(X, m_c_And(m_Not(m_Value(A)), m_Value(B))) &&
        match(Y, m_c_Or(m_Specific(A), m_Specific(B)))) {
      LLVM_DEBUG(dbgs() << "match : "; A->dump());
      return A;
    }

    // (~A | B) ^ (A & B) --> ~A -- There are 8 commuted variants.
    // The 'not' op must contain a complete -1 operand (no undef elements for
    // vector) for the transform to be safe.
    Value *NotA;
    if (match(X,
              m_c_Or(m_CombineAnd(m_NotForbidUndef(m_Value(A)), m_Value(NotA)),
                     m_Value(B))) &&
        match(Y, m_c_And(m_Specific(A), m_Specific(B)))) {
      LLVM_DEBUG(dbgs() << "match  : "; NotA->dump());
      return NotA;
    }

    return nullptr;
  };
  if (Value *R = foldAndOrNot(Op0, Op1)) {
    LLVM_DEBUG(dbgs() << "fold A & ~A  : "; Op0->dump(); Op1->dump());
    return R;
  }
  if (Value *R = foldAndOrNot(Op1, Op0)) {
    LLVM_DEBUG(dbgs() << "fold ~A & A  : "; Op0->dump(); Op1->dump());
    return R;
  }

  if (Value *V = _simplifyLogicOfAddSub(Op0, Op1, Instruction::Xor)) {
    LLVM_DEBUG(dbgs() << "_simplifyLogicOfAddSub  : "; V->dump());
    return V;
  }

  // Try some generic simplifications for associative operations.
  if (Op0->getType()->isIntOrIntVectorTy(1)) {
    if (Instruction *I = dyn_cast<Instruction>(Op0)) {
      if (I->getOpcode() == Instruction::Add) {
        if (Value *V =
                simplifyAssociativeBinOp(Instruction::Add, Op0, Op1, Q, MaxRecurse)) {
          LLVM_DEBUG(dbgs() << "simplifyAssociativeBinOp : "; V->dump());
          return V;
        }
      }
      // Sub operator did not associatedBinOp
      if (I->getOpcode() == Instruction::Sub) {
        if (Value *V =
                _simplifySubInst(Op0, Op1, false, false, Q, MaxRecurse)) {
          LLVM_DEBUG(dbgs() << "simplifyAssociativeBinOp : "; V->dump());
          return V;
        }
      }
    }
  }

  // Try some generic simplifications for associative operations.
  if (Value *V =
          simplifyAssociativeBinOp(Instruction::Xor, Op0, Op1, Q, MaxRecurse)) {
    LLVM_DEBUG(dbgs() << "simplifyAssociativeBinOp : "; V->dump());
    return V;
  }

  // Threading Xor over selects and phi nodes is pointless, so don't bother.
  // Threading over the select in "A ^ select(cond, B, C)" means evaluating
  // "A^B" and "A^C" and seeing if they are equal; but they are equal if and
  // only if B and C are equal.  If B and C are equal then (since we assume
  // that operands have already been simplified) "select(cond, B, C)" should
  // have been simplified to the common value of B and C already.  Analysing
  // "A^B" and "A^C" thus gains nothing, but costs compile time.  Similarly
  // for threading over phi nodes.
  LLVM_DEBUG(dbgs() << "simplify inst not matched : \n";);
  return nullptr;
}

static Value *_simplifyAddInst(Value *Op0, Value *Op1, bool IsNSW, bool IsNUW,
                               const SimplifyQuery &Q, unsigned MaxRecurse) {
  LLVM_DEBUG(dbgs() << "Enter simplifyAddInst : "; Op0->dump(); Op1->dump(););
  if (Constant *C = foldOrCommuteConstant(Instruction::Add, Op0, Op1, Q)) {
    LLVM_DEBUG(dbgs() << "foldOrCommuteConstant : "; C->dump());
    return C;
  }
  // X + poison -> poison
  if (isa<PoisonValue>(Op1)) {
    LLVM_DEBUG(dbgs() << "PoisonValue>(Op1) : "; Op1->dump());
    return Op1;
  }

  // X + undef -> undef
  if (Q.isUndefValue(Op1)) {
    LLVM_DEBUG(dbgs() << "Q.isUndefValue(Op1) : "; Op1->dump());
    return Op1;
  }

  // X + 0 -> X
  if (match(Op1, m_Zero())) {
    LLVM_DEBUG(dbgs() << "match(Op1, m_Zero() : "; Op1->dump());
    return Op0;
  }

  // If two operands are negative, return 0.
  if (isKnownNegation(Op0, Op1)) {
    LLVM_DEBUG(dbgs() << "isKnownNegation(Op0, Op1) : "; Op0->dump());
    return Constant::getNullValue(Op0->getType());
  }

  // X + (Y - X) -> Y
  // (Y - X) + X -> Y
  // Eg: X + -X -> 0
  Value *Y = nullptr;
  if (match(Op1, m_Sub(m_Value(Y), m_Specific(Op0))) ||
      match(Op0, m_Sub(m_Value(Y), m_Specific(Op1)))) {
    LLVM_DEBUG(dbgs() << "OP0 : "; Op0->dump());
    LLVM_DEBUG(dbgs() << "OP1 : "; Op1->dump());
    return Y;
  }

  // X + ~X -> -1   since   ~X = -X-1
  Type *Ty = Op0->getType();
  if (match(Op0, m_Not(m_Specific(Op1))) ||
      match(Op1, m_Not(m_Specific(Op0)))) {
    LLVM_DEBUG(dbgs() << "OP0 : "; Op0->dump());
    LLVM_DEBUG(dbgs() << "OP1 : "; Op1->dump());
    return Constant::getAllOnesValue(Ty);
  }

  // add nsw/nuw (xor Y, signmask), signmask --> Y
  // The no-wrapping add guarantees that the top bit will be set by the add.
  // Therefore, the xor must be clearing the already set sign bit of Y.
  if ((IsNSW || IsNUW) && match(Op1, m_SignMask()) &&
      match(Op0, m_Xor(m_Value(Y), m_SignMask()))) {
    LLVM_DEBUG(dbgs() << "OP0 : "; Op0->dump());
    LLVM_DEBUG(dbgs() << "OP1 : "; Op1->dump());
    return Y;
  }

  // add nuw %x, -1  ->  -1, because %x can only be 0.
  if (IsNUW && match(Op1, m_AllOnes())) {
    LLVM_DEBUG(dbgs() << "IsNUW && match(Op1, m_AllOnes()) : "; Op1->dump());
    return Op1; // Which is -1.
  }

  /// i1 add -> xor.
  if (MaxRecurse && Op0->getType()->isIntOrIntVectorTy(1)) {
    if (Value *V = _simplifyXorInst(Op0, Op1, Q, MaxRecurse)) {
      LLVM_DEBUG(dbgs() << "simplifyXorInst : "; V->dump());
      return V;
    }
  }

  // Try some generic simplifications for associative operations.
  if (Value *V =
          simplifyAssociativeBinOp(Instruction::Add, Op0, Op1, Q, MaxRecurse)) {
    LLVM_DEBUG(dbgs() << "simplifyAssociativeBinOp : "; V->dump());
    return V;
  }
  // Threading Add over selects and phi nodes is pointless, so don't bother.
  // Threading over the select in "A + select(cond, B, C)" means evaluating
  // "A+B" and "A+C" and seeing if they are equal; but they are equal if and
  // only if B and C are equal.  If B and C are equal then (since we assume
  // that operands have already been simplified) "select(cond, B, C)" should
  // have been simplified to the common value of B and C already.  Analysing
  // "A+B" and "A+C" thus gains nothing, but costs compile time.  Similarly
  // for threading over phi nodes.
  LLVM_DEBUG(dbgs() << "simplify inst not matched : \n");
  return nullptr;
}

static std::unique_ptr<Module> makeLLVMModule(LLVMContext &context,
                                              const char *modulestr) {
  SMDiagnostic err;
  return parseAssemblyString(modulestr, err, context);
}

static Value *simplifyInstructionWithOperands(Instruction *I,
                                              ArrayRef<Value *> NewOps,
                                              const SimplifyQuery &SQ,
                                              OptimizationRemarkEmitter *ORE) {
  const SimplifyQuery Q = SQ.CxtI ? SQ : SQ.getWithInstruction(I);
  Value *Result = nullptr;

  switch (I->getOpcode()) {
  default:
    llvm_unreachable("Not Handled!");
  case Instruction::Add:
    Result = simplifyAddInst(
        NewOps[0], NewOps[1], Q.IIQ.hasNoSignedWrap(cast<BinaryOperator>(I)),
        Q.IIQ.hasNoUnsignedWrap(cast<BinaryOperator>(I)), Q);
  case Instruction::Xor:
    Result = simplifyXorInst(NewOps[0], NewOps[1], Q);
  }

  return Result == I ? UndefValue::get(I->getType()) : Result;
}

Value *llvm::simplifyAddInst(Value *Op0, Value *Op1, bool IsNSW, bool IsNUW,
                             const SimplifyQuery &Query) {
  return ::_simplifyAddInst(Op0, Op1, IsNSW, IsNUW, Query, RecursionLimit);
}

Value *llvm::simplifyXorInst(Value *Op0, Value *Op1, const SimplifyQuery &Q) {
  return ::_simplifyXorInst(Op0, Op1, Q, RecursionLimit);
}


Value *llvm::simplifySubInst(Value *Op0, Value *Op1, bool isNSW, bool isNUW,
                             const SimplifyQuery &Q) {
  return ::_simplifySubInst(Op0, Op1, isNSW, isNUW, Q, RecursionLimit);
}

Value *llvm::simplifyInstructionWithOperands(Instruction *I,
                                             ArrayRef<Value *> NewOps,
                                             const SimplifyQuery &SQ,
                                             OptimizationRemarkEmitter *ORE) {
  const SimplifyQuery Q = SQ.CxtI ? SQ : SQ.getWithInstruction(I);
  Value *Result = nullptr;

  switch (I->getOpcode()) {
  default:
    if (llvm::all_of(NewOps, [](Value *V) { return isa<Constant>(V); })) {
      SmallVector<Constant *, 8> NewConstOps(NewOps.size());
      transform(NewOps, NewConstOps.begin(),
                [](Value *V) { return cast<Constant>(V); });
      Result = ConstantFoldInstOperands(I, NewConstOps, Q.DL, Q.TLI);
    }
    break;
  case Instruction::FNeg:
    Result = simplifyFNegInst(NewOps[0], I->getFastMathFlags(), Q);
    break;
  case Instruction::FAdd:
    Result = simplifyFAddInst(NewOps[0], NewOps[1], I->getFastMathFlags(), Q);
    break;
  case Instruction::Add:
    Result = simplifyAddInst(
        NewOps[0], NewOps[1], Q.IIQ.hasNoSignedWrap(cast<BinaryOperator>(I)),
        Q.IIQ.hasNoUnsignedWrap(cast<BinaryOperator>(I)), Q);
    break;
  case Instruction::FSub:
    Result = simplifyFSubInst(NewOps[0], NewOps[1], I->getFastMathFlags(), Q);
    break;
  case Instruction::Sub:
    Result = simplifySubInst(
        NewOps[0], NewOps[1], Q.IIQ.hasNoSignedWrap(cast<BinaryOperator>(I)),
        Q.IIQ.hasNoUnsignedWrap(cast<BinaryOperator>(I)), Q);
    break;
  case Instruction::FMul:
    Result = simplifyFMulInst(NewOps[0], NewOps[1], I->getFastMathFlags(), Q);
    break;
  case Instruction::Mul:
    Result = simplifyMulInst(NewOps[0], NewOps[1], Q);
    break;
  case Instruction::SDiv:
    Result = simplifySDivInst(NewOps[0], NewOps[1], Q);
    break;
  case Instruction::UDiv:
    Result = simplifyUDivInst(NewOps[0], NewOps[1], Q);
    break;
  case Instruction::FDiv:
    Result = simplifyFDivInst(NewOps[0], NewOps[1], I->getFastMathFlags(), Q);
    break;
  case Instruction::SRem:
    Result = simplifySRemInst(NewOps[0], NewOps[1], Q);
    break;
  case Instruction::URem:
    Result = simplifyURemInst(NewOps[0], NewOps[1], Q);
    break;
  case Instruction::FRem:
    Result = simplifyFRemInst(NewOps[0], NewOps[1], I->getFastMathFlags(), Q);
    break;
  case Instruction::Shl:
    Result = simplifyShlInst(
        NewOps[0], NewOps[1], Q.IIQ.hasNoSignedWrap(cast<BinaryOperator>(I)),
        Q.IIQ.hasNoUnsignedWrap(cast<BinaryOperator>(I)), Q);
    break;
  case Instruction::LShr:
    Result = simplifyLShrInst(NewOps[0], NewOps[1],
                              Q.IIQ.isExact(cast<BinaryOperator>(I)), Q);
    break;
  case Instruction::AShr:
    Result = simplifyAShrInst(NewOps[0], NewOps[1],
                              Q.IIQ.isExact(cast<BinaryOperator>(I)), Q);
    break;
  case Instruction::And:
    Result = simplifyAndInst(NewOps[0], NewOps[1], Q);
    break;
  case Instruction::Or:
    Result = simplifyOrInst(NewOps[0], NewOps[1], Q);
    break;
  case Instruction::Xor:
    Result = ::simplifyXorInst(NewOps[0], NewOps[1], Q);
    break;
  case Instruction::ICmp:
    Result = simplifyICmpInst(cast<ICmpInst>(I)->getPredicate(), NewOps[0],
                              NewOps[1], Q);
    break;
  case Instruction::FCmp:
    Result = simplifyFCmpInst(cast<FCmpInst>(I)->getPredicate(), NewOps[0],
                              NewOps[1], I->getFastMathFlags(), Q);
    break;
  case Instruction::Select:
    Result = simplifySelectInst(NewOps[0], NewOps[1], NewOps[2], Q);
    break;
  case Instruction::GetElementPtr: {
    auto *GEPI = cast<GetElementPtrInst>(I);
    Result =
        simplifyGEPInst(GEPI->getSourceElementType(), NewOps[0],
                        makeArrayRef(NewOps).slice(1), GEPI->isInBounds(), Q);
    break;
  }
  case Instruction::InsertValue: {
    InsertValueInst *IV = cast<InsertValueInst>(I);
    Result = simplifyInsertValueInst(NewOps[0], NewOps[1], IV->getIndices(), Q);
    break;
  }
  case Instruction::InsertElement: {
    Result = simplifyInsertElementInst(NewOps[0], NewOps[1], NewOps[2], Q);
    break;
  }
  case Instruction::ExtractValue: {
    auto *EVI = cast<ExtractValueInst>(I);
    Result = simplifyExtractValueInst(NewOps[0], EVI->getIndices(), Q);
    break;
  }
  case Instruction::ExtractElement: {
    Result = simplifyExtractElementInst(NewOps[0], NewOps[1], Q);
    break;
  }
  case Instruction::ShuffleVector: {
    auto *SVI = cast<ShuffleVectorInst>(I);
    Result = simplifyShuffleVectorInst(
        NewOps[0], NewOps[1], SVI->getShuffleMask(), SVI->getType(), Q);
    break;
  }
  /*
  case Instruction::PHI:
    Result = simplifyPHINode(cast<PHINode>(I), NewOps, Q);
    break;
  */
  case Instruction::Call: {
    // TODO: Use NewOps
    Result = simplifyCall(cast<CallInst>(I), Q);
    break;
  }
  case Instruction::Freeze:
    Result = llvm::simplifyFreezeInst(NewOps[0], Q);
    break;
  }

  /// If called on unreachable code, the above logic may report that the
  /// instruction simplified to itself.  Make life easier for users by
  /// detecting that case here, returning a safe value instead.

  if (Result) 
    LLVM_DEBUG(dbgs() << "simplify success ; ";Result->dump());

  return Result == I ? UndefValue::get(I->getType()) : Result;
}

Value *llvm::simplifyInstruction(Instruction *I, const SimplifyQuery &SQ,
                                 OptimizationRemarkEmitter *ORE) {
  SmallVector<Value *, 8> Ops(I->operands());
  return llvm::simplifyInstructionWithOperands(I, Ops, SQ, ORE);
}

STATISTIC(NumSimplified, "Number of redundant instructions removed");
static bool runImpl(Function &F, const SimplifyQuery &SQ,
                    OptimizationRemarkEmitter *ORE) {
  SmallPtrSet<const Instruction *, 8> S1, S2, *ToSimplify = &S1, *Next = &S2;
  bool Changed = false;

  do {
    for (BasicBlock &BB : F) {
      // Unreachable code can take on strange forms that we are not prepared to
      // handle. For example, an instruction may have itself as an operand.
      if (!SQ.DT->isReachableFromEntry(&BB))
        continue;

      SmallVector<WeakTrackingVH, 8> DeadInstsInBB;
      for (Instruction &I : BB) {
        LLVM_DEBUG(dbgs() << "Instruction : "; I.dump());
        // The first time through the loop, ToSimplify is empty and we try to
        // simplify all instructions. On later iterations, ToSimplify is not
        // empty and we only bother simplifying instructions that are in it.
        if (!ToSimplify->empty() && !ToSimplify->count(&I))
          continue;

        // Don't waste time simplifying dead/unused instructions.
        if (isInstructionTriviallyDead(&I)) {
          DeadInstsInBB.push_back(&I);
          Changed = true;
        } else if (!I.use_empty()) {
          if (Value *V = simplifyInstruction(&I, SQ, ORE)) {
            LLVM_DEBUG(dbgs() << "has Simplified : "; V->dump());
            // Mark all uses for resimplification next time round the loop.
            for (User *U : I.users())
              Next->insert(cast<Instruction>(U));
            I.replaceAllUsesWith(V);
            ++NumSimplified;
            Changed = true;
            // A call can get simplified, but it may not be trivially dead.
            if (isInstructionTriviallyDead(&I))
              DeadInstsInBB.push_back(&I);
          }
        }
      }
      RecursivelyDeleteTriviallyDeadInstructions(DeadInstsInBB, SQ.TLI);
    }

    // Place the list of instructions to simplify on the next loop iteration
    // into ToSimplify.
    std::swap(ToSimplify, Next);
    Next->clear();
  } while (!ToSimplify->empty());

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      LLVM_DEBUG(dbgs() << "[RESULT] Instruction : "; I.dump());
    }
  }

  return Changed;
}

TEST(LOCAL, simplifyXor) {
  std::stringstream IR;
  std::ifstream in("simplify/addsub.ll");
  IR << in.rdbuf();

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());

  std::vector<std::string> test = {
    //"test7", 
    "test10",
    "test8"};
  for (std::string el : test) {
    LLVM_DEBUG(dbgs() << el << '\n';);
    auto *F = M->getFunction(el);
    DominatorTree DT(*F);
    AssumptionCache AC(*F);
    TargetLibraryInfoImpl TLII;
    TargetLibraryInfo TLI(TLII);
    DataLayout DL(F->getParent());
    SimplifyQuery SQ(DL, &TLI, &DT, &AC);
    OptimizationRemarkEmitter ORE(F);
    runImpl(*F, SQ, &ORE);
  }
}

/*
TEST(LOCAL, simplifyAddSub) {
  std::stringstream IR;
  std::ifstream in("simplify/addsub.ll");
  IR << in.rdbuf();

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());

  for (Function &F : M->functions()) {
    LLVM_DEBUG(dbgs() << F.getName() << '\n';);
    DominatorTree DT(F);

    // Visit the instructions in the function using the InstVisitor
    for (BasicBlock &BB : F) {
      if (!DT.isReachableFromEntry(&BB))
        continue;
      for (Instruction &I : BB) {
        if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
          // simplifier.visit(I);
        }
      }
    }
  }
}


TEST(LOCAL, simplifyAdd) {
  std::stringstream IR;
  std::ifstream in("simplify/add.ll");
  IR << in.rdbuf();

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());

  for (Function &F : M->functions()) {
    LLVM_DEBUG(dbgs() << F.getName() << '\n';);
    DominatorTree DT(F);

    for (BasicBlock &BB : F) {
      if (!DT.isReachableFromEntry(&BB))
        continue;
      for (Instruction &I : BB) {
        if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
          // simplifier.visit(I);
        }
      }
    }
  }
}

*/
int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}