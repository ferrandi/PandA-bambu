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
 * @file plugin_customSROA.cpp
 * @brief Implementation of the Custom Scalar Replacement of Aggregates Pass
 * Aiming to introduce memory partitioning into the PandA Framework.
 *
 * @author Andrea Mannarino <mannarinoandrea98@gmail.com>
 * @author Tommaso Fellegara <tommaso.fellegara@polimi.it>
 *
 * WARNING: the current implementation of array partition works ONLY with
 * CLANG <= 16 and without opaque pointers.
 * The main problem is this: how can I know if the pointer is an array, an element of an array
 * or a pointer to a pointer to an array? With typed pointers this can be inferred
 * thanks to the type, but without it is difficult.
 * For now, to understand the problem of before, it is implemented a naive mechanism
 * that infers from the type of the pointer, but with opaque pointers this is not
 * the right way. I think we should convert the implementation to an analysis
 * that does this particular thing.
 */
#ifndef NDEBUG
#define NDEBUG
#endif
// #undef NDEBUG
#include "ArrPart.hpp"
#include "ScalarizeArrayOfFifo.hpp"
#include "PointerResolutionPass.hpp"
#include "ScalarizeSingletonBanks.hpp"
#include "llvm/Transforms/Scalar/EarlyCSE.h"
#include "llvm/Transforms/Scalar/SCCP.h"
#include <algorithm>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Operator.h>
#include <llvm/Support/ErrorHandling.h>
#if LLVM_VERSION_MAJOR >= 13
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Transforms/IPO/FunctionAttrs.h>
#endif
#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/ADCE.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/IndVarSimplify.h>
#include <llvm/Transforms/Scalar/LICM.h>
#include <llvm/Transforms/Scalar/LoopDeletion.h>
#include <llvm/Transforms/Scalar/LoopPassManager.h>
#include <llvm/Transforms/Scalar/LoopRotation.h>
#include <llvm/Transforms/Scalar/LoopUnrollPass.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/LoopSimplify.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Verifier.h"
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/IR/Dominators.h>
#include <numeric>
#include <optional>
#include <set>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO/DeadArgumentElimination.h>
#include <llvm/Transforms/IPO/GlobalOpt.h>
#include <llvm/Transforms/IPO/SCCP.h>
#include <llvm/Transforms/Utils/Local.h>
#include <vector>

#if LLVM_VERSION_MAJOR >= 16
#include <llvm/IRPrinter/IRPrintingPasses.h>
#else
#include <llvm/IR/IRPrintingPasses.h>
#endif

#include <pugixml.hpp>

#if LLVM_VERSION_MAJOR >= 13
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#else
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#endif

// If the function is a pointer
#if LLVM_VERSION_MAJOR >= 10
#define GET_ARG_PT(fn, i) fn->getArg(i)
#define GET_ARG_OPERAND_NO(inst, use) (inst->getArgOperandNo(use))
#define CALL_INST_ARG_SIZE(inst) (inst->arg_size())
#else
#define GET_ARG_PT(fn, i) (&*std::next(fn->arg_begin(), i))
#define GET_ARG_OPERAND_NO(inst, use) (use - inst->arg_begin())
#define CALL_INST_ARG_SIZE(inst) (inst->getNumArgOperands())
#endif

#if LLVM_VERSION_MAJOR <= 9
#define GET_ALIGN(alloca) alloca->getAlignment()
#elif LLVM_VERSION_MAJOR == 10
#define GET_ALIGN(alloca) llvm::MaybeAlign(alloca->getAlignment())
#else
#define GET_ALIGN(alloca) alloca->getAlign()
#endif

#include "debug_print.hpp"
#include <string>

#define CREATE_FATAL_REPORT(msg) (llvm::Twine(msg) + " (" + __func__ + ":" + llvm::Twine(__LINE__) + ")")
#define REPORT_FATAL_ERROR_WITH_REPORT(msg)         \
   do                                               \
   {                                                \
      report_fatal_error(CREATE_FATAL_REPORT(msg)); \
   } while(false)

#define REPORT_WITH_PRINT(v, msg)          \
   do                                      \
   {                                       \
      llvm::errs() << "[UNEXPECTED] ";     \
      v->print(llvm::errs());              \
      llvm::errs() << "\n";                \
      REPORT_FATAL_ERROR_WITH_REPORT(msg); \
   } while(false)

/// This file forces NDEBUG (see the top), so an `assert` here is not a check: it compiles to
/// nothing and `verifyFunction` is not even called. Every verification in this pass therefore
/// has to go through this macro, which prints the offending function and then really aborts.
#define THROW_ERROR_WITH_PRINT_FN(cond, fn, msg) \
   do                                            \
   {                                             \
      if(!(cond))                                \
      {                                          \
         fn->print(errs());                      \
         REPORT_FATAL_ERROR_WITH_REPORT(msg);    \
      }                                          \
   } while(false)

using namespace llvm;

namespace
{
   cl::opt<std::string> topFunctionName_CSROA("panda-TFN-csroa", cl::desc("Specify the name of the top function"),
                                              cl::value_desc("name of the top function"));

   cl::opt<std::string> pandaTempPath("panda-outputdir-csroa",
                                      cl::desc("Specify the directory where the bambu IR raw file will be written"),
                                      cl::value_desc("directory path"), cl::Required);

   cl::opt<std::string> architectureFile("csroa-architecture-file",
                                         cl::desc("Specify the path to the architecture.xml file"),
                                         cl::value_desc("file path"));

   cl::opt<bool> debug_lock("panda-lock-csroa", cl::init(false), cl::desc("Obtain default from a run"));

   /// Kill switch for the reachable-bank analysis of section "bank range analysis" below. Off
   /// means every dynamically indexed access gets a case per bank, which is what this pass did
   /// before the analysis existed - the way to bisect a regression without rebuilding.
   cl::opt<bool> bankRangeAnalysis("csroa-bank-range-analysis", cl::init(true),
                                   cl::desc("Restrict the bank switch of a dynamically indexed access to the "
                                            "banks its index can actually select"));

   /// Above this many cases the analysis stops trying: the enumeration itself would cost more
   /// than it saves. It is also the point at which an *unbounded* access starts being reported,
   /// because a switch that wide is never something the author meant to ask for.
   cl::opt<unsigned> maxBankCases("csroa-max-bank-cases", cl::init(4096),
                                  cl::desc("Largest reachable-bank set the CSROA range analysis will enumerate, "
                                           "and the width above which an unbounded bank switch is reported"));

   bool isCallToLifetimeStartOrEnd(const CallInst* callInst)
   {
      const Function* fn = callInst->getCalledFunction();
      if(!fn)
         return false;

      Intrinsic::ID id = fn->getIntrinsicID();
      return id == Intrinsic::lifetime_start || id == Intrinsic::lifetime_end;
   }

   /**
    * @brief Get the size of the dimension of the array type
    *
    * @param t: the array type
    * @param dim: the 0 indexed dimension
    */
   uint64_t getSizeDim(ArrayType* t, uint64_t dim)
   {
      for(; dim > 0; dim--)
      {
         t = cast<ArrayType>(t->getElementType());
      }
      return t->getNumElements();
   }

   void printValueDebug(const char* pref, Value* v)
   {
      LLVM_DEBUG({
         dbgs() << pref;
         v->print(dbgs());
         dbgs() << "\n";
      });
   }

   /**
    * @brief Creates partitions given the infos about the configuration of the final memory in infos.
    *
    * @param typePartInfo
    * @param b: the builder should be placed where the alloca will be created.
    */
   void createPartitions(AllocaPartInfo& allocaPartInfo, IRBuilder<>& b)
   {
      auto* origAlloca = allocaPartInfo.inst;
      uint64_t numPartitions = allocaPartInfo.getNumPartitions();
      ArrayType* partAllocaType = allocaPartInfo.getPartitionedType();

      for(uint64_t i = 0; i < numPartitions; i++)
      {
         auto* alloc = b.CreateAlloca(partAllocaType);
         alloc->setAlignment(GET_ALIGN(origAlloca));
         allocaPartInfo.partitionMap.insert({i, alloc});
      }
   }

   /// Compute the index cell in the partitioned memory
   std::vector<Value*> computeIndexCellInMemory(IRBuilder<>& b, std::vector<Value*>& indices,
                                                const PartitionScheme& partInfos, ArrayType* arrTyPart)
   {
      assert(indices.size() >= partInfos.size());
      std::vector<Value*> idxsGepInst;

      for(uint64_t i = 0; i < partInfos.size(); i++)
      {
         auto info = partInfos[i];
         switch(info.format)
         {
            case COMPLETE:
            {
               auto* idx = ConstantInt::get(b.getInt32Ty(), 0);
               idxsGepInst.push_back(idx);
               break;
            }
            case BLOCK:
            {
               auto* idx = b.CreateURem(indices[i], ConstantInt::get(indices[i]->getType(), getSizeDim(arrTyPart, i)));
               idxsGepInst.push_back(idx);
               break;
            }
            case CYCLIC:
            {
               auto* idx = b.CreateUDiv(indices[i], ConstantInt::get(indices[i]->getType(), info.factor));
               idxsGepInst.push_back(idx);
               break;
            }
            case PartInfoFormat::NONE:
            {
               idxsGepInst.push_back(indices[i]);
               break;
            }
            default:
               break;
         }
      }

      idxsGepInst.insert(idxsGepInst.end(), indices.begin() + idxsGepInst.size(), indices.end());
      return idxsGepInst;
   }

   /// Whether the object is addressed from its base, so that the leading GEP index walks whole
   /// objects and has to be added (or stripped) before the partitioning indices begin. An
   /// alloca or a global is, a (decayed) array argument is not.
   template <typename PartInfoT>
   constexpr bool addressedFromBase()
   {
      return std::remove_cv_t<std::remove_reference_t<PartInfoT>>::AddressedFromBase;
   }

   template <typename PartInfoT>
   std::vector<Value*> computeIndexCellInMemory(IRBuilder<>& b, std::vector<Value*>& indices, const PartInfoT& partInfo,
                                                ArrayType* arrTyPart)
   {
      std::vector<Value*> idxsGepInst = computeIndexCellInMemory(b, indices, partInfo.scheme, arrTyPart);
      if constexpr(addressedFromBase<PartInfoT>())
      {
         idxsGepInst.insert(idxsGepInst.begin(), b.getInt32(0));
      }
      return idxsGepInst;
   }

   uint64_t getSkipNumber(const PartitionScheme& scheme, const uint64_t level)
   {
      uint64_t skip = 1;
      for(uint64_t i = level; i < scheme.size(); i++)
      {
         auto info = scheme[i];
         switch(info.format)
         {
            case COMPLETE:
            case BLOCK:
            case CYCLIC:
               skip *= info.factor;
               break;
            case PartInfoFormat::NONE:
               break;
            default:
               REPORT_FATAL_ERROR_WITH_REPORT("Incorrect partition format");
         }
      }
      return skip;
   }

   Value* computeIndexMemory(IRBuilder<>& b, std::vector<Value*>& indices, const PartitionScheme& partInfos,
                             ArrayType* partTy)
   {
      // A GEP index vector legitimately mixes widths - i64 for the object stride, i32 for
      // the dimensions - so the arithmetic has to happen in one common type instead of in
      // whatever type index 0 happens to have, which produced mismatched operands.
      auto* intType = cast<IntegerType>(indices[0]->getType());
      for(uint64_t i = 0; i < partInfos.size(); i++)
      {
         auto* idxType = cast<IntegerType>(indices[i]->getType());
         if(idxType->getBitWidth() > intType->getBitWidth())
         {
            intType = idxType;
         }
      }
      const auto idxAt = [&](uint64_t i) { return b.CreateZExtOrTrunc(indices[i], intType); };

      Value* returnValue = ConstantInt::get(intType, 0);

      for(uint64_t i = 0; i < partInfos.size(); i++)
      {
         auto info = partInfos[i];
         uint64_t skipNumber = getSkipNumber(partInfos, i + 1);
         auto* constIntSkipNumber = ConstantInt::get(intType, skipNumber);
         switch(info.format)
         {
            case COMPLETE:
            {
               auto* toAdd = b.CreateMul(idxAt(i), constIntSkipNumber);
               returnValue = b.CreateAdd(returnValue, toAdd);
               break;
            }
            case BLOCK:
            {
               auto* blockIdx = b.CreateUDiv(idxAt(i), ConstantInt::get(intType, getSizeDim(partTy, i)));
               auto* toAdd = b.CreateMul(blockIdx, constIntSkipNumber);
               returnValue = b.CreateAdd(returnValue, toAdd);
               break;
            }
            case CYCLIC:
            {
               auto* blockIdx = b.CreateURem(idxAt(i), ConstantInt::get(intType, info.factor));
               auto* toAdd = b.CreateMul(blockIdx, constIntSkipNumber);
               returnValue = b.CreateAdd(returnValue, toAdd);
               break;
            }
            case PartInfoFormat::NONE:
               break;
            default:
               REPORT_FATAL_ERROR_WITH_REPORT("Incorrect partition format");
               break;
         }
      }

      return returnValue;
   }

   uint64_t computeIndexMemory(const std::vector<uint64_t>& indices, const std::vector<size_t>& origDims,
                               const PartitionScheme& scheme)
   {
      uint64_t returnIdx = 0;

      for(size_t i = 0; i < scheme.size(); i++)
      {
         const auto& info = scheme[i];
         uint64_t skipNumber = getSkipNumber(scheme, i + 1);
         switch(info.format)
         {
            case COMPLETE:
               returnIdx += indices[i] * skipNumber;
               break;
            case BLOCK:
               returnIdx += skipNumber * (indices[i] / (origDims[i] / info.factor));
               break;
            case CYCLIC:
               returnIdx += skipNumber * (indices[i] % info.factor);
               break;
            case PartInfoFormat::NONE:
               break;
            default:
               REPORT_FATAL_ERROR_WITH_REPORT("Incorrect partition format");
               break;
         }
      }
      return returnIdx;
   }

   // --- bank range analysis ------------------------------------------------------------------
   //
   // An access whose index is not a constant does not therefore reach every bank. The index of
   // an hls4ml-style kernel is affine in the surrounding induction variables, so the banks it
   // selects are a sublattice of the bank space, and SCEV can name it exactly. What follows
   // computes that set so the switch emitted by emitBankPhi carries a case per *reachable* bank
   // instead of a case per bank.
   //
   // The one invariant everything here is written around: the returned set must be a SUPERSET of
   // the banks really reachable. The default edge of that switch is the function's `unreachable`
   // block, so a set that is too tight is not a worse schedule, it is undefined behaviour. Every
   // step below therefore gives up (std::nullopt) rather than guess.

   /// Why a set could not be bounded, for the diagnostic. Empty means it was.
   using BankAnalysisFailure = std::string;

