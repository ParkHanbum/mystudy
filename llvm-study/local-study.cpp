#include "llvm/ADT/None.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AssumeBundleQueries.h"
#include "llvm/Analysis/AssumptionCache.h"
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
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/ScalarEvolutionExpander.h"
#include "llvm/Transforms/Utils/SimplifyIndVar.h"
#include "gtest/gtest.h"

#include <fstream>
#include <iostream>

using namespace llvm;

#define DEBUG_TYPE "local-study"

#define LLVM_DEBUG(X)                                                              \
  do {                                                                             \
    dbgs() << "[" << DEBUG_TYPE << "]" << __FUNCTION__ << ":" << __LINE__ << "\t"; \
    X;                                                                             \
  } while (false)

enum AllocType : uint8_t {
  OpNewLike = 1 << 0,        // allocates; never returns null
  MallocLike = 1 << 1,       // allocates; may return null
  AlignedAllocLike = 1 << 2, // allocates with alignment; may return null
  CallocLike = 1 << 3,       // allocates + bzero
  ReallocLike = 1 << 4,      // reallocates
  StrDupLike = 1 << 5,
  MallocOrOpNewLike = MallocLike | OpNewLike,
  MallocOrCallocLike = MallocLike | OpNewLike | CallocLike | AlignedAllocLike,
  AllocLike = MallocOrCallocLike | StrDupLike,
  AnyAlloc = AllocLike | ReallocLike
};

enum class MallocFamily {
  Malloc,
  CPPNew,             // new(unsigned int)
  CPPNewAligned,      // new(unsigned int, align_val_t)
  CPPNewArray,        // new[](unsigned int)
  CPPNewArrayAligned, // new[](unsigned long, align_val_t)
  MSVCNew,            // new(unsigned int)
  MSVCArrayNew,       // new[](unsigned int)
  VecMalloc,
  KmpcAllocShared,
};

struct AllocFnsTy {
  AllocType AllocTy;
  unsigned NumParams;
  // First and Second size parameters (or -1 if unused)
  int FstParam, SndParam;
  // Alignment parameter for aligned_alloc and aligned new
  int AlignParam;
  // Name of default allocator function to group malloc/free calls by family
  MallocFamily Family;
};

