
#include "llvm/IR/PatternMatch.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AssumeBundleQueries.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/CmpInstAnalysis.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopNestAnalysis.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/OverflowInstAnalysis.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Analysis/VectorUtils.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

using namespace llvm;
using namespace PatternMatch;

#define DEBUG_TYPE "issue"

#define LLVM_DEBUG(X)                                                          \
  do {                                                                         \
    dbgs() << "[" << DEBUG_TYPE << "]" << __FUNCTION__ << ":" << __LINE__      \
           << "\t";                                                            \
    X;                                                                         \
  } while (false)

#define LLVM_DEBUGM(X)                                                         \
  do {                                                                         \
    dbgs() << "[" << DEBUG_TYPE << "]" << __FUNCTION__ << ":" << __LINE__      \
           << "\t" X; dbgs() << "\n";                                                     \
  } while (false)

static std::unique_ptr<Module> makeLLVMModule(LLVMContext &context,
                                              const char *modulestr) {
  SMDiagnostic err;
  return parseAssemblyString(modulestr, err, context);
}

struct BitFieldAddBitMask {
  const APInt *Lower;
  const APInt *Upper;
};
struct BitFieldOptBitMask {
  const APInt *Lower;
  const APInt *Upper;
  const APInt *New;
};
struct BitFieldAddInfo {
  Value *X;
  Value *Y;
  bool opt;
  union {
    BitFieldAddBitMask AddMask;
    BitFieldOptBitMask OptMask;
  };
};