   /// The value a constant SCEV holds, if it fits.
   std::optional<uint64_t> constantSCEVValue(const SCEV* s)
   {
      auto* c = dyn_cast<SCEVConstant>(s);
      if(c == nullptr || c->getAPInt().getActiveBits() > 64)
      {
         return std::nullopt;
      }
      return c->getAPInt().getZExtValue();
   }

   /// The values an affine (possibly nested) recurrence takes over its whole iteration space.
   ///
   /// A nested recurrence is `{{start,+,outerStep}<Louter>,+,innerStep}<Linner>`, with the
   /// *innermost* loop at the top and the start descending outwards, so this recurses on
   /// getStart(). Every level must be affine with a constant step and a constant maximum trip
   /// count; an over-estimated trip count only adds values, which keeps the result a superset.
   std::optional<std::vector<uint64_t>> addRecValueSet(const SCEV* s, ScalarEvolution& SE, uint64_t cap, uint64_t limit,
                                                       BankAnalysisFailure& why)
   {
      if(auto value = constantSCEVValue(s))
      {
         return std::vector<uint64_t>{*value};
      }

      auto* addRec = dyn_cast<SCEVAddRecExpr>(s);
      if(addRec == nullptr || !addRec->isAffine())
      {
         why = "the index is not an affine function of the loop counters";
         return std::nullopt;
      }

      auto* stepConst = dyn_cast<SCEVConstant>(addRec->getStepRecurrence(SE));
      if(stepConst == nullptr)
      {
         why = "the loop counter advances by a step that is not a compile time constant";
         return std::nullopt;
      }
      const unsigned tripCount = SE.getSmallConstantMaxTripCount(addRec->getLoop());
      if(tripCount == 0)
      {
         why = "the trip count of loop '" + addRec->getLoop()->getHeader()->getName().str() + "' is not known";
         return std::nullopt;
      }

      auto starts = addRecValueSet(addRec->getStart(), SE, cap, limit, why);
      if(!starts)
      {
         return std::nullopt;
      }
      if(starts->size() * static_cast<uint64_t>(tripCount) > cap)
      {
         why = "the index takes more distinct values than the object has banks";
         return std::nullopt;
      }

      // Signed, because a loop can count down. The result is validated against `limit` below,
      // which is what rejects a step that wraps the index out of the object.
      const int64_t step = stepConst->getAPInt().getSExtValue();
      std::set<uint64_t> values;
      for(uint64_t start : *starts)
      {
         for(unsigned i = 0; i < tripCount; i++)
         {
            const uint64_t value = start + static_cast<uint64_t>(step) * i;
            if(value >= limit)
            {
               // Either the trip count over-estimates far enough to leave the object, or the
               // recurrence wraps. Both mean the enumeration is no longer trustworthy.
               why = "the index range computed for the loop leaves the bounds of the object";
               return std::nullopt;
            }
            values.insert(value);
         }
      }
      return std::vector<uint64_t>(values.begin(), values.end());
   }

   /// The residues `s mod modulus` can take.
   ///
   /// Unlike the value set this needs no trip count, which is what makes it the analysis that
   /// matters for a cyclic partition: an affine recurrence stepping by `step` only ever reaches,
   /// modulo `modulus`, the coset of gcd(step, modulus) - and that is a *single* residue exactly
   /// when the step is a multiple of the modulus, which is the shape every copy of a loop
   /// unrolled by the partition factor has. The bound on the loop is irrelevant to it, so this
   /// still fires when the trip count is a runtime value.
   ///
   /// Assuming the recurrence runs long enough to close the coset over-approximates it for a
   /// short loop, which is the safe direction.
   std::optional<std::vector<uint64_t>> residueSet(const SCEV* s, uint64_t modulus, ScalarEvolution& SE, uint64_t cap)
   {
      if(auto value = constantSCEVValue(s))
      {
         return std::vector<uint64_t>{*value % modulus};
      }

      auto* addRec = dyn_cast<SCEVAddRecExpr>(s);
      if(addRec == nullptr || !addRec->isAffine())
      {
         return std::nullopt;
      }
      auto* stepConst = dyn_cast<SCEVConstant>(addRec->getStepRecurrence(SE));
      // getSExtValue below is exact only up to 64 bits, and the accessor that would report the
      // significant bits of a wider one is spelled differently on either side of LLVM 17.
      if(stepConst == nullptr || stepConst->getAPInt().getBitWidth() > 64)
      {
         return std::nullopt;
      }
      auto starts = residueSet(addRec->getStart(), modulus, SE, cap);
      if(!starts)
      {
         return std::nullopt;
      }

      // Signed then folded back into [0, modulus): a loop counting down reaches the same coset.
      const int64_t step = stepConst->getAPInt().getSExtValue();
      const uint64_t stepMod =
          static_cast<uint64_t>((step % static_cast<int64_t>(modulus) + static_cast<int64_t>(modulus))) % modulus;
      // gcd(0, modulus) is modulus, which collapses the coset to the single starting residue -
      // the case this whole function exists for.
      const uint64_t stride = std::gcd(stepMod, modulus);
      const uint64_t reached = modulus / stride;
      if(starts->size() * reached > cap)
      {
         return std::nullopt;
      }

      std::set<uint64_t> residues;
      for(uint64_t start : *starts)
      {
         for(uint64_t t = 0; t < reached; t++)
         {
            residues.insert((start + stride * t) % modulus);
         }
      }
      return std::vector<uint64_t>(residues.begin(), residues.end());
   }

   /// The values one partitioning index of a GEP can take, bounded to at most `cap` of them and
   /// all below `limit` (the size of the dimension it indexes).
   std::optional<std::vector<uint64_t>> indexValueSet(Value* index, uint64_t limit, ScalarEvolution& SE, uint64_t cap,
                                                      BankAnalysisFailure& why)
   {
      if(auto* constIndex = dyn_cast<ConstantInt>(index))
      {
         const uint64_t value = constIndex->getZExtValue();
         if(value >= limit)
         {
            why = "the index is a constant outside the bounds of the object";
            return std::nullopt;
         }
         return std::vector<uint64_t>{value};
      }

      if(!SE.isSCEVable(index->getType()))
      {
         why = "the index has a type scalar evolution cannot reason about";
         return std::nullopt;
      }
      const SCEV* s = SE.getSCEV(index);

      BankAnalysisFailure addRecWhy;
      if(auto values = addRecValueSet(s, SE, cap, limit, addRecWhy))
      {
         return values;
      }

      // Not a recurrence this pass can walk: fall back to the interval scalar evolution can
      // still prove. Much weaker - it does not see the stride - but on a short dimension it is
      // often all that is needed, and it is what covers zext/sext-wrapped indices.
      const ConstantRange range = SE.getUnsignedRange(s);
      if(range.isFullSet() || range.isWrappedSet() || range.isEmptySet())
      {
         why = addRecWhy;
         return std::nullopt;
      }
      const uint64_t lo = range.getUnsignedMin().getLimitedValue();
      const uint64_t hi = range.getUnsignedMax().getLimitedValue();
      if(hi >= limit || hi - lo + 1 > cap)
      {
         why = addRecWhy;
         return std::nullopt;
      }
      std::vector<uint64_t> values;
      values.reserve(hi - lo + 1);
      for(uint64_t v = lo; v <= hi; v++)
      {
         values.push_back(v);
      }
      return values;
   }

   /// The banks `indices` can select, or None when they cannot be narrowed below the whole bank
   /// space. Mirrors computeIndexMemory dimension by dimension - the two must agree on which
   /// bank an index lands in, so any change to one belongs in the other.
   ///
   /// A dimension whose index cannot be bounded contributes all of its banks rather than sinking
   /// the whole query: that is what keeps a two dimensional object with one analyzable dimension
   /// useful.
   template <typename PartInfoT>
   std::optional<std::vector<uint64_t>> reachableBanks(const PartInfoT& partInfo, const std::vector<Value*>& indices,
                                                       ScalarEvolution& SE, BankAnalysisFailure& why)
   {
      const PartitionScheme& scheme = partInfo.scheme;
      const std::vector<size_t> origDims = partInfo.getOrigTypeDims();
      // Enumerating more values than the switch would have cases cannot pay for itself, so the
      // bank count is the natural budget: the analysis can never cost more than the emission it
      // replaces.
      const uint64_t cap = partInfo.getNumPartitions();

      std::vector<uint64_t> banks{0};
      bool narrowed = false;
      for(size_t i = 0; i < scheme.size(); i++)
      {
         const PartInfo& info = scheme[i];
         if(info.format == PartInfoFormat::NONE)
         {
            continue;
         }
         const uint64_t skipNumber = getSkipNumber(scheme, i + 1);

         std::set<uint64_t> contributions;
         BankAnalysisFailure dimWhy;

         // A cyclic dimension only needs the index modulo the factor, and that survives an
         // unknown trip count, so it is worth trying before the value set - which does not.
         if(info.format == CYCLIC && SE.isSCEVable(indices[i]->getType()))
         {
            if(auto residues = residueSet(SE.getSCEV(indices[i]), info.factor, SE, cap))
            {
               for(uint64_t residue : *residues)
               {
                  contributions.insert(residue * skipNumber);
               }
            }
            else
            {
               dimWhy = "the index modulo the cyclic factor is not an affine function of the loop counters";
            }
         }

         if(contributions.empty())
         {
            if(auto values = indexValueSet(indices[i], origDims[i], SE, cap, dimWhy))
            {
               for(uint64_t value : *values)
               {
                  uint64_t bank = 0;
                  switch(info.format)
                  {
                     case COMPLETE:
                        bank = value;
                        break;
                     case BLOCK:
                        bank = value / (origDims[i] / info.factor);
                        break;
                     case CYCLIC:
                        bank = value % info.factor;
                        break;
                     default:
                        REPORT_FATAL_ERROR_WITH_REPORT("Incorrect partition format");
                  }
                  if(bank >= info.factor)
                  {
                     // computeIndexMemory would compute the same out of range bank and index the
                     // partition map with it. Refusing here is what keeps the two consistent.
                     dimWhy = "the index selects a bank outside the partition map";
                     contributions.clear();
                     break;
                  }
                  contributions.insert(bank * skipNumber);
               }
            }
         }

         if(contributions.empty())
         {
            if(why.empty())
            {
               why = dimWhy;
            }
            for(uint64_t bank = 0; bank < info.factor; bank++)
            {
               contributions.insert(bank * skipNumber);
            }
         }
         else if(contributions.size() < info.factor)
         {
            narrowed = true;
         }

         if(banks.size() * contributions.size() > cap)
         {
            return std::nullopt;
         }
         std::vector<uint64_t> combined;
         combined.reserve(banks.size() * contributions.size());
         for(uint64_t bank : banks)
         {
            for(uint64_t contribution : contributions)
            {
               combined.push_back(bank + contribution);
            }
         }
         banks.swap(combined);
      }

      if(!narrowed)
      {
         // Nothing was learned; let the caller keep its own full-range path rather than hand it
         // back an enumeration of every bank.
         return std::nullopt;
      }
      std::sort(banks.begin(), banks.end());
      banks.erase(std::unique(banks.begin(), banks.end()), banks.end());
      return banks;
   }

   /// Dump the users of a value one per line, indented. Every diagnostic in this file that
   /// has to explain "this still has uses" goes through here.
   void printUsers(raw_ostream& os, const Value* v, StringRef indent = "  ")
   {
      for(const User* user : v->users())
      {
         os << indent;
         user->print(os);
         os << "\n";
      }
   }

   void deleteInstructions(std::vector<Instruction*>& instToDelete)
   {
      for(Instruction* inst : instToDelete)
      {
         if(!inst->use_empty())
         {
            llvm::errs() << "Trying to delete instruction with users:\n";
            printUsers(llvm::errs(), inst);
            REPORT_WITH_PRINT(inst, "Trying to delete an instruction that still has uses");
         }
         inst->eraseFromParent();
      }
   }

   /// Create an error basic block
   BasicBlock* createErrBB(Function* f)
   {
      BasicBlock* errBB = BasicBlock::Create(f->getContext(), "", f);
      IRBuilder<> b(errBB);
      b.CreateUnreachable();
      return errBB;
   }

   BasicBlock* getOrCreateErrBB(Function* f, FnPartInfo& fnPartInfo)
   {
      if(fnPartInfo.errBB != nullptr)
      {
         return fnPartInfo.errBB;
      }

      return fnPartInfo.errBB = createErrBB(f);
   }

   ArrayType* getPartitionTypeFromGep(GEPOperator* gep, const std::vector<size_t>& partitionedDims)
   {
      auto* srcTy = gep->getSourceElementType();
      auto* ty = getArrayBaseType(srcTy);
      for(auto dimIt = partitionedDims.rbegin(); dimIt != partitionedDims.rend(); dimIt++)
      {
         ty = ArrayType::get(ty, *dimIt);
      }
      return cast<ArrayType>(ty);
   }

   // ==================================================================================
   // Access rewriting
   //
   // Only the *root* GEP - a direct user of the partitioned base - is rewritten, into a
   // pointer to the bank its indices select. Everything hanging off it (loads, stores,
   // further GEPs, casts, call operands) is left exactly as it is: the RAUW reattaches it
   // to the right bank. When the bank is not known at compile time the pointer becomes a
   // phi fed by a switch, which is the shape PointerResolutionPass::runOnSwitchPointers
   // expands back into one memory operation per bank later in the pipeline.
   // ==================================================================================

   /// The lifetime intrinsics are the only calls that may take a partitioned object directly
   /// and be simply dropped. The __bambu_csroa_partition__ markers are the other such calls,
   /// but deleteAllUsersOfArrPartFunction has already erased them - along with the function
   /// itself - before any partitioning starts, so they never reach this walk.
   bool isMarkerCall(const User* user)
   {
      const auto* callInst = dyn_cast<CallInst>(user);
      return callInst != nullptr && isCallToLifetimeStartOrEnd(callInst);
   }

   /// Vacuously true for an unused cast: __bambu_csroa_partition__ takes a `const void*`,
   /// so the pragma leaves a uniqued ConstantExpr bitcast behind on the object, and
   /// deleteAllUsersOfArrPartFunction erases the marker call before partitioning starts.
   /// The cast is then dead but still listed among the object's users.
   bool allUsersAreMarkerCalls(const User* user)
   {
      return std::all_of(user->user_begin(), user->user_end(), [](const User* u) { return isMarkerCall(u); });
   }

   /// Single diagnostic site for everything the rewriting cannot express, replacing the ten
   /// scattered REPORT_WITH_PRINT calls the per-shape handlers used to carry.
   void reportUnsupportedAccess(Value* base, User* offender, const char* what)
   {
      llvm::errs() << "[CSROA] " << what << "\n[CSROA]   partitioned object: ";
      base->print(llvm::errs());
      llvm::errs() << "\n[CSROA]   offending user: ";
      offender->print(llvm::errs());
      llvm::errs() << "\n";
      REPORT_FATAL_ERROR_WITH_REPORT("Unsupported use of an array-partitioned object");
   }

