/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file ScalarizeFifoArrayPass.cpp
 * @brief Scalarize singleton hls::stream array wrappers (LLVM 14 typed-pointer edition)
 * ------------
 * Handles IR of the form produced by Vitis HLS where streams are wrapped in
 * one or more layers of singleton arrays, e.g.:
 *
 *   %25 = alloca [1 x [1 x %"class.hls::stream"]], align 1
 *   %33 = getelementptr [1 x [1 x %"class.hls::stream"]],
 *             [1 x [1 x %"class.hls::stream"]]* %29, i32 0, i32 0
 *   call void @_Z5node1(..., [1 x %"class.hls::stream"]* %33, ...)
 *
 * Three complementary transformations, run in a loop until fixpoint:
 *
 * (A) ALLOCA SCALARIZATION
 *     Finds every  alloca [1 x [1 x ... [1 x hls::stream]...]]  in every
 *     function entry block.  All array dimensions must be 1.  Replaces it
 *     with an alloca of the innermost element type (the stream struct).
 *     All-zero GEPs that indexed through the wrapper are removed and their
 *     uses replaced by the new alloca.
 *
 *     Before:
 *       %25 = alloca [1 x [1 x %"class.hls::stream"]], align 1
 *       %33 = getelementptr [1 x [1 x %"class.hls::stream"]],
 *                 [1 x [1 x %"class.hls::stream"]]* %25, i32 0, i32 0
 *       call void @node(..., [1 x %"class.hls::stream"]* %33, ...)
 *
 *     After (A):
 *       %25.scalar = alloca %"class.hls::stream", align 1
 *       call void @node(..., %"class.hls::stream"* %25.scalar, ...)
 *       ; NOTE: type mismatch at call site — fixed by (B)
 *
 * (B) CALLEE SIGNATURE REWRITING
 *     After (A), a call site may pass  stream*  where the declared callee
 *     parameter type is still  [1 x stream]*.  This is a verifier error in
 *     typed-pointer IR.
 *
 *     For every CallInst where at least one argument type differs from the
 *     corresponding declared parameter type due to scalarization, the pass
 *     clones the callee with updated parameter types (using CloneFunctionInto),
 *     rewrites the call site to target the clone, and erases the old function.
 *
 *     Functions with external linkage that are called from outside the module
 *     (i.e. have uses that are not CallInsts inside the module) are skipped
 *     with a warning — changing their signature would break the ABI.
 *
 *     Propagation is transitive: the rewritten clone may itself call further
 *     functions with stale signatures.  The outer fixpoint loop in exec()
 *     catches these.
 *
 * (C) PARAMETER GEP CLEANUP
 *     After (B) the clone's body still contains GEPs of the form:
 *
 *       %p = getelementptr [1 x stream], [1 x stream]* %param, i32 0, i32 0
 *
 *     which are now stale (the parameter is stream*, not [1 x stream]*).
 *     If ALL GEPs on a given parameter are all-zero singleton-fifo GEPs,
 *     they are replaced by the parameter itself and erased.
 *     Run to fixpoint for multi-level nesting.
 *
 * TYPED POINTER NOTE
 *     This version targets LLVM ≤ 15 (typed pointers).  It uses
 *     PointerType::getElementType() and GEP::getSourceElementType() which
 *     are unavailable in opaque-pointer builds (LLVM 17+).
 *
 * @author Tommaso Fellegara <tommaso.fellegara@polimi.it>
 *
 */

#ifndef NDEBUG
#define NDEBUG
#endif
// #undef NDEBUG
#include "ScalarizeArrayOfFifo.hpp"
#include "llvm/Config/llvm-config.h"
#include "panda_clang_compat.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h" // CloneFunctionInto, ValueToValueMapTy
#include <algorithm>
#include <cxxabi.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/LazyCallGraph.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/ErrorHandling.h>
#include <utility>
#include <vector>

#include "debug_print.hpp"

#define PREFIX "[SCALARIZE FIFOS] "

#if PANDA_LLVM_CLANG_MAJOR >= 17
#warning "ScalarizeFifoArrayPass (typed-pointer edition) on LLVM 17+: " \
           "PointerType::getElementType() is removed. " \
           "Use ScalarizeFifoArrayPass_opaque.cpp instead."
#endif

