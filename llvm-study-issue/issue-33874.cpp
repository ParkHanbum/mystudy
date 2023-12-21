
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

int main(int argc, char **argv) {
  std::stringstream IR;
  std::ifstream in("issue-33874.ll");
  IR << in.rdbuf();

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());

  auto *F = M->getFunction("src");
  TargetLibraryInfoImpl TLII;
  TargetLibraryInfo TLI(TLII);
  AssumptionCache AC(*F);
  DominatorTree DT(*F);
  LoopInfo LI(DT);
  ScalarEvolution SE(*F, TLI, AC, DT, LI);
  AAResults AA(TLI);
  DataLayout DL(F->getParent());

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

      if (auto *Op = dyn_cast<PossiblyDisjointInst>(&I)) {
        if (Op->isDisjoint()) {
          LLVM_DEBUGM("disjoint instruction"; I.dump(););

          struct BitFieldAddInfo {
            Value *X;
            const APInt *added;
            const APInt *bitmask;
          };

          auto matchFieldAdd = [&](Value *Op) -> std::optional<BitFieldAddInfo> {
            const APInt *bitmask, *added;
            Value *X;
            auto m_Matcher = m_CombineOr(
                m_And(m_Add(m_Value(X), m_APInt(added)), m_APInt(bitmask)),
                m_Add(m_And(m_Value(X), m_APInt(bitmask)), m_APInt(added)));
            if (match(Op, m_Matcher)) {
              LLVM_DEBUGM("match : " << *added << " " << *bitmask;);
              KnownBits OpKB = computeKnownBits(Op, DL, 0, &AC, &I, &DT);
              KnownBits BitmaskKB = KnownBits::makeConstant(*bitmask);
              if (added->isSubsetOf(*bitmask)) {
                LLVM_DEBUGM("added->isSubsetOf(*bitmask) : ";);
                if (OpKB == (OpKB & BitmaskKB)) {
                  LLVM_DEBUGM("OpKB == (OpKB & BitmaskKB)): ";
                              OpKB.print(errs()););
                  return {{X, added, bitmask}};
                }
              }
            }
            return std::nullopt;
          };

          auto LHS = matchFieldAdd(Op0);
          auto RHS = matchFieldAdd(Op1);

          if (!LHS || !RHS)
            return -1;

          LLVM_DEBUGM("bitfieldadd true");
          IRBuilder builder(&I);

          // countr_zero(LHS.bitmask)
          // and = X & (clearbit(countr_zero(LHS.bitmask) | clearbit(countr_zero(RHS.bitmask))
          // add = and + (LHS.added | RHS.added)
          // and2 = and ( setbit(clearAllBits(LHS.bitmask), )
          Value *X = LHS->X;
          unsigned LHS_Highest = LHS->bitmask->countl_zero();
          unsigned RHS_Highest = RHS->bitmask->countl_zero();
          unsigned bitWidth = LHS->bitmask->getBitWidth();
          LLVM_DEBUGM("LHS RHS Highest : " << LHS_Highest << " " << RHS_Highest);

          APInt andMask = *LHS->bitmask | *RHS->bitmask;
          LLVM_DEBUGM("andmask : " << andMask << " " << bitWidth);
          andMask.clearBit(bitWidth-1 - LHS_Highest);
          andMask.clearBit(bitWidth-1 - RHS_Highest);
          LLVM_DEBUGM("andmask : " << andMask);
          
          APInt added = *LHS->added | *RHS->added;
          LLVM_DEBUGM("added : " << added);

          APInt andMask2(bitWidth, 0);
          andMask2.setBit(bitWidth-1 - LHS_Highest);
          andMask2.setBit(bitWidth-1 - RHS_Highest);
          LLVM_DEBUGM("andMask2: " << andMask2);

          auto and1 = builder.CreateAnd(X, andMask, "and");
          auto add = builder.CreateAdd(and1, ConstantInt::get(and1->getType(), added), "add", true);
          auto and2 = builder.CreateAnd(X, andMask2, "and2");
          auto xor1 = builder.CreateXor(add, and2);
          auto ret = builder.CreateRet(xor1);

        }
      }
    }

    F->dump();
  }
}