   /// The source element type of the GEP addressing a bank. An argument points at an array
   /// stripped of its outermost dimension, so it indexes the element type instead.
   template <typename PartInfoT>
   Type* bankGepSourceType(ArrayType* partTy)
   {
      if constexpr(addressedFromBase<PartInfoT>())
      {
         return partTy;
      }
      else
      {
         return partTy->getElementType();
      }
   }

   /// The indices of `rootGep` that address the partitioned dimensions. Replaces the three
   /// former getArgDataIndices / getAllocaDataIndices / getDataIndicesGlobal variants, and
   /// checks the leading index really is a constant zero instead of assuming it.
   template <typename PartInfoT>
   bool getPartitioningIndices(GEPOperator* rootGep, const PartInfoT& partInfo, std::vector<Value*>& indices)
   {
      auto it = rootGep->idx_begin();
      if constexpr(addressedFromBase<PartInfoT>())
      {
         if(rootGep->getNumIndices() == 0)
         {
            return false;
         }
         auto* leading = dyn_cast<ConstantInt>(it->get());
         if(leading == nullptr || !leading->isZero())
         {
            return false;
         }
         ++it;
      }
      indices.insert(indices.end(), it, rootGep->idx_end());
      return indices.size() >= partInfo.scheme.size();
   }

   bool allConstantIndices(const std::vector<Value*>& indices)
   {
      return llvm::all_of(indices, [](Value* v) { return isa<ConstantInt>(v); });
   }

   /// The bank a fully constant index vector selects. Only valid when allConstantIndices holds.
   template <typename PartInfoT>
   uint64_t constantBankOf(const std::vector<Value*>& indices, const PartInfoT& partInfo)
   {
      std::vector<uint64_t> constIndices;
      constIndices.reserve(indices.size());
      for(Value* v : indices)
      {
         constIndices.push_back(cast<ConstantInt>(v)->getZExtValue());
      }
      return computeIndexMemory(constIndices, partInfo.getOrigTypeDims(), partInfo.scheme);
   }

   /// The bank is known at compile time, so the root GEP is replaced by a GEP addressing that
   /// bank - no control flow needed. A root GEP that is a ConstantExpr (the shape a global gets)
   /// is replaced by a ConstantExpr too, so the RAUW stays valid inside other constants, where an
   /// instruction could not be used.
   ///
   /// `bank` is passed in rather than derived from the indices because it is known in two
   /// different ways: from constant indices, and from a range analysis that proved the indices
   /// can only ever land in one bank even though they are not constants.
   template <typename PartInfoT>
   Value* emitBankPointer(GEPOperator* rootGep, PartInfoT& partInfo, std::vector<Value*>& indices, ArrayType* partTy,
                          uint64_t bank)
   {
      Type* srcTy = bankGepSourceType<PartInfoT>(partTy);

      auto* gepInst = dyn_cast<GetElementPtrInst>(rootGep);
      if(gepInst == nullptr)
      {
         // Constant folding only: every index of a ConstantExpr GEP is a constant by
         // construction, and so is every value in the partition map of a global.
         IRBuilder<> b(rootGep->getContext());
         std::vector<Value*> indicesBank = computeIndexCellInMemory(b, indices, partInfo, partTy);
         SmallVector<Constant*, 4> constIdxs;
         for(Value* v : indicesBank)
         {
            constIdxs.push_back(cast<Constant>(v));
         }
         return ConstantExpr::getGetElementPtr(srcTy, cast<Constant>(partInfo.partitionMap.at(bank)), constIdxs);
      }

      IRBuilder<> b(gepInst);
      std::vector<Value*> indicesBank = computeIndexCellInMemory(b, indices, partInfo, partTy);
      return b.CreateGEP(srcTy, partInfo.partitionMap.at(bank), indicesBank);
   }

   /// Constant indices: the bank they select is the one to address.
   template <typename PartInfoT>
   Value* emitStaticBankPointer(GEPOperator* rootGep, PartInfoT& partInfo, std::vector<Value*>& indices,
                                ArrayType* partTy)
   {
      return emitBankPointer(rootGep, partInfo, indices, partTy, constantBankOf(indices, partInfo));
   }

   /// Dynamic indices: the bank is only known at run time, so the pointer becomes a phi of the
   /// bank pointers, fed by a switch on the computed bank index.
   ///
   /// The block holding `splitPoint` is cut in two, the switch is planted in the first half and
   /// the phi at `splitPoint`. The case blocks are left holding nothing but their branch, and
   /// the switch default is an `unreachable` block: that is the diamond shape
   /// PointerResolutionPass::runOnSwitchPointers recognises, which sinks the memory operations
   /// hanging off this pointer back into one copy per bank. Everything a caller needs to build
   /// the element pointer - the per-bank indices, computed once before the switch - comes back
   /// in `indicesBank`.
   ///
   /// `banks` are the banks to emit a case for, or null for all of them. Narrowing it is what
   /// keeps the switch proportional to what the index can actually select instead of to the size
   /// of the object: see the bank range analysis above.
   template <typename PartInfoT>
   PHINode* emitBankPhi(Instruction* splitPoint, PartInfoT& partInfo, std::vector<Value*>& indices, ArrayType* partTy,
                        BasicBlock* errBB, std::vector<Value*>& indicesBank, const std::vector<uint64_t>* banks)
   {
      auto& ctx = splitPoint->getContext();
      Function* f = splitPoint->getFunction();
      const uint64_t numCases = banks != nullptr ? banks->size() : partInfo.getNumPartitions();
      const auto bankAt = [&](uint64_t i) { return banks != nullptr ? (*banks)[i] : i; };

      auto* bbBefore = splitPoint->getParent();
      auto* bbFromSplitOn = bbBefore->splitBasicBlock(splitPoint);
      bbBefore->getTerminator()->eraseFromParent();

      IRBuilder<> b(bbBefore);
      Value* idxMemory = computeIndexMemory(b, indices, partInfo.scheme, partTy);
      indicesBank = computeIndexCellInMemory(b, indices, partInfo, partTy);
      SwitchInst* switchInst = b.CreateSwitch(idxMemory, errBB, numCases);

      b.SetInsertPoint(splitPoint);
      auto* phiInst = b.CreatePHI(partInfo.partitionMap.at(bankAt(0))->getType(), numCases);
      for(uint64_t i = 0; i < numCases; i++)
      {
         const uint64_t bank = bankAt(i);
         auto* caseBB = BasicBlock::Create(ctx, "", f, bbFromSplitOn);
         IRBuilder<>(caseBB).CreateBr(bbFromSplitOn);
         phiInst->addIncoming(partInfo.partitionMap.at(bank), caseBB);
         switchInst->addCase(ConstantInt::get(cast<IntegerType>(idxMemory->getType()), bank), caseBB);
      }

      return phiInst;
   }

   /// The root GEP replaced by a GEP on the bank phi.
   template <typename PartInfoT>
   Value* emitBankSwitchPointer(GetElementPtrInst* gepInst, PartInfoT& partInfo, std::vector<Value*>& indices,
                                ArrayType* partTy, BasicBlock* errBB, const std::vector<uint64_t>* banks)
   {
      std::vector<Value*> indicesBank;
      PHINode* phiInst = emitBankPhi(gepInst, partInfo, indices, partTy, errBB, indicesBank, banks);
      IRBuilder<> b(gepInst);
      return b.CreateGEP(bankGepSourceType<PartInfoT>(partTy), phiInst, indicesBank);
   }

   /// The unreachable default target of the bank switches, created on demand so a function
   /// with no dynamically indexed access does not get an orphan block. When the object being
   /// partitioned belongs to a function, that function's single shared block (FnPartInfo::errBB)
   /// is reused; a global spans functions, so those get one block per function instead.
   struct ErrBBProvider
   {
      FnPartInfo* owner = nullptr;
      DenseMap<Function*, BasicBlock*> perFunction;

      explicit ErrBBProvider(FnPartInfo* owner) : owner(owner)
      {
      }

      BasicBlock* get(Function* f)
      {
         if(owner != nullptr)
         {
            return getOrCreateErrBB(f, *owner);
         }
         auto& slot = perFunction[f];
         if(slot == nullptr)
         {
            slot = createErrBB(f);
         }
         return slot;
      }
   };

   /// The banks each dynamically indexed root GEP of one partitioned object can select.
   ///
   /// Scalar evolution is built here by hand instead of being taken from an analysis manager, for
   /// two reasons. It has to answer about the functions applyArrayPartition *clones*, which no
   /// manager has ever seen; and rewriting one access splits basic blocks, so a cached analysis
   /// would answer every later query from stale data - wrong, not merely conservative. Building
   /// it locally makes the lifetime explicit: it never outlives the IR it was computed from.
   ///
   /// `precompute` is the answer to the same staleness for a base with many accesses: it reads
   /// every one of them off the pristine IR in a single pass, before the first rewrite. A base
   /// reached without it (one call operand, say) falls back to building the analysis per query,
   /// which is correct but quadratic, hence the flag that keeps the two from being confused.
   struct BankOracle
   {
      DenseMap<const Value*, std::vector<uint64_t>> narrowed;
      bool precomputed = false;

      /// The banks `rootGep` can select, or null when they could not be narrowed and the caller
      /// should fall back to every bank.
      template <typename PartInfoT>
      const std::vector<uint64_t>* banksFor(GEPOperator* rootGep, PartInfoT& partInfo)
      {
         auto it = narrowed.find(rootGep);
         if(it != narrowed.end())
         {
            return &it->second;
         }
         if(precomputed || !bankRangeAnalysis)
         {
            return nullptr;
         }
         auto* gepInst = dyn_cast<GetElementPtrInst>(rootGep);
         if(gepInst == nullptr)
         {
            return nullptr;
         }
         Function& f = *gepInst->getFunction();
         FunctionAnalyses analyses(f);
         return analyseOne(rootGep, partInfo, analyses.SE);
      }

      /// Read the reachable banks of every dynamically indexed root GEP among `users`, off IR no
      /// rewrite has touched yet. One scalar evolution per function, not per access.
      template <typename PartInfoT>
      void precompute(ArrayRef<User*> users, PartInfoT& partInfo)
      {
         precomputed = true;
         if(!bankRangeAnalysis)
         {
            return;
         }
         MapVector<Function*, SmallVector<GEPOperator*, 8>> byFunction;
         for(User* user : users)
         {
            auto* gep = dyn_cast<GEPOperator>(user);
            if(gep == nullptr || !isa<GetElementPtrInst>(gep) || gep->use_empty())
            {
               // A ConstantExpr GEP has none but constant indices, and a GEP with no uses left
               // was taken over by the call fixup: neither ever reaches a switch.
               continue;
            }
            std::vector<Value*> indices;
            if(!getPartitioningIndices(gep, partInfo, indices) || allConstantIndices(indices))
            {
               continue;
            }
            byFunction[cast<GetElementPtrInst>(gep)->getFunction()].push_back(gep);
         }

         for(auto& entry : byFunction)
         {
            FunctionAnalyses analyses(*entry.first);
            for(GEPOperator* gep : entry.second)
            {
               analyseOne(gep, partInfo, analyses.SE);
            }
         }
      }

    private:
      /// Everything scalar evolution needs, owned so it dies with this object. The declaration
      /// order is the construction order the constructors below require.
      struct FunctionAnalyses
      {
         DominatorTree DT;
         LoopInfo LI;
         AssumptionCache AC;
         TargetLibraryInfoImpl TLII;
         TargetLibraryInfo TLI;
         ScalarEvolution SE;

         explicit FunctionAnalyses(Function& f) : DT(f), LI(DT), AC(f), TLII(), TLI(TLII), SE(f, TLI, AC, DT, LI)
         {
         }
      };

      template <typename PartInfoT>
      const std::vector<uint64_t>* analyseOne(GEPOperator* rootGep, PartInfoT& partInfo, ScalarEvolution& SE)
      {
         std::vector<Value*> indices;
         if(!getPartitioningIndices(rootGep, partInfo, indices))
         {
            return nullptr;
         }
         BankAnalysisFailure why;
         auto banks = reachableBanks(partInfo, indices, SE, why);
         const uint64_t cases = banks ? banks->size() : partInfo.getNumPartitions();
         if(cases > maxBankCases)
         {
            // A switch this wide is never what the author asked for, and up to now it produced no
            // output at all - just a compilation that appeared to hang. Say which access it is
            // and what stopped the analysis from narrowing it.
            errs() << "[CSROA] warning: " << cases << " bank cases for one access to " << partInfo.to_string()
                   << "\n          in function "
                   << (isa<GetElementPtrInst>(rootGep) ? cast<GetElementPtrInst>(rootGep)->getFunction()->getName() :
                                                         StringRef("<constant>"))
                   << ", ";
            rootGep->print(errs());
            errs() << "\n          " << (why.empty() ? "the reachable banks really are that many" : why) << "\n";
         }
         if(!banks)
         {
            return nullptr;
         }
         return &narrowed.insert({rootGep, std::move(*banks)}).first->second;
      }
   };

   /// Replace `gepInst` with one copy per use, each inserted right in front of its user, and
   /// erase it. The copies are what gets a bank switch.
   ///
   /// Two properties are needed, and both come from this placement. PointerResolutionPass
   /// sinks a memory operation through a pointer phi only when the pointer has a single use
   /// (findSingleMemOpThroughChain) and the phi sits in the same block as the operation
   /// (areInstsInSameBB). Splitting at a GEP that is immediately followed by its own user
   /// puts exactly that user at the top of the merge block, whatever the other uses do, so
   /// every use ends up with its own well-formed diamond. Two of them could not share one
   /// switch anyway: `a[i] += x` computes the stored value between the load and the store,
   /// so the store cannot be sunk without the arithmetic that feeds it.
   ///
   /// One switch per use is one per memory operation - exactly what rewriting each access
   /// separately used to produce.
   void gepPerUse(GetElementPtrInst* gepInst, SmallVectorImpl<GetElementPtrInst*>& perUse)
   {
      SmallVector<Use*, 4> uses;
      for(Use& use : gepInst->uses())
      {
         uses.push_back(&use);
      }
      for(Use* use : uses)
      {
         auto* user = dyn_cast<Instruction>(use->getUser());
         if(user == nullptr || isa<PHINode>(user))
         {
            // A phi has no insertion point in front of it that all its edges reach.
            reportUnsupportedAccess(gepInst->getPointerOperand(), use->getUser(),
                                    "a dynamically indexed element of a partitioned object cannot be "
                                    "reached through a phi");
         }
         auto* copy = cast<GetElementPtrInst>(gepInst->clone());
         copy->insertBefore(user);
         use->set(copy);
         perUse.push_back(copy);
      }
      gepInst->eraseFromParent();
   }

