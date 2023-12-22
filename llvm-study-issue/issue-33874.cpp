
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

  auto *F = M->getFunction("AddCounters");
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

      struct BitFieldAddInfo {
        Value *X;
        Value *Y;
        const APInt *bitmask;
        const APInt *bitmask2;
      };

      auto matchFieldAdd = [&](Value *Op) -> std::optional<BitFieldAddInfo> {
        const APInt *bitmaskAnd, *bitmaskAdd = nullptr;
        Value *X, *Y;

        auto bitFieldAdd = m_CombineOr(
            m_CombineOr(
                m_And(m_Add(m_Value(X), m_Value(Y)), m_APInt(bitmaskAnd)),
                m_Add(m_And(m_Value(X), m_APInt(bitmaskAnd)), m_Value(Y))),
            m_And(m_Add(m_And(m_Value(X), m_APInt(bitmaskAnd)), m_Value(Y)),
                  m_APInt(bitmaskAdd)));

        auto optBitFieldAdd =
            m_c_Xor(m_And(m_Value(X), m_APInt(bitmaskAdd)),
                    m_Add(m_And(m_Value(X), m_APInt(bitmaskAnd)), m_Value(Y)));

        LLVM_DEBUGM("Match Trying  : "; Op->dump());
        if (match(Op, bitFieldAdd)) {
          LLVM_DEBUGM("bitmaskAnd : " << *bitmaskAnd);
          if (bitmaskAdd != nullptr && bitmaskAdd != bitmaskAnd) {
            LLVM_DEBUGM("bitmaskAdd : " << *bitmaskAdd);
            return std::nullopt;
          }

          KnownBits OpKB = computeKnownBits(Op, DL, 0, &AC, &I, &DT);
          KnownBits BitmaskKB = KnownBits::makeConstant(*bitmaskAnd);
          LLVM_DEBUGM("OpKB == (OpKB & BitmaskKB)): "; OpKB.print(errs());
                      BitmaskKB.print(errs()));

          if (OpKB != (OpKB & BitmaskKB))
            return std::nullopt;

          return {{X, Y, bitmaskAnd, nullptr}};
        }
        if (match(Op, optBitFieldAdd)) {
          LLVM_DEBUGM("bitmaskAnd : " << *bitmaskAnd << " " << *bitmaskAdd);
          return {{X, Y, bitmaskAnd, bitmaskAdd}};
        }

        return std::nullopt;
      };

        
      if (auto *Op = dyn_cast<PossiblyDisjointInst>(&I)) {
        if (Op->isDisjoint()) {
          LLVM_DEBUGM("disjoint instruction"; I.dump(););

          auto LHS = matchFieldAdd(Op0);
          auto RHS = matchFieldAdd(Op1);

          if (!LHS || !RHS)
            continue;

          LLVM_DEBUGM("bitfieldadd true");
          IRBuilder builder(&I);
          if (LHS->bitmask2 || RHS->bitmask2) {
            LLVM_DEBUGM("opt pass");
            if (RHS->bitmask2)
              std::swap(LHS, RHS);

            Value *X = LHS->X;
            Value *Y = LHS->Y;
            unsigned Highest = RHS->bitmask->countl_zero();
            unsigned bitWidth = RHS->bitmask->getBitWidth();
            // top bit excluded
            APInt bitmask = *RHS->bitmask;
            bitmask.clearBit(bitWidth -1 - Highest);
            // top bit only 
            APInt bitmask2(bitWidth, 0);
            bitmask2.setBit(bitWidth -1 - Highest);

            APInt bitmaskBeforeAdd = *LHS->bitmask | bitmask;
            APInt bitmaskAfterAdd = *LHS->bitmask2 | bitmask2;

            LLVM_DEBUGM("0 : " << bitmask << " 1: " << bitmask2);
            LLVM_DEBUGM("0 : " << bitmaskBeforeAdd << " 1: " << bitmaskAfterAdd);

            auto and1 = builder.CreateAnd(X, bitmaskBeforeAdd, "and");
            auto add = builder.CreateAdd(and1, Y, "add", true);
            auto and2 = builder.CreateAnd(X, bitmaskAfterAdd, "and2");
            auto xor1 = builder.CreateXor(add, and2);

          } else {
            Value *X = LHS->X;
            Value *Y = LHS->Y;
            unsigned LHS_Highest = LHS->bitmask->countl_zero();
            unsigned RHS_Highest = RHS->bitmask->countl_zero();
            unsigned bitWidth = LHS->bitmask->getBitWidth();
            LLVM_DEBUGM("LHS RHS Highest : " << LHS_Highest << " "
                                             << RHS_Highest);

            APInt andMask = *LHS->bitmask | *RHS->bitmask;
            LLVM_DEBUGM("andmask : " << andMask << " " << bitWidth);
            andMask.clearBit(bitWidth - 1 - LHS_Highest);
            andMask.clearBit(bitWidth - 1 - RHS_Highest);
            LLVM_DEBUGM("andmask : " << andMask);

            APInt andMask2(bitWidth, 0);
            andMask2.setBit(bitWidth - 1 - LHS_Highest);
            andMask2.setBit(bitWidth - 1 - RHS_Highest);
            LLVM_DEBUGM("andMask2: " << andMask2);

            auto and1 = builder.CreateAnd(X, andMask, "and");
            auto add = builder.CreateAdd(and1, Y, "add", true);
            auto and2 = builder.CreateAnd(X, andMask2, "and2");
            auto xor1 = builder.CreateXor(add, and2);

            I.replaceAllUsesWith(xor1);
          }
          F->dump();
        }
      }
    }
  }
}