// clang-format off
// FIXME: certain users need more information. E.g., SimplifyLibCalls needs to
// know which functions are nounwind, noalias, nocapture parameters, etc.
static const std::pair<LibFunc, AllocFnsTy> AllocationFnData[] = {
    {LibFunc_vec_malloc,                        {MallocLike,       1,  0, -1, -1, MallocFamily::VecMalloc}},
    {LibFunc_Znwj,                              {OpNewLike,        1,  0, -1, -1, MallocFamily::CPPNew}},             // new(unsigned int)
    {LibFunc_ZnwjRKSt9nothrow_t,                {MallocLike,       2,  0, -1, -1, MallocFamily::CPPNew}},             // new(unsigned int, nothrow)
    {LibFunc_ZnwjSt11align_val_t,               {OpNewLike,        2,  0, -1,  1, MallocFamily::CPPNewAligned}},      // new(unsigned int, align_val_t)
    {LibFunc_ZnwjSt11align_val_tRKSt9nothrow_t, {MallocLike,       3,  0, -1,  1, MallocFamily::CPPNewAligned}},      // new(unsigned int, align_val_t, nothrow)
    {LibFunc_Znwm,                              {OpNewLike,        1,  0, -1, -1, MallocFamily::CPPNew}},             // new(unsigned long)
    {LibFunc_ZnwmRKSt9nothrow_t,                {MallocLike,       2,  0, -1, -1, MallocFamily::CPPNew}},             // new(unsigned long, nothrow)
    {LibFunc_ZnwmSt11align_val_t,               {OpNewLike,        2,  0, -1,  1, MallocFamily::CPPNewAligned}},      // new(unsigned long, align_val_t)
    {LibFunc_ZnwmSt11align_val_tRKSt9nothrow_t, {MallocLike,       3,  0, -1,  1, MallocFamily::CPPNewAligned}},      // new(unsigned long, align_val_t, nothrow)
    {LibFunc_Znaj,                              {OpNewLike,        1,  0, -1, -1, MallocFamily::CPPNewArray}},        // new[](unsigned int)
    {LibFunc_ZnajRKSt9nothrow_t,                {MallocLike,       2,  0, -1, -1, MallocFamily::CPPNewArray}},        // new[](unsigned int, nothrow)
    {LibFunc_ZnajSt11align_val_t,               {OpNewLike,        2,  0, -1,  1, MallocFamily::CPPNewArrayAligned}}, // new[](unsigned int, align_val_t)
    {LibFunc_ZnajSt11align_val_tRKSt9nothrow_t, {MallocLike,       3,  0, -1,  1, MallocFamily::CPPNewArrayAligned}}, // new[](unsigned int, align_val_t, nothrow)
    {LibFunc_Znam,                              {OpNewLike,        1,  0, -1, -1, MallocFamily::CPPNewArray}},        // new[](unsigned long)
    {LibFunc_ZnamRKSt9nothrow_t,                {MallocLike,       2,  0, -1, -1, MallocFamily::CPPNewArray}},        // new[](unsigned long, nothrow)
    {LibFunc_ZnamSt11align_val_t,               {OpNewLike,        2,  0, -1,  1, MallocFamily::CPPNewArrayAligned}}, // new[](unsigned long, align_val_t)
    {LibFunc_ZnamSt11align_val_tRKSt9nothrow_t, {MallocLike,       3,  0, -1,  1, MallocFamily::CPPNewArrayAligned}}, // new[](unsigned long, align_val_t, nothrow)
    {LibFunc_msvc_new_int,                      {OpNewLike,        1,  0, -1, -1, MallocFamily::MSVCNew}},            // new(unsigned int)
    {LibFunc_msvc_new_int_nothrow,              {MallocLike,       2,  0, -1, -1, MallocFamily::MSVCNew}},            // new(unsigned int, nothrow)
    {LibFunc_msvc_new_longlong,                 {OpNewLike,        1,  0, -1, -1, MallocFamily::MSVCNew}},            // new(unsigned long long)
    {LibFunc_msvc_new_longlong_nothrow,         {MallocLike,       2,  0, -1, -1, MallocFamily::MSVCNew}},            // new(unsigned long long, nothrow)
    {LibFunc_msvc_new_array_int,                {OpNewLike,        1,  0, -1, -1, MallocFamily::MSVCArrayNew}},       // new[](unsigned int)
    {LibFunc_msvc_new_array_int_nothrow,        {MallocLike,       2,  0, -1, -1, MallocFamily::MSVCArrayNew}},       // new[](unsigned int, nothrow)
    {LibFunc_msvc_new_array_longlong,           {OpNewLike,        1,  0, -1, -1, MallocFamily::MSVCArrayNew}},       // new[](unsigned long long)
    {LibFunc_msvc_new_array_longlong_nothrow,   {MallocLike,       2,  0, -1, -1, MallocFamily::MSVCArrayNew}},       // new[](unsigned long long, nothrow)
    {LibFunc_memalign,                          {AlignedAllocLike, 2,  1, -1,  0, MallocFamily::Malloc}},
    {LibFunc_vec_calloc,                        {CallocLike,       2,  0,  1, -1, MallocFamily::VecMalloc}},
    {LibFunc_vec_realloc,                       {ReallocLike,      2,  1, -1, -1, MallocFamily::VecMalloc}},
    {LibFunc_strdup,                            {StrDupLike,       1, -1, -1, -1, MallocFamily::Malloc}},
    {LibFunc_dunder_strdup,                     {StrDupLike,       1, -1, -1, -1, MallocFamily::Malloc}},
    {LibFunc_strndup,                           {StrDupLike,       2,  1, -1, -1, MallocFamily::Malloc}},
    {LibFunc_dunder_strndup,                    {StrDupLike,       2,  1, -1, -1, MallocFamily::Malloc}},
    {LibFunc___kmpc_alloc_shared,               {MallocLike,       1,  0, -1, -1, MallocFamily::KmpcAllocShared}},
};
// clang-format on

struct FreeFnsTy {
  unsigned NumParams;
  // Name of default allocator function to group malloc/free calls by family
  MallocFamily Family;
};