// If the function is a pointer
#if PANDA_LLVM_CLANG_MAJOR >= 10
#define GET_ARG_PT(fn, i) fn->getArg(i)
#else
#define GET_ARG_PT(fn, i) (&*std::next(fn->arg_begin(), i))
#endif

#if PANDA_LLVM_CLANG_MAJOR >= 16
#define GET_POINTER_TO_TY(ty) (ty->getNonOpaquePointerElementType())
#else
#define GET_POINTER_TO_TY(ty) (ty->getPointerElementType())
#endif

// setAlignment takes an unsigned before LLVM 10, a MaybeAlign on LLVM 10 and an Align since LLVM 11
#if PANDA_LLVM_CLANG_MAJOR >= 11
#define SET_ALIGNMENT(inst, n) inst->setAlignment(Align(n))
#define GET_PREF_ALIGNMENT(dl, ty) (dl.getPrefTypeAlign(ty).value())
#elif PANDA_LLVM_CLANG_MAJOR == 10
#define SET_ALIGNMENT(inst, n) inst->setAlignment(MaybeAlign(n))
#define GET_PREF_ALIGNMENT(dl, ty) (dl.getPrefTypeAlignment(ty))
#else
#define SET_ALIGNMENT(inst, n) inst->setAlignment(n)
#define GET_PREF_ALIGNMENT(dl, ty) (dl.getPrefTypeAlignment(ty))
#endif