   /// Replace one root GEP with a pointer into the bank its indices select. Everything
   /// hanging off the GEP is left untouched: the RAUW is what moves it onto the bank.
   template <typename PartInfoT>
   void rewriteRootGep(GEPOperator* rootGep, PartInfoT& partInfo, ErrBBProvider& errBBs, BankOracle& banksOracle)
   {
      std::vector<Value*> indices;
      if(!getPartitioningIndices(rootGep, partInfo, indices))
      {
         reportUnsupportedAccess(rootGep->getPointerOperand(), rootGep,
                                 "GEP does not address the object from its base with enough "
                                 "indices to select a partition");
      }

      auto* partTy = getPartitionTypeFromGep(rootGep, partInfo.getPartitionedDims());

      const std::vector<uint64_t>* banks = nullptr;
      std::optional<uint64_t> singleBank;
      if(allConstantIndices(indices))
      {
         singleBank = constantBankOf(indices, partInfo);
      }
      else
      {
         banks = banksOracle.banksFor(rootGep, partInfo);
         if(banks != nullptr && banks->size() == 1)
         {
            // Not constant, but provably confined to one bank - the shape a cyclic or block
            // partition gives an unrolled loop. Addressing that bank directly is not just a
            // smaller switch, it is no switch: no block split, no phi, and nothing for
            // PointerResolutionPass to sink afterwards.
            singleBank = banks->front();
         }
      }

      if(singleBank)
      {
         Value* bankPtr = emitBankPointer(rootGep, partInfo, indices, partTy, *singleBank);
         rootGep->replaceAllUsesWith(bankPtr);
         if(auto* gepInst = dyn_cast<GetElementPtrInst>(rootGep))
         {
            gepInst->eraseFromParent();
         }
         return;
      }

      // A ConstantExpr GEP cannot reach here: every one of its indices is a constant.
      auto* gepInst = dyn_cast<GetElementPtrInst>(rootGep);
      if(gepInst == nullptr)
      {
         reportUnsupportedAccess(rootGep->getPointerOperand(), rootGep,
                                 "a constant expression GEP cannot carry a dynamic partitioning index");
      }

      // The copies carry the same operands, so the indices computed above still apply - and so
      // does the bank set, which is a function of those operands alone.
      SmallVector<GetElementPtrInst*, 4> perUse;
      gepPerUse(gepInst, perUse);

      for(GetElementPtrInst* gep : perUse)
      {
         Value* bankPtr = emitBankSwitchPointer(gep, partInfo, indices, partTy, errBBs.get(gep->getFunction()), banks);
         gep->replaceAllUsesWith(bankPtr);
         gep->eraseFromParent();
      }
   }

   /// Rewrite every access to a partitioned base by rewriting only its *direct* users: a
   /// root GEP becomes a pointer into a bank, a load or a store taken straight off the base
   /// addresses element 0 and so is retargeted to bank 0, and the lifetime markers are
   /// dropped. Whatever hangs off a root GEP is never looked at.
   ///
   /// Calls are left alone on purpose: fixupCallInstructions has already run against the
   /// same partition map and consumed the operands it owns, so any GEP it took over is dead
   /// by now and is skipped here.
   template <typename PartInfoT>
   void applyArrayPartitionOnBase(Value* base, PartInfoT& partInfo, FnPartInfo* owner)
   {
      // Drop the uniqued ConstantExprs left dangling in the user list by the markers that
      // deleteAllUsersOfArrPartFunction has already erased, so only live uses are seen.
      if(auto* constBase = dyn_cast<Constant>(base))
      {
         constBase->removeDeadConstantUsers();
      }

      // Snapshot: rewriting a user mutates the very use list this loop reads.
      SmallVector<User*, 8> users(base->user_begin(), base->user_end());
      SmallVector<Instruction*, 4> markers;
      SmallVector<Instruction*, 4> deadCasts;
      // One user shows up once per use, and a marker is reachable both directly and through
      // the cast that feeds it: without this, the same instruction would be rewritten or
      // erased twice, which is undefined behaviour.
      SmallPtrSet<User*, 8> seen;
      ErrBBProvider errBBs(owner);

      // Before the loop below rewrites anything: it splits basic blocks, and the reachable-bank
      // analysis has to see the loops the accesses actually sit in.
      BankOracle banksOracle;
      banksOracle.precompute(users, partInfo);

      // Only for the markers reached through a cast: the direct users are already
      // deduplicated by the insert at the top of the loop.
      const auto recordMarker = [&](User* user) {
         if(seen.insert(user).second)
         {
            markers.push_back(cast<Instruction>(user));
         }
      };

      for(User* user : users)
      {
         if(!seen.insert(user).second)
         {
            continue;
         }

         if(isa<CallInst>(user))
         {
            if(isMarkerCall(user))
            {
               markers.push_back(cast<Instruction>(user));
            }
            continue;
         }

         if(isa<LoadInst>(user) || isa<StoreInst>(user))
         {
            // For a store the base must feed the *address*, not the stored value.
            auto* ST = dyn_cast<StoreInst>(user);
            if(ST != nullptr && ST->getValueOperand() == base)
            {
               reportUnsupportedAccess(base, user, "the address of a partitioned object cannot be stored");
            }
            // Addressing the base without a GEP means element 0, which lives in bank 0
            // whatever the scheme. It only types when a bank has the same shape as the base,
            // which is the decayed-pointer argument case: an alloca or a global would be
            // accessed as a whole array, and no bank holds the whole array any more.
            Value* bank0 = partInfo.partitionMap.at(0);
            const size_t ptrIdx = ST != nullptr ? StoreInst::getPointerOperandIndex() : LoadInst::getPointerOperandIndex();
            if(cast<Instruction>(user)->getOperand(ptrIdx)->getType() != bank0->getType())
            {
               reportUnsupportedAccess(base, user, "a partitioned object cannot be accessed as a whole");
            }
            cast<Instruction>(user)->setOperand(ptrIdx, bank0);
            continue;
         }

         if(auto* gep = dyn_cast<GEPOperator>(user))
         {
            // A GEP the call fixup already took over is dead: it has no users left to move.
            if(!gep->use_empty())
            {
               rewriteRootGep(gep, partInfo, errBBs, banksOracle);
            }
            continue;
         }

         if(auto* bitCast = dyn_cast<BitCastOperator>(user))
         {
            // A cast taken straight off the base is only ever the operand of a lifetime
            // marker. Anything else reinterprets the array, which would invalidate the
            // partitioning indices of every GEP that follows it.
            if(!allUsersAreMarkerCalls(bitCast))
            {
               reportUnsupportedAccess(base, user, "a partitioned object cannot be reinterpreted by a cast");
            }
            for(User* markerUser : bitCast->users())
            {
               recordMarker(markerUser);
            }
            if(auto* castInst = dyn_cast<Instruction>(bitCast))
            {
               deadCasts.push_back(castInst);
            }
            continue;
         }

         reportUnsupportedAccess(base, user,
                                 "only GEPs, loads, stores and lifetime markers can be redirected to a "
                                 "partitioned memory");
      }

      // The intrinsics have to go before the casts that feed them.
      for(Instruction* marker : markers)
      {
         marker->eraseFromParent();
      }
      for(Instruction* castInst : deadCasts)
      {
         castInst->eraseFromParent();
      }
   }

   /// The banks of every alloca of the function. They have to exist before the call fixup
   /// runs, and the call fixup has to run before any access is rewritten - see
   /// applyArrayPartition.
   void createAllocaPartitions(FnPartInfo& fnPartInfo)
   {
      for(auto& allocaPartInfo : fnPartInfo.allocs)
      {
         IRBuilder<> b(allocaPartInfo.inst);
         createPartitions(allocaPartInfo, b);
      }
   }

   void applyArrayPartitionOnAllocas(FnPartInfo& fnPartInfo)
   {
      for(auto& allocaPartInfo : fnPartInfo.allocs)
      {
         LLVM_DEBUG({ llvm::dbgs() << "[CSROA] Partitioning the alloca: " << allocaPartInfo.to_string() << "\n"; });
         applyArrayPartitionOnBase(allocaPartInfo.inst, allocaPartInfo, &fnPartInfo);
         // The alloca itself is left in place: AllocaPartInfo::inst is still the lookup key
         // for the call fixup. It is dead by now and cleanUnusedInst removes it.
      }
   }

   /// Recursively traverses a constant initializer and distributes its elements
   /// across multiple partitions based on the TypePartInfo mapping.
   ///
   /// @param initializer The constant to partition (may be scalar or aggregate)
   /// @param indices Current position in the nested array structure
   /// @param partInitializers Output vector where each partition's constants are collected
   /// @param typePartInfo Mapping information that determines which partition each element belongs to
   void createPartitionInitializers(Constant* initializer, std::vector<size_t>& indices,
                                    std::vector<std::vector<Constant*>>& partInitializers,
                                    const GlobalPartInfo& globPartInfo)
   {
      if(isa<ConstantInt>(initializer) || isa<ConstantFP>(initializer) || isa<ConstantStruct>(initializer) ||
         isa<UndefValue>(initializer) || isa<ConstantAggregateZero>(initializer)
#if LLVM_VERSION_MAJOR >= 12
         || isa<PoisonValue>(initializer)
#endif
      )
      {
         size_t idxMem = computeIndexMemory(indices, globPartInfo.getOrigTypeDims(), globPartInfo.scheme);

         if(idxMem >= partInitializers.size())
         {
            REPORT_WITH_PRINT(initializer, "Partition index " + Twine(idxMem) +
                                               " out of bounds (max: " + Twine(partInitializers.size()) + ")");
         }

         partInitializers[idxMem].push_back(initializer);
         return;
      }

      if(auto* constArr = dyn_cast<ConstantArray>(initializer))
      {
         for(size_t i = 0; i < constArr->getNumOperands(); i++)
         {
            indices.push_back(i);
            createPartitionInitializers(constArr->getOperand(i), indices, partInitializers, globPartInfo);
            indices.pop_back();
         }
         return;
      }

      if(auto* dataArray = dyn_cast<ConstantDataArray>(initializer))
      {
         for(size_t i = 0; i < dataArray->getNumElements(); i++)
         {
            indices.push_back(i);
            createPartitionInitializers(dataArray->getElementAsConstant(i), indices, partInitializers, globPartInfo);
            indices.pop_back();
         }
         return;
      }

      REPORT_WITH_PRINT(initializer, "Unsupported constant type in createPartitionInitializers. "
                                     "Expected ConstantInt, ConstantFP, ConstantStruct, ConstantArray, "
                                     "ConstantDataArray, UndefValue, or PoisonValue");
   }

   /// Re-nest a bank's flat element list into the bank's own array shape.
   /// createPartitionInitializers walks the original initializer depth-first, so each bank
   /// collects its elements already in row-major order but flattened; handing that flat list
   /// straight to ConstantArray::get was only ever type-correct for a one-dimensional bank.
   Constant* buildNestedInitializer(Type* ty, const std::vector<Constant*>& flat, size_t& consumed)
   {
      auto* arrTy = dyn_cast<ArrayType>(ty);
      if(arrTy == nullptr)
      {
         if(consumed >= flat.size())
         {
            REPORT_FATAL_ERROR_WITH_REPORT("Ran out of elements while rebuilding a partitioned initializer");
         }
         return flat[consumed++];
      }

      std::vector<Constant*> elems;
      elems.reserve(arrTy->getNumElements());
      for(uint64_t i = 0; i < arrTy->getNumElements(); i++)
      {
         elems.push_back(buildNestedInitializer(arrTy->getElementType(), flat, consumed));
      }
      return ConstantArray::get(arrTy, elems);
   }

   /// Creates partition-specific initializers for a global array.
   /// Splits a monolithic array initializer into multiple smaller initializers,
   /// one per partition, based on the partitioning scheme defined in typePartInfo.
   ///
   /// @param numPartitions Number of partitions to create
   /// @param partitionTy Type of each partition array
   /// @param typePartInfo Partitioning scheme that maps array elements to partitions
   /// @param initializer Original global array initializer
   /// @return Vector of constant initializers, one per partition
   std::vector<Constant*> createGlobalPartitionInitializers(uint64_t numPartitions, ArrayType* partitionTy,
                                                            const GlobalPartInfo& info, Constant* initializer)
   {
      if(isa<ConstantAggregateZero>(initializer))
      {
         Constant* zeroInit = ConstantAggregateZero::get(partitionTy);
         return std::vector<Constant*>(numPartitions, zeroInit);
      }

      // ConstantArray are global arrays that contain other arrays like
      // int glob[2][3] = {{1, 2, 3}, {4, 5, 6}}
      if(isa<ConstantArray>(initializer) || isa<ConstantDataArray>(initializer))
      {
         std::vector<size_t> indices;
         std::vector<std::vector<Constant*>> partInitializers(numPartitions);

         createPartitionInitializers(initializer, indices, partInitializers, info);

         std::vector<Constant*> ret(numPartitions);
         for(size_t i = 0; i < numPartitions; i++)
         {
            size_t consumed = 0;
            ret[i] = buildNestedInitializer(partitionTy, partInitializers[i], consumed);
            if(consumed != partInitializers[i].size())
            {
               REPORT_WITH_PRINT(initializer, "Partitioned initializer has " + Twine(partInitializers[i].size()) +
                                                  " elements but bank " + Twine(i) + " holds " + Twine(consumed));
            }
         }

         return ret;
      }

      REPORT_WITH_PRINT(initializer, "Initializer should be only zero initialized or constant arrays");
   }

   void createGlobalVarPartitions(GlobalPartInfo& info)
   {
      const auto& var = info.var;
      uint64_t numPartitions = info.getNumPartitions();
      ArrayType* partGlobTy = info.getPartitionedType();
      info.partitionMap.clear();

      std::vector<Constant*> initializers =
          createGlobalPartitionInitializers(numPartitions, partGlobTy, info, info.var->getInitializer());
      for(uint64_t i = 0; i < numPartitions; i++)
      {
         // The banks are the original object split N ways, so everything that describes *how* it is
         // allocated has to survive the split. Address space and thread-local mode are constructor
         // parameters; alignment, section, visibility, unnamed_addr, DLL storage and the
         // externally-initialized flag are not, and copyAttributesFrom is what carries them over.
         // The name matters too: an unnamed bank reaches the raw dump as @0, @1, ... and there is no
         // way left to tell which global bambu is reporting a memory for.
         auto* globalVar = new GlobalVariable(*(var->getParent()), partGlobTy, var->isConstant(), var->getLinkage(),
                                              initializers[i], Twine(var->getName()) + "_" + Twine(i),
                                              /*InsertBefore*/ nullptr, var->getThreadLocalMode(),
                                              // GlobalValue::getAddressSpace only exists from LLVM 8 on.
                                              var->getType()->getPointerAddressSpace());
         globalVar->copyAttributesFrom(var);
         info.partitionMap.insert({i, globalVar});
         LLVM_DEBUG(dbgs() << "[CSROA] Global variable created "; globalVar->print(dbgs()); dbgs() << "\n");
      }
   }