// clang-format off
static const std::pair<LibFunc, FreeFnsTy> FreeFnData[] = {
    {LibFunc_vec_free,                           {1, MallocFamily::VecMalloc}},
    {LibFunc_ZdlPv,                              {1, MallocFamily::CPPNew}},             // operator delete(void*)
    {LibFunc_ZdaPv,                              {1, MallocFamily::CPPNewArray}},        // operator delete[](void*)
    {LibFunc_msvc_delete_ptr32,                  {1, MallocFamily::MSVCNew}},            // operator delete(void*)
    {LibFunc_msvc_delete_ptr64,                  {1, MallocFamily::MSVCNew}},            // operator delete(void*)
    {LibFunc_msvc_delete_array_ptr32,            {1, MallocFamily::MSVCArrayNew}},       // operator delete[](void*)
    {LibFunc_msvc_delete_array_ptr64,            {1, MallocFamily::MSVCArrayNew}},       // operator delete[](void*)
    {LibFunc_ZdlPvj,                             {2, MallocFamily::CPPNew}},             // delete(void*, uint)
    {LibFunc_ZdlPvm,                             {2, MallocFamily::CPPNew}},             // delete(void*, ulong)
    {LibFunc_ZdlPvRKSt9nothrow_t,                {2, MallocFamily::CPPNew}},             // delete(void*, nothrow)
    {LibFunc_ZdlPvSt11align_val_t,               {2, MallocFamily::CPPNewAligned}},      // delete(void*, align_val_t)
    {LibFunc_ZdaPvj,                             {2, MallocFamily::CPPNewArray}},        // delete[](void*, uint)
    {LibFunc_ZdaPvm,                             {2, MallocFamily::CPPNewArray}},        // delete[](void*, ulong)
    {LibFunc_ZdaPvRKSt9nothrow_t,                {2, MallocFamily::CPPNewArray}},        // delete[](void*, nothrow)
    {LibFunc_ZdaPvSt11align_val_t,               {2, MallocFamily::CPPNewArrayAligned}}, // delete[](void*, align_val_t)
    {LibFunc_msvc_delete_ptr32_int,              {2, MallocFamily::MSVCNew}},            // delete(void*, uint)
    {LibFunc_msvc_delete_ptr64_longlong,         {2, MallocFamily::MSVCNew}},            // delete(void*, ulonglong)
    {LibFunc_msvc_delete_ptr32_nothrow,          {2, MallocFamily::MSVCNew}},            // delete(void*, nothrow)
    {LibFunc_msvc_delete_ptr64_nothrow,          {2, MallocFamily::MSVCNew}},            // delete(void*, nothrow)
    {LibFunc_msvc_delete_array_ptr32_int,        {2, MallocFamily::MSVCArrayNew}},       // delete[](void*, uint)
    {LibFunc_msvc_delete_array_ptr64_longlong,   {2, MallocFamily::MSVCArrayNew}},       // delete[](void*, ulonglong)
    {LibFunc_msvc_delete_array_ptr32_nothrow,    {2, MallocFamily::MSVCArrayNew}},       // delete[](void*, nothrow)
    {LibFunc_msvc_delete_array_ptr64_nothrow,    {2, MallocFamily::MSVCArrayNew}},       // delete[](void*, nothrow)
    {LibFunc___kmpc_free_shared,                 {2, MallocFamily::KmpcAllocShared}},    // OpenMP Offloading RTL free
    {LibFunc_ZdlPvSt11align_val_tRKSt9nothrow_t, {3, MallocFamily::CPPNewAligned}},      // delete(void*, align_val_t, nothrow)
    {LibFunc_ZdaPvSt11align_val_tRKSt9nothrow_t, {3, MallocFamily::CPPNewArrayAligned}}, // delete[](void*, align_val_t, nothrow)
    {LibFunc_ZdlPvjSt11align_val_t,              {3, MallocFamily::CPPNewAligned}},      // delete(void*, unsigned int, align_val_t)
    {LibFunc_ZdlPvmSt11align_val_t,              {3, MallocFamily::CPPNewAligned}},      // delete(void*, unsigned long, align_val_t)
    {LibFunc_ZdaPvjSt11align_val_t,              {3, MallocFamily::CPPNewArrayAligned}}, // delete[](void*, unsigned int, align_val_t)
    {LibFunc_ZdaPvmSt11align_val_t,              {3, MallocFamily::CPPNewArrayAligned}}, // delete[](void*, unsigned long, align_val_t)
};
// clang-format on

