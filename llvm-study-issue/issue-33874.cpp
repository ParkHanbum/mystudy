
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

  auto *Disjoint = dyn_cast<PossiblyDisjointInst>(&I);
  if (!Disjoint || !Disjoint->isDisjoint())
    return nullptr;

  LLVM_DEBUGM("disjoint : ");

  auto AccumulateY = [&](Value *LoY, Value *UpY, APInt LoMask,
                         APInt UpMask) -> Value * {
    Value *Y = nullptr;

    auto CLoY = dyn_cast_or_null<Constant>(LoY);
    auto CUpY = dyn_cast_or_null<Constant>(UpY);

    if ((CLoY == nullptr) ^ (CUpY == nullptr))
      return nullptr;

    if (CLoY && CUpY) {
      // TODO : Y|UpY 가 상수인 경우, Y|UpY 는 각 BitMask 범위 내에 존재해야
      // 한다.
      APInt IUpY = CUpY->getUniqueInteger();
      APInt ILoY = CLoY->getUniqueInteger();

      LLVM_DEBUGM("LO : "; KnownBits::makeConstant(ILoY).print(errs()));
      LLVM_DEBUGM("LO : "; KnownBits::makeConstant(LoMask).print(errs()));
      LLVM_DEBUGM("UP : "; KnownBits::makeConstant(IUpY).print(errs()));
      LLVM_DEBUGM("UP : "; KnownBits::makeConstant(UpMask).print(errs()));
      if (!(IUpY.isSubsetOf(UpMask) && ILoY.isSubsetOf(LoMask)))
        return nullptr;
      // TODO : X|Y 가 상수인 경우에는, X|Y를 누적시켜가며 더해줘야 한다.
      Y = ConstantInt::get(CLoY->getType(), ILoY + IUpY);
    } else if (LoY == UpY) {
      Y = LoY;
    }

    return Y;
  };

  auto MatchBitFieldAdd = [&](Value *Op0,
                              Value *Op1) -> std::optional<BitFieldAddInfo> {
    const APInt *OptLoMask, *OptHiMask, *LoMask, *HiMask, *HiMask2 = nullptr;
    Value *X, *Y, *UpY;

    unsigned Highest;
    unsigned BitWidth = I.getType()->getScalarSizeInBits();

    auto BitFieldAddLower =
        m_And(m_c_Add(m_Deferred(X), m_Value(Y)), m_APInt(LoMask));
    auto BitFieldAddUpper = m_CombineOr(
        m_c_Add(m_And(m_Value(X), m_APInt(HiMask)), m_Value(UpY)),
        m_And(m_c_Add(m_And(m_Value(X), m_APInt(HiMask)), m_Value(UpY)),
              m_APInt(HiMask2)));
    auto OptBitFieldAdd =
        m_c_Xor(m_c_Add(m_c_And(m_Value(X), m_APInt(OptLoMask)), m_Value(Y)),
                m_c_And(m_Deferred(X), m_APInt(OptHiMask)));

    if ((match(Op0, BitFieldAddUpper) && match(Op1, BitFieldAddLower)) ||
        (match(Op1, BitFieldAddUpper) && match(Op0, BitFieldAddLower))) {
      LLVM_DEBUGM("match BitField : " << *LoMask << " " << *HiMask);

      if ((Y = AccumulateY(Y, UpY, *LoMask, *HiMask)) == nullptr)
        return std::nullopt;

      // 00000111 ; 1st 
      // 00011000 ; 2nd
      APInt Mask(BitWidth, 0);
      Highest = HiMask->countl_zero(); 
      Mask.setBits(BitWidth - Highest, BitWidth);
      // 1. 1st 2nd bitmask 는 같은 비트가 있어서는 안된다.
      // 2. 1st 2nd bitmask 의 비트는 연쇄적이어야 한다. 
      // 3. 각 bitmask는 2자리 이상이어야 한다.

      LLVM_DEBUGM("popcount : " << LoMask->popcount() << " " << HiMask->popcount());
      LLVM_DEBUGM("sm : " << LoMask->isShiftedMask() << " " << HiMask->isShiftedMask());
      LLVM_DEBUGM("bm : " ;KnownBits::makeConstant(Mask).print(errs()));
      LLVM_DEBUGM("bm : " ;KnownBits::makeConstant(*LoMask).print(errs()));
      LLVM_DEBUGM("bm : " ;KnownBits::makeConstant(*HiMask).print(errs()));
      LLVM_DEBUGM("bm : " ;KnownBits::makeConstant(Mask ^ *LoMask ^ *HiMask).print(errs()));

      if (!((HiMask2 == nullptr || *HiMask == *HiMask2) &&
            (LoMask->popcount() >= 2 && HiMask->popcount() >= 2) &&
            (LoMask->isShiftedMask() && HiMask->isShiftedMask()) &&
            ((*LoMask & *HiMask) == 0) &&
            ((Mask ^ *LoMask ^ *HiMask).isAllOnes())))
        return std::nullopt;

      return {{X, Y, false, {{LoMask, HiMask}}}};
    }

    if ((match(Op0, OptBitFieldAdd) && match(Op1, BitFieldAddUpper)) ||
        (match(Op1, OptBitFieldAdd) && match(Op0, BitFieldAddUpper))) {
      LLVM_DEBUGM("match BitField opt: " << *OptLoMask << " " << *OptHiMask);

      if ((Y = AccumulateY(Y, UpY, (*OptLoMask + *OptHiMask), *HiMask)) ==
          nullptr)
        return std::nullopt;

      // check bits are fit our purpose
      // ; 00001011 ; 11
      // ; 00010100 ; 20
      // ; 01101011 ; 107
      // ; 10010100 ; 108
      // opt1 opt2 검사
      // 1. opt1 를 l 부터 (최상위 1) +1 까지 비트가 1인지 체크
      // 2. 1에서 0을 만날때 같은 위치에 opt2의 비트가 1인지 체크
      // -> 1에 not하고 2의 1이 나올때까지 비트를 제거하고 같은지 비교.
      APInt Mask(BitWidth, 0), Mask2(BitWidth, 0);
      LLVM_DEBUGM("test : " << OptLoMask->countl_zero());
      LLVM_DEBUGM("Opt1 : "; KnownBits::makeConstant(*OptLoMask).print(errs()));
      LLVM_DEBUGM("Opt2 : "; KnownBits::makeConstant(*OptHiMask).print(errs()));
      LLVM_DEBUGM("Send : "; KnownBits::makeConstant(*HiMask).print(errs()));

      Highest = OptHiMask->countl_zero(); 
      Mask.setBits(BitWidth - Highest, BitWidth);
      LLVM_DEBUGM("Highest : " << Highest << " ";KnownBits::makeConstant(Mask).print(errs()));

      APInt NotOpt1 = ~*OptLoMask;
      LLVM_DEBUGM("test : " ;KnownBits::makeConstant(NotOpt1).print(errs()));
      LLVM_DEBUGM("test : " ;KnownBits::makeConstant(NotOpt1 ^ Mask).print(errs()));
      LLVM_DEBUGM(" std::nullopt here : " << (NotOpt1 ^ Mask) << " "
                                          << *OptHiMask);
      LLVM_DEBUGM("isMask1 : " << HiMask->isShiftedMask() << " " << HiMask->isMask());
      LLVM_DEBUGM("isMask2 : " << OptLoMask->isShiftedMask() << " "
                               << OptLoMask->isMask());
      LLVM_DEBUGM("isMask3 : " << OptHiMask->isShiftedMask() << " "
                               << OptHiMask->isMask());

      Highest = HiMask->countl_zero(); 
      LLVM_DEBUGM("Highest : " << Highest);
      Mask2.setBits(BitWidth - Highest, BitWidth);
      LLVM_DEBUGM(" bit : " ; KnownBits::makeConstant(Mask2).print(errs()));

      // 1. HiMask 는 2개 이상의 비트가 연속되어 세팅되야 한다. 
      // 2. OptLoMask과 OptHiMask 에 공통 비트는 없어야 한다.
      // 3. HiMask와 OptLoMask, OptHiMask 에 공통 비트는 없어야 한다.
      // 4. OptLoMask의 not 은 OptHiMask와 같아야 한다. 단, OptHiMask의 countl_zero() 제외.
      // 5. OptLoMask과 OptHiMask 의 XOR은 연속된 비트의 세팅이어야 한다.
      if (!((HiMask2 != nullptr && *HiMask == *HiMask2) &&
            (HiMask->isShiftedMask() && HiMask->popcount() >= 2) &&
            ((*HiMask & (*OptLoMask | *OptHiMask)) == 0) &&
            ((~*OptLoMask ^ Mask) == *OptHiMask) &&
            (Mask2 ^ *HiMask ^ (*OptLoMask ^ *OptHiMask)).isAllOnes()))
        return std::nullopt;

      struct BitFieldAddInfo Info = {X, Y, true, {{OptLoMask, OptHiMask}}};
      Info.OptMask.New = HiMask;
      return {Info};
    }

    return std::nullopt;
  };

  auto Info = MatchBitFieldAdd(I.getOperand(0), I.getOperand(1));

  if (Info) {
    Value *X = Info->X;
    Value *Y = Info->Y;
    APInt BitLoMask, BitHiMask;
    unsigned BitWidth = Info->AddMask.Lower->getBitWidth();

    if (Info->opt) {
      LLVM_DEBUGM("OPT : " << *Info->OptMask.Lower << " " << *Info->OptMask.Upper << " " << *Info->OptMask.New);
      unsigned NewHiBit = BitWidth - (Info->OptMask.New->countl_zero() + 1);
      BitLoMask = *Info->OptMask.Lower | *Info->OptMask.New;
      BitLoMask.clearBit(NewHiBit);
      BitHiMask = *Info->OptMask.Upper;
      BitHiMask.setBit(NewHiBit);
    } else {
      unsigned LowerHiBit =
          BitWidth - (Info->AddMask.Lower->countl_zero() + 1);
      unsigned UpperHiBit =
          BitWidth - (Info->AddMask.Upper->countl_zero() + 1);

      BitLoMask = *Info->AddMask.Lower | *Info->AddMask.Upper;
      BitLoMask.clearBit(LowerHiBit);
      BitLoMask.clearBit(UpperHiBit);
      BitHiMask = APInt::getOneBitSet(BitWidth, LowerHiBit);
      BitHiMask.setBit(UpperHiBit);
    }

    auto AndLower = Builder.CreateAnd(X, BitLoMask);
    auto Add = Builder.CreateNUWAdd(AndLower, Y);
    auto AndUpper = Builder.CreateAnd(X, BitHiMask);
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