   /// Creates the partitioned global variables and redirects every access to the original
   /// one to the bank its indices select. The original global is left in the module here:
   /// GlobalPartInfo::var is still the lookup key for the call fixup, so it is dropped in
   /// cleanIR once every function has been processed.
   void applyArrayPartitioningOnGlobalVar(GlobalPartInfo& info)
   {
      LLVM_DEBUG({ dbgs() << "[CSROA] Partitioning the global variable: " << info.to_string() << "\n"; });
      // A global is reachable from any function, so there is no single error block to
      // share: applyArrayPartitionOnBase creates one per function that needs one.
      applyArrayPartitionOnBase(info.var, info, /*owner*/ nullptr);
      LLVM_DEBUG({ dbgs() << "[CSROA] Finished to partitioning the variable\n"; });
   }

   /**
    * @brief Check if the argument is in the ArgPartInfo vector and if it has at least one type of partitions
    */
   template <typename IterT, typename ArgsContainerT>
   bool isArgPartitioned(const IterT& it, const ArgsContainerT& argsPartitionInfos)
   {
      return it != argsPartitionInfos.end() && hasAnyPartitionedDim(it->scheme);
   }

   void applyArrayPartitionOnArguments(FnPartInfo& fnPartInfo)
   {
      for(auto& argPartInfo : fnPartInfo.args)
      {
         LLVM_DEBUG(dbgs() << "[CSROA] Partitioning the arg: " << argPartInfo.to_string() << "\n");
         applyArrayPartitionOnBase(argPartInfo.arg, argPartInfo, &fnPartInfo);
      }
   }

   bool isFnToBePartitioned(const FnPartInfo& fnPartInfo)
   {
      return llvm::any_of(fnPartInfo.args,
                          [](const ArgPartInfo& argPartInfo) { return hasAnyPartitionedDim(argPartInfo.scheme); });
   }

   Value* getUnderlyingObjectCompat(Value* v, const DataLayout& DL)
   {
#if LLVM_VERSION_MAJOR >= 12
      (void)DL;
      return getUnderlyingObject(v);
#else
      return GetUnderlyingObject(v, DL);
#endif
   }

   // ==================================================================================
   // Call operands
   //
   // A call may take a partitioned object directly, or a GEP/BitCast of it. The operand is
   // classified by walking *up* to the object it is rooted at - the mirror of the downward
   // walk used for the accesses - and is then expanded into one operand per bank when the
   // callee has a partitioned version, or narrowed to the single bank it addresses when it
   // does not.
   // ==================================================================================

   struct OperandChain
   {
      /// The partitioned object: an Argument, an AllocaInst or a GlobalVariable.
      Value* base = nullptr;
      /// base -> ... -> operand, in that order. Empty when the operand is the base itself.
      SmallVector<User*, 4> links;
      /// The first GEP off the base, if any.
      GEPOperator* rootGep = nullptr;
   };

   /// Walk from a call operand up to the object it is rooted at. Returns false on anything
   /// that is not a straight GEP/BitCast chain: a phi or a select rooted at a partitioned
   /// object arrives here, and used to fall through to an llvm_unreachable that compiles to
   /// __builtin_unreachable under this file's NDEBUG.
   bool classifyCallOperand(Value* operand, OperandChain& out)
   {
      SmallVector<User*, 4> reversed;
      Value* cur = operand;
      while(!isa<Argument>(cur) && !isa<AllocaInst>(cur) && !isa<GlobalVariable>(cur))
      {
         if(auto* gep = dyn_cast<GEPOperator>(cur))
         {
            reversed.push_back(gep);
            cur = gep->getPointerOperand();
         }
         else if(auto* bitCast = dyn_cast<BitCastOperator>(cur))
         {
            reversed.push_back(bitCast);
            cur = bitCast->getOperand(0);
         }
         else
         {
            return false;
         }
      }

      out.base = cur;
      out.links.assign(reversed.rbegin(), reversed.rend());
      for(User* link : out.links)
      {
         if(auto* gep = dyn_cast<GEPOperator>(link))
         {
            out.rootGep = gep;
            break;
         }
      }
      return true;
   }

   /// Run `f` with the PartInfo describing `base`, whichever of the three kinds it is.
   /// Returns false when the object is not partitioned - which is also how the pass now
   /// answers "is this value partitioned?", so the pass that selects the calls to fix up
   /// and the pass that rewrites them can no longer disagree.
   template <typename F>
   bool withPartInfoOfBase(Value* base, ArrPartCtx& arrPartCtx, FnPartInfo& callerFnPartInfo, F&& f)
   {
      if(auto* arg = dyn_cast<Argument>(base))
      {
         auto it = findPartInfoInContainer(arg, callerFnPartInfo.args);
         if(it == callerFnPartInfo.args.end())
         {
            return false;
         }
         f(*it);
         return true;
      }
      if(auto* allocaInst = dyn_cast<AllocaInst>(base))
      {
         auto it = findPartInfoInContainer(allocaInst, callerFnPartInfo.allocs);
         if(it == callerFnPartInfo.allocs.end())
         {
            return false;
         }
         f(*it);
         return true;
      }
      if(auto* globalVar = dyn_cast<GlobalVariable>(base))
      {
         auto it = findPartInfoInContainer(globalVar, arrPartCtx.globalVars);
         if(it == arrPartCtx.globalVars.end())
         {
            return false;
         }
         f(*it);
         return true;
      }
      return false;
   }

   /// Is this value one of the partitioned objects of the function being processed?
   /// The former two overloads of this - one keyed on a `"partitioned_"` name prefix, one
   /// on the FnPartInfo - could answer differently for the same call, so a call selected by
   /// the first could reach the rewriter and match none of its cases.
   bool isValuePartitioned(Value* v, ArrPartCtx& arrPartCtx, FnPartInfo& fnPartInfo)
   {
      return withPartInfoOfBase(v, arrPartCtx, fnPartInfo, [](auto&) {});
   }

   void getCallInstsToUpdate(ArrPartCtx& arrPartCtx, Function* fn, FnPartInfo& callerFnPartInfo,
                             std::set<CallInst*>& callInstsToUpdate)
   {
      const DataLayout& DL = fn->getParent()->getDataLayout();
      for(auto& bb : *fn)
      {
         for(auto& inst : bb)
         {
            auto* callInst = dyn_cast<CallInst>(&inst);
            if(callInst == nullptr)
            {
               continue;
            }
            for(auto* argIt = callInst->arg_begin(); argIt != callInst->arg_end(); argIt++)
            {
               if(isValuePartitioned(getUnderlyingObjectCompat(*argIt, DL), arrPartCtx, callerFnPartInfo))
               {
                  callInstsToUpdate.insert(callInst);
                  break;
               }
            }
         }
      }
   }

   /// Replay the links that follow the root GEP onto `ptr`.
   Value* replayTailLinks(Value* ptr, const OperandChain& chain, IRBuilder<>& b)
   {
      bool pastRoot = chain.rootGep == nullptr;
      for(User* link : chain.links)
      {
         if(!pastRoot)
         {
            pastRoot = (link == chain.rootGep);
            continue;
         }
         if(auto* gep = dyn_cast<GEPOperator>(link))
         {
            SmallVector<Value*, 4> idxs(gep->idx_begin(), gep->idx_end());
            ptr = gep->isInBounds() ? b.CreateInBoundsGEP(gep->getSourceElementType(), ptr, idxs) :
                                      b.CreateGEP(gep->getSourceElementType(), ptr, idxs);
         }
         else if(auto* bitCast = dyn_cast<BitCastOperator>(link))
         {
            ptr = b.CreateBitCast(ptr, bitCast->getDestTy());
         }
      }
      return ptr;
   }

   /// Whether the indices addressing *partitioned* dimensions are all constant zero. Only
   /// then does the operand still denote the whole object, so the callee can be handed one
   /// pointer per bank. Indices into non-partitioned dimensions are free to be anything and
   /// are carried through to each bank unchanged.
   bool partitioningIndicesAllZero(const std::vector<Value*>& indices, const PartitionScheme& scheme)
   {
      for(size_t i = 0; i < scheme.size(); i++)
      {
         if(scheme[i].format == PartInfoFormat::NONE)
         {
            continue;
         }
         auto* constIdx = dyn_cast<ConstantInt>(indices[i]);
         if(constIdx == nullptr || !constIdx->isZero())
         {
            return false;
         }
      }
      return true;
   }

   /// Does the operand still address the object as a whole, rather than one element of it?
   bool addressesWholeObject(const OperandChain& chain)
   {
      if(chain.rootGep == nullptr)
      {
         return true;
      }
      std::vector<Value*> idxs(chain.rootGep->idx_begin(), chain.rootGep->idx_end());
      Type* indexed = GetElementPtrInst::getIndexedType(chain.rootGep->getSourceElementType(), idxs);
      return indexed != nullptr && indexed->isArrayTy();
   }

   /// The operand addresses one element, and the callee has no partitioned version: hand it
   /// the single bank that element lives in.
   template <typename PartInfoT>
   void pushSingleBankOperand(IRBuilder<>& b, std::vector<Value*>& operands, const OperandChain& chain,
                              PartInfoT& partInfo, std::vector<Value*>& indices, ArrayType* partTy,
                              FnPartInfo& callerFnPartInfo)
   {
      // One call operand at a time, so the analysis is built per query rather than precomputed.
      // The oracle owns the set it returns, hence function scope.
      BankOracle banksOracle;
      const std::vector<uint64_t>* banks = nullptr;
      std::optional<uint64_t> singleBank;
      if(allConstantIndices(indices))
      {
         singleBank = constantBankOf(indices, partInfo);
      }
      else if(chain.rootGep != nullptr)
      {
         banks = banksOracle.banksFor(chain.rootGep, partInfo);
         if(banks != nullptr && banks->size() == 1)
         {
            singleBank = banks->front();
         }
      }

      if(singleBank)
      {
         std::vector<Value*> indicesBank = computeIndexCellInMemory(b, indices, partInfo, partTy);
         Value* ptr =
             b.CreateGEP(bankGepSourceType<PartInfoT>(partTy), partInfo.partitionMap.at(*singleBank), indicesBank);
         operands.push_back(replayTailLinks(ptr, chain, b));
         return;
      }

      // Dynamic index into a single element passed to a non-partitioned callee: the bank is
      // only known at run time, so the operand becomes a GEP on a phi of the bank pointers -
      // the same diamond an access gets, so PointerResolutionPass sees one shape only.
      auto* gepi = dyn_cast<GetElementPtrInst>(chain.rootGep);
      if(gepi == nullptr || gepi != chain.links.back())
      {
         reportUnsupportedAccess(chain.base, chain.rootGep,
                                 "a dynamically indexed element of a partitioned object can only be passed "
                                 "to a non-partitioned callee directly, without further casts or GEPs");
      }

      std::vector<Value*> indicesBank;
      PHINode* phiInst = emitBankPhi(gepi, partInfo, indices, partTy,
                                     getOrCreateErrBB(gepi->getFunction(), callerFnPartInfo), indicesBank, banks);
      b.SetInsertPoint(gepi);
      operands.push_back(b.CreateGEP(bankGepSourceType<PartInfoT>(partTy), phiInst, indicesBank));
   }

   /// Expand one call operand rooted at a partitioned object into the operands the callee
   /// expects: every bank when it has a partitioned version, one bank otherwise.
   template <typename PartInfoT>
   void pushBankOperands(IRBuilder<>& b, std::vector<Value*>& operands, const OperandChain& chain,
                         PartInfoT& partInfo, bool calleeIsPartitioned, FnPartInfo& callerFnPartInfo)
   {
      std::vector<Value*> indices;
      ArrayType* partTy = nullptr;
      if(chain.rootGep != nullptr)
      {
         if(!getPartitioningIndices(chain.rootGep, partInfo, indices))
         {
            reportUnsupportedAccess(chain.base, chain.rootGep,
                                    "GEP call operand does not address the object from its base with enough "
                                    "indices to select a partition");
         }
         partTy = getPartitionTypeFromGep(chain.rootGep, partInfo.getPartitionedDims());
      }

      if(!calleeIsPartitioned)
      {
         if(!addressesWholeObject(chain))
         {
            pushSingleBankOperand(b, operands, chain, partInfo, indices, partTy, callerFnPartInfo);
            return;
         }
         if(chain.rootGep != nullptr)
         {
            // A GEP still denoting a whole (sub)array, handed to a callee that knows nothing
            // about banks: it would see only one bank's worth of a non-contiguous array.
            reportUnsupportedAccess(chain.base, chain.rootGep,
                                    "whole-array GEP passed to a callee with no partitioned version");
         }
      }

      // Whole-object operand: the indices into partitioned dimensions must all be zero,
      // otherwise the operand denotes a slice spread across banks and no set of bank
      // pointers can represent it. The old code silently zeroed every index here instead,
      // which turned foo(&arr[i]) into foo(&arr[0]).
      if(chain.rootGep != nullptr && !partitioningIndicesAllZero(indices, partInfo.scheme))
      {
         reportUnsupportedAccess(chain.base, chain.rootGep,
                                 "a call operand indexing into a partitioned dimension cannot be passed as a "
                                 "whole partitioned array");
      }

      const uint64_t numBanks = calleeIsPartitioned ? partInfo.getNumPartitions() : 1;
      for(uint64_t i = 0; i < numBanks; i++)
      {
         Value* ptr = partInfo.partitionMap.at(i);
         if(chain.rootGep != nullptr)
         {
            std::vector<Value*> indicesBank = computeIndexCellInMemory(b, indices, partInfo, partTy);
            ptr = b.CreateGEP(bankGepSourceType<PartInfoT>(partTy), ptr, indicesBank);
         }
         operands.push_back(replayTailLinks(ptr, chain, b));
      }
   }