//static Value *foldBitfieldArithmetic(BinaryOperator &I, InstCombinerImpl &IC,
//                                     InstCombiner::BuilderTy &Builder) {
static Value *foldBitfieldArithmetic(BinaryOperator &I, const SimplifyQuery &Q,
                                     IRBuilder<> &Builder) {

  if (auto *disjoint = dyn_cast<PossiblyDisjointInst>(&I))
    if (!disjoint->isDisjoint())
      return nullptr;

  LLVM_DEBUGM("disjoint : ");

  auto matchBitFieldArithmetic =
      [&](Value *Op0, Value *Op1) -> std::optional<BitFieldAddInfo> {
    const APInt *maskOpt1, *maskOpt2, *maskLower, *maskUpper,
        *maskUpper2 = nullptr;
    Value *X, *Y;
    unsigned Highest, BitWidth;

    auto bitFieldAddLower =
        m_And(m_Add(m_Value(X), m_Value(Y)), m_APInt(maskLower));
    auto bitFieldAddUpper = m_CombineOr(
        m_Add(m_And(m_Value(X), m_APInt(maskUpper)), m_Value(Y)),
        m_And(m_Add(m_And(m_Value(X), m_APInt(maskUpper)), m_Value(Y)),
              m_APInt(maskUpper2)));

    auto optBitFieldAdd =
        m_c_Xor(m_c_Add(m_c_And(m_Value(X), m_APInt(maskOpt1)), m_Value(Y)),
                m_c_And(m_Deferred(X), m_APInt(maskOpt2)));

    if ((match(Op0, bitFieldAddLower) && match(Op1, bitFieldAddUpper)) ||
        (match(Op1, bitFieldAddLower) && match(Op0, bitFieldAddUpper))) {
      LLVM_DEBUGM("match BitField : " << *maskLower << " " << *maskUpper);
      // 00000111 ; 1st 
      // 00011000 ; 2nd
      BitWidth = maskLower->getBitWidth();
      APInt Mask(BitWidth, 0);
      Highest = maskUpper->countl_zero(); 
      Mask.setBits(BitWidth - Highest, BitWidth);
      // 1. 1st 2nd bitmask 는 같은 비트가 있어서는 안된다.
      // 2. 1st 2nd bitmask 의 비트는 연쇄적이어야 한다. 
      // 3. 각 bitmask는 2자리 이상이어야 한다.
      // TODO : X는 maskLower 비트 범위 내에, Y는 maskUpper 비트 범위 내에만 존재하는 값이여야 한다. 

      LLVM_DEBUGM("popcount : " << maskLower->popcount() << " " << maskUpper->popcount());
      LLVM_DEBUGM("sm : " << maskLower->isShiftedMask() << " " << maskUpper->isShiftedMask());
      LLVM_DEBUGM("bm : " ;KnownBits::makeConstant(Mask).print(errs()));
      LLVM_DEBUGM("bm : " ;KnownBits::makeConstant(*maskLower).print(errs()));
      LLVM_DEBUGM("bm : " ;KnownBits::makeConstant(*maskUpper).print(errs()));
      LLVM_DEBUGM("bm : " ;KnownBits::makeConstant(Mask ^ *maskLower ^ *maskUpper).print(errs()));
      if (!((maskUpper2 == nullptr || *maskUpper == *maskUpper2) &&
            (maskLower->popcount() >= 2 && maskUpper->popcount() >= 2) &&
            (maskLower->isShiftedMask() && maskUpper->isShiftedMask()) &&
            ((*maskLower & *maskUpper) == 0) &&
            ((Mask ^ *maskLower ^ *maskUpper).isAllOnes())))
        return std::nullopt;

      return {{X, Y, false, {{maskLower, maskUpper}}}};
    }

    if ((match(Op0, optBitFieldAdd) && match(Op1, bitFieldAddUpper)) ||
        (match(Op1, optBitFieldAdd) && match(Op0, bitFieldAddUpper))) {
      LLVM_DEBUGM("match BitField opt: " << *maskOpt1 << " " << *maskOpt2);

      // check bits are fit our purpose

      // ; 00001011 ; 11
      // ; 00010100 ; 20
      // ; 01101011 ; 107
      // ; 10010100 ; 108
      // opt1 opt2 검사
      // 1. opt1 를 l 부터 (최상위 1) +1 까지 비트가 1인지 체크
      // 2. 1에서 0을 만날때 같은 위치에 opt2의 비트가 1인지 체크
      // -> 1에 not하고 2의 1이 나올때까지 비트를 제거하고 같은지 비교.
      BitWidth = maskOpt1->getBitWidth();
      APInt Mask(BitWidth, 0), Mask2(BitWidth, 0);
      LLVM_DEBUGM("test : " << maskOpt1->countl_zero());
      LLVM_DEBUGM("Opt1 : "; KnownBits::makeConstant(*maskOpt1).print(errs()));
      LLVM_DEBUGM("Opt2 : "; KnownBits::makeConstant(*maskOpt2).print(errs()));
      LLVM_DEBUGM("Send : "; KnownBits::makeConstant(*maskUpper).print(errs()));

      Highest = maskOpt2->countl_zero(); 
      Mask.setBits(BitWidth - Highest, BitWidth);
      LLVM_DEBUGM("Highest : " << Highest << " ";KnownBits::makeConstant(Mask).print(errs()));

      APInt NotOpt1 = ~*maskOpt1;
      LLVM_DEBUGM("test : " ;KnownBits::makeConstant(NotOpt1).print(errs()));
      LLVM_DEBUGM("test : " ;KnownBits::makeConstant(NotOpt1 ^ Mask).print(errs()));
      LLVM_DEBUGM(" std::nullopt here : " << (NotOpt1 ^ Mask) << " "
                                          << *maskOpt2);
      LLVM_DEBUGM("isMask1 : " << maskUpper->isShiftedMask() << " " << maskUpper->isMask());
      LLVM_DEBUGM("isMask2 : " << maskOpt1->isShiftedMask() << " "
                               << maskOpt1->isMask());
      LLVM_DEBUGM("isMask3 : " << maskOpt2->isShiftedMask() << " "
                               << maskOpt2->isMask());

      Highest = maskUpper->countl_zero(); 
      LLVM_DEBUGM("Highest : " << Highest);
      Mask2.setBits(BitWidth - Highest, BitWidth);
      LLVM_DEBUGM(" bit : " ; KnownBits::makeConstant(Mask2).print(errs()));

      // 1. maskUpper 는 2개 이상의 비트가 연속되어 세팅되야 한다. 
      // 2. maskOpt1과 maskOpt2 에 공통 비트는 없어야 한다.
      // 3. maskUpper와 maskOpt1, maskOpt2 에 공통 비트는 없어야 한다.
      // 4. maskOpt1의 not 은 maskOpt2와 같아야 한다. 단, maskOpt2의 countl_zero() 제외.
      // 5. maskOpt1과 maskOpt2 의 XOR은 연속된 비트의 세팅이어야 한다.
      if (!((maskUpper2 != nullptr && *maskUpper == *maskUpper2) &&
            (maskUpper->isShiftedMask() && maskUpper->popcount() >= 2) &&
            ((*maskUpper & (*maskOpt1 | *maskOpt2)) == 0) &&
            ((~*maskOpt1 ^ Mask) == *maskOpt2) &&
            (Mask2 ^ *maskUpper ^ (*maskOpt1 ^ *maskOpt2)).isAllOnes()))
        return std::nullopt;

      struct BitFieldAddInfo info = {X, Y, true, {{maskOpt1, maskOpt2}}};
      info.OptMask.New = maskUpper;
      return {info};
    }

    return std::nullopt;
  };

  auto info = matchBitFieldArithmetic(I.getOperand(0), I.getOperand(1));

  if (info) {
    Value *X = info->X;
    Value *Y = info->Y;
    APInt BitMaskLower, BitMaskUpper;
    unsigned BitWidth = info->AddMask.Lower->getBitWidth();

    if (info->opt) {
      unsigned NewHiBit = info->OptMask.New->countl_zero() + 1;
      BitMaskLower = *info->OptMask.Lower | *info->OptMask.New;
      BitMaskLower.clearBit(NewHiBit);
      BitMaskUpper = *info->OptMask.Upper;
      BitMaskUpper.setBit(NewHiBit);
    } else {
      unsigned LowerHiBit =
          BitWidth - (info->AddMask.Lower->countl_zero() + 1);
      unsigned UpperHiBit =
          BitWidth - (info->AddMask.Upper->countl_zero() + 1);

      BitMaskLower = *info->AddMask.Lower | *info->AddMask.Upper;
      BitMaskLower.clearBit(LowerHiBit);
      BitMaskLower.clearBit(UpperHiBit);
      BitMaskUpper = APInt::getOneBitSet(BitWidth, LowerHiBit);
      BitMaskUpper.setBit(UpperHiBit);
    }

    auto AndLower = Builder.CreateAnd(X, BitMaskLower);
    auto Add = Builder.CreateNUWAdd(AndLower, Y);
    auto AndUpper = Builder.CreateAnd(X, BitMaskUpper);
    auto Xor = Builder.CreateXor(Add, AndUpper);

    return Xor;
  }

  return nullptr;
}