StringRef mangledNameForMallocFamily(const MallocFamily &Family) {
  switch (Family) {
  case MallocFamily::Malloc:
    return "malloc";
  case MallocFamily::CPPNew:
    return "_Znwm";
  case MallocFamily::CPPNewAligned:
    return "_ZnwmSt11align_val_t";
  case MallocFamily::CPPNewArray:
    return "_Znam";
  case MallocFamily::CPPNewArrayAligned:
    return "_ZnamSt11align_val_t";
  case MallocFamily::MSVCNew:
    return "??2@YAPAXI@Z";
  case MallocFamily::MSVCArrayNew:
    return "??_U@YAPAXI@Z";
  case MallocFamily::VecMalloc:
    return "vec_malloc";
  case MallocFamily::KmpcAllocShared:
    return "__kmpc_alloc_shared";
  }
  llvm_unreachable("missing an alloc family");
}

static const Function *getCalledFunction(const Value *V,
                                         bool &IsNoBuiltin) {
  // Don't care about intrinsics in this case.
  if (isa<IntrinsicInst>(V))
    return nullptr;

  const auto *CB = dyn_cast<CallBase>(V);
  if (!CB)
    return nullptr;

  IsNoBuiltin = CB->isNoBuiltin();

  if (const Function *Callee = CB->getCalledFunction())
    return Callee;
  return nullptr;
}

/// Returns the allocation data for the given value if it's a call to a known
/// allocation function.
static Optional<AllocFnsTy>
getAllocationDataForFunction(const Function *Callee, AllocType AllocTy,
                             const TargetLibraryInfo *TLI) {
  // Don't perform a slow TLI lookup, if this function doesn't return a pointer
  // and thus can't be an allocation function.
  if (!Callee->getReturnType()->isPointerTy())
    return None;

  // Make sure that the function is available.
  LibFunc TLIFn;
  if (!TLI || !TLI->getLibFunc(*Callee, TLIFn) || !TLI->has(TLIFn))
    return None;

  const auto *Iter = find_if(
      AllocationFnData, [TLIFn](const std::pair<LibFunc, AllocFnsTy> &P) {
        return P.first == TLIFn;
      });

  if (Iter == std::end(AllocationFnData))
    return None;

  const AllocFnsTy *FnData = &Iter->second;
  if ((FnData->AllocTy & AllocTy) != FnData->AllocTy)
    return None;

  // Check function prototype.
  int FstParam = FnData->FstParam;
  int SndParam = FnData->SndParam;
  FunctionType *FTy = Callee->getFunctionType();

  if (FTy->getReturnType() == Type::getInt8PtrTy(FTy->getContext()) &&
      FTy->getNumParams() == FnData->NumParams &&
      (FstParam < 0 ||
       (FTy->getParamType(FstParam)->isIntegerTy(32) ||
        FTy->getParamType(FstParam)->isIntegerTy(64))) &&
      (SndParam < 0 ||
       FTy->getParamType(SndParam)->isIntegerTy(32) ||
       FTy->getParamType(SndParam)->isIntegerTy(64)))
    return *FnData;
  return None;
}

static Optional<AllocFnsTy>
getAllocationData(const Value *V, AllocType AllocTy,
                  function_ref<const TargetLibraryInfo &(Function &)> GetTLI) {
  bool IsNoBuiltinCall;
  if (const Function *Callee = getCalledFunction(V, IsNoBuiltinCall))
    if (!IsNoBuiltinCall)
      return getAllocationDataForFunction(
          Callee, AllocTy, &GetTLI(const_cast<Function &>(*Callee)));
  return None;
}

static Optional<AllocFnsTy> getAllocationData(const Value *V, AllocType AllocTy,
                                              const TargetLibraryInfo *TLI) {
  bool IsNoBuiltinCall;
  if (const Function *Callee = getCalledFunction(V, IsNoBuiltinCall))
    if (!IsNoBuiltinCall)
      return getAllocationDataForFunction(Callee, AllocTy, TLI);
  return None;
}

static AllocFnKind getAllocFnKind(const Value *V) {
  if (const auto *CB = dyn_cast<CallBase>(V)) {
    Attribute Attr = CB->getFnAttr(Attribute::AllocKind);
    if (Attr.isValid())
      return AllocFnKind(Attr.getValueAsInt());
  }
  return AllocFnKind::Unknown;
}