   void fixupSingleCallInstruction(ArrPartCtx& arrPartCtx, CallInst* callInst, FnPartInfo& callerFnPartInfo,
                                   std::vector<Instruction*>& instToDelete)
   {
      LLVM_DEBUG({
         dbgs() << "[CSROA] Processing call instruction: ";
         callInst->print(dbgs());
         dbgs() << "\n";
      });

      if(isMarkerCall(callInst))
      {
         // A lifetime marker is dropped, not rewritten: applyArrayPartitionOnBase erases the
         // markers of every partitioned object, which happens after this fixup.
         return;
      }

      auto* calledFn = callInst->getCalledFunction();
      if(calledFn == nullptr)
      {
         REPORT_WITH_PRINT(callInst, "Indirect calls are not supported while fixing up calls with partitioned args");
      }
      if(calledFn->isIntrinsic())
      {
         // A partitioned object is no longer one contiguous memory, so a memcpy/memset over
         // it has no meaning and cannot be expanded into per-bank operands either - the
         // intrinsic has a fixed arity. This used to be dropped silently, leaving the
         // intrinsic addressing the original, now unpartitioned, memory.
         REPORT_WITH_PRINT(callInst, "Intrinsic applied to an array-partitioned object");
      }
      auto fnPartitionedIt = arrPartCtx.fnTable.find(calledFn->getName());
      const bool calleeHasPartitionedVersion =
          fnPartitionedIt != arrPartCtx.fnTable.end() && fnPartitionedIt->second.fnPartitioned != nullptr;
      const auto& DL = arrPartCtx.topFn->getParent()->getDataLayout();

      std::vector<Value*> operands;
      for(size_t i = 0; i < CALL_INST_ARG_SIZE(callInst); i++)
      {
         Value* operand = callInst->getArgOperand(i);

         // One classification drives everything: walk up from the operand to the object it
         // is rooted at, then look that object up. The selection pass that picked this call
         // uses the very same predicate, so the two can no longer disagree and leave an
         // operand falling off the end of a syntactic if/else ladder.
         OperandChain chain;
         if(!classifyCallOperand(operand, chain))
         {
            if(withPartInfoOfBase(getUnderlyingObjectCompat(operand, DL), arrPartCtx, callerFnPartInfo,
                                  [](auto&) {}))
            {
               reportUnsupportedAccess(operand, callInst,
                                       "a call operand rooted at a partitioned object must be a plain GEP/BitCast "
                                       "chain: phi, select and similar merges are not supported");
            }
            LLVM_DEBUG({
               dbgs() << "[CSROA] Processing other operand: ";
               operand->print(dbgs());
               dbgs() << "\n";
            });
            operands.push_back(operand);
            continue;
         }

         IRBuilder<> b(callInst);
         const bool expanded =
             withPartInfoOfBase(chain.base, arrPartCtx, callerFnPartInfo, [&](auto& partInfo) {
                printValueDebug("[CSROA] Expanding call operand rooted at: ", chain.base);
                pushBankOperands(b, operands, chain, partInfo, calleeHasPartitionedVersion, callerFnPartInfo);
             });

         if(!expanded)
         {
            LLVM_DEBUG({
               dbgs() << "[CSROA] Processing other operand: ";
               operand->print(dbgs());
               dbgs() << "\n";
            });
            operands.push_back(operand);
         }
      }

      IRBuilder<> b(callInst);
      CallInst* newCall;
      if(calleeHasPartitionedVersion)
      {
         // The callee has a partitioned version: call it with the expanded operands.
         newCall = b.CreateCall(fnPartitionedIt->second.fnPartitioned, operands);
      }
      else
      {
         // No partitioned version exists: call the original function.
         newCall = b.CreateCall(calledFn, operands);
      }
      // The callee's calling convention must be mirrored on the call site: LLVM treats a mismatch as UB and
      // the downstream -O2 pipeline folds the caller to `unreachable`. This only shows up when a partitioned
      // function survives as a real call instead of being inlined before CSROA.
      newCall->setCallingConv(newCall->getCalledFunction()->getCallingConv());
      newCall->setTailCallKind(callInst->getTailCallKind());
      if(callInst->getCalledFunction()->getName().contains("read_bambu_internal") ||
         callInst->getCalledFunction()->getName().contains("write_bambu_internal"))
      {
         newCall->setAttributes(callInst->getAttributes());
      }
      // The operand list may have changed length, so the parameter attributes cannot be
      // copied wholesale, but everything not tied to an operand position can be.
      newCall->copyMetadata(*callInst);
      newCall->setDebugLoc(callInst->getDebugLoc());
      callInst->replaceAllUsesWith(newCall);
      instToDelete.push_back(callInst);
   }

   void fixupCallsWithPartitionedValues(ArrPartCtx& arrPartCtx, Function* fnToSearchCallInsts,
                                        FnPartInfo& callerFnPartInfo)
   {
      std::set<CallInst*> callInstsToUpdate;
      getCallInstsToUpdate(arrPartCtx, fnToSearchCallInsts, callerFnPartInfo, callInstsToUpdate);
      LLVM_DEBUG(dbgs() << "[CSROA] Number of calls to update: " << callInstsToUpdate.size() << "\n");

      std::vector<Instruction*> instToDelete;
      for(auto* callInst : callInstsToUpdate)
      {
         fixupSingleCallInstruction(arrPartCtx, callInst, callerFnPartInfo, instToDelete);
      }
      deleteInstructions(instToDelete);
   }

   /// Carry over the function-level attributes only: the two clones change the number of
   /// arguments, so the parameter attributes cannot be copied by position.
   void copyFnAttributes(Function* to, const Function* from)
   {
      LLVMContext& ctx = to->getContext();
      AttributeSet AS = from->getAttributes().getAttributes(AttributeList::FunctionIndex);
#if PANDA_LLVM_CLANG_MAJOR >= 16
      AttrBuilder B(ctx, AS);
      to->setAttributes(to->getAttributes().addFnAttributes(ctx, B));
#else
      (void)ctx;
      AttrBuilder B(AS);
      to->addAttributes(AttributeList::FunctionIndex, B);
#endif
   }

   /// Clone every basic block and instruction of `from` into the (empty) `to`, remapping
   /// through `vMap`. The caller has already put the argument mapping into `vMap`; the second
   /// remap pass is what fixes the phi edges whose incoming block was not cloned yet the
   /// first time round.
   void cloneBodyInto(Function* from, Function* to, ValueToValueMapTy& vMap)
   {
      LLVMContext& ctx = to->getContext();

      for(BasicBlock& bb : *from)
      {
         vMap[&bb] = BasicBlock::Create(ctx, bb.getName(), to);
      }

      for(BasicBlock& bb : *from)
      {
         auto* newBB = cast<BasicBlock>(vMap[&bb]);
         IRBuilder<> b(newBB);
         b.SetInsertPoint(newBB, newBB->end());

         for(auto& inst : bb)
         {
            Instruction* newInst = inst.clone();
            RemapInstruction(newInst, vMap, RF_IgnoreMissingLocals | RF_NoModuleLevelChanges);
            b.Insert(newInst);
            vMap[&inst] = newInst;
         }
      }

      for(BasicBlock& bb : *to)
      {
         for(Instruction& inst : bb)
         {
            RemapInstruction(&inst, vMap, RF_IgnoreMissingLocals | RF_NoModuleLevelChanges);
         }
      }
   }

   void arrPartCloneFunctionWithDupl(FnPartInfo& fnPartInfo)
   {
      auto* fn = fnPartInfo.fnOriginal;
      Module& M = *fn->getParent();
      StringRef fnName = fn->getName();
      auto& argsPartitionInfos = fnPartInfo.args;
      ValueToValueMapTy vMap;
      std::vector<Type*> newFnArgTypes;
      LLVM_DEBUG(llvm::dbgs() << "[CSROA] Modifying the " << fnName << " function\n");

      for(const auto& arg : fn->args())
      {
         auto argPartInfoIt = findPartInfoInContainer(&arg, argsPartitionInfos);
         newFnArgTypes.insert(newFnArgTypes.begin() + arg.getArgNo(), arg.getType());

         if(isArgPartitioned(argPartInfoIt, argsPartitionInfos))
         {
            uint64_t numPartitions = argPartInfoIt->getNumPartitions();
#if PANDA_LLVM_CLANG_MAJOR < 16
            // The "cast<ArrayType>(argPartInfoIt->getPartitionedType())->getArrayElementType()->getPointerTo()" is done
            // for this reason: An array partition type is stored like this [4 x [3 x i32]] But in LLVM the arguments
            // types are *[3 x i32]
            auto* ty = cast<ArrayType>(argPartInfoIt->getPartitionedType())->getElementType()->getPointerTo();
#elif PANDA_LLVM_CLANG_MAJOR == 16
            auto* ty = argPartInfoIt->getPartitionedType()->isOpaquePointerTy() ?
                           argPartInfoIt->getPartitionedType() :
                           cast<ArrayType>(argPartInfoIt->getPartitionedType())->getElementType()->getPointerTo();
#else
            auto* ty = argPartInfoIt->getPartitionedType();
#endif
            LLVM_DEBUG({
               dbgs() << "[CSROA] Argument with idx " << arg.getArgNo() << " is partitioned into " << numPartitions
                      << " partitions. Adding " << numPartitions << " arguments of type ";
               ty->print(dbgs());
               dbgs() << "\n";
            });
            newFnArgTypes.insert(newFnArgTypes.end(), numPartitions, ty);
         }
         else
         {
            LLVM_DEBUG({
               dbgs() << "[CSROA] Argument with idx " << arg.getArgNo() << " is not partitioned. Keeping the same "
                      << "argument of type ";
               arg.getType()->print(dbgs());
               dbgs() << "\n";
            });
            newFnArgTypes.push_back(arg.getType());
         }
      }

      auto* fnPartitionedTy = FunctionType::get(fn->getReturnType(), newFnArgTypes, fn->isVarArg());
      auto* fnPartitionedWithDupl =
          Function::Create(fnPartitionedTy, fn->getLinkage(), "partitioned_" + fnPartInfo.name, &M);
      fnPartitionedWithDupl->setCallingConv(fn->getCallingConv());
      copyFnAttributes(fnPartitionedWithDupl, fn);

      // Map the arguments
      for(const auto& arg : fn->args())
      {
         vMap[&arg] = GET_ARG_PT(fnPartitionedWithDupl, arg.getArgNo());
      }

      cloneBodyInto(fn, fnPartitionedWithDupl, vMap);

      // Map the partitions
      for(uint64_t itOrig = 0, itPart = fn->arg_size(); itPart < fnPartitionedWithDupl->arg_size(); itOrig++)
      {
         auto* arg = GET_ARG_PT(fn, itOrig);
         auto argPartInfoIt = findPartInfoInContainer(arg, argsPartitionInfos);
         if(isArgPartitioned(argPartInfoIt, argsPartitionInfos))
         {
            for(uint64_t numPartition = 0; numPartition < argPartInfoIt->getNumPartitions(); numPartition++)
            {
               argPartInfoIt->partitionMap.insert({numPartition, GET_ARG_PT(fnPartitionedWithDupl, itPart)});
               itPart++;
            }
         }
         else
         {
            LLVM_DEBUG({
               dbgs() << "[CSROA] Argument with idx " << itOrig
                      << " is not partitioned. Replace all the uses with the new argument with idx " << itPart << "\n";
            });
            GET_ARG_PT(fnPartitionedWithDupl, itOrig)->replaceAllUsesWith(GET_ARG_PT(fnPartitionedWithDupl, itPart));
            itPart++;
         }
      }

      fnPartInfo.remap(vMap);
      fnPartInfo.fnPartitionedWithDupl = fnPartitionedWithDupl;
   }

   void cleanUnusedInst(Function* fn)
   {
      std::vector<Instruction*> deadInsts;
      do
      {
         deadInsts.clear();
         for(auto& BB : *fn)
         {
            for(auto& inst : BB)
            {
               if(isInstructionTriviallyDead(&inst))
               {
                  deadInsts.push_back(&inst);
               }
            }
         }

         deleteInstructions(deadInsts);
      } while(!deadInsts.empty());
   }

   void arrPartCloneFunctionFinal(FnPartInfo& fnPartInfo)
   {
      auto* fnPartitionedWithDupl = fnPartInfo.fnPartitionedWithDupl;
      Module& M = *fnPartitionedWithDupl->getParent();
      std::vector<Type*> newFnArgTypes;

      cleanUnusedInst(fnPartitionedWithDupl);

      // Precondition: the original arguments that have been partitioned should not have any uses at this point since
      // all their uses should have been replaced with the new partitioned arguments in the "fnPartitionedWithDupl"
      // function
      for(size_t i = 0; i < fnPartInfo.fnOriginal->arg_size(); i++)
      {
         auto* arg = GET_ARG_PT(fnPartitionedWithDupl, i);
         if(!arg->use_empty())
         {
            errs() << "Original argument with idx " << i << " should not have any uses at this point. Uses:\n";
            printUsers(errs(), arg);
            for(auto* user : arg->users())
            {
               if(!user->user_empty())
               {
                  errs() << "  users of ";
                  user->print(errs());
                  errs() << ":\n";
                  printUsers(errs(), user, "    ");
               }
            }
            REPORT_FATAL_ERROR_WITH_REPORT("Original argument should not have any uses at this point");
         }
      }

      for(size_t i = fnPartInfo.fnOriginal->arg_size(); i < fnPartitionedWithDupl->arg_size(); i++)
      {
         auto* arg = GET_ARG_PT(fnPartitionedWithDupl, i);
         newFnArgTypes.push_back(arg->getType());
      }

      auto* fnPartitionedTy =
          FunctionType::get(fnPartitionedWithDupl->getReturnType(), newFnArgTypes, fnPartitionedWithDupl->isVarArg());
      auto* fnPartitioned =
          Function::Create(fnPartitionedTy, fnPartitionedWithDupl->getLinkage(), "partitioned_" + fnPartInfo.name, &M);
      fnPartitioned->setCallingConv(fnPartitionedWithDupl->getCallingConv());
      copyFnAttributes(fnPartitioned, fnPartitionedWithDupl);

      // The final arguments mirror the original ones in order: a partitioned argument contributes one
      // argument per partition, a non-partitioned one a single pass-through copy. Partitions point to
      // distinct memory regions, so they are noalias by construction; a pass-through pointer is still the
      // caller's pointer and may only claim the noalias the original argument already carried - asserting
      // it unconditionally is a lie as soon as the partitioned function survives as a real call instead of
      // being inlined before CSROA.
      // Note: fnPartInfo.args has already been remapped onto fnPartitionedWithDupl, so the lookup key is the
      // argument of that function; the attributes still have to be read from fnOriginal, since the clones
      // only inherit the function-level ones.
      for(size_t i = 0, itFinal = 0; i < fnPartInfo.fnOriginal->arg_size(); ++i)
      {
         auto argPartInfoIt = findPartInfoInContainer(GET_ARG_PT(fnPartitionedWithDupl, i), fnPartInfo.args);
         if(isArgPartitioned(argPartInfoIt, fnPartInfo.args))
         {
            for(uint64_t numPartition = 0; numPartition < argPartInfoIt->getNumPartitions(); ++numPartition, ++itFinal)
            {
               GET_ARG_PT(fnPartitioned, itFinal)->addAttr(llvm::Attribute::NoAlias);
            }
         }
         else
         {
            auto* argFinal = GET_ARG_PT(fnPartitioned, itFinal);
            if(argFinal->getType()->isPointerTy() &&
               GET_ARG_PT(fnPartInfo.fnOriginal, i)->hasAttribute(llvm::Attribute::NoAlias))
            {
               argFinal->addAttr(llvm::Attribute::NoAlias);
            }
            ++itFinal;
         }
      }

      ValueToValueMapTy vMap;

      // Map the arguments
      for(size_t i = fnPartInfo.fnOriginal->arg_size(); i < fnPartitionedWithDupl->arg_size(); i++)
      {
         auto* argDupl = GET_ARG_PT(fnPartitionedWithDupl, i);
         auto* argFinal = GET_ARG_PT(fnPartitioned, i - fnPartInfo.fnOriginal->arg_size());
         vMap[argDupl] = argFinal;
      }

      for(size_t i = 0; i < fnPartInfo.fnOriginal->arg_size(); i++)
      {
         auto* argOriginal = GET_ARG_PT(fnPartInfo.fnOriginal, i);
         auto argPartInfoIt = findPartInfoInContainer(argOriginal, fnPartInfo.args);
         if(!isArgPartitioned(argPartInfoIt, fnPartInfo.args))
         {
            continue;
         }

         auto firstPartitionIt = argPartInfoIt->partitionMap.find(0);
         if(firstPartitionIt == argPartInfoIt->partitionMap.end())
         {
            REPORT_FATAL_ERROR_WITH_REPORT("Missing partition 0 while remapping original partitioned arguments");
         }

         auto* firstPartitionArgWithDupl = dyn_cast<Argument>(firstPartitionIt->second);
         if(firstPartitionArgWithDupl == nullptr)
         {
            REPORT_FATAL_ERROR_WITH_REPORT("Expected partitioned argument mapped to a function argument");
         }

         auto mappedFirstPartitionIt = vMap.find(firstPartitionArgWithDupl);
         if(mappedFirstPartitionIt == vMap.end())
         {
            REPORT_FATAL_ERROR_WITH_REPORT("Missing remapping for partition 0 argument in final cloned function");
         }

         auto* argWithDupl = GET_ARG_PT(fnPartitionedWithDupl, i);
         vMap[argWithDupl] = mappedFirstPartitionIt->second;
      }

      cloneBodyInto(fnPartitionedWithDupl, fnPartitioned, vMap);

      fnPartInfo.fnPartitioned = fnPartitioned;
   }