int main(int argc, char **argv) {
  std::stringstream IR;
  std::ifstream in("issue-33874.ll");
  IR << in.rdbuf();

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());

  auto *F = M->getFunction(argv[1]);
  TargetLibraryInfoImpl TLII;
  TargetLibraryInfo TLI(TLII);
  AssumptionCache AC(*F);
  DominatorTree DT(*F);
  LoopInfo LI(DT);
  ScalarEvolution SE(*F, TLI, AC, DT, LI);
  AAResults AA(TLI);
  DataLayout DL(F->getParent());
  const SimplifyQuery Q(DL, &TLI, &DT, &AC, nullptr, /*UseInstrInfo*/ true,
                        /*CanUseUndef*/ true);

  for (BasicBlock &BB : *F) {
    for (auto &I : BB) {
      LLVM_DEBUGM("handle instruction"; I.dump(););

      Value *Op0, *Op1;
      if (I.getNumOperands() > 0) {
        Op0 = I.getOperand(0);
        KnownBits Op0KB = computeKnownBits(Op0, DL, 0, &AC, &I, &DT);
        LLVM_DEBUGM("Op0 :"; Op0KB.print(errs()););
        if (I.getNumOperands() > 1) {
          Op1 = I.getOperand(1);
          KnownBits Op1KB = computeKnownBits(Op1, DL, 0, &AC, &I, &DT);
          LLVM_DEBUGM("Op1 :"; Op1KB.print(errs()););
        }
      }

      if (auto Bop = dyn_cast<BinaryOperator>(&I)) {
        IRBuilder Builder(&I);
        LLVM_DEBUGM("enter : ");
        if (Value *Res = foldBitfieldArithmetic(*Bop, Q, Builder))
          I.replaceAllUsesWith(Res);
          F->dump();
      }
    }
  }
}