using namespace llvm;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace
{
   /// Walk through every ArrayType layer and return the innermost element type.
   Type* getArrayBaseType(Type* ty)
   {
      while(auto* arr = dyn_cast<ArrayType>(ty))
         ty = arr->getElementType();
      return ty;
   }

   /// True iff every ArrayType layer in `ty` has exactly one element.
   bool allArrayDimsAreOne(Type* ty)
   {
      while(auto* arr = dyn_cast<ArrayType>(ty))
      {
         if(arr->getNumElements() != 1)
            return false;
         ty = arr->getElementType();
      }
      return true;
   }

   bool nameIsHlsStream(StringRef name)
   {
      return name.contains("hls::stream");
   }

   bool isHlsStreamTy(Type* ty)
   {
      auto* st = dyn_cast<StructType>(ty);
      return st && st->hasName() && nameIsHlsStream(st->getName());
   }

   /// True iff stripping all array wrappers from `ty` yields an hls::stream struct.
   bool baseTypeIsHlsStream(Type* ty)
   {
      return isHlsStreamTy(getArrayBaseType(ty));
   }

   /// True iff `AI` allocates a [1 x [1 x ... [1 x hls::stream]...]] with all
   /// singleton dimensions.
   bool isSingletonFifoArrayAlloca(const AllocaInst* AI)
   {
      Type* allocTy = AI->getAllocatedType();
      if(!isa<ArrayType>(allocTy))
         return false;
      if(!allArrayDimsAreOne(allocTy))
         return false;
      return baseTypeIsHlsStream(allocTy);
   }

   /// True iff every index of the GEP is the constant zero.
   bool onlyConstantZeroGep(GetElementPtrInst* gep)
   {
      return std::all_of(gep->idx_begin(), gep->idx_end(), [](Value* v) {
         auto* C = dyn_cast<ConstantInt>(v);
         return C && C->isZero();
      });
   }

   /// True iff `ty` is a singleton array (all dims == 1) wrapping an hls::stream.
   bool isSingletonFifoArrayType(Type* ty)
   {
      if(!isa<ArrayType>(ty))
         return false;
      if(!allArrayDimsAreOne(ty))
         return false;
      return baseTypeIsHlsStream(ty);
   }

   /// True iff `ptrTy` is a pointer to a singleton fifo array that should be
   /// scalarized (i.e. the argument type at a call site has already changed but
   /// the declared parameter type has not).
   static bool isSingletonParamPtr(Argument* arg)
   {
      Type* ptrTy = arg->getType();
      if(!ptrTy->isPointerTy())
         return false;
      return isSingletonFifoArrayType(GET_POINTER_TO_TY(ptrTy));
   }

   /// One argument of a function that needs to change.
   struct ArgInfo
   {
      unsigned argIndex; // position in the original argument list
      Type* scalarTy;    // what it becomes

      explicit ArgInfo(unsigned argIndex) : argIndex(argIndex), scalarTy(nullptr)
      {
      }
   };

   struct AllocaInfo
   {
      AllocaInst* allocaInst;
      Type* scalarTy;

      AllocaInfo(AllocaInst* alloca, Type* scalarTy) : allocaInst(alloca), scalarTy(scalarTy)
      {
      }
   };

   /// Everything we know about a function that needs rewriting.
   /// Filled during analysis; mutated during transformation.
   struct FnInfo
   {
      Function* oldFn;
      Function* newFn;                    // null until transformation runs
      std::vector<ArgInfo> argsToRewrite; // only args that change
      std::vector<AllocaInfo> allocasToRewrite;

      explicit FnInfo(Function* oldFn) : oldFn(oldFn), newFn(nullptr)
      {
         argsToRewrite.reserve(oldFn->arg_size());
         for(size_t i = 0; i < oldFn->arg_size(); i++)
         {
            argsToRewrite.emplace_back(ArgInfo(i));
         }
      }

      void insertAlloca(AllocaInst* alloca, Type* scalarType)
      {
         allocasToRewrite.emplace_back(AllocaInfo(alloca, scalarType));
      }

      std::vector<ArgInfo>::iterator getArgInfo(unsigned argIdx)
      {
         return std::find_if(argsToRewrite.begin(), argsToRewrite.end(),
                             [&](const ArgInfo& argInfo) { return argInfo.argIndex == argIdx; });
      }

      std::vector<ArgInfo>::const_iterator getArgInfo(unsigned argIdx) const
      {
         return std::find_if(argsToRewrite.begin(), argsToRewrite.end(),
                             [&](const ArgInfo& argInfo) { return argInfo.argIndex == argIdx; });
      }

      void setArgInfo(unsigned argIdx, Type* scalarTy)
      {
         auto argInfoIt = getArgInfo(argIdx);
         assert(argInfoIt->scalarTy == nullptr || argInfoIt->scalarTy == scalarTy);
         argInfoIt->scalarTy = scalarTy;
      }

      bool needsRewrite() const
      {
         return std::any_of(argsToRewrite.begin(), argsToRewrite.end(),
                            [](const ArgInfo& argInfo) { return argInfo.scalarTy != nullptr; }) ||
                !allocasToRewrite.empty();
      }

      bool needsCreateNewFunc() const
      {
         return std::any_of(argsToRewrite.begin(), argsToRewrite.end(),
                            [](const ArgInfo& argInfo) { return argInfo.scalarTy != nullptr; });
      }
   };

   Value* getUnderlyingObjectCompat(Value* v, const DataLayout& DL)
   {
#if PANDA_LLVM_CLANG_MAJOR >= 12
      (void)DL;
      return getUnderlyingObject(v);
#else
      return GetUnderlyingObject(v, DL);
#endif
   }

   class Impl
   {
    public:
      explicit Impl(Module& M, Function* topFn) : M(M), topFn(topFn)
      {
         createRootToLeaf();
         createLeafToRoot();
      }
      bool execute();

    private:
      Module& M;
      Function* topFn;
      std::vector<Function*> rootToLeaf;
      std::vector<Function*> leafToRoot;
      std::vector<Instruction*> deadInsts;

      // --- analysis results ---
      // keyed by original Function* for fast lookup at call sites
      std::map<Function*, FnInfo> fnInfos;

      // --- construction phase ---
      void createRootToLeaf();
      void createLeafToRoot();

      // --- analysis phase ---
      void collectAllocasAndArgs();
      void collectFuncSignatures();

      // --- transformation phase ---
      bool rewriteFuncs();
      void rewriteFunc(FnInfo&);
      bool rewriteBody(FnInfo&);
      void rewriteAlloca(AllocaInfo&, IRBuilder<>&);
      void rewriteCalls();

      // --- helpers ---
      Type* getTypeUsedInCalls(GetElementPtrInst* gep);
      void eraseDeadInsts();
      void eraseOldFns();
   };

   void inverseTopologicalSort(Function* fn, std::vector<Function*>& rootToLeaf)
   {
      for(auto& bb : *fn)
      {
         for(auto& inst : bb)
         {
            if(auto* callInst = dyn_cast<CallInst>(&inst))
            {
               auto* calleeFn = callInst->getCalledFunction();
               if(!calleeFn)
                  continue;
               if(calleeFn->isIntrinsic())
                  continue;
               if(llvm::is_contained(rootToLeaf, callInst->getCalledFunction()))
                  continue;

               inverseTopologicalSort(callInst->getCalledFunction(), rootToLeaf);
            }
         }
      }
      rootToLeaf.push_back(fn);
   }

   void Impl::createRootToLeaf()
   {
      inverseTopologicalSort(topFn, rootToLeaf);
      std::reverse(rootToLeaf.begin(), rootToLeaf.end());
   }

   void Impl::createLeafToRoot()
   {
      leafToRoot = std::vector<Function*>(rootToLeaf.rbegin(), rootToLeaf.rend());
   }

   void Impl::eraseOldFns()
   {
      for(auto& [_, fnInfo] : fnInfos)
      {
         if(fnInfo.newFn != nullptr)
         {
            assert(fnInfo.oldFn->uses().empty());
            auto name = fnInfo.oldFn->getName().str();
            fnInfo.oldFn->eraseFromParent();
            fnInfo.newFn->setName(name);
         }
      }
   }

   bool Impl::execute()
   {
      bool changed = false;

      collectAllocasAndArgs();
      changed |= rewriteFuncs();
      rewriteCalls();
      eraseDeadInsts();
      eraseOldFns();

      return changed;
   }

   void Impl::collectAllocasAndArgs()
   {
      for(Function& F : M)
      {
         if(F.isDeclaration())
            continue;

         std::vector<std::pair<AllocaInst*, Type*>> allocaAndScalarTyVec;
         std::vector<std::pair<Argument*, Type*>> argAndScalarTyVec;
         bool toScalarize = false;

         for(Instruction& I : F.getEntryBlock())
         {
            if(auto* AI = dyn_cast<AllocaInst>(&I))
            {
               if(isSingletonFifoArrayAlloca(AI))
               {
                  toScalarize |= true;
                  Type* scalarTy = getArrayBaseType(AI->getAllocatedType());
                  allocaAndScalarTyVec.emplace_back(AI, scalarTy);
               }
            }
         }

         for(auto& arg : F.args())
         {
            if(isSingletonParamPtr(&arg))
            {
               toScalarize |= true;
               Type* scalarTy = getArrayBaseType(GET_POINTER_TO_TY(arg.getType()));
               argAndScalarTyVec.emplace_back(&arg, scalarTy);
            }
         }

         if(toScalarize)
         {
            LLVM_DEBUG(dbgs() << "\n" PREFIX "Insert " << F.getName() << "\n");
            auto& fnInfo = fnInfos.insert(std::make_pair(&F, FnInfo(&F))).first->second;
            for(auto& [arg, scalarTy] : argAndScalarTyVec)
            {
               LLVM_DEBUG({
                  dbgs() << PREFIX "  Setting the arg with idx " << arg->getArgNo() << " from type ";
                  arg->getType()->print(dbgs());
                  dbgs() << " to type ";
                  scalarTy->getPointerTo()->print(dbgs());
                  dbgs() << "\n";
               });
               fnInfo.setArgInfo(arg->getArgNo(), scalarTy);
            }

            for(auto& [alloca, scalarTy] : allocaAndScalarTyVec)
            {
               LLVM_DEBUG({
                  dbgs() << PREFIX "  Changing the alloca ";
                  alloca->print(dbgs());
                  dbgs() << " to alloca of type ";
                  scalarTy->getPointerTo()->print(dbgs());
                  dbgs() << "\n";
               });
               fnInfo.insertAlloca(alloca, scalarTy);
            }
         }
      }
   }

   bool onlyCallInstInUsers(Instruction* inst)
   {
      return std::all_of(inst->user_begin(), inst->user_end(), [](User* user) { return isa<CallInst>(user); });
   }

   unsigned numOfArrays(Type* ty)
   {
      unsigned num = 0;
      auto* arrTy = dyn_cast<ArrayType>(ty);
      while(arrTy)
      {
         num++;
         arrTy = dyn_cast<ArrayType>(arrTy->getElementType());
      }
      return num;
   }

   Type* Impl::getTypeUsedInCalls(GetElementPtrInst* gep)
   {
      Type* ty = nullptr;
      for(Use& use : gep->uses())
      {
         auto* callInst = cast<CallInst>(use.getUser());
         auto* calleeFn = callInst->getCalledFunction();
         // Argument operands come first, so the operand number is the argument number
         unsigned argNo = use.getOperandNo();
         Type* argTy = nullptr;
         if(fnInfos.count(calleeFn))
         {
            const auto& fnInfo = fnInfos.at(calleeFn);
            argTy = GET_ARG_PT(fnInfo.newFn, argNo)->getType();
         }
         else
         {
            argTy = GET_ARG_PT(calleeFn, argNo)->getType();
         }
         assert(argTy != nullptr);
         assert(ty == nullptr || argTy == ty);
         ty = argTy;
      }
      assert(ty != nullptr);
      return ty;
   }

   Value* createGEPToMatchTypesAlloca(GetElementPtrInst* oldGep, Type* scalarTy, AllocaInst* alloca, IRBuilder<>& b)
   {
      b.SetInsertPoint(oldGep);
      unsigned numIdxToRemove = numOfArrays(oldGep->getSourceElementType());
      std::vector<Value*> indices(oldGep->idx_begin() + numIdxToRemove, oldGep->idx_end());
      return b.CreateGEP(scalarTy, alloca, indices);
   }

   /// The users of an alloca of stream can be only GetElementPtr and Call instructions
   void Impl::rewriteAlloca(AllocaInfo& allocaInfo, IRBuilder<>& b)
   {
      const auto& DL = M.getDataLayout();
      b.SetInsertPoint(allocaInfo.allocaInst);
      auto* newAlloca = b.CreateAlloca(allocaInfo.scalarTy, /*ArraySize=*/nullptr);
      SET_ALIGNMENT(newAlloca,
                    isHlsStreamTy(allocaInfo.scalarTy) ? 1 : GET_PREF_ALIGNMENT(DL, allocaInfo.scalarTy));
      std::vector<User*> users(allocaInfo.allocaInst->user_begin(), allocaInfo.allocaInst->user_end());
      for(auto* user : users)
      {
         if(auto* gep = dyn_cast<GetElementPtrInst>(user))
         {
            assert(onlyCallInstInUsers(gep));
            assert(onlyConstantZeroGep(gep));
            assert(baseTypeIsHlsStream(gep->getResultElementType()));

            // The type of the SSA value associated with the alloca is the pointer to scalar
            auto* allocaTy = allocaInfo.scalarTy->getPointerTo();
            auto* toTy = getTypeUsedInCalls(gep);
            LLVM_DEBUG({
               dbgs() << "\n" PREFIX "DEBUGGING ALLOCA in function " << gep->getFunction()->getName() << "\n";
               gep->print(dbgs());
               dbgs() << "\n" PREFIX "ALLOCA_TY: ";
               allocaTy->print(dbgs());
               dbgs() << "\n" PREFIX "TO_TYPE: ";
               toTy->print(dbgs());
               dbgs() << "\n" PREFIX "IF vero ? " << (allocaTy == toTy) << "\n";
            });
            Value* replacement =
                (allocaTy == toTy) ? newAlloca : createGEPToMatchTypesAlloca(gep, allocaInfo.scalarTy, newAlloca, b);
            gep->replaceAllUsesWith(replacement);
            deadInsts.push_back(gep);
         }
         else
         {
            errs() << "[SCALARIZE] Other user: ";
            user->print(errs());
            errs() << "\n";
            llvm_unreachable(
                "Alloca Instruction of a hls::stream cannot have other users than a GetElementPtr instruction");
         }
      }
      deadInsts.push_back(allocaInfo.allocaInst);
   }

   bool Impl::rewriteBody(FnInfo& fnInfo)
   {
      bool changed = false;
      auto& ctx = M.getContext();
      IRBuilder<> b(ctx);
      for(auto allocaInfo : fnInfo.allocasToRewrite)
      {
         changed = true;
         rewriteAlloca(allocaInfo, b);
      }
      return changed;
   }

   void Impl::eraseDeadInsts()
   {
      for(Instruction* inst : deadInsts)
      {
         inst->eraseFromParent();
      }
      deadInsts.clear();
   }

   Value* createGEPToMatchTypesArg(GetElementPtrInst* oldGep, Type* scalarTy, Argument* arg, IRBuilder<>& b)
   {
      unsigned numIdxToRemove = numOfArrays(oldGep->getSourceElementType());
      std::vector<Value*> indices(oldGep->idx_begin() + numIdxToRemove, oldGep->idx_end());
      return b.CreateGEP(scalarTy, arg, indices);
   }

   /// @brief Rewrite the function with the new scalar signature and replace GEPs of argument with
   ///        other GEP or the argument itself
   /// @param fnInfo e
   void Impl::rewriteFunc(FnInfo& fnInfo)
   {
      auto& ctx = M.getContext();
      Function* oldFn = fnInfo.oldFn;
      IRBuilder<> b(ctx);

      std::vector<Type*> argsTy;
      argsTy.resize(oldFn->arg_size());
      for(auto& argIt : fnInfo.argsToRewrite)
      {
         if(argIt.scalarTy != nullptr)
         {
            argsTy[argIt.argIndex] = argIt.scalarTy->getPointerTo();
         }
         else
         {
            argsTy[argIt.argIndex] = GET_ARG_PT(oldFn, argIt.argIndex)->getType();
         }
      }

      auto* fnTy = FunctionType::get(oldFn->getReturnType(), argsTy, oldFn->isVarArg());
      auto* NF = fnInfo.newFn = Function::Create(fnTy, oldFn->getLinkage(), "scalar_" + oldFn->getName(), &M);
      NF->setCallingConv(oldFn->getCallingConv());
      NF->copyAttributesFrom(oldFn);

      ValueToValueMapTy vMap;
      // Map the arguments
      for(const auto& arg : oldFn->args())
      {
         vMap[&arg] = GET_ARG_PT(NF, arg.getArgNo());
      }

      // Map the basic blocks
      for(BasicBlock& BB : *oldFn)
      {
         BasicBlock* newBB = BasicBlock::Create(M.getContext(), BB.getName(), NF);
         vMap[&BB] = newBB;
      }

      // Map the instructions
      for(BasicBlock& bb : *oldFn)
      {
         auto* newBB = cast<BasicBlock>(vMap[&bb]);
         IRBuilder<> b(newBB);
         b.SetInsertPoint(newBB, newBB->end());

         for(Instruction& inst : bb)
         {
            Value* replacement = nullptr;

            if(auto* gep = dyn_cast<GetElementPtrInst>(&inst))
            {
               if(auto* arg = dyn_cast<Argument>(gep->getPointerOperand()))
               {
                  if(fnInfo.getArgInfo(arg->getArgNo())->scalarTy != nullptr)
                  {
                     LLVM_DEBUG(dbgs() << "\n" PREFIX "DEBUGGING ARG in function " << gep->getFunction()->getName()
                                       << "\n";);
                     LLVM_DEBUG(dbgs() << PREFIX "  gep: "; gep->print(dbgs()); dbgs() << "\n";);
                     assert(onlyCallInstInUsers(gep));
                     assert(onlyConstantZeroGep(gep));

                     auto argInfo = fnInfo.getArgInfo(arg->getArgNo());

                     // The type of the SSA value associated with the alloca is the pointer to scalar
                     auto* argTy = argInfo->scalarTy->getPointerTo();
                     auto* toTy = getTypeUsedInCalls(gep);
                     LLVM_DEBUG({
                        dbgs() << PREFIX "ARG_TY: ";
                        argTy->print(dbgs());
                        dbgs() << "\n" PREFIX "TO_TYPE: ";
                        toTy->print(dbgs());
                        dbgs() << "\n" PREFIX "IF vero ? " << (argTy == toTy) << "\n";
                     });
                     replacement =
                         (argTy == toTy) ?
                             GET_ARG_PT(NF, arg->getArgNo()) :
                             createGEPToMatchTypesArg(gep, argInfo->scalarTy, GET_ARG_PT(NF, arg->getArgNo()), b);
                  }
               }
            }

            if(replacement)
            {
               vMap[&inst] = replacement;
               continue;
            }

            Instruction* newInst = inst.clone();
            RemapInstruction(newInst, vMap, RF_IgnoreMissingLocals | RF_NoModuleLevelChanges);
            b.Insert(newInst);
            vMap[&inst] = newInst;
         }
      }

      // Fixup broken phi
      for(BasicBlock& bb : *NF)
      {
         for(Instruction& inst : bb)
         {
            RemapInstruction(&inst, vMap, RF_IgnoreMissingLocals | RF_NoModuleLevelChanges);
         }
      }
   }

   bool Impl::rewriteFuncs()
   {
      bool changed = false;
      // From leaf to root
      for(Function* F : leafToRoot)
      {
         if(F->isDeclaration() || !fnInfos.count(F))
            continue;

         LLVM_DEBUG(dbgs() << "\n" PREFIX "Rewriting func " << F->getName() << "\n");
         auto& fnInfo = fnInfos.at(F);
         if(fnInfo.needsRewrite())
         {
            // rewrite the allocas
            LLVM_DEBUG(dbgs() << PREFIX "  Rewriting body\n");
            changed |= rewriteBody(fnInfo);
            // cleanup dead allocas
            eraseDeadInsts();
            if(fnInfo.needsCreateNewFunc())
            {
               // rewrite the function
               LLVM_DEBUG(dbgs() << PREFIX "  Rewriting function signature\n");
               changed = true;
               rewriteFunc(fnInfo);
            }
         }
      }
      return changed;
   }

   void Impl::rewriteCalls()
   {
      auto& ctx = M.getContext();
      IRBuilder<> b(ctx);
      for(Function& F : M)
      {
         if(F.isDeclaration())
            continue;

         for(BasicBlock& BB : F)
         {
            for(Instruction& I : BB)
            {
               if(auto* CI = dyn_cast<CallInst>(&I))
               {
                  auto* calleeFn = CI->getCalledFunction();
                  if(!calleeFn)
                     continue;

                  if(calleeFn->isIntrinsic())
                     continue;

                  if(!fnInfos.count(calleeFn))
                     continue;

                  auto& fnInfo = fnInfos.at(calleeFn);
                  if(fnInfo.needsCreateNewFunc())
                  {
                     auto* NF = fnInfo.newFn;
                     b.SetInsertPoint(CI);
                     std::vector<Value*> args(CI->arg_begin(), CI->arg_end());
                     auto* NC = b.CreateCall(NF, args);
                     NC->setCallingConv(CI->getCallingConv());
                     NC->setAttributes(CI->getAttributes());
                     CI->replaceAllUsesWith(NC);
                     deadInsts.push_back(CI);
                  }
               }
            }
         }
      }
   }

   std::string getDemangled(const std::string& declname)
   {
      int status;
      char* demangledOutbuffer = abi::__cxa_demangle(declname.c_str(), nullptr, nullptr, &status);
      if(status == 0)
      {
         std::string res = declname;
         if(std::string(demangledOutbuffer).find_last_of('(') != std::string::npos)
         {
            res = demangledOutbuffer;
            auto parPos = res.find('(');
            assert(parPos != std::string::npos);
            res = res.substr(0, parPos);
         }
         free(demangledOutbuffer);
         return res;
      }
      assert(demangledOutbuffer == nullptr);
      return declname;
   }

   Function* findTopFunction(Module& M, std::string& topFunctionNameArgPass)
   {
      auto pred = [&](const Function& f) { return getDemangled(f.getName().str()) == topFunctionNameArgPass; };
      size_t numTopFn = std::count_if(M.functions().begin(), M.functions().end(), pred);

      if(numTopFn == 0)
      {
         errs() << PREFIX "The top function specified `" << topFunctionNameArgPass
                << "` does not correspond to any function in the program\n";
         return nullptr;
      }

      if(numTopFn != 1)
      {
         errs() << PREFIX "The top function specified `" << topFunctionNameArgPass
                << "` correspond to 2 or more functions in the program\n";
         return nullptr;
      }

      return &(*std::find_if(M.functions().begin(), M.functions().end(), pred));
   }

} // anonymous namespace

// ---------------------------------------------------------------------------
// ScalarizeFifoArrayPass method definitions
// ---------------------------------------------------------------------------

char ScalarizeFifoArrayPass::ID = 0;

bool ScalarizeFifoArrayPass::exec(Module& M)
{
#if PANDA_LLVM_CLANG_MAJOR <= 16
   Function* topFn = findTopFunction(M, topFnName);
   if(!topFn)
   {
      // The top function is not in this translation unit: nothing to scalarize here, and
      // Impl::createRootToLeaf would dereference the null pointer.
      return false;
   }
   Impl impl(M, topFn);
   return impl.execute();
#else
   return false;
#endif
}

PreservedAnalyses ScalarizeFifoArrayPass::run(Module& M, ModuleAnalysisManager& /*AM*/)
{
   return exec(M) ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool ScalarizeFifoArrayPass::runOnModule(Module& M)
{
   return exec(M);
}

StringRef ScalarizeFifoArrayPass::getPassName() const
{
   return "ScalarizeFifoArrayPass";
}

void ScalarizeFifoArrayPass::getAnalysisUsage(AnalysisUsage& /*AU*/) const
{
}