static AllocFnKind getAllocFnKind(const Function *F) {
  Attribute Attr = F->getFnAttribute(Attribute::AllocKind);
  if (Attr.isValid())
    return AllocFnKind(Attr.getValueAsInt());
  return AllocFnKind::Unknown;
}

static bool checkFnAllocKind(const Value *V, AllocFnKind Wanted) {
  return (getAllocFnKind(V) & Wanted) != AllocFnKind::Unknown;
}

/// Tests if a value is a call or invoke to a library function that
/// allocates memory (either malloc, calloc, or strdup like).
bool isAllocLikeFn(const Value *V, const TargetLibraryInfo *TLI) {
  return getAllocationData(V, AllocLike, TLI).has_value() ||
         checkFnAllocKind(V, AllocFnKind::Alloc);
}

bool isRemovableAlloc(const CallBase *CB, const TargetLibraryInfo *TLI) {
  // Note: Removability is highly dependent on the source language.  For
  // example, recent C++ requires direct calls to the global allocation
  // [basic.stc.dynamic.allocation] to be observable unless part of a new
  // expression [expr.new paragraph 13].

  // Historically we've treated the C family allocation routines and operator
  // new as removable
  return isAllocLikeFn(CB, TLI);
}

Optional<FreeFnsTy> getFreeFunctionDataForFunction(const Function *Callee,
                                                   const LibFunc TLIFn) {
  const auto *Iter =
      find_if(FreeFnData, [TLIFn](const std::pair<LibFunc, FreeFnsTy> &P) {
        return P.first == TLIFn;
      });
  if (Iter == std::end(FreeFnData))
    return None;
  return Iter->second;
}

// isLibFreeFunction - Returns true if the function is a builtin free()
bool isLibFreeFunction(const Function *F, const LibFunc TLIFn) {
  Optional<FreeFnsTy> FnData = getFreeFunctionDataForFunction(F, TLIFn);
  if (!FnData)
    return checkFnAllocKind(F, AllocFnKind::Free);

  // Check free prototype.
  // FIXME: workaround for PR5130, this will be obsolete when a nobuiltin
  // attribute will exist.
  FunctionType *FTy = F->getFunctionType();
  if (!FTy->getReturnType()->isVoidTy())
    return false;
  if (FTy->getNumParams() != FnData->NumParams)
    return false;
  if (FTy->getParamType(0) != Type::getInt8PtrTy(F->getContext()))
    return false;

  return true;
}

Value *getFreedOperand(const CallBase *CB, const TargetLibraryInfo *TLI) {
  bool IsNoBuiltinCall;
  const Function *Callee = getCalledFunction(CB, IsNoBuiltinCall);
  if (Callee == nullptr || IsNoBuiltinCall)
    return nullptr;

  LibFunc TLIFn;
  if (TLI && TLI->getLibFunc(*Callee, TLIFn) && TLI->has(TLIFn) &&
      isLibFreeFunction(Callee, TLIFn)) {
    // All currently supported free functions free the first argument.
    return CB->getArgOperand(0);
  }

  if (checkFnAllocKind(CB, AllocFnKind::Free))
    return CB->getArgOperandWithAttribute(Attribute::AllocatedPointer);

  return nullptr;
}