   /// Every partitioned dimension has to divide evenly into its banks: the bank extent is
   /// computed with an integer division, and the BLOCK/CYCLIC index arithmetic assumes the
   /// remainder is zero. A factor that does not divide the dimension silently dropped the
   /// tail elements into the wrong bank.
   void checkPartitionFactorsDivideDims(const std::vector<size_t>& origDims, const PartitionScheme& scheme,
                                        const Twine& what)
   {
      for(size_t i = 0; i < scheme.size() && i < origDims.size(); i++)
      {
         if(scheme[i].format == PartInfoFormat::NONE)
         {
            continue;
         }
         if(scheme[i].factor == 0 || origDims[i] % scheme[i].factor != 0)
         {
            report_fatal_error(CREATE_FATAL_REPORT(what + ": array partition factor " + Twine(scheme[i].factor) +
                                                   " does not divide dimension " + Twine(i) + " of size " +
                                                   Twine(origDims[i])));
         }
      }
   }

   void checkAllPartitionFactors(ArrPartCtx& arrPartCtx)
   {
      for(auto& globalPartInfo : arrPartCtx.globalVars)
      {
         checkPartitionFactorsDivideDims(globalPartInfo.getOrigTypeDims(), globalPartInfo.scheme,
                                         "global '" + globalPartInfo.var->getName() + "'");
      }
      for(auto& item : arrPartCtx.fnTable)
      {
         auto& fnPartInfo = item.getValue();
         for(auto& argPartInfo : fnPartInfo.args)
         {
            checkPartitionFactorsDivideDims(argPartInfo.getOrigTypeDims(), argPartInfo.scheme,
                                            "parameter '" + argPartInfo.argName + "' of '" + fnPartInfo.name + "'");
         }
         for(auto& allocaPartInfo : fnPartInfo.allocs)
         {
            checkPartitionFactorsDivideDims(allocaPartInfo.getOrigTypeDims(), allocaPartInfo.scheme,
                                            "local array in '" + fnPartInfo.name + "'");
         }
      }
   }

   void applyArrayPartition(Module& M, ArrPartCtx& arrPartCtx, std::vector<std::string>& invTopSortFns)
   {
      LLVM_DEBUG({
         dbgs() << "[CSROA] Processing function order\n";
         for(size_t i = 0; i < invTopSortFns.size(); i++)
         {
            dbgs() << "[CSROA] " << i << ") " << invTopSortFns[i] << "\n";
         }
      });

      // Only the banks here: the accesses to a global are rewritten at the very end, once
      // every call has been fixed up. A GEP that only decays the array for a call - the
      // shape foo(A) has - has to reach fixupCallsWithPartitionedValues intact, because
      // that is what expands it into one operand per bank for a partitioned callee.
      for(auto& globalPartInfo : arrPartCtx.globalVars)
      {
         createGlobalVarPartitions(globalPartInfo);
      }

      for(const auto& fnName : invTopSortFns)
      {
         Function* fn = M.getFunction(fnName);
         assert(fn != nullptr);
         if(!arrPartCtx.fnTable.count(fnName))
         {
            LLVM_DEBUG(dbgs() << "[CSROA] The function " << fnName
                              << " will not be partitioned since it does not have any argument to be partitioned, but "
                                 "it could have call to function partitioned\n");
            auto emptyFnInfo = FnPartInfo(fn);
            fixupCallsWithPartitionedValues(arrPartCtx, fn, emptyFnInfo);
            continue;
         }

         auto& fnPartInfo = getFnInfoOrDie(arrPartCtx.fnTable, fnName);
         LLVM_DEBUG(dbgs() << "\n[CSROA] Processing the function " << fnName << "\n");

         if(isFnToBePartitioned(fnPartInfo))
         {
            LLVM_DEBUG(dbgs() << "[CSROA] The function " << fnName
                              << " will be partitioned since it has at least one argument to be partitioned\n");
            arrPartCloneFunctionWithDupl(fnPartInfo);
            THROW_ERROR_WITH_PRINT_FN(!verifyFunction(*fnPartInfo.fnPartitionedWithDupl, &errs()),
                                      fnPartInfo.fnPartitionedWithDupl, "Clone function with duplicates is invalid");

            createAllocaPartitions(fnPartInfo);

            fixupCallsWithPartitionedValues(arrPartCtx, fnPartInfo.fnPartitionedWithDupl, fnPartInfo);
            THROW_ERROR_WITH_PRINT_FN(!verifyFunction(*fnPartInfo.fnPartitionedWithDupl, &errs()),
                                      fnPartInfo.fnPartitionedWithDupl,
                                      "Cloned function with duplicates is invalid after call inst fixup");

            applyArrayPartitionOnArguments(fnPartInfo);
            THROW_ERROR_WITH_PRINT_FN(!verifyFunction(*fnPartInfo.fnPartitionedWithDupl, &errs()),
                                      fnPartInfo.fnPartitionedWithDupl,
                                      "Cloned function with duplicates is invalid after argument partitioning");

            applyArrayPartitionOnAllocas(fnPartInfo);
            THROW_ERROR_WITH_PRINT_FN(!verifyFunction(*fnPartInfo.fnPartitionedWithDupl, &errs()),
                                      fnPartInfo.fnPartitionedWithDupl,
                                      "Cloned function with duplicates is invalid after alloca partitioning");

            arrPartCloneFunctionFinal(fnPartInfo);
            THROW_ERROR_WITH_PRINT_FN(!verifyFunction(*fnPartInfo.fnPartitioned, &errs()), fnPartInfo.fnPartitioned,
                                      "Cloned function final is invalid");
            LLVM_DEBUG(dbgs() << "[CSROA] Created the partitioned function named "
                              << fnPartInfo.fnPartitioned->getName() << " with " << fnPartInfo.fnPartitioned->arg_size()
                              << " arguments\n");

            if(arrPartCtx.topFn == fn)
            {
               arrPartCtx.topFn = fnPartInfo.fnPartitioned;
            }
         }
         else
         {
            LLVM_DEBUG(dbgs() << "[CSROA] The function " << fnName
                              << " will not be partitioned since it does not have any argument to be partitioned\n");

            createAllocaPartitions(fnPartInfo);

            fixupCallsWithPartitionedValues(arrPartCtx, fnPartInfo.fnOriginal, fnPartInfo);
            THROW_ERROR_WITH_PRINT_FN(!verifyFunction(*fnPartInfo.fnOriginal, &errs()), fnPartInfo.fnOriginal,
                                      "Function is invalid after call inst fixup");

            applyArrayPartitionOnAllocas(fnPartInfo);
            LLVM_DEBUG(dbgs() << "[CSROA] Applied array partitioning on allocas of function " << fnName << "\n");
            THROW_ERROR_WITH_PRINT_FN(!verifyFunction(*fnPartInfo.fnOriginal, &errs()), fnPartInfo.fnOriginal,
                                      "Function is invalid after partitioning only allocas");

            // The partitioned function path gets this from arrPartCloneFunctionFinal; this
            // one has no clone, so the now dead original allocas would otherwise survive
            // alongside their banks.
            cleanUnusedInst(fnPartInfo.fnOriginal);
         }
      }

      // Now that every call has been fixed up, the accesses to the globals can be moved onto
      // their banks. The intermediate fnPartitionedWithDupl clones still around get rewritten
      // too; they are dead code that cleanIR drops.
      for(auto& globalPartInfo : arrPartCtx.globalVars)
      {
         applyArrayPartitioningOnGlobalVar(globalPartInfo);
      }

      for(auto& item : arrPartCtx.fnTable)
      {
         auto fnName = item.getKey();
         auto& value = item.getValue();
         if(value.fnPartitioned != nullptr)
         {
            value.fnOriginal->setName("old_" + fnName);
            value.fnPartitioned->setName(fnName);
         }
      }
   }

   void deleteAllUsersOfArrPartFunction(Module& M)
   {
      Function* builtinArrPartFun = M.getFunction(BAMBU_CSROA_PARTITION_FUN_NAME);
      std::vector<Instruction*> instToDelete;

      for(auto* U : builtinArrPartFun->users())
      {
         if(auto* i = dyn_cast<Instruction>(U))
         {
            instToDelete.push_back(i);
         }
      }

      deleteInstructions(instToDelete);
      builtinArrPartFun->eraseFromParent();
   }

   void cleanIR(Module& M, ArrPartCtx& arrPartCtx, const std::vector<std::string>& invTopSortFns)
   {
      LLVM_DEBUG(llvm::dbgs() << "[CSROA] Cleaning up the IR\n");

      for(auto revIt = invTopSortFns.rbegin(); revIt != invTopSortFns.rend(); ++revIt)
      {
         const auto& fnName = *revIt;
         if(!arrPartCtx.fnTable.count(fnName))
         {
            continue;
         }
         auto& fnPartInfo = getFnInfoOrDie(arrPartCtx.fnTable, fnName);
         if(fnPartInfo.fnPartitioned != nullptr)
         {
            // Erasing a Function that still has users is undefined behaviour, and the assert
            // inside LLVM that would catch it is disabled by this file's NDEBUG just like the
            // two that used to guard these lines.
            THROW_ERROR_WITH_PRINT_FN(fnPartInfo.fnOriginal->use_empty(), fnPartInfo.fnOriginal,
                                      "Original function should not have any use at this point");
            fnPartInfo.fnOriginal->eraseFromParent();
            THROW_ERROR_WITH_PRINT_FN(fnPartInfo.fnPartitionedWithDupl->use_empty(),
                                      fnPartInfo.fnPartitionedWithDupl,
                                      "PartitionedWithDupl function should not have any uses at this point");
            fnPartInfo.fnPartitionedWithDupl->eraseFromParent();
         }
      }

      // Now that every function has been rewritten, the partitioned globals have been
      // replaced by their banks and nothing looks them up any more. Left behind they would
      // reach the raw dump and bambu would allocate a whole extra memory for each of them.
      for(auto& globalPartInfo : arrPartCtx.globalVars)
      {
         auto* var = globalPartInfo.var;
         var->removeDeadConstantUsers();
         if(var->use_empty())
         {
            var->eraseFromParent();
            globalPartInfo.var = nullptr;
         }
         else
         {
            LLVM_DEBUG(dbgs() << "[CSROA] Partitioned global " << var->getName()
                              << " still has users and is kept alongside its banks\n");
         }
      }
   }
} // namespace

namespace llvm
{