static bool _wouldInstructionBeTriviallyDead(Instruction *I,
                                             const TargetLibraryInfo *TLI) {
  LLVM_DEBUG(dbgs() << "_wouldInstructionBeTriviallyDead : "; I->dump());
  if (I->isTerminator()) {
    LLVM_DEBUG(dbgs() << "I->isTerminator() : "; I->dump());
    return false;
  }

  // We don't want the landingpad-like instructions removed by anything this
  // general.
  if (I->isEHPad()) {
    LLVM_DEBUG(dbgs() << "I->isEHPad() : "; I->dump());
    return false;
  }

  // We don't want debug info removed by anything this general, unless
  // debug info is empty.
  if (DbgDeclareInst *DDI = dyn_cast<DbgDeclareInst>(I)) {
    if (DDI->getAddress())
      return false;
    return true;
  }
  if (DbgValueInst *DVI = dyn_cast<DbgValueInst>(I)) {
    if (DVI->hasArgList() || DVI->getValue(0))
      return false;
    return true;
  }
  if (DbgLabelInst *DLI = dyn_cast<DbgLabelInst>(I)) {
    if (DLI->getLabel())
      return false;
    return true;
  }

  if (auto *CB = dyn_cast<CallBase>(I))
    if (isRemovableAlloc(CB, TLI)) {
      LLVM_DEBUG(dbgs() << "CallBase : "; CB->dump());
      return true;
    }

  if (!I->willReturn()) {
    LLVM_DEBUG(dbgs() << "willReturn : "; I->dump());
    return false;
  }

  if (!I->mayHaveSideEffects()) {
    LLVM_DEBUG(dbgs() << "mayHaveSideEffects : "; I->dump());
    return true;
  }

  // Special case intrinsics that "may have side effects" but can be deleted
  // when dead.
  if (IntrinsicInst *II = dyn_cast<IntrinsicInst>(I)) {
    // Safe to delete llvm.stacksave and launder.invariant.group if dead.
    if (II->getIntrinsicID() == Intrinsic::stacksave ||
        II->getIntrinsicID() == Intrinsic::launder_invariant_group) {
      LLVM_DEBUG(dbgs() << "IntrinsicInst : "; II->dump());
      return true;
    }

    if (II->isLifetimeStartOrEnd()) {
      auto *Arg = II->getArgOperand(1);
      // Lifetime intrinsics are dead when their right-hand is undef.
      if (isa<UndefValue>(Arg)) {
        LLVM_DEBUG(dbgs() << "mayHaveSideEffects : "; II->dump());
        return true;
      }
      // If the right-hand is an alloc, global, or argument and the only uses
      // are lifetime intrinsics then the intrinsics are dead.
      if (isa<AllocaInst>(Arg) || isa<GlobalValue>(Arg) || isa<Argument>(Arg))
        return llvm::all_of(Arg->uses(), [](Use &Use) {
          IntrinsicInst *IntrinsicUse =
              dyn_cast<IntrinsicInst>(Use.getUser());
          LLVM_DEBUG(dbgs() << "IntrinsicUse : "; IntrinsicUse->dump());
          if (IntrinsicUse)
            return IntrinsicUse->isLifetimeStartOrEnd();
          return false;
        });
      return false;
    }

    // Assumptions are dead if their condition is trivially true.  Guards on
    // true are operationally no-ops.  In the future we can consider more
    // sophisticated tradeoffs for guards considering potential for check
    // widening, but for now we keep things simple.
    if ((II->getIntrinsicID() == Intrinsic::assume &&
         isAssumeWithEmptyBundle(cast<AssumeInst>(*II))) ||
        II->getIntrinsicID() == Intrinsic::experimental_guard) {
      ConstantInt *Cond = dyn_cast<ConstantInt>(II->getArgOperand(0));
      LLVM_DEBUG(dbgs() << "ConstantInt : "; Cond->dump());
      if (Cond)
        return !Cond->isZero();

      return false;
    }

    if (auto *FPI = dyn_cast<ConstrainedFPIntrinsic>(I)) {
      Optional<fp::ExceptionBehavior> ExBehavior = FPI->getExceptionBehavior();
      LLVM_DEBUG(dbgs() << "ExBehavior : " << ExBehavior);

      return *ExBehavior != fp::ebStrict;
    }
  }

  if (auto *Call = dyn_cast<CallBase>(I)) {
    LLVM_DEBUG(dbgs() << "CallBase : "; Call->dump());
    if (Value *FreedOp = getFreedOperand(Call, TLI)) {
      LLVM_DEBUG(dbgs() << "FreedOp : "; FreedOp->dump());
      if (Constant *C = dyn_cast<Constant>(FreedOp)) {
        LLVM_DEBUG(dbgs() << "Constant : "; C->dump());
        return C->isNullValue() || isa<UndefValue>(C);
      }
    }
    if (isMathLibCallNoop(Call, TLI))
      return true;
  }

  // Non-volatile atomic loads from constants can be removed.
  if (auto *LI = dyn_cast<LoadInst>(I)) {
    LLVM_DEBUG(dbgs() << "LoadInst : "; LI->dump());
    if (auto *GV = dyn_cast<GlobalVariable>(
            LI->getPointerOperand()->stripPointerCasts())) {
      LLVM_DEBUG(dbgs() << "GlobalVariable : "; GV->dump());
      if (!LI->isVolatile() && GV->isConstant()) {
        LLVM_DEBUG(dbgs() << "LI non volatile & GV is Constant ");
        return true;
      }
    }
  }

  LLVM_DEBUG(dbgs() << "not matcheda"; I->dump());
  return false;
}