   struct CustomSROA : public ModulePass
#if LLVM_VERSION_MAJOR >= 13
       ,
                       public PassInfoMixin<CustomSROA>
#endif
   {
      static char ID;

      CustomSROA() : ModulePass(ID)
      {
         initializeCallGraphWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeLoopInfoWrapperPassPass(*PassRegistry::getPassRegistry());
      }

#if LLVM_VERSION_MAJOR >= 13
      CustomSROA(const CustomSROA&) : CustomSROA()
      {
      }
#endif

      bool exec(Module& M)
      {
         LLVM_DEBUG(llvm::dbgs() << "Started Pass: CUSTOM SCALAR REPLACEMENT of AGGREGATES\n");

         if(topFunctionName_CSROA.empty())
         {
            LLVM_DEBUG(dbgs() << "[CSROA] Top function not specified, skipping the pass\n");
            return false;
         }

         if(!isArrPartFunctionPresent(M))
         {
            LLVM_DEBUG(llvm::dbgs() << "[CSROA] CSROA partition function not present. Skipping CSROA pass.\n");
            return false;
         }

         if(debug_lock)
         {
            LLVM_DEBUG(llvm::dbgs() << "[CSROA] Debug lock CSROA is enabled. Skipping CSROA pass.\n");
            deleteAllUsersOfArrPartFunction(M);
            return true;
         }

         ArrPartCtx arrPartCtx;
         pugi::xml_document doc;
         arrPartCtx.doc = &doc;
         arrPartCtx.topFn = findTopFunction(M, topFunctionName_CSROA);
         if(!arrPartCtx.topFn)
         {
            return false;
         }

         LLVM_DEBUG(llvm::dbgs() << "[CSROA] TopFunctionName_CSROA: " << arrPartCtx.topFn->getName() << "\n";);

         if(!loadXMLModule(doc, architectureFile))
         {
            return false;
         }

         populateArrPartCtx(M, arrPartCtx);

         std::vector<std::string> invTopSortFns;
         inverseTopologicalSort(arrPartCtx.topFn, invTopSortFns);
         LLVM_DEBUG({
            dbgs() << "\n=====================================================================\n";
            dbgs() << "[CSROA] Initial array partition requests\n";
            describeArrPartRequests(arrPartCtx);
            dbgs() << "=====================================================================\n";
         });

         diffuseArrPartConfigs(arrPartCtx, invTopSortFns);

         // Deliberately not under LLVM_DEBUG, unlike the "initial" dump above: the scheme the
         // diffusion settled on is what the user has to see to make sense of the memories bambu
         // then reports, since a pragma on one function propagates to its callers and callees.
         dbgs() << "\n=====================================================================\n";
         dbgs() << "[CSROA] Final array partition requests\n";
         describeArrPartRequests(arrPartCtx);
         dbgs() << "=====================================================================\n";

         checkAllPartitionFactors(arrPartCtx);

         deleteAllUsersOfArrPartFunction(M);
         applyArrayPartition(M, arrPartCtx, invTopSortFns);

         modifyXMLModule(arrPartCtx, architectureFile);

         cleanIR(M, arrPartCtx, invTopSortFns);

         if(llvm::verifyModule(M, &llvm::errs()))
         {
            REPORT_FATAL_ERROR_WITH_REPORT("Module is invalid after the CSROA pass");
         }
         LLVM_DEBUG(llvm::dbgs() << "Ended Pass: CUSTOM SCALAR REPLACEMENT of AGGREGATES\n");
         return true;
      }

      bool runOnModule(Module& M) override
      {
#if LLVM_VERSION_MAJOR < 13

         CallGraphWrapperPass* CGPass = getAnalysisIfAvailable<CallGraphWrapperPass>();
         if(!CGPass)
         {
            REPORT_FATAL_ERROR_WITH_REPORT("not able to retrieve the call graph");
         }
         return exec(M);
#else
         REPORT_FATAL_ERROR_WITH_REPORT("Call to runOnModule not expected with current LLVM version");
         return false;
#endif
      }

      StringRef getPassName() const override
      {
         return "customSROA";
      }

      void getAnalysisUsage(AnalysisUsage& AU) const override
      {
         AU.addRequired<CallGraphWrapperPass>();
         // AU.addRequired<AAResultsWrapperPass>();
         AU.addRequired<LoopInfoWrapperPass>();
      }

#if LLVM_VERSION_MAJOR >= 13
      llvm::PreservedAnalyses run(llvm::Module& M, llvm::ModuleAnalysisManager& MAM)
      {
         (void)MAM;
         const auto changed = exec(M);
         return (changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
      }
#endif
   };
   char CustomSROA::ID = 0;
} // namespace llvm

#ifndef _WIN32
static llvm::RegisterPass<llvm::CustomSROA> customSROAPass("customSROA", "Custom Scalar Replacement of Aggregates",
                                                           false /* Only looks at CFG */, false /* Analysis Pass */);
#endif

#if LLVM_VERSION_MAJOR >= 13
llvm::PassPluginLibraryInfo getCustomSROAInfo()
{
   return {LLVM_PLUGIN_API_VERSION, "customSROA", "v0.1", [](llvm::PassBuilder& PB) {
              const auto load = [](llvm::ModulePassManager& MPM) {
               auto createOutputStream = [](const std::string& Filename) -> std::unique_ptr<llvm::raw_fd_ostream> {
                    std::error_code EC;
                    auto OS = std::make_unique<llvm::raw_fd_ostream>(pandaTempPath + "/" + Filename, EC);
                    if(EC)
                    {
                       llvm::errs() << "Error opening " << Filename << ": " << EC.message() << "\n";
                       return nullptr;
                    }
                    return OS;
                 };

                 // This mirrors the order of -O2's buildModuleSimplificationPipeline: interprocedural
                 // constant folding and inlining first, then the function simplification pipeline whose
                 // loop stage ends in the unroller. The order is what matters: the trip counts of the
                 // hls4ml kernels only become visible to SCEV once the calls are inlined and the
                 // config<N> template constants and the constant weight globals are folded in.

                 // --- pre-inline cleanup -------------------------------------------------------
                 static auto pluginBeginOS = createOutputStream("begin_plugin_CSROA.ll");
                 if(pluginBeginOS)
                    MPM.addPass(llvm::PrintModulePass(*pluginBeginOS));
                 FunctionPassManager FPM_Early;
                 FPM_Early.addPass(llvm::SimplifyCFGPass());
                 FPM_Early.addPass(llvm::EarlyCSEPass());
                 MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM_Early)));

                 // Constant globals passed as parameters (hls4ml weights/biases) must be folded into the
                 // callee and the now unused parameters dropped, otherwise they reach CSROA as bare
                 // non-partitioned pointers.
                 MPM.addPass(llvm::IPSCCPPass());
                 MPM.addPass(llvm::GlobalOptPass());
                 MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::PromotePass()));
                 MPM.addPass(llvm::DeadArgumentEliminationPass());

                 FunctionPassManager FPM_PreInline;
                 FPM_PreInline.addPass(llvm::InstCombinePass());
                 FPM_PreInline.addPass(llvm::SimplifyCFGPass());
                 MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM_PreInline)));

                 // --- function simplification, -O2 order, ending in the unroller ----------------
                 // NOTE: this is deliberately *not* added to MPM here. It goes inside the inliner's
                 // CGSCC pass manager further down, see the comment there.
                 FunctionPassManager FPM_Simpl;
                 FPM_Simpl.addPass(llvm::EarlyCSEPass(/*UseMemorySSA*/ true));
                 FPM_Simpl.addPass(llvm::SimplifyCFGPass());
                 FPM_Simpl.addPass(llvm::InstCombinePass());

                 // Canonicalize the loops
                 FPM_Simpl.addPass(llvm::LoopSimplifyPass());
                 // Reorganize the loops in the closed form (used in many passes)
                 FPM_Simpl.addPass(llvm::LCSSAPass());
                 LoopPassManager LPM_Rotate;
#if LLVM_VERSION_MAJOR >= 16
                 LPM_Rotate.addPass(llvm::LICMPass(llvm::LICMOptions()));
#else
                 LPM_Rotate.addPass(llvm::LICMPass());
#endif
                 // Rotation is what turns the exit test into a form SCEV can count.
                 LPM_Rotate.addPass(llvm::LoopRotatePass());
#if LLVM_VERSION_MAJOR >= 16
                 LPM_Rotate.addPass(llvm::LICMPass(llvm::LICMOptions()));
                 // LPM_Rotate.addPass(llvm::LoopFlattenBambuPass());
#else
                 LPM_Rotate.addPass(llvm::LICMPass());
#endif
                 FPM_Simpl.addPass(
                     llvm::createFunctionToLoopPassAdaptor(std::move(LPM_Rotate), /*UseMemorySSA*/ true));
                 FPM_Simpl.addPass(llvm::SimplifyCFGPass());
                 FPM_Simpl.addPass(llvm::InstCombinePass());

                 // Simplify the induction variables and drop the dead loops, then unroll: same grouping
                 // -O2 uses in LPM2 of buildFunctionSimplificationPipeline.
                 LoopPassManager LPM_Unroll;
                 LPM_Unroll.addPass(llvm::IndVarSimplifyPass());
                 LPM_Unroll.addPass(llvm::LoopDeletionPass());
                 FPM_Simpl.addPass(llvm::createFunctionToLoopPassAdaptor(std::move(LPM_Unroll)));
                 FPM_Simpl.addPass(llvm::SCCPPass());
                 // Same configuration -O2 gives its unroller: every loop with a computable trip count
                 // that fits the cost threshold, not only the ones carrying a pragma.
                 FPM_Simpl.addPass(LoopUnrollPass(LoopUnrollOptions(/*OptLevel=*/2,
                                                                    /*OnlyWhenForced=*/false,
                                                                    /*ForgetSCEV=*/false)));

                 // --- post-unroll cleanup ------------------------------------------------------
                 FPM_Simpl.addPass(llvm::SCCPPass());
                 FPM_Simpl.addPass(llvm::SimplifyCFGPass());
                 FPM_Simpl.addPass(llvm::InstCombinePass());
#if LLVM_VERSION_MAJOR == 13
                 FPM_Simpl.addPass(llvm::GVN());
#else
                 FPM_Simpl.addPass(llvm::GVNPass());
#endif
                 FPM_Simpl.addPass(llvm::ADCEPass());
                 FPM_Simpl.addPass(llvm::SimplifyCFGPass());
                 FPM_Simpl.addPass(llvm::InstCombinePass());

                 // --- inliner, with the simplification nested inside it ------------------------
                 // The inliner is a CGSCC pass and the CGSCC walk is bottom-up, so putting the
                 // function simplification in the *same* CGSCC pass manager means every callee is
                 // already simplified and unrolled by the time the inliner computes the cost of
                 // inlining it into its callers. Running the unroller after a module-level inliner
                 // instead would let the inliner price the pre-unroll (cheap) body and inline what
                 // only becomes huge afterwards. This is exactly how -O2 nests them
                 // (buildInlinerPipeline), and the reason LLVM can afford a plain cost threshold.
                 llvm::ModuleInlinerWrapperPass MIWP;
                 MIWP.getPM().addPass(llvm::PostOrderFunctionAttrsPass());
                 MIWP.getPM().addPass(llvm::createCGSCCToFunctionPassAdaptor(std::move(FPM_Simpl)));
                 MPM.addPass(std::move(MIWP));
                 // MPM.addPass(llvm::PrintModulePass());

                 // Fold again what inlining exposed at module scope.
                 MPM.addPass(llvm::IPSCCPPass());
                 MPM.addPass(llvm::GlobalOptPass());
                 MPM.addPass(llvm::DeadArgumentEliminationPass());
                 // Re-fold the GEP chains the signature rewrite leaves behind: CSROA only knows how to
                 // replace Load/Store/Call users of an alloca GEP. InstCombine refuses to merge an
                 // all-zero GEP into a multi-use one (shouldMergeGEPs), so CSE the duplicate element
                 // GEPs down to a single use first.
                 {
                    FunctionPassManager FPM_GEP;
                    FPM_GEP.addPass(llvm::EarlyCSEPass());
                    FPM_GEP.addPass(llvm::InstCombinePass());
                    // The array-ctor loops clang emits around ac_fixed arrays are empty once the default
                    // constructor is dead, but they keep a pointer induction variable whose exit test
                    // compares two pointers into a partitioned alloca. LoopDeletion in FPM_Simpl runs
                    // before the body is emptied, so run it again here.
                    FPM_GEP.addPass(llvm::LoopSimplifyPass());
                    FPM_GEP.addPass(llvm::LCSSAPass());
                    LoopPassManager LPM_Del;
                    LPM_Del.addPass(llvm::LoopDeletionPass());
                    FPM_GEP.addPass(llvm::createFunctionToLoopPassAdaptor(std::move(LPM_Del)));
                    FPM_GEP.addPass(llvm::EarlyCSEPass());
                    FPM_GEP.addPass(llvm::InstCombinePass());
                    MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM_GEP)));
                 }
                 // -O2 infers the argument/function attributes with PostOrderFunctionAttrs (run inside the
                 // inliner's CGSCC pipeline above) plus this module-level pass, not with the Attributor:
                 // on the unrolled module the latter spends minutes in MustBeExecutedIterator::advance()
                 // for the same nocapture/readonly facts CSROA needs.
                 MPM.addPass(llvm::ReversePostOrderFunctionAttrsPass());

                 static auto BeforeOS = createOutputStream("before_CSROA.ll");
                 if(BeforeOS)
                    MPM.addPass(llvm::PrintModulePass(*BeforeOS));
                 MPM.addPass(llvm::CustomSROA());
                 static auto AfterOS = createOutputStream("after_CSROA.ll");
                 if(AfterOS)
                    MPM.addPass(llvm::PrintModulePass(*AfterOS));
                 MPM.addPass(llvm::VerifierPass());

                 // An access whose bank is only known at run time leaves CSROA as a switch
                 // feeding a phi of bank pointers. SimplifyCFG canonicalises that shape - a
                 // two-bank switch on an unreachable default collapses into a select - and
                 // PointerResolutionPass then sinks the memory operations hanging off the
                 // phi back into one copy per bank. That has to happen here, before the two
                 // passes below: a bank whose alloca is used by a phi is neither classified
                 // as a singleton bank nor promotable by mem2reg.
                 MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::SimplifyCFGPass()));
                 MPM.addPass(llvm::PointerResolutionPass(pandaTempPath));
                 MPM.addPass(llvm::VerifierPass());

                 // A completely partitioned array becomes one bank per cell, and each bank keeps
                 // the element type wrapped in a singleton array and in the single-member structs
                 // the C++ type brought along. Nothing upstream promotes that shape: mem2reg wants
                 // the accessed type to equal the allocated type, and SROA refuses any integer
                 // whose bit width differs from its store size in bits and degrades the alloca to
                 // [5 x i8]. Every survivor then costs a memory object in bambu - 4412 of them on
                 // issue1b_plain_backend_timeout. Peeling the wrappers is enough for mem2reg,
                 // which has no scan limit, unlike the store-to-load forwarding in InstCombine /
                 // GVN / MemorySSA whose cl::opt caps sit far below the ~7000 instructions between
                 // a bank's store and its load.
                 //
                 // PromotePass must run here and not be left to the -O2 pipeline that follows:
                 // that one reaches SROA first, and SROA degrades the scalar allocas just created.
                 MPM.addPass(llvm::ScalarizeSingletonBanksPass());
                 MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::PromotePass()));
                 MPM.addPass(llvm::VerifierPass());
#if PANDA_LLVM_CLANG_MAJOR >= 16
                 MPM.addPass(llvm::ScalarizeFifoArrayPass(topFunctionName_CSROA));
                 MPM.addPass(llvm::VerifierPass());
#endif

                 return true;
              };

              PB.registerPipelineParsingCallback([&](llvm::StringRef Name, llvm::ModulePassManager& MPM,
                                                     llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                 if(Name == "customSROA")
                 {
                    return load(MPM);
                 }
                 // Registered on its own too: it is the piece whose effect can be checked with
                 // opt on a dumped after_CSROA.ll without replaying the whole pipeline.
                 if(Name == "scalarizeSingletonBanks")
                 {
                    MPM.addPass(llvm::ScalarizeSingletonBanksPass());
                    return true;
                 }
                 return false;
              });
              PB.registerPipelineEarlySimplificationEPCallback([&](llvm::ModulePassManager& MPM,
#if LLVM_VERSION_MAJOR < 16
                                                                   llvm::PassBuilder::OptimizationLevel
#else
                                                                   llvm::OptimizationLevel
#endif
                                                               ) { return load(MPM); });
           }};
}

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK __attribute__((visibility("default")))
llvmGetPassPluginInfo()
{
   return getCustomSROAInfo();
}
#else
#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
#if PANDA_LLVM_CLANG_MAJOR >= 11
   PM.add(llvm::createInstructionCombiningPass(1000));
#else
   PM.add(llvm::createInstructionCombiningPass(true));
#endif
   PM.add(new llvm::CustomSROA());
   // PM.add(new llvm::ScalarizeSingletonArrayPass());
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses customsroa_Ox(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);
#endif
#endif