bool _isInstructionTriviallyDead(Instruction *I,
                                const TargetLibraryInfo *TLI) {
  if (!I->use_empty()) {
    LLVM_DEBUG(dbgs() << "use_empty : ";I->dump());
    return false;
  }
  return _wouldInstructionBeTriviallyDead(I, TLI);
}

static std::unique_ptr<Module> makeLLVMModule(LLVMContext &context,
                                              const char *modulestr) {
  SMDiagnostic err;
  return parseAssemblyString(modulestr, err, context);
}

TEST(LOCAL, wouldInstructionBeTriviallyDead1) {
  std::stringstream IR;
  std::ifstream in("Loop/loop-simplify4.ll");
  IR << in.rdbuf();

  // Parse the module.
  LLVMContext Context;
  std::unique_ptr<Module> M = makeLLVMModule(Context, IR.str().c_str());

  auto *F = M->getFunction("test4");
  TargetLibraryInfoImpl TLII;
  TargetLibraryInfo TLI(TLII);
  AssumptionCache AC(*F);
  DominatorTree DT(*F);
  LoopInfo LI(DT);
  ScalarEvolution SE(*F, TLI, AC, DT, LI);
  AAResults AA(TLI);
  DataLayout DL(F->getParent());
  TargetTransformInfo TTI(DL);

  MemorySSA MSSA(*F, &AA, &DT);
  MemorySSAUpdater MSSAU(&MSSA);

  Function::iterator FI = F->begin();
  // First basic block is entry - skip it.
  BasicBlock *Header = &*(++FI);
  assert(Header->getName() == "loop");
  Loop *L = LI.getLoopFor(Header);
  LoopBlocksRPO RPOT(L);
  RPOT.perform(&LI);

  SimplifyQuery SQ(DL, &TLI, &DT, &AC);
  SmallPtrSet<const Instruction *, 8> S1, S2, *ToSimplify = &S1, *Next = &S2;
  SmallPtrSet<PHINode *, 4> VisitedPHIs;
  SmallVector<WeakTrackingVH, 8> DeadInsts;

  for (BasicBlock *BB : RPOT) {
    LLVM_DEBUG(dbgs() << "BB : "; BB->dump());
    for (Instruction &I : *BB) {
      LLVM_DEBUG(dbgs() << "inst : "; I.dump());
      if (_isInstructionTriviallyDead(&I, &TLI))
        LLVM_DEBUG(dbgs() << "Found Dead Instruction : "; I.dump());
      
      Value *V = simplifyInstruction(&I, SQ.getWithInstruction(&I));
      if (!V || !LI.replacementPreservesLCSSAForm(&I, V))
        continue;

      LLVM_DEBUG(dbgs() << "Simplified :"; V->dump());
      for (Use &U : llvm::make_early_inc_range(I.uses())) {
        auto *UserI = cast<Instruction>(U.getUser());
        U.set(V);

        if (!DT.isReachableFromEntry(UserI->getParent()))
          continue;
        if (auto *UserPI = dyn_cast<PHINode>(UserI))
          if (VisitedPHIs.count(UserPI)) {
            Next->insert(UserPI);
            continue;
          }
        assert((L->contains(UserI) || isa<PHINode>(UserI)) &&
               "Uses outside the loop should be PHI nodes due to LCSSA!");
        if (L->contains(UserI))
          ToSimplify->insert(UserI);
      }
      if (Instruction *SimpleI = dyn_cast_or_null<Instruction>(V))
        if (MemoryAccess *MA = MSSA.getMemoryAccess(&I))
          if (MemoryAccess *ReplacementMA = MSSA.getMemoryAccess(SimpleI))
            MA->replaceAllUsesWith(ReplacementMA);
      assert(I.use_empty() && "Should always have replaced all uses!");
      if (_isInstructionTriviallyDead(&I, &TLI)) {
        LLVM_DEBUG(dbgs() << "Found Dead Instruction : "; I.dump());
      }
    }
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}