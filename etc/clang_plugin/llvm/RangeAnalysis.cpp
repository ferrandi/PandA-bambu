//===-------------------------- RangeAnalysis.cpp -------------------------===//
//===----- Performs a range analysis of the variables of the function -----===//
//
//					 The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Copyright (C) 2011-2012, 2015, 2017  Victor Hugo Sperle Campos
//               2011               	  Douglas do Couto Teixeira
//               2012               	  Igor Rafael de Assis Costa
//
//===----------------------------------------------------------------------===//

// The code have been revised and extended by Fabrizio Ferrandi from Politecnico di Milan.
// In particular, it has ben added:
// - Added anti range support
// - Fixed select reduction of the range.
// - Integrated the LLVM lazy value range analysis.



#include "RangeAnalysis.hpp"

#include <stdint.h>
#include <cassert>
#include <iterator>
#include <string>
#include <system_error>

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ilist_iterator.h"
#include "llvm/ADT/iterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/LazyValueInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/PassSupport.h"
#include "llvm/Support/FileSystem.h"
#if __clang_major__ != 4
#include "llvm/Support/KnownBits.h"
#endif

int __builtin_clzll(unsigned long long int);

#define DEBUG_TYPE "range-analysis"

//************************** Log Transactions ********************************//
#ifdef LOG_TRANSACTIONS
std::error_code _log_ErrorInfo;
llvm::Twine _log_fileName = "/tmp/ratransactions";
llvm::raw_fd_ostream _log_file(_log_fileName.str().c_str(), _log_ErrorInfo,
                               llvm::sys::fs::F_Text);
#define LOG_TRANSACTION(str) _log_file << str << "\n"
#define FINISH_LOG _log_file.close();
#else
#define LOG_TRANSACTION(str)
#define FINISH_LOG
#endif

using namespace llvm;

namespace RangeAnalysis {

   using namespace llvm;

   // These macros are used to get stats regarding the precision of our analysis.
   STATISTIC(usedBits, "Initial number of bits.");
   STATISTIC(needBits, "Needed bits.");
   STATISTIC(percentReduction, "Percentage of reduction of the number of bits.");
   STATISTIC(numSCCs, "Number of strongly connected components.");
   STATISTIC(numAloneSCCs, "Number of SCCs containing only one node.");
   STATISTIC(sizeMaxSCC, "Size of largest SCC.");
   STATISTIC(numVars, "Number of variables");
   STATISTIC(numUnknown, "Number of unknown variables");
   STATISTIC(numEmpty, "Number of empty-set variables");
   STATISTIC(numCPlusInf, "Number of variables [c, +inf].");
   STATISTIC(numCC, "Number of variables [c, c].");
   STATISTIC(numMinInfC, "Number of variables [-inf, c].");
   STATISTIC(numMaxRange, "Number of variables [-inf, +inf].");
   STATISTIC(numConstants, "Number of constants.");
   STATISTIC(numZeroUses, "Number of variables without any use.");
   STATISTIC(numNotInt, "Number of variables that are not Integer.");
   STATISTIC(numOps, "Number of operations");
   STATISTIC(maxVisit, "Max number of times a value has been visited.");

   namespace {
      // The number of bits needed to store the largest variable of the function
      // (APInt).
      unsigned MAX_BIT_INT = 1;

      // This map is used to store the number of times that the narrow_meet
      // operator is called on a variable. It was a Fernando's suggestion.
      // TODO(vhscampos): remove this
      DenseMap<const Value *, unsigned> FerMap;

      // ========================================================================== //
      // Static global functions and definitions
      // ========================================================================== //

      APInt Min, Max, Zero, One;

      // String used to identify sigmas
      // IMPORTANT: the range-analysis identifies sigmas by comparing
      // to this hard-coded instruction name prefix.
      // TODO(vhscampos): remove this as we will migrate to PredicateInfo
      const std::string sigmaString = "vSSA_sigma";

      // Used to print pseudo-edges in the Constraint Graph dot
      std::string pestring;
      raw_string_ostream pseudoEdgesString(pestring);

#ifdef STATS
      // Used to profile
      Profile prof;
#endif

      // Print name of variable according to its type
      void printVarName(const Value *V, raw_ostream &OS) {
         const Argument *A = nullptr;
         const Instruction *I = nullptr;

         if ((A = dyn_cast<Argument>(V)) != nullptr) {
            const llvm::Function *currentFunction = A->getParent();
            llvm::ModuleSlotTracker MST(currentFunction->getParent());
            MST.incorporateFunction(*currentFunction);
            auto argID = MST.getLocalSlot(A);
            OS << A->getParent()->getName() << ".%" << argID;
         } else if ((I = dyn_cast<Instruction>(V)) != nullptr) {
            llvm::ModuleSlotTracker MST(I->getParent()->getParent()->getParent());
            MST.incorporateFunction(*I->getParent()->getParent());
            auto instID= MST.getLocalSlot(I);
            auto BBID=MST.getLocalSlot(I->getParent());

            OS << I->getFunction()->getName() << ".BB"
               << BBID << ".%" << instID;
         } else {
            OS << V->getName();
         }
      }

      /// Selects the instructions that we are going to evaluate.
      bool isValidInstruction(const Instruction *I) {
         switch (I->getOpcode()) {
            case Instruction::PHI:
            case Instruction::Add:
            case Instruction::Sub:
            case Instruction::Mul:
            case Instruction::UDiv:
            case Instruction::SDiv:
            case Instruction::URem:
            case Instruction::SRem:
            case Instruction::Shl:
            case Instruction::LShr:
            case Instruction::AShr:
            case Instruction::And:
            case Instruction::Or:
            case Instruction::Xor:
            case Instruction::Trunc:
            case Instruction::ZExt:
            case Instruction::SExt:
            case Instruction::Select:
            case Instruction::ICmp:
            case Instruction::FPToSI:
            case Instruction::FPToUI:
               return true;
            default:
               return false;
         }
      }

      Range getLLVM_range(Instruction *I, ModulePass *modulePass, const llvm::DataLayout* DL)
      {
         auto bw = I->getType()->getPrimitiveSizeInBits();
         if(bw==1)
            return Range(Regular,bw,Zero,One);
         if(modulePass==nullptr || DL==nullptr)
            return Range(Regular, bw);
         auto Fun = I->getFunction();
         assert(modulePass);
         unsigned long long int zeroMask=0;
#if __clang_major__ != 4
         llvm::KnownBits KnownOneZero;
         auto AC = modulePass->getAnalysis<llvm::AssumptionCacheTracker>().getAssumptionCache(*Fun);
         auto& DT = modulePass->getAnalysis<llvm::DominatorTreeWrapperPass>(*Fun).getDomTree();
         KnownOneZero = llvm::computeKnownBits(I, *DL, 0, &AC, I, &DT);
         zeroMask = KnownOneZero.Zero.getZExtValue();
#else
//         llvm::APInt KnownZero;
//         llvm::APInt KnownOne;
//         auto AC = modulePass->getAnalysis<llvm::AssumptionCacheTracker>().getAssumptionCache(*Fun);
//         const auto& DT = modulePass->getAnalysis<llvm::DominatorTreeWrapperPass>(*Fun).getDomTree();
//         llvm::computeKnownBits(I,KnownZero, KnownOne, *DL, 0, &AC, I, &DT);
//         zeroMask = KnownZero.getZExtValue();
#endif
         llvm::LazyValueInfo &LVI = modulePass->getAnalysis<llvm::LazyValueInfoWrapperPass>(*Fun).getLVI();
         llvm::ConstantRange range = LVI.getConstantRange(I, I->getParent(), I);
         auto ty = I->getType();
         auto obj_size = ty->isSized() ? DL->getTypeAllocSizeInBits(ty) : 8ULL;
         auto active_size = ty->isSized() ? DL->getTypeSizeInBits(ty) : 8ULL;
         auto i=active_size;
         for (;i>1;--i)
         {
            if((zeroMask&(1ULL<<(i-1)))==0)
               break;
         }
         if(i!=active_size)
         {
            ++i;
            if(i<active_size)
               active_size = i;
         }
         if(obj_size != active_size)
         {
            assert(active_size<64);

            APInt l(MAX_BIT_INT, -(1LL << (active_size-1)),true);
            if(range.getSignedMin().sext(MAX_BIT_INT).sgt(l))
               l = range.getSignedMin().sext(MAX_BIT_INT);
            APInt u(MAX_BIT_INT, (1LL << (active_size-1)),true);
            if(range.getSignedMax().sext(MAX_BIT_INT).slt(u))
               u = range.getSignedMax().sext(MAX_BIT_INT);
            return Range(Regular, bw,l,u);

         }
         else if(!range.isFullSet())
         {
            auto l = range.getLower();
            auto u = range.getUpper();
            if(range.isEmptySet())
               return Range(Empty,bw,Min,Max);
            else if(range.isSignWrappedSet())
               return Range(Anti,bw,u.sext(MAX_BIT_INT),(l-1).sext(MAX_BIT_INT));
            else if(!range.isWrappedSet())
               return Range(Regular,bw,l.zext(MAX_BIT_INT),(u-1).zext(MAX_BIT_INT));
            else if(!range.isSignWrappedSet())
               return Range(Regular,bw,l.sext(MAX_BIT_INT),(u-1).sext(MAX_BIT_INT));
            else
               llvm_unreachable("unexpected case");
         }
         else
            return Range(Regular,bw);
      }
   } // end anonymous namespace



   // ========================================================================== //
   // RangeAnalysis
   // ========================================================================== //
   unsigned RangeAnalysis::getMaxBitWidth(const Function &F) {
      unsigned int InstBitSize = 0, opBitSize = 0, max = 0;

      // Obtains the maximum bit width of the instructions of the function.
      for (const Instruction &I : instructions(F)) {
         InstBitSize = I.getType()->getPrimitiveSizeInBits();
         if (I.getType()->isIntegerTy() && InstBitSize > max) {
            max = InstBitSize;
         }

         // Obtains the maximum bit width of the operands of the instruction.
         for (const Value *operand : I.operands()) {
            opBitSize = operand->getType()->getPrimitiveSizeInBits();
            if (operand->getType()->isIntegerTy() && opBitSize > max) {
               max = opBitSize;
            }
         }
      }

      // Bitwidth equal to 0 is not valid, so we increment to 1
      if (max == 0) {
         ++max;
      }

      return max;
   }

   void RangeAnalysis::updateConstantIntegers(unsigned maxBitWidth) {
      // Updates the Min and Max values.
      Min = APInt::getSignedMinValue(maxBitWidth);
      Max = APInt::getSignedMaxValue(maxBitWidth);
      Zero = APInt(MAX_BIT_INT, 0UL, true);
      One = APInt(MAX_BIT_INT, 1UL, true);
   }

   // unsigned RangeAnalysis::getBitWidth() {
   //	return MAX_BIT_INT;
   //}

   // ========================================================================== //
   // IntraProceduralRangeAnalysis
   // ========================================================================== //
   template<class CGT>
   char IntraProceduralRA<CGT>::ID = 0;

   template <class CGT> APInt IntraProceduralRA<CGT>::getMin() { return Min; }

   template <class CGT> APInt IntraProceduralRA<CGT>::getMax() { return Max; }

   template <class CGT> Range IntraProceduralRA<CGT>::getRange(const Value *v) {
      return CG->getRange(v);
   }

   template <class CGT> bool IntraProceduralRA<CGT>::runOnFunction(Function &F) {
      //	if(CG) delete CG;
      CG = new CGT();

      MAX_BIT_INT = getMaxBitWidth(F);
      updateConstantIntegers(MAX_BIT_INT);

      // Build the graph and find the intervals of the variables.
#ifdef STATS
      Timer *timer = prof.registerNewTimer("BuildGraph", "Build constraint graph");
      timer->startTimer();
#endif
      CG->buildGraph(F,nullptr,nullptr);
      CG->buildVarNodes();
#ifdef STATS
      timer->stopTimer();
      prof.addTimeRecord(timer);

      prof.registerMemoryUsage();
#endif
#ifdef DEBUG_RA
      CG->printToFile(F, "/tmp/" + F.getName().str() + "cgpre.dot");
      errs() << "Analyzing function " << F.getName() << ":\n";
#endif
      CG->findIntervals();
#ifdef DEBUG_RA
      CG->printToFile(F, "/tmp/" + F.getName().str() + "cgpos.dot");
#endif

      return false;
   }

   template <class CGT>
   void IntraProceduralRA<CGT>::getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
   }

   template <class CGT> IntraProceduralRA<CGT>::~IntraProceduralRA() {
#ifdef STATS
      prof.printTime("BuildGraph");
      prof.printTime("Nuutila");
      prof.printTime("SCCs resolution");
      prof.printTime("ComputeStats");
      prof.printMemoryUsage();

      std::ostringstream formated;
      formated << 100 * (1.0 - (static_cast<double>(needBits) / usedBits));
      errs() << formated.str() << "\t - "
             << " Percentage of reduction\n";

      // max visit computation
      unsigned maxtimes = 0;
      for (auto &pair : FerMap) {
         unsigned times = pair.second;
         if (times > maxtimes) {
            maxtimes = times;
         }
      }
      maxVisit = maxtimes;
#endif
      //	delete CG;
   }

   // ========================================================================== //
   // InterProceduralRangeAnalysis
   // ========================================================================== //
   template<class CGT>
   char InterProceduralRA<CGT>::ID = 0;

   template <class CGT> APInt InterProceduralRA<CGT>::getMin() { return Min; }

   template <class CGT> APInt InterProceduralRA<CGT>::getMax() { return Max; }

   template <class CGT> Range InterProceduralRA<CGT>::getRange(const Value *v) {
      return CG->getRange(v);
   }

   template <class CGT>
   unsigned InterProceduralRA<CGT>::getMaxBitWidth(Module &M) {
      unsigned max = 0U;
      // Search through the functions for the max int bitwidth
      for (const Function &F : M.functions()) {
         if (!F.isDeclaration()) {
            unsigned bitwidth = RangeAnalysis::getMaxBitWidth(F);

            if (bitwidth > max) {
               max = bitwidth;
            }
         }
      }
      return max;
   }

   template <class CGT> bool InterProceduralRA<CGT>::runOnModule(Module &M) {
      // Constraint Graph
      //	if(CG) delete CG;
      CG = new CGT();

      MAX_BIT_INT = getMaxBitWidth(M);
      updateConstantIntegers(MAX_BIT_INT);

      // Build the Constraint Graph by running on each function
#ifdef STATS
      Timer *timer = prof.registerNewTimer("BuildGraph", "Build constraint graph");
      timer->startTimer();
#endif
      for (Function &F : M.functions()) {
         // If the function is only a declaration, or if it has variable number of
         // arguments, do not match
         if (F.isDeclaration() || F.isVarArg()) {
            continue;
         }

         CG->buildGraph(F,nullptr,nullptr);
         MatchParametersAndReturnValues(F, *CG);
      }
      CG->buildVarNodes();

#ifdef STATS
      timer->stopTimer();
      prof.addTimeRecord(timer);

      prof.registerMemoryUsage();
#endif
#ifdef DEBUG_RA
      std::string moduleIdentifier = M.getModuleIdentifier();
      int pos = moduleIdentifier.rfind('/');
      std::string mIdentifier =
            pos > 0 ? moduleIdentifier.substr(pos) : moduleIdentifier;
      CG->printToFile(*(M.begin()), "/tmp/" + mIdentifier + ".cgpre.dot");
#endif
      CG->findIntervals();
#ifdef DEBUG_RA
      CG->printToFile(*(M.begin()), "/tmp/" + mIdentifier + ".cgpos.dot");
#endif

      return false;
   }

   template <class CGT>
   void InterProceduralRA<CGT>::getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
   }

   template <class CGT>
   void InterProceduralRA<CGT>::MatchParametersAndReturnValues(
         Function &F, ConstraintGraph &G) {
      // Only do the matching if F has any use
      if (!F.hasNUsesOrMore(1)) {
         return;
      }

      // Data structure which contains the matches between formal and real
      // parameters
      // First: formal parameter
      // Second: real parameter
      SmallVector<std::pair<Value *, Value *>, 4> parameters(F.arg_size());

      // Fetch the function arguments (formal parameters) into the data structure
      Function::arg_iterator argptr;
      Function::arg_iterator e;
      unsigned i;

      for (i = 0, argptr = F.arg_begin(), e = F.arg_end(); argptr != e;
           ++i, ++argptr) {
         parameters[i].first = &*argptr;
      }

      // Check if the function returns a supported value type. If not, no return
      // value matching is done
      bool noReturn = F.getReturnType()->isVoidTy();

      // Creates the data structure which receives the return values of the
      // function, if there is any
      SmallPtrSet<Value *, 4> returnValues;

      if (!noReturn) {
         // Iterate over the basic blocks to fetch all possible return values
         for (BasicBlock &BB : F) {
            // Get the terminator instruction of the basic block and check if it's
            // a return instruction: if it's not, continue to next basic block
            Instruction *terminator = BB.getTerminator();

            ReturnInst *RI = dyn_cast<ReturnInst>(terminator);

            if (RI == nullptr) {
               continue;
            }

            // Get the return value and insert in the data structure
            returnValues.insert(RI->getReturnValue());
         }
      }

      // For each use of F, get the real parameters and the caller instruction to do
      // the matching
      SmallVector<PhiOp *, 4> matchers(F.arg_size(), nullptr);

      for (unsigned i = 0, e = parameters.size(); i < e; ++i) {
         VarNode *sink = G.addVarNode(parameters[i].first);

         matchers[i] = new PhiOp(new BasicInterval(), sink, nullptr);

         // Insert the operation in the graph.
         G.getOprs()->insert(matchers[i]);

         // Insert this definition in defmap
         (*G.getDefMap())[sink->getValue()] = matchers[i];
      }

      // For each return value, create a node
      SmallVector<VarNode *, 4> returnVars;

      for (Value *returnValue : returnValues) {
         // Add VarNode to the CG
         VarNode *from = G.addVarNode(returnValue);

         returnVars.push_back(from);
      }

      for (Use &U : F.uses()) {
         User *Us = U.getUser();

         // Ignore blockaddress uses
         if (isa<BlockAddress>(Us)) {
            continue;
         }

         // Used by a non-instruction, or not the callee of a function, do not
         // match.
         if (!isa<CallInst>(Us) && !isa<InvokeInst>(Us)) {
            continue;
         }

         Instruction *caller = cast<Instruction>(Us);

         CallSite CS(caller);

         if (!CS.isCallee(&U)) {
            continue;
         }

         // Iterate over the real parameters and put them in the data structure
         CallSite::arg_iterator AI;
         CallSite::arg_iterator EI;

         for (i = 0, AI = CS.arg_begin(), EI = CS.arg_end(); AI != EI; ++i, ++AI) {
            parameters[i].second = *AI;
         }

         // // Do the interprocedural construction of CG
         VarNode *to = nullptr;
         VarNode *from = nullptr;

         // Match formal and real parameters
         for (i = 0; i < parameters.size(); ++i) {
            // Add real parameter to the CG
            from = G.addVarNode(parameters[i].second);

            // Connect nodes
            matchers[i]->addSource(from);

            // Inserts the sources of the operation in the use map list.
            G.getUseMap()->find(from->getValue())->second.insert(matchers[i]);
         }

         // Match return values
         if (!noReturn) {
            // Add caller instruction to the CG (it receives the return value)
            to = G.addVarNode(caller);

            PhiOp *phiOp = new PhiOp(new BasicInterval(), to, nullptr);

            // Insert the operation in the graph.
            G.getOprs()->insert(phiOp);

            // Insert this definition in defmap
            (*G.getDefMap())[to->getValue()] = phiOp;

            for (VarNode *var : returnVars) {
               phiOp->addSource(var);

               // Inserts the sources of the operation in the use map list.
               G.getUseMap()->find(var->getValue())->second.insert(phiOp);
            }
         }

         // Real parameters are cleaned before moving to the next use (for safety's
         // sake)
         for (auto &pair : parameters) {
            pair.second = nullptr;
         }
      }
   }

   template <class CGT> InterProceduralRA<CGT>::~InterProceduralRA() {
#ifdef STATS
      // TODO(vhscampos): Deprecated by the usage of llvm::Timer
      //  prof.printTime("BuildGraph");
      //  prof.printTime("Nuutila");
      //  prof.printTime("SCCs resolution");
      //  prof.printTime("ComputeStats");
      prof.printMemoryUsage();

      std::ostringstream formated;
      formated << 100 * (1.0 - (static_cast<double>(needBits) / usedBits));
      errs() << formated.str() << "\t - "
             << " Percentage of reduction\n";

      // max visit computation
      unsigned maxtimes = 0;
      for (auto &pair : FerMap) {
         unsigned times = pair.second;
         if (times > maxtimes) {
            maxtimes = times;
         }
      }
      maxVisit = maxtimes;
#endif
   }

   static RegisterPass<IntraProceduralRA<Cousot>>
   Y("ra-intra-cousot", "Range Analysis (Cousot - intra)");
   static RegisterPass<IntraProceduralRA<CropDFS>>
   Z("ra-intra-crop", "Range Analysis (Crop - intra)");
   static RegisterPass<InterProceduralRA<Cousot>>
   W("ra-inter-cousot", "Range Analysis (Cousot - inter)");
   static RegisterPass<InterProceduralRA<CropDFS>>
   X("ra-inter-crop", "Range Analysis (Crop - inter)");

   // ========================================================================== //
   // Range
   // ========================================================================== //
   Range_base::Range_base(RangeType rType, unsigned rbw) : l(Min), u(Max), bw(rbw), type(rType) {}

   void Range_base::normalizeRange(const APInt &lb, const APInt &ub, RangeType rType)
   {
      if(rType==Empty)
      {
         l = Min;
         u = Max;
      }
      else if(rType==Anti)
      {
         assert(lb.sle(ub));
         if(lb.eq(Min) && ub.eq(Max))
            type = Empty;
         else if(lb.eq(Min))
         {
            type = Regular;
            l = ub+1;
            u = Max;
         }
         else if(ub.eq(Max))
         {
            type = Regular;
            l = Min;
            u = lb-1;
         }
      }
      else if (lb.sgt(ub))
      {
         type = Empty;
         l = Min;
         u = Max;
      }
      assert(u.sge(l));
   }

   unsigned Range_base::neededBits(const llvm::APInt& a, const llvm::APInt& b, bool sign) const
   {
      unsigned nbit;
      if(sign)
      {
         auto as = a.getSExtValue();
         auto bs = b.getSExtValue();
         assert(as<=bs);
         unsigned long long new_max;
         if(as < 0)
            new_max = static_cast<unsigned long long>(bs > -(1+as) ? bs : -(1+as));
         else
            new_max = static_cast<unsigned long long>(as);
         for(nbit = 1; (new_max >= (1ull<<nbit)) && (nbit < 64); ++nbit);
         ++nbit;
      }
      else
      {
         auto au = a.getZExtValue();
         auto bu = b.getZExtValue();
         au = std::max(au,bu);
         for(nbit = 1; (au >= (1ull<<nbit)) && (nbit < 64); ++nbit);
      }
      return nbit;
   }

   Range_base::Range_base(RangeType rType, unsigned rbw, const APInt &lb, const APInt &ub)
      : l(lb), u(ub), bw(rbw), type(rType) {
      normalizeRange(lb,ub,rType);
   }

   Range_base Range_base::getAnti(const Range_base &o)
   {
      if(o.type==Anti)
         return Range_base(Regular,o.bw, o.l,o.u);
      else if(o.type==Regular)
         return Range_base(Anti,o.bw, o.l,o.u);
      else if(o.type==Empty)
         return Range_base(Regular,o.bw, Min,Max);
      else if (o.type==Unknown)
         return o;
      else
         llvm_unreachable("unexpected condition");
   }

   unsigned int Range_base::getBitWidth() const
   {
      return bw;
   }

   const APInt Range_base::getLower() const
   {
      if (type!=Anti) return l;
      if(l.eq(u))
         return Min;
      ConstantRange tmpT(l,u+1);
      ConstantRange tmpF = tmpT.inverse();
      auto sigMin = tmpF.getSignedMin();
      auto sigMax = tmpF.getSignedMax();
      assert (sigMin.getBitWidth() == MAX_BIT_INT);
      assert (sigMax.getBitWidth() == MAX_BIT_INT);
      assert(sigMax.sge(sigMin));
      return sigMin;
   }

   const APInt Range_base::getUpper() const
   {
      if (type!=Anti) return u;
      if(l.eq(u))
         return Max;
      ConstantRange tmpT(l,u+1);
      ConstantRange tmpF = tmpT.inverse();
      auto sigMax = tmpF.getSignedMax();
      assert (sigMax.getBitWidth() == MAX_BIT_INT);
      return sigMax;
   }

   const APInt Range_base::getSignedMax() const
   {
      if(type!=Anti)
      {
         auto maxS = APInt::getSignedMaxValue(bw);
         if (maxS.getBitWidth() < MAX_BIT_INT)
            maxS = maxS.sext(MAX_BIT_INT);
         if(u.sgt(maxS))
            return APInt::getSignedMaxValue(bw);
         else
            return APInt(bw,u.getSExtValue(),true);
      }
      else
         return APInt::getSignedMaxValue(bw);
   }
   const APInt Range_base::getSignedMin() const
   {
      if(type!=Anti)
      {
         auto minS = APInt::getSignedMinValue(bw);
         if (minS.getBitWidth() < MAX_BIT_INT)
            minS = minS.sext(MAX_BIT_INT);
         if(l.slt(minS))
            return APInt::getSignedMinValue(bw);
         else
            return APInt(bw,l.getSExtValue(),true);
      }
      else
         return APInt::getSignedMinValue(bw);
   }
   const APInt Range_base::getUnsignedMax() const
   {
      if(type!=Anti)
      {
         auto maxU = APInt::getMaxValue(bw);
         if (maxU.getBitWidth() < MAX_BIT_INT)
            maxU = maxU.zext(MAX_BIT_INT);
         if(u.sgt(maxU) || l.slt(Zero))
            return APInt::getMaxValue(bw);
         else
            return APInt(bw,u.getZExtValue(),false);
      }
      else
         return APInt::getMaxValue(bw);
   }
   const APInt Range_base::getUnsignedMin() const
   {
      if(type!=Anti)
      {
         if(l.slt(Zero))
            return APInt(bw,0,false);
         else
            return APInt(bw,l.getSExtValue(),false);
      }
      else
         return APInt(bw,0,false);
   }

   bool Range::isMaxRange() const
   {
      if(isAnti())
         return false;
      return this->getLower().eq(Min) && this->getUpper().eq(Max);
   }

   /// Add and Mul are commutative. So, they are a little different
   /// than the other operations.
   Range Range::add(const Range &other) const
   {
      if (isEmpty()||isUnknown()||isMaxRange())
         return *this;
      if (other.isEmpty()||other.isUnknown()||other.isMaxRange())
         return other;
      if(this->isAnti() || other.isAnti())
         return Range(Regular,getBitWidth(),Min,Max);

      auto this_min = getSignedMin();
      auto this_max = getSignedMax();
      auto other_min = other.getSignedMin();
      auto other_max = other.getSignedMax();
      ConstantRange thisR = ConstantRange(this_min,this_max+1);
      ConstantRange otherR = ConstantRange(other_min,other_max+1);

      auto thisU_min = getUnsignedMin();
      auto thisU_max = getUnsignedMax();
      auto otherU_min = other.getUnsignedMin();
      auto otherU_max = other.getUnsignedMax();
      ConstantRange thisUR = ConstantRange(thisU_min,thisU_max+1);
      ConstantRange otherUR = ConstantRange(otherU_min,otherU_max+1);

      auto AddU = thisR.add(otherR);
      auto AddS = thisR.add(otherR);
      if(AddU.isFullSet() && AddS.isFullSet())
         return Range(Regular, getBitWidth(), Min, Max);
      else if(AddU.isFullSet())
         return Range(Regular, getBitWidth(), AddS.getSignedMin().sext(MAX_BIT_INT), (AddS.getSignedMax()).sext(MAX_BIT_INT));
      else if(AddS.isFullSet())
         return Range(Regular, getBitWidth(), AddS.getUnsignedMin().zext(MAX_BIT_INT), AddS.getUnsignedMax().sext(MAX_BIT_INT));
      else
         return BestRange(AddU,AddS,getBitWidth());
   }

   /// [a, b] − [c, d] =
   /// [min (a − c, a − d, b − c, b − d),
   /// max (a − c, a − d, b − c, b − d)] = [a − d, b − c]
   /// The other operations are just like this.
   Range Range::sub(const Range &other) const
   {
      if (isEmpty()||isUnknown()||isMaxRange())
         return *this;
      if (other.isEmpty()||other.isUnknown()||other.isMaxRange())
         return other;
      if(this->isAnti() || other.isAnti())
         return Range(Regular,getBitWidth(),Min,Max);

      auto this_min = getSignedMin();
      auto this_max = getSignedMax();
      auto other_min = other.getSignedMin();
      auto other_max = other.getSignedMax();
      ConstantRange thisR = ConstantRange(this_min,this_max+1);
      ConstantRange otherR = ConstantRange(other_min,other_max+1);

      auto thisU_min = getUnsignedMin();
      auto thisU_max = getUnsignedMax();
      auto otherU_min = other.getUnsignedMin();
      auto otherU_max = other.getUnsignedMax();
      ConstantRange thisUR = ConstantRange(thisU_min,thisU_max+1);
      ConstantRange otherUR = ConstantRange(otherU_min,otherU_max+1);

      auto SubU = thisR.sub(otherR);
      auto SubS = thisR.sub(otherR);
      if(SubU.isFullSet() && SubS.isFullSet())
         return Range(Regular, getBitWidth(), Min, Max);
      else if(SubU.isFullSet())
      {
         assert(SubS.getSignedMin().sext(MAX_BIT_INT).sle((SubS.getSignedMax()).sext(MAX_BIT_INT)));
         return Range(Regular, getBitWidth(), SubS.getSignedMin().sext(MAX_BIT_INT), (SubS.getSignedMax()).sext(MAX_BIT_INT));
      }
      else if(SubS.isFullSet())
      {
         assert(SubU.getUnsignedMin().zext(MAX_BIT_INT).ule(SubU.getUnsignedMax().zext(MAX_BIT_INT)));
         return Range(Regular, getBitWidth(), SubS.getUnsignedMin().zext(MAX_BIT_INT), SubS.getUnsignedMax().sext(MAX_BIT_INT));
      }
      else
         return BestRange(SubU,SubS,getBitWidth());
   }

   /// Add and Mul are commutatives. So, they are a little different
   /// of the other operations.
   // [a, b] * [c, d] = [Min(a*c, a*d, b*c, b*d), Max(a*c, a*d, b*c, b*d)]
   Range Range::mul(const Range &other) const
   {
      if (isEmpty()||isUnknown()||isMaxRange())
         return *this;
      if (other.isEmpty()||other.isUnknown()||other.isMaxRange())
         return other;
      if(this->isAnti() || other.isAnti())
         return Range(Regular,getBitWidth(),Min,Max);

      // Multiplication is signedness-independent. However different ranges can be
      // obtained depending on how the input ranges are treated. These different
      // ranges are all conservatively correct, but one might be better than the
      // other. We calculate two ranges; one treating the inputs as unsigned
      // and the other signed, then return the smallest of these ranges.

      // Unsigned range first.
      APInt this_min = getUnsignedMin().zext(getBitWidth() * 2);
      APInt this_max = getUnsignedMax().zext(getBitWidth() * 2);
      APInt Other_min = other.getUnsignedMin().zext(getBitWidth() * 2);
      APInt Other_max = other.getUnsignedMax().zext(getBitWidth() * 2);

      ConstantRange Result_zext = ConstantRange(this_min * Other_min,
                                                this_max * Other_max + 1);
      ConstantRange UR = Result_zext.truncate(getBitWidth());

      // Now the signed range. Because we could be dealing with negative numbers
      // here, the lower bound is the smallest of the cartesian product of the
      // lower and upper ranges; for example:
      //   [-1,4) * [-2,3) = min(-1*-2, -1*2, 3*-2, 3*2) = -6.
      // Similarly for the upper bound, swapping min for max.

      this_min = getSignedMin().sext(getBitWidth() * 2);
      this_max = getSignedMax().sext(getBitWidth() * 2);
      Other_min = other.getSignedMin().sext(getBitWidth() * 2);
      Other_max = other.getSignedMax().sext(getBitWidth() * 2);

      auto L = {this_min * Other_min, this_min * Other_max,
                this_max * Other_min, this_max * Other_max};
      auto Compare = [](const APInt &A, const APInt &B) { return A.slt(B); };
      ConstantRange Result_sext(std::min(L, Compare), std::max(L, Compare) + 1);
      ConstantRange SR = Result_sext.truncate(getBitWidth());
      return BestRange(UR,SR,getBitWidth());
   }

#define DIV_HELPER(OP, x, y)                                                   \
   (x).eq(Max)                                                                  \
   ? ((y).slt(Zero) ? Min : ((y).eq(Zero) ? Zero : Max))                    \
   : ((y).eq(Max)                                                           \
   ? ((x).slt(Zero) ? Min : ((x).eq(Zero) ? Zero : Max))             \
   : ((x).eq(Min)                                                    \
   ? ((y).slt(Zero) ? Max : ((y).eq(Zero) ? Zero : Min))      \
   : ((y).eq(Min)                                             \
   ? ((x).slt(Zero) ? Max                              \
   : ((x).eq(Zero) ? Zero : Min))     \
   : ((x).OP((y))))))

   Range Range::udiv(const Range &other) const
   {
      if (isEmpty()||isUnknown()||isMaxRange())
         return *this;
      if (other.isEmpty()||other.isUnknown()||other.isMaxRange())
         return other;
      if(this->isAnti() || other.isAnti())
         return Range(Regular,getBitWidth(),Min,Max);

      llvm::errs() << "this:";
      this->print(llvm::errs());
      llvm::errs() << "\n";
      llvm::errs() << "other:";
      other.print(llvm::errs());
      llvm::errs() << "\n";
      const APInt a = this->getLower();
      const APInt b = this->getUpper();
      APInt c = other.getLower();
      const APInt d = other.getUpper();

      // Deal with division by 0 exception
      if (c.eq(Zero) && d.eq(Zero))
         return Range(Regular,getBitWidth(), Min, Max);
      else if(c.eq(Zero))
         c = APInt(MAX_BIT_INT,1);

      llvm::errs() << "0\n";
      APInt candidates[4];
      // value[1]: lb(c) / leastpositive(d)
      candidates[0] = DIV_HELPER(udiv, a, c);
      llvm::errs() << "1\n";
      candidates[1] = DIV_HELPER(udiv, a, d);
      llvm::errs() << "2\n";
      candidates[2] = DIV_HELPER(udiv, b, c);
      llvm::errs() << "3\n";
      candidates[3] = DIV_HELPER(udiv, b, d);
      llvm::errs() << "4\n";
      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt *min = &candidates[0];
      APInt *max = &candidates[0];
      for (unsigned i = 1; i < 4; ++i)
      {
         if (candidates[i].sgt(*max))
            max = &candidates[i];
         else if (candidates[i].slt(*min))
            min = &candidates[i];
      }
      return Range(Regular,getBitWidth(),*min,*max);
   }

   Range Range::sdiv(const Range &other) const
   {
      if (isEmpty()||isUnknown()||isMaxRange())
         return *this;
      if (other.isEmpty()||other.isUnknown()||other.isMaxRange())
         return other;
      if(this->isAnti() || other.isAnti())
         return Range(Regular,getBitWidth(),Min,Max);

      const APInt &a = this->getLower();
      const APInt &b = this->getUpper();
      APInt c = other.getLower();
      const APInt &d = other.getUpper();

      // Deal with division by 0 exception
      if (c.eq(Zero) && d.eq(Zero))
         return Range(Regular,getBitWidth(), Min, Max);
      else if(c.eq(Zero))
         c = APInt(MAX_BIT_INT,1);

      APInt candidates[4];
      candidates[0] = DIV_HELPER(sdiv, a, c);
      candidates[1] = DIV_HELPER(sdiv, a, d);
      candidates[2] = DIV_HELPER(sdiv, b, c);
      candidates[3] = DIV_HELPER(sdiv, b, d);
      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt *min = &candidates[0];
      APInt *max = &candidates[0];
      for (unsigned i = 1; i < 4; ++i)
      {
         if (candidates[i].sgt(*max))
            max = &candidates[i];
         else if (candidates[i].slt(*min))
            min = &candidates[i];
      }
      return Range(Regular,getBitWidth(), *min, *max);
   }

   Range Range::urem(const Range &other) const
   {
      if (isEmpty()||isUnknown()||isMaxRange())
         return *this;
      if (other.isEmpty()||other.isUnknown()||other.isMaxRange())
         return other;
      if(this->isAnti() || other.isAnti())
         return Range(Regular,getBitWidth(),Min,Max);

      const APInt &a = this->getLower();
      const APInt &b = this->getUpper();
      APInt c = other.getLower();
      const APInt &d = other.getUpper();

      // Deal with mod 0 exception
      if (c.eq(Zero) && d.eq(Zero))
         return Range(Regular,getBitWidth(),Min,Max);
      else if(c.eq(Zero))
         c = APInt(MAX_BIT_INT,1);

      APInt candidates[4];
      candidates[0] = Min;
      candidates[1] = Min;
      candidates[2] = Max;
      candidates[3] = Max;

      if (a.ne(Min) && c.ne(Min))
         candidates[0] = a.urem(c); // lower lower
      if (a.ne(Min) && d.ne(Max))
         candidates[1] = a.urem(d); // lower upper
      if (b.ne(Max) && c.ne(Min))
         candidates[2] = b.urem(c); // upper lower
      if (b.ne(Max) && d.ne(Max))
         candidates[3] = b.urem(d); // upper upper

      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt *min = &candidates[0];
      APInt *max = &candidates[0];
      for (unsigned i = 1; i < 4; ++i)
      {
         if (candidates[i].sgt(*max))
            max = &candidates[i];
         else if (candidates[i].slt(*min))
            min = &candidates[i];
      }
      return Range(Regular,getBitWidth(),*min,*max);
   }

   Range Range::srem(const Range &other) const
   {
      if (isEmpty()||isUnknown()||isMaxRange())
         return *this;
      if (other.isEmpty()||other.isUnknown()||other.isMaxRange())
         return other;
      if(this->isAnti() || other.isAnti())
         return Range(Regular,getBitWidth(),Min,Max);
      if (other == Range(Regular,other.getBitWidth(),Zero,Zero))
         return Range(Empty,getBitWidth(),Min,Max);

      const APInt &a = this->getLower();
      const APInt &b = this->getUpper();
      APInt c = other.getLower();
      const APInt &d = other.getUpper();

      // Deal with mod 0 exception
      if (c.eq(Zero) && d.eq(Zero))
         return Range(Regular,getBitWidth(), Min, Max);
      else if(c.eq(Zero))
         c = APInt(MAX_BIT_INT,1);

      APInt candidates[4];
      candidates[0] = Min;
      candidates[1] = Min;
      candidates[2] = Max;
      candidates[3] = Max;
      if (a.ne(Min) && c.ne(Min))
         candidates[0] = a.srem(c); // lower lower
      if (a.ne(Min) && d.ne(Max))
         candidates[1] = a.srem(d); // lower upper
      if (b.ne(Max) && c.ne(Min))
         candidates[2] = b.srem(c); // upper lower
      if (b.ne(Max) && d.ne(Max))
         candidates[3] = b.srem(d); // upper upper
      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt *min = &candidates[0];
      APInt *max = &candidates[0];
      for (unsigned i = 1; i < 4; ++i)
      {
         if (candidates[i].sgt(*max))
            max = &candidates[i];
          else if (candidates[i].slt(*min))
            min = &candidates[i];
      }
      return Range(Regular,getBitWidth(), *min, *max);
   }

   // Logic has been borrowed from ConstantRange
   Range Range::shl(const Range &other) const
   {
      if (isEmpty()||isUnknown()||isMaxRange())
         return *this;
      if (other.isEmpty()||other.isUnknown()||other.isMaxRange())
         return other;
      if(this->isAnti() || other.isAnti())
         return Range(Regular,getBitWidth(),Min,Max);

      const APInt &a = this->getLower();
      const APInt &b = this->getUpper();
      const APInt &c = other.getLower();
      const APInt &d = other.getUpper();
      APInt Prec(MAX_BIT_INT,getBitWidth());

      if (a.eq(Min) || b.eq(Max) || c.slt(Zero) || d.sge(Prec))
         return Range(Regular,getBitWidth(),Min,Max);

      APInt min = a.shl(c);
      APInt max = b.shl(d);
      APInt Zeros(MAX_BIT_INT, b.countLeadingZeros()-(MAX_BIT_INT-getBitWidth()));
      if (Zeros.ugt(d))
         return Range(Regular,getBitWidth(), min, max);
      // [-inf, +inf]
      return Range(Regular,getBitWidth(),Min,Max);
   }

   // Logic has been borrowed from ConstantRange
   Range Range::lshr(const Range &other) const
   {
      if (isEmpty()||isUnknown())
         return *this;
      if (other.isEmpty()||other.isUnknown())
         return other;

      APInt this_min = getUnsignedMin();
      APInt this_max = getUnsignedMax();
      APInt other_min = other.getUnsignedMin();
      APInt other_max = other.getUnsignedMax();
      const ConstantRange thisR = ConstantRange(this_min,this_max+1);
      const ConstantRange otherR = ConstantRange(other_min,other_max+1);
      auto lshrU = thisR.lshr(otherR);
      if(lshrU.isFullSet())
         return Range(Regular,getBitWidth(), Min, Max);
      assert(lshrU.getLower().ult(lshrU.getUpper()));

      return Range(Regular,getBitWidth(),lshrU.getSignedMin().sext(MAX_BIT_INT),lshrU.getSignedMax().sext(MAX_BIT_INT));
   }

#if __clang_major__ < 6

   static ConstantRange
   local_ashr(const ConstantRange &lthis, const ConstantRange &Other) {
     if (lthis.isEmptySet() || Other.isEmptySet())
       return ConstantRange(lthis.getBitWidth(), /*isFullSet=*/false);

     // May straddle zero, so handle both positive and negative cases.
     // 'PosMax' is the upper bound of the result of the ashr
     // operation, when Upper of the LHS of ashr is a non-negative.
     // number. Since ashr of a non-negative number will result in a
     // smaller number, the Upper value of LHS is shifted right with
     // the minimum value of 'Other' instead of the maximum value.
     APInt PosMax = lthis.getSignedMax().ashr(Other.getUnsignedMin()) + 1;

     // 'PosMin' is the lower bound of the result of the ashr
     // operation, when Lower of the LHS is a non-negative number.
     // Since ashr of a non-negative number will result in a smaller
     // number, the Lower value of LHS is shifted right with the
     // maximum value of 'Other'.
     APInt PosMin = lthis.getSignedMin().ashr(Other.getUnsignedMax());

     // 'NegMax' is the upper bound of the result of the ashr
     // operation, when Upper of the LHS of ashr is a negative number.
     // Since 'ashr' of a negative number will result in a bigger
     // number, the Upper value of LHS is shifted right with the
     // maximum value of 'Other'.
     APInt NegMax = lthis.getSignedMax().ashr(Other.getUnsignedMax()) + 1;

     // 'NegMin' is the lower bound of the result of the ashr
     // operation, when Lower of the LHS of ashr is a negative number.
     // Since 'ashr' of a negative number will result in a bigger
     // number, the Lower value of LHS is shifted right with the
     // minimum value of 'Other'.
     APInt NegMin = lthis.getSignedMin().ashr(Other.getUnsignedMin());

     APInt max, min;
     if (lthis.getSignedMin().isNonNegative()) {
       // Upper and Lower of LHS are non-negative.
       min = PosMin;
       max = PosMax;
     } else if (lthis.getSignedMax().isNegative()) {
       // Upper and Lower of LHS are negative.
       min = NegMin;
       max = NegMax;
     } else {
       // Upper is non-negative and Lower is negative.
       min = NegMin;
       max = PosMax;
     }
     if (min == max)
       return ConstantRange(lthis.getBitWidth(), /*isFullSet=*/true);

     return ConstantRange(std::move(min), std::move(max));
   }
#endif

   Range Range::ashr(const Range &other) const
   {
      if (isEmpty()||isUnknown())
         return *this;
      if (other.isEmpty()||other.isUnknown())
         return other;

      auto this_min = getSignedMin();
      auto this_max = getSignedMax();
      auto other_min = other.getUnsignedMin();
      auto other_max = other.getUnsignedMax();
      const ConstantRange thisR = ConstantRange(this_min,this_max+1);
      const ConstantRange otherR = ConstantRange(other_min,other_max+1);
#if __clang_major__ < 6
      auto AshrU = local_ashr(thisR,otherR);
#else
      auto AshrU = thisR.ashr(otherR);
#endif
      return Range(Regular,getBitWidth(),AshrU.getSignedMin().sext(MAX_BIT_INT),AshrU.getSignedMax().sext(MAX_BIT_INT));
   }

   /*
 * 	This and operation is coded following Hacker's Delight algorithm.
 * 	According to the author, it provides tight results.
 */
   Range Range::And(const Range &other) const
   {
      if (isEmpty())
         return *this;
      if (other.isEmpty())
         return other;
      if ((this->isUnknown() && other.isUnknown()) || (this->isMaxRange() && other.isMaxRange()))
      {
         auto bw = getBitWidth();
         assert(bw==other.getBitWidth());
         auto maxU = APInt::getMaxValue(bw);
         if (maxU.getBitWidth() < MAX_BIT_INT)
            maxU = maxU.zext(MAX_BIT_INT);
         return Range(Regular,getBitWidth(),Zero,maxU);
      }
      llvm::errs() << "this:";
      this->print(llvm::errs());
      llvm::errs() << "\n";
      llvm::errs() << "& other:";
      other.print(llvm::errs());
      llvm::errs() << "\n";
      APInt a = this->getSignedMin();
      if (a.getBitWidth() < MAX_BIT_INT)
         a = a.sext(MAX_BIT_INT);
      APInt b = this->getSignedMax();
      if (b.getBitWidth() < MAX_BIT_INT)
         b = b.sext(MAX_BIT_INT);
      APInt c = other.getSignedMin();
      if (c.getBitWidth() < MAX_BIT_INT)
         c = c.sext(MAX_BIT_INT);
      APInt d = other.getSignedMax();
      if (d.getBitWidth() < MAX_BIT_INT)
         d = d.sext(MAX_BIT_INT);
      // negate everybody
      APInt negA = APInt(a);
      negA.flipAllBits();
      APInt negB = APInt(b);
      negB.flipAllBits();
      APInt negC = APInt(c);
      negC.flipAllBits();
      APInt negD = APInt(d);
      negD.flipAllBits();

      Range inv1 = Range(Regular,getBitWidth(), negB, negA);
      Range inv2 = Range(Regular,getBitWidth(), negD, negC);
      Range invres = inv1.Or(inv2);

      // negate the result of the 'or'
      APInt invLower = invres.getUpper();
      invLower.flipAllBits();
      APInt invUpper = invres.getLower();
      invUpper.flipAllBits();
      auto res = Range(Regular,getBitWidth(),invLower,invUpper);
      llvm::errs() << "res:";
      res.print(llvm::errs());
      llvm::errs() << "\n";
      return res;
   }

   namespace {
      int64_t minOR(int64_t a, int64_t b, int64_t c, int64_t d)
      {
         int64_t m, temp;

         m = 0x80000000 >> __builtin_clzll(a ^ c);
         while (m != 0)
         {
            if ((~a & c & m) != 0)
            {
               temp = (a | m) & -m;
               if (temp <= b)
               {
                  a = temp;
                  break;
               }
            }
            else if ((a & ~c & m) != 0)
            {
               temp = (c | m) & -m;
               if (temp <= d)
               {
                  c = temp;
                  break;
               }
            }
            m = m >> 1;
         }
         return a | c;
      }

      int64_t maxOR(int64_t a, int64_t b, int64_t c, int64_t d)
      {
         int64_t m, temp;

         m = 0x80000000 >> __builtin_clzll(b & d);
         while (m != 0) {
            if ((b & d & m) != 0)
            {
               temp = (b - m) | (m - 1);
               if (temp >= a)
               {
                  b = temp;
                  break;
               }
               temp = (d - m) | (m - 1);
               if (temp >= c)
               {
                  d = temp;
                  break;
               }
            }
            m = m >> 1;
         }
         return b | d;
      }
   } // namespace

   // This operator is used when we are dealing with values
   // with more than 64-bits
   Range Range::Or_conservative(const Range &other) const
   {
      if (isEmpty()||isUnknown())
         return *this;
      if (other.isEmpty()||other.isUnknown())
         return other;
      if(this->isAnti() || other.isAnti())
         return Range(Regular,getBitWidth(),Min,Max);
      const APInt &a = this->getLower();
      const APInt &c = other.getLower();
      if(a.sgt(Zero) && c.sgt(Zero))
         return Range(Anti,getBitWidth(),Zero,Zero);
      else
         return Range(Regular,getBitWidth(),Min,Max);
   }

   /*
 * 	This or operation is coded following Hacker's Delight algorithm.
 * 	According to the author, it provides tight results.
 */
   Range Range::Or(const Range &other) const
   {
      if (isEmpty()||isUnknown())
         return *this;
      if (other.isEmpty()||other.isUnknown())
         return other;
      if(this->isAnti() || other.isAnti())
         return Range(Regular,getBitWidth(),Min,Max);
      const APInt &aM = this->getLower();
      const APInt &bM = this->getUpper();
      const APInt &cM = other.getLower();
      const APInt &dM = other.getUpper();


      if (aM.eq(Min) || bM.eq(Max) || cM.eq(Min) || dM.eq(Max))
         return Range(Regular,getBitWidth(),Min,Max);

      unsigned char switchval = 0;
      switchval += (aM.isNonNegative() ? 1 : 0);
      switchval <<= 1;
      switchval += (bM.isNonNegative() ? 1 : 0);
      switchval <<= 1;
      switchval += (cM.isNonNegative() ? 1 : 0);
      switchval <<= 1;
      switchval += (dM.isNonNegative() ? 1 : 0);

      unsigned bw = this->getBitWidth();
      const APInt a = aM.trunc(bw);
      const APInt b = bM.trunc(bw);
      const APInt c = cM.trunc(bw);
      const APInt d = dM.trunc(bw);

      APInt l = Min, u = Max;

      switch (switchval)
      {
         case 0:
            l = APInt(bw,minOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue()),true);
            l = l.sext(MAX_BIT_INT);
            u = APInt(bw,maxOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue()),true);
            u = u.sext(MAX_BIT_INT);
            break;
         case 1:
            l = a;
            l = l.sext(MAX_BIT_INT);
            u = -1;
            u= u.sext(MAX_BIT_INT);
            break;
         case 3:
            l = APInt(bw,minOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue()),true);
            l = l.sext(MAX_BIT_INT);
            u = APInt(bw,maxOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue()),true);
            u = u.sext(MAX_BIT_INT);
            break;
         case 4:
            l = c;
            l = l.sext(MAX_BIT_INT);
            u = -1;
            u = u.sext(MAX_BIT_INT);
            break;
         case 5:
            l = (a.slt(c) ? a : c);
            l = l.sext(MAX_BIT_INT);
            u = APInt(bw,maxOR(0, b.getSExtValue(), 0, d.getSExtValue()),true);
            u = u.sext(MAX_BIT_INT);
            break;
         case 7:
            l = APInt(bw,minOR(a.getSExtValue(), 0xFFFFFFFF, c.getSExtValue(), d.getSExtValue()),true);
            l= l.sext(MAX_BIT_INT);
            u = APInt(bw,minOR(0, b.getSExtValue(), c.getSExtValue(), d.getSExtValue()),true);
            u = u.sext(MAX_BIT_INT);
            break;
         case 12:
            l = APInt(bw,minOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue()),true);
            l = l.sext(MAX_BIT_INT);
            u = APInt(bw,maxOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue()),true);
            u= u.sext(MAX_BIT_INT);
            break;
         case 13:
            l = APInt(bw,minOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), 0xFFFFFFFF),true);
            l = l.sext(MAX_BIT_INT);
            u = APInt(bw,maxOR(a.getSExtValue(), b.getSExtValue(), 0, d.getSExtValue()),true);
            u = u.sext(MAX_BIT_INT);
            break;
         case 15:
            l = APInt(bw,minOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue()),true);
            l = l.sext(MAX_BIT_INT);
            u = APInt(bw,maxOR(a.getSExtValue(), b.getSExtValue(), c.getSExtValue(), d.getSExtValue()),true);
            u = u.sext(MAX_BIT_INT);
            break;
      }
      return Range(Regular,getBitWidth(),l, u);
   }

   /*
 * 	We don't have a xor implementation yet.
 * 	To be in the safe side, we just give maxrange as result.
 */
   Range Range::Xor(const Range &other) const
   {
      if (isEmpty())
         return *this;
      if (other.isEmpty())
         return other;
      if (this->isUnknown() || other.isUnknown())
         return Range(Unknown,getBitWidth(),Min,Max);

      return Range(Regular,getBitWidth(),Min,Max);
   }

   // Truncate
   //		- if the source range is entirely inside max bit range, he is
   // the
   // result
   //      - else, the result is the max bit range
   Range Range::truncate(unsigned bitwidth) const
   {
      if (isEmpty())
         return Range(Empty,bitwidth,Min,Max);
      APInt maxupper = APInt::getSignedMaxValue(bitwidth);
      APInt maxlower = APInt::getSignedMinValue(bitwidth);

      if (bitwidth < MAX_BIT_INT)
      {
         maxupper = maxupper.sext(MAX_BIT_INT);
         maxlower = maxlower.sext(MAX_BIT_INT);
      }

      // Check if source range is contained by max bit range
      if (this->getLower().sge(maxlower) && this->getUpper().sle(maxupper))
         return Range(Regular,bitwidth,this->getLower(),this->getUpper());

      return Range(Regular,bitwidth, maxlower, maxupper);
   }

   Range Range::sextOrTrunc(unsigned bitwidth) const { return truncate(bitwidth); }

   Range Range::zextOrTrunc(unsigned bitwidth) const
   {
      APInt a = this->getLower();
      APInt b = this->getUpper();
      assert(getBitWidth() <= MAX_BIT_INT);
      if (a.slt(Zero))
         return Range(Regular,bitwidth,Zero, APInt::getMaxValue(getBitWidth()).zext(MAX_BIT_INT));
      else
         return Range(Regular,bitwidth,a,b);
   }

   Range Range::intersectWith(const Range &other) const
   {
      if (this->isEmpty()||other.isEmpty())
         return Range(Empty,getBitWidth(),Min,Max);
      if (this->isUnknown())
         return other;
      if (other.isUnknown())
         return *this;
//      llvm::errs() << "intersectWith-a: ";
//      this->print(llvm::errs());
//      llvm::errs() << "\n";
//      llvm::errs() << "intersectWith-b: ";
//      other.print(llvm::errs());
//      llvm::errs() << "\n";

      if(!this->isAnti() && !other.isAnti())
      {
         APInt l = getLower().sgt(other.getLower()) ? getLower() : other.getLower();
         APInt u = getUpper().slt(other.getUpper()) ? getUpper() : other.getUpper();
         return Range(Regular,getBitWidth(),l,u);
      }
      else if(this->isAnti() && !other.isAnti())
      {
         auto antiRange = getAnti(*this);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         if(antil.sle(other.getLower()))
         {
            if(other.getUpper().sle(antiu))
               return Range(Empty,getBitWidth(),Min,Max);
            APInt l = other.getLower().sgt(antiu) ? other.getLower() : antiu+1;
            APInt u = other.getUpper();
            return Range(Regular,getBitWidth(),l,u);
         }
         else if(antiu.sge(other.getUpper()))
         {
            assert(!other.getLower().sge(antil));
            APInt l = other.getLower();
            APInt u = other.getUpper().slt(antil) ? other.getUpper() : antil-1;
            return Range(Regular,getBitWidth(),l,u);
         }
         else if(other.getLower().eq(Min) && other.getUpper().eq(Max))
            return *this;
         else if(antil.sgt(other.getUpper()) || antiu.slt(other.getLower()))
            return other;
         else // we approximate to the range of other
            return other;
      }
      else if(!this->isAnti() && other.isAnti())
      {
         auto antiRange = getAnti(other);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         if(antil.sle(this->getLower()))
         {
            if(this->getUpper().sle(antiu))
               return Range(Empty,getBitWidth(),Min,Max);
            APInt l = this->getLower().sgt(antiu) ? this->getLower() : antiu+1;
            APInt u = this->getUpper();
            return Range(Regular,getBitWidth(),l,u);
         }
         else if(antiu.sge(this->getUpper()))
         {
            assert(!this->getLower().sge(antil));
            APInt l = this->getLower();
            APInt u = this->getUpper().slt(antil) ? this->getUpper() : antil-1;
            return Range(Regular,getBitWidth(),l,u);
         }
         else if(this->getLower().eq(Min) && this->getUpper().eq(Max))
            return other;
         else if(antil.sgt(this->getUpper()) || antiu.slt(this->getLower()))
            return *this;
         else // we approximate to the range of this
            return *this;
      }
      else
      {
         auto antiRange_a = getAnti(*this);
         auto antiRange_b = getAnti(other);
         auto antil_a = antiRange_a.getLower();
         auto antiu_a = antiRange_a.getUpper();
         auto antil_b = antiRange_b.getLower();
         auto antiu_b = antiRange_b.getUpper();
         APInt l = antil_a.sgt(antil_b) ? antil_b : antil_a;
         APInt u = antiu_a.sgt(antiu_b) ? antiu_a : antiu_b;
         if(l.eq(Min) && u.eq(Max))
            return Range(Empty,getBitWidth(),Min,Max);
         else
            return Range(Anti,getBitWidth(),l,u);
      }
   }

   Range Range::unionWith(const Range &other) const
   {
      if (this->isEmpty())
         return other;
      if (other.isEmpty())
         return *this;
      if (this->isUnknown())
         return other;
      if (other.isUnknown())
         return *this;
      if(!this->isAnti() && !other.isAnti())
      {
         APInt l = getLower().slt(other.getLower()) ? getLower() : other.getLower();
         APInt u = getUpper().sgt(other.getUpper()) ? getUpper() : other.getUpper();
         return Range(Regular,getBitWidth(), l, u);
      }
      else if(this->isAnti() && !other.isAnti())
      {
         auto antiRange = getAnti(*this);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         assert(antil.ne(Min));
         assert(antiu.ne(Max));
         if(antil.sgt(other.getUpper()) || antiu.slt(other.getLower()))
            return *this;
         else if(antil.sgt(other.getLower()) && antiu.slt(other.getUpper()))
            return Range(Regular,getBitWidth(),Min,Max);
         else if(antil.sge(other.getLower()) && antiu.sgt(other.getUpper()))
            return Range(Anti,getBitWidth(),other.getUpper()+1,antiu);
         else if(antil.slt(other.getLower()) && antiu.sle(other.getUpper()))
            return Range(Anti,getBitWidth(), antil,other.getLower()-1);
         else
            return Range(Regular,getBitWidth(),Min,Max); //approximate to the full set
      }
      else if(!this->isAnti() && other.isAnti())
      {
         auto antiRange = getAnti(other);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         assert(antil.ne(Min));
         assert(antiu.ne(Max));
         if(antil.sgt(this->getUpper()) || antiu.slt(this->getLower()))
            return other;
         else if(antil.sgt(this->getLower()) && antiu.slt(this->getUpper()))
            return Range(Regular,getBitWidth(),Min,Max);
         else if(antil.sge(this->getLower()) && antiu.sgt(this->getUpper()))
            return Range(Anti,getBitWidth(),this->getUpper()+1,antiu);
         else if(antil.slt(this->getLower()) && antiu.sle(this->getUpper()))
            return Range(Anti,getBitWidth(),antil,this->getLower()-1);
         else
            return Range(Regular,getBitWidth(),Min,Max); //approximate to the full set

      }
      else
      {
         auto antiRange_a = getAnti(*this);
         auto antiRange_b = getAnti(other);
         auto antil_a = antiRange_a.getLower();
         auto antiu_a = antiRange_a.getUpper();
         assert(antil_a.ne(Min));
         assert(antiu_a.ne(Max));
         auto antil_b = antiRange_b.getLower();
         auto antiu_b = antiRange_b.getUpper();
         assert(antil_b.ne(Min));
         assert(antiu_b.ne(Max));
         if(antil_a.sgt(antiu_b) || antiu_a.slt(antil_b))
            return Range(Regular,getBitWidth(),Min,Max);
         else if(antil_a.sgt(antil_b) && antiu_a.slt(antiu_b))
            return *this;
         else if(antil_b.sgt(antil_a) && antiu_b.slt(antiu_a))
            return *this;
         else if(antil_a.sge(antil_b) && antiu_b.sle(antiu_a))
            return Range(Anti,getBitWidth(),antil_a, antiu_b);
         else if(antil_b.sge(antil_a) && antiu_a.sle(antiu_b))
            return Range(Anti,getBitWidth(),antil_b, antiu_a);
         else
            llvm_unreachable("unsupported condition");
      }
   }

   Range Range::BestRange(const llvm::ConstantRange&UR,const llvm::ConstantRange&SR, unsigned bw) const
   {
      auto nbitU = neededBits(UR.getUnsignedMin(), UR.getUnsignedMax(), false);
      auto nbitS = neededBits(SR.getSignedMin(), SR.getSignedMax(), true);
      if(nbitU<nbitS)
         return Range(Regular,bw,UR.getUnsignedMin().zext(MAX_BIT_INT),UR.getUnsignedMax().zext(MAX_BIT_INT));
      else
         return Range(Regular,bw,SR.getSignedMin().sext(MAX_BIT_INT),SR.getSignedMax().sext(MAX_BIT_INT));
   }

   bool Range::operator==(const Range &other) const
   {
      return getBitWidth() == other.getBitWidth() &&
            Range::isSameType(*this, other) &&
            getLower().eq(other.getLower()) &&
            getUpper().eq(other.getUpper());
   }

   bool Range::operator!=(const Range &other) const
   {
      return getBitWidth() != other.getBitWidth() ||
            !Range::isSameType(*this, other) ||
            getLower().ne(other.getLower()) ||
            getUpper().ne(other.getUpper());
   }

   void Range::print(raw_ostream &OS) const {
      if (this->isUnknown())
      {
         OS << "Unknown";
         return;
      }
      if (this->isEmpty())
      {
         OS << "Empty";
         return;
      }
      if(this->isAnti())
      {
         auto antiObj = getAnti(*this);
         if (antiObj.getLower().eq(Min))
            OS << ")-inf,";
         else
            OS << ")" << antiObj.getLower() << ",";
         OS << getBitWidth() << ",";
         if (antiObj.getUpper().eq(Max))
            OS << "+inf(";
         else
            OS << antiObj.getUpper() << "(";
      }
      else
      {
         if (getLower().eq(Min))
            OS << "[-inf,";
         else
            OS << "[" << getLower() << ",";
         OS << getBitWidth() << ",";
         if (getUpper().eq(Max))
            OS << "+inf]";
         else
            OS << getUpper() << "]";
      }
   }

   raw_ostream &operator<<(raw_ostream &OS, const Range &R) {
      R.print(OS);
      return OS;
   }

   // ========================================================================== //
   // BasicInterval
   // ========================================================================== //

   BasicInterval::BasicInterval(Range range) : range(std::move(range)) {}

   BasicInterval::BasicInterval() : range(Range(Regular,MAX_BIT_INT,Min,Max)) {}

   // This is a base class, its dtor must be virtual.
   BasicInterval::~BasicInterval() = default;

   /// Pretty print.
   void BasicInterval::print(raw_ostream &OS) const { this->getRange().print(OS); }

   // ========================================================================== //
   // SymbInterval
   // ========================================================================== //

   SymbInterval::SymbInterval(const Range &range, const Value *bound,
                              CmpInst::Predicate pred)
      : BasicInterval(range), bound(bound), pred(pred) {}

   SymbInterval::~SymbInterval() = default;

   Range SymbInterval::fixIntersects(VarNode *bound, VarNode *sink) {
      // Get the lower and the upper bound of the
      // node which bounds this intersection.
      APInt l = bound->getRange().getLower();
      APInt u = bound->getRange().getUpper();

      // Get the lower and upper bound of the interval of this operation
      APInt lower = sink->getRange().getLower();
      APInt upper = sink->getRange().getUpper();

      switch (this->getOperation())
      {
         case ICmpInst::ICMP_EQ: // equal
            return Range(Regular,getRange().getBitWidth(),l,u);
         case ICmpInst::ICMP_SLE: // signed less or equal
            return Range(Regular,getRange().getBitWidth(),lower,u);
         case ICmpInst::ICMP_SLT: // signed less than
            if (u != Max)
               return Range(Regular,getRange().getBitWidth(),lower,u-1);
            else
               return Range(Regular,getRange().getBitWidth(),lower,u);
         case ICmpInst::ICMP_SGE: // signed greater or equal
            return Range(Regular,getRange().getBitWidth(),l,upper);
         case ICmpInst::ICMP_SGT: // signed greater than
            if (l != Min)
               return Range(Regular,getRange().getBitWidth(),l+1,upper);
             else
               return Range(Regular,getRange().getBitWidth(),l,upper);
         default:
            return Range(Regular,getRange().getBitWidth(),Min,Max);
      }
      llvm_unreachable("unexpected condition");
   }

   /// Pretty print.
   void SymbInterval::print(raw_ostream &OS) const
   {
      switch (this->getOperation())
      {
         case ICmpInst::ICMP_EQ: // equal
            OS << "[lb(";
            printVarName(getBound(), OS);
            OS << "), ub(";
            printVarName(getBound(), OS);
            OS << ")]";
            break;
         case ICmpInst::ICMP_SLE: // sign less or equal
            OS << "[-inf, ub(";
            printVarName(getBound(), OS);
            OS << ")]";
            break;
         case ICmpInst::ICMP_SLT: // sign less than
            OS << "[-inf, ub(";
            printVarName(getBound(), OS);
            OS << ") - 1]";
            break;
         case ICmpInst::ICMP_SGE: // sign greater or equal
            OS << "[lb(";
            printVarName(getBound(), OS);
            OS << "), +inf]";
            break;
         case ICmpInst::ICMP_SGT: // sign greater than
            OS << "[lb(";
            printVarName(getBound(), OS);
            OS << " - 1), +inf]";
            break;
         default:
            OS << "Unknown Instruction.\n";
      }
   }

   // ========================================================================== //
   // VarNode
   // ========================================================================== //

   /// The ctor.
   VarNode::VarNode(const Value *V)
      : V(V), interval(Range(Unknown,V->getType()->getPrimitiveSizeInBits(), Min, Max)), abstractState(0)
   {
      if(dyn_cast<FPToSIInst>(V))
      {
         auto bw = V->getType()->getPrimitiveSizeInBits();
         interval = Range(Regular,bw, APInt::getSignedMinValue(bw).sext(MAX_BIT_INT),APInt::getSignedMaxValue(bw).sext(MAX_BIT_INT));
      }
      else if(dyn_cast<FPToUIInst>(V))
      {
         auto bw = V->getType()->getPrimitiveSizeInBits();
         interval = Range(Regular,bw, APInt::getMinValue(bw).sext(MAX_BIT_INT),APInt::getMaxValue(bw).sext(MAX_BIT_INT));
      }
   }

   /// The dtor.
   VarNode::~VarNode() = default;

   /// Initializes the value of the node.
   void VarNode::init(bool outside)
   {
      auto bw = V->getType()->getPrimitiveSizeInBits();
      if (const ConstantInt *CI = dyn_cast<ConstantInt>(V))
      {
         APInt tmp = CI->getValue();
         if (tmp.getBitWidth() < MAX_BIT_INT)
            tmp = tmp.sext(MAX_BIT_INT);
         this->setRange(Range(Regular,bw, tmp, tmp));
      }
      else
      {
         if (!outside)
         {
            // Initialize with a basic, unknown, interval.
            this->setRange(Range(Unknown,bw,Min,Max));
         } else
            this->setRange(Range(Regular,bw,Min,Max));
      }
   }

   /// Pretty print.
   void VarNode::print(raw_ostream &OS) const
   {
      if (const ConstantInt *C = dyn_cast<ConstantInt>(this->getValue()))
         OS << C->getValue();
      else
         printVarName(this->getValue(), OS);
      OS << " ";
      this->getRange().print(OS);
   }

   void VarNode::storeAbstractState()
   {
      ASSERT(!this->interval.isUnknown(),
             "storeAbstractState doesn't handle empty set");

      if (this->interval.getLower().eq(Min))
      {
         if (this->interval.getUpper().eq(Max))
            this->abstractState = '?';
         else
            this->abstractState = '-';
      }
      else if (this->interval.getUpper().eq(Max))
         this->abstractState = '+';
      else
         this->abstractState = '0';
   }

   raw_ostream &operator<<(raw_ostream &OS, const VarNode *VN)
   {
      VN->print(OS);
      return OS;
   }

   // ========================================================================== //
   // BasicOp
   // ========================================================================== //

   /// We can not want people creating objects of this class,
   /// but we want to inherit of it.
   BasicOp::BasicOp(BasicInterval *intersect, VarNode *sink,
                    const Instruction *inst)
      : intersect(intersect), sink(sink), inst(inst)
   {
//      llvm::errs()<<"BasicOp:";
//      intersect->print(llvm::errs());
//      llvm::errs()<<"\n";
//      sink->print(llvm::errs());
//      llvm::errs()<<"\n";
   }

   /// We can not want people creating objects of this class,
   /// but we want to inherit of it.
   BasicOp::~BasicOp() { delete intersect; }

   /// Replace symbolic intervals with hard-wired constants.
   void BasicOp::fixIntersects(VarNode *V)
   {
      if (SymbInterval *SI = dyn_cast<SymbInterval>(getIntersect()))
      {
         Range r = SI->fixIntersects(V, getSink());
         this->setIntersect(SI->fixIntersects(V, getSink()));
      }
   }

   // ========================================================================== //
   // ControlDep
   // ========================================================================== //

   ControlDep::ControlDep(VarNode *sink, VarNode *source)
      : BasicOp(new BasicInterval(), sink, nullptr), source(source) {}

   ControlDep::~ControlDep() = default;

   Range ControlDep::eval() const { return Range(Regular,MAX_BIT_INT,Min,Max); }

   void ControlDep::print(raw_ostream & /*OS*/) const {}

   // ========================================================================== //
   // UnaryOp
   // ========================================================================== //

   UnaryOp::UnaryOp(BasicInterval *intersect, VarNode *sink,
                    const Instruction *inst, VarNode *source, unsigned int opcode)
      : BasicOp(intersect, sink, inst), source(source), opcode(opcode) {}

   // The dtor.
   UnaryOp::~UnaryOp() = default;

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   Range UnaryOp::eval() const
   {
      unsigned bw = getSink()->getValue()->getType()->getPrimitiveSizeInBits();
      Range oprnd = source->getRange();
      Range result(Unknown, bw, Min, Max);

      if (oprnd.isRegular())
      {
         switch (this->getOpcode())
         {
            case Instruction::Trunc:
               result = oprnd.truncate(bw);
               break;
            case Instruction::ZExt:
               result = oprnd.zextOrTrunc(bw);
               break;
            case Instruction::SExt:
               result = oprnd.sextOrTrunc(bw);
               break;
            case Instruction::FPToSI:
               result = oprnd.sextOrTrunc(bw);
               break;
            case Instruction::FPToUI:
               result = oprnd.zextOrTrunc(bw);
               break;
            default:
               // Loads and Stores are handled here.
               result = oprnd;
               break;
         }
      }
      else if (oprnd.isEmpty())
         result = Range(Empty,bw,Min,Max);

      llvm::errs() << "intersection-pre0: ";
      this->getIntersect()->getRange().print(llvm::errs());
      llvm::errs() << "\n";
      if (!getIntersect()->getRange().isMaxRange())
      {
         Range aux(getIntersect()->getRange());
         result = result.intersectWith(aux);
         llvm::errs() << "intersection: ";
         result.print(llvm::errs());
         llvm::errs() << "\n";
      }
      // To ensure that we always are dealing with the correct bit width.
      return result;
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void UnaryOp::print(raw_ostream &OS) const
   {
      const char *quot = R"(")";
      OS << " " << quot << this << quot << R"( [label=")";

      // Instruction bitwidth
      unsigned bw = getSink()->getValue()->getType()->getPrimitiveSizeInBits();

      switch (this->opcode)
      {
         case Instruction::Trunc:
            OS << "trunc i" << bw;
            break;
         case Instruction::ZExt:
            OS << "zext i" << bw;
            break;
         case Instruction::SExt:
            OS << "sext i" << bw;
            break;
         case Instruction::FPToSI:
            OS << "fptosi i" << bw;
            break;
         case Instruction::FPToUI:
            OS << "fptoui i" << bw;
            break;
         default:
            // Phi functions, Loads and Stores are handled here.
            this->getIntersect()->print(OS);
            break;
      }

      OS << "\"]\n";

      const Value *V = this->getSource()->getValue();
      if (const ConstantInt *C = dyn_cast<ConstantInt>(V))
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      else
      {
         OS << " " << quot;
         printVarName(V, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }

      const Value *VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      printVarName(VS, OS);
      OS << quot << "\n";
   }

   // ========================================================================== //
   // SigmaOp
   // ========================================================================== //

   SigmaOp::SigmaOp(BasicInterval *intersect, VarNode *sink,
                    const Instruction *inst, VarNode *source, unsigned int opcode)
      : UnaryOp(intersect, sink, inst, source, opcode), unresolved(false) {}

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   Range SigmaOp::eval() const
   {
      Range result = this->getSource()->getRange();
      result = result.intersectWith(getIntersect()->getRange());
      return result;
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void SigmaOp::print(raw_ostream &OS) const
   {
      const char *quot = R"(")";
      OS << " " << quot << this << quot << R"( [label=")";
      this->getIntersect()->print(OS);
      OS << "\"]\n";
      const Value *V = this->getSource()->getValue();
      if (const ConstantInt *C = dyn_cast<ConstantInt>(V))
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      else
      {
         OS << " " << quot;
         printVarName(V, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }

      const Value *VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      printVarName(VS, OS);
      OS << quot << "\n";
   }

   // ========================================================================== //
   // BinaryOp
   // ========================================================================== //

   // The ctor.
   BinaryOp::BinaryOp(BasicInterval *intersect, VarNode *sink,
                      const Instruction *inst, VarNode *source1, VarNode *source2,
                      unsigned int opcode)
      : BasicOp(intersect, sink, inst), source1(source1), source2(source2),
        opcode(opcode) {}

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   /// Basically, this function performs the operation indicated in its opcode
   /// taking as its operands the source1 and the source2.
   Range BinaryOp::eval() const
   {
      Range op1 = this->getSource1()->getRange();
      Range op2 = this->getSource2()->getRange();
      // Instruction bitwidth
      unsigned bw = getSink()->getValue()->getType()->getPrimitiveSizeInBits();
      Range result(Unknown,bw,Min,Max);

      // only evaluate if all operands are Regular
      if (op1.isRegular() && op2.isRegular())
      {
         assert(op1.getBitWidth()==bw || this->getOpcode()==llvm::Instruction::ICmp);
         assert(op2.getBitWidth()==bw || this->getOpcode()==llvm::Instruction::ICmp);
         switch (this->getOpcode())
         {
            case Instruction::Add:
               result = op1.add(op2);
               break;
            case Instruction::Sub:
               result = op1.sub(op2);
               break;
            case Instruction::Mul:
               result = op1.mul(op2);
               break;
            case Instruction::UDiv:
               result = op1.udiv(op2);
               break;
            case Instruction::SDiv:
               result = op1.sdiv(op2);
               break;
            case Instruction::URem:
               result = op1.urem(op2);
               break;
            case Instruction::SRem:
               result = op1.srem(op2);
               break;
            case Instruction::Shl:
               result = op1.shl(op2);
               break;
            case Instruction::LShr:
               result = op1.lshr(op2);
               break;
            case Instruction::AShr:
               result = op1.ashr(op2);
               break;
            case Instruction::And:
               result = op1.And(op2);
               break;
            case Instruction::Or:
               // We have two versions of the 'or' operator
               // One of them gives tight results, but only works
               // for 64-bit values or less.
               if (bw <= 64)
                  result = op1.Or(op2);
                else
                  result = op1.Or_conservative(op2);
               break;
            case Instruction::Xor:
               result = op1.Xor(op2);
               break;
            case llvm::Instruction::ICmp:
               result = Range(Regular,bw,Zero,One);
               break;
            default:
               break;
         }

         llvm::errs() << "intersection-pre: ";
         this->getIntersect()->getRange().print(llvm::errs());
         llvm::errs() << "\n";
         bool test = this->getIntersect()->getRange().isMaxRange();
         if (!test)
         {
            Range aux = this->getIntersect()->getRange();
            result = result.intersectWith(aux);
            llvm::errs() << "intersection: ";
            result.print(llvm::errs());
            llvm::errs() << "\n";
         }
      }
      else
      {
         if (op1.isEmpty() || op2.isEmpty())
            result = Range(Empty,bw,Min,Max);
      }
      return result;
   }

   /// Pretty print.
   void BinaryOp::print(raw_ostream &OS) const
   {
      const char *quot = R"(")";
      const char *opcodeName = Instruction::getOpcodeName(this->getOpcode());
      OS << " " << quot << this << quot << R"( [label=")" << opcodeName << "\"]\n";
      const Value *V1 = this->getSource1()->getValue();
      if (const ConstantInt *C = dyn_cast<ConstantInt>(V1))
      {
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         printVarName(V1, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      const Value *V2 = this->getSource2()->getValue();
      if (const ConstantInt *C = dyn_cast<ConstantInt>(V2))
      {
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         printVarName(V2, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      const Value *VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      printVarName(VS, OS);
      OS << quot << "\n";
   }

   // ========================================================================== //
   // TernaryOp
   // ========================================================================== //

   // The ctor.
   TernaryOp::TernaryOp(BasicInterval *intersect, VarNode *sink,
                        const Instruction *inst, VarNode *source1,
                        VarNode *source2, VarNode *source3, unsigned int opcode)
      : BasicOp(intersect, sink, inst), source1(source1), source2(source2),
        source3(source3), opcode(opcode) {}

   Range TernaryOp::eval() const {

      Range op1 = this->getSource1()->getRange();
      Range op2 = this->getSource2()->getRange();
      Range op3 = this->getSource3()->getRange();
      // Instruction bitwidth
      unsigned bw = getSink()->getValue()->getType()->getPrimitiveSizeInBits();
      assert(bw==op2.getBitWidth());
      assert(bw==op3.getBitWidth());
      Range result(Unknown,bw, Min, Max);

      // only evaluate if all operands are Regular
      if (op1.isRegular() && op2.isRegular() && op3.isRegular())
      {
         switch (this->getOpcode())
         {
            case Instruction::Select:
            {
               // Source1 is the selector
               if (op1 == Range(Regular,op1.getBitWidth(), One, One))
                  result = op2;
               else if (op1 == Range(Regular,op1.getBitWidth(), Zero, Zero))
                  result = op3;
               else
               {
                  auto I = getInstruction();
                  auto opV0 = I->getOperand(0);

                  if(isa<ICmpInst>(opV0))
                  {
                     const Value *CondOp0 = cast<ICmpInst>(opV0)->getOperand(0);
                     const Value *CondOp1 = cast<ICmpInst>(opV0)->getOperand(1);
                     if (isa<ConstantInt>(CondOp0) || isa<ConstantInt>(CondOp1))
                     {
                        auto variable = isa<ConstantInt>(CondOp0) ? CondOp1 : CondOp0;
                        auto constant = isa<ConstantInt>(CondOp0) ? cast<ConstantInt>(CondOp0) : cast<ConstantInt>(CondOp1);
                        auto opV1 = I->getOperand(1);
                        auto opV2 = I->getOperand(2);
                        if(variable==opV1 || variable==opV2)
                        {
                           ConstantRange CR(constant->getValue(), constant->getValue() + 1);
                           CmpInst::Predicate pred = cast<ICmpInst>(opV0)->getPredicate();
                           CmpInst::Predicate swappred = cast<ICmpInst>(opV0)->getSwappedPredicate();

                           ConstantRange tmpT =
                                 (variable == CondOp0)
                                 ? ConstantRange::makeSatisfyingICmpRegion(pred, CR)
                                 : ConstantRange::makeSatisfyingICmpRegion(swappred, CR);

                           APInt sigMin = tmpT.getSignedMin();
                           APInt sigMax = tmpT.getSignedMax();

                           if (sigMin.getBitWidth() < MAX_BIT_INT)
                              sigMin = sigMin.sext(MAX_BIT_INT);
                           if (sigMax.getBitWidth() < MAX_BIT_INT)
                              sigMax = sigMax.sext(MAX_BIT_INT);
                           assert(sigMax.sge(sigMin));
                           if(variable==opV2)
                           {
                              Range FValues = Range(Anti,op3.getBitWidth(), sigMin, sigMax);
                              op3= op3.intersectWith(FValues);
                           }
                           else
                           {
                              Range TValues = Range(Regular,op2.getBitWidth(), sigMin, sigMax);
                              op2= op2.intersectWith(TValues);
                           }
                        }
                     }
                  }
                  result = op2.unionWith(op3);
               }
               break;
            }
            default:
               break;
         }
         assert(result.getUpper().sge(result.getLower()));
         bool test = this->getIntersect()->getRange().isMaxRange();
         if (!test)
         {
            Range aux = this->getIntersect()->getRange();
            result = result.intersectWith(aux);
         }
      }
      else
      {
         if (op1.isEmpty() || op2.isEmpty() || op3.isEmpty())
            result = Range(Empty,bw,Min,Max);
      }
      return result;
   }

   /// Pretty print.
   void TernaryOp::print(raw_ostream &OS) const
   {
      const char *quot = R"(")";
      const char *opcodeName = Instruction::getOpcodeName(this->getOpcode());
      OS << " " << quot << this << quot << R"( [label=")" << opcodeName << "\"]\n";

      const Value *V1 = this->getSource1()->getValue();
      if (const ConstantInt *C = dyn_cast<ConstantInt>(V1))
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      else
      {
         OS << " " << quot;
         printVarName(V1, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      const Value *V2 = this->getSource2()->getValue();
      if (const ConstantInt *C = dyn_cast<ConstantInt>(V2))
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      else
      {
         OS << " " << quot;
         printVarName(V2, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }

      const Value *V3 = this->getSource3()->getValue();
      if (const ConstantInt *C = dyn_cast<ConstantInt>(V3))
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      else
      {
         OS << " " << quot;
         printVarName(V3, OS);
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      const Value *VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      printVarName(VS, OS);
      OS << quot << "\n";
   }

   // ========================================================================== //
   // PhiOp
   // ========================================================================== //

   // The ctor.
   PhiOp::PhiOp(BasicInterval *intersect, VarNode *sink, const Instruction *inst)
      : BasicOp(intersect, sink, inst) {}

   // Add source to the vector of sources
   void PhiOp::addSource(const VarNode *newsrc)
   {
      this->sources.push_back(newsrc);
   }

   /// Computes the interval of the sink based on the interval of the sources.
   /// The result of evaluating a phi-function is the union of the ranges of
   /// every variable used in the phi.
   Range PhiOp::eval() const
   {
      Range result = this->getSource(0)->getRange();
      // Iterate over the sources of the phiop
      for (const VarNode *varNode : sources)
         result = result.unionWith(varNode->getRange());
      return result;
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void PhiOp::print(raw_ostream &OS) const
   {
      const char *quot = R"(")";
      OS << " " << quot << this << quot << R"( [label=")";
      OS << "phi";
      OS << "\"]\n";
      for (const VarNode *varNode : sources)
      {
         const Value *V = varNode->getValue();
         if (const ConstantInt *C = dyn_cast<ConstantInt>(V))

            OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
         else
         {
            OS << " " << quot;
            printVarName(V, OS);
            OS << quot << " -> " << quot << this << quot << "\n";
         }
      }
      const Value *VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      printVarName(VS, OS);
      OS << quot << "\n";
   }

   // ========================================================================== //
   // ValueBranchMap
   // ========================================================================== //

   ValueBranchMap::ValueBranchMap(const Value *V, const BasicBlock *BBTrue,
                                  const BasicBlock *BBFalse, BasicInterval *ItvT,
                                  BasicInterval *ItvF)
      : V(V), BBTrue(BBTrue), BBFalse(BBFalse), ItvT(ItvT), ItvF(ItvF) {}

   ValueBranchMap::~ValueBranchMap() = default;

   void ValueBranchMap::clear()
   {
      if (ItvT != nullptr)
      {
         delete ItvT;
         ItvT = nullptr;
      }
      if (ItvF != nullptr)
      {
         delete ItvF;
         ItvF = nullptr;
      }
   }

   // ========================================================================== //
   // ValueSwitchMap
   // ========================================================================== //

   ValueSwitchMap::ValueSwitchMap(
         const Value *V,
         SmallVector<std::pair<BasicInterval *, const BasicBlock *>, 4> &BBsuccs)
      : V(V), BBsuccs(BBsuccs) {}

   ValueSwitchMap::~ValueSwitchMap() = default;

   void ValueSwitchMap::clear()
   {
      for (auto &succ : BBsuccs)
      {
         if (succ.first != nullptr)
         {
            delete succ.first;
            succ.first = nullptr;
         }
      }
   }

   // ========================================================================== //
   // ConstraintGraph
   // ========================================================================== //

   /// The dtor.
   ConstraintGraph::~ConstraintGraph() {
      for (auto &pair : vars)
      {
         delete pair.second;
      }
      for (BasicOp *op : oprs)
      {
         delete op;
      }
      for (auto &pair : valuesBranchMap)
      {
         pair.second.clear();
      }
      for (auto &pair : valuesSwitchMap)
      {
         pair.second.clear();
      }
   }

   Range ConstraintGraph::getRange(const Value *v)
   {
      VarNodes::iterator vit = this->vars.find(v);

      if (vit == this->vars.end())
      {
         // If the value doesn't have a range,
         // it wasn't considered by the range analysis
         // for some reason.
         // It gets an unknown range if it's a variable,
         // or the tight range if it's a constant
         //
         // I decided NOT to insert these uncovered
         // values to the node set after their range
         // is created here.
         auto bw = v->getType()->getPrimitiveSizeInBits();
         const ConstantInt *ci = dyn_cast<ConstantInt>(v);
         if (ci == nullptr)
            return Range(Unknown,bw,Min,Max);
         APInt tmp = ci->getValue();
         if (tmp.getBitWidth() < MAX_BIT_INT)
            tmp = tmp.sext(MAX_BIT_INT);
         return Range(Regular,bw, tmp, tmp);
      }
      return vit->second->getRange();
   }

   /// Adds a VarNode to the graph.
   VarNode *ConstraintGraph::addVarNode(const Value *V)
   {
      VarNodes::iterator vit = this->vars.find(V);

      if (vit != this->vars.end())
         return vit->second;

      VarNode *node = new VarNode(V);
      this->vars.insert(std::make_pair(V, node));

      // Inserts the node in the use map list.
      SmallPtrSet<BasicOp *, 8> useList;
      this->useMap.insert(std::make_pair(V, useList));
      return node;
   }

   /// Adds an UnaryOp in the graph.
   void ConstraintGraph::addUnaryOp(Instruction *I, ModulePass *modulePass, const llvm::DataLayout* DL)
   {
      assert(I->getNumOperands() == 1U);
      // Create the sink.
      VarNode *sink = addVarNode(I);
      // Create the source.
      VarNode *source = nullptr;

      switch (I->getOpcode())
      {
         case Instruction::Trunc:
         case Instruction::ZExt:
         case Instruction::SExt:
            source = addVarNode(I->getOperand(0));
            break;
         default:
            return;
      }
      UnaryOp *UOp = nullptr;
      UOp = new UnaryOp(new BasicInterval(getLLVM_range(I,modulePass, DL)), sink, I, source, I->getOpcode());
      this->oprs.insert(UOp);
      // Insert this definition in defmap
      this->defMap[sink->getValue()] = UOp;
      // Inserts the sources of the operation in the use map list.
      this->useMap.find(source->getValue())->second.insert(UOp);
   }

   /// XXX: I'm assuming that we are always analyzing bytecodes in e-SSA form.
   /// So, we don't have intersections associated with binary oprs.
   /// To have an intersect, we must have a Sigma instruction.
   /// Adds a BinaryOp in the graph.
   void ConstraintGraph::addBinaryOp(Instruction *I, ModulePass *modulePass, const llvm::DataLayout* DL)
   {
      assert(I->getNumOperands() == 2U);
      // Create the sink.
      VarNode *sink = addVarNode(I);

      // Create the sources.
      VarNode *source1 = addVarNode(I->getOperand(0));
      VarNode *source2 = addVarNode(I->getOperand(1));

      // Create the operation using the intersect to constrain sink's interval.
      BasicInterval *BI = new BasicInterval(getLLVM_range(I,modulePass, DL));
      BinaryOp *BOp = new BinaryOp(BI, sink, I, source1, source2, I->getOpcode());

      // Insert the operation in the graph.
      this->oprs.insert(BOp);

      // Insert this definition in defmap
      this->defMap[sink->getValue()] = BOp;

      // Inserts the sources of the operation in the use map list.
      this->useMap.find(source1->getValue())->second.insert(BOp);
      this->useMap.find(source2->getValue())->second.insert(BOp);
   }

   void ConstraintGraph::addTernaryOp(Instruction *I, ModulePass *modulePass, const llvm::DataLayout* DL)
   {
      assert(I->getNumOperands() == 3U);
      // Create the sink.
      VarNode *sink = addVarNode(I);

      // Create the sources.
      VarNode *source1 = addVarNode(I->getOperand(0));
      VarNode *source2 = addVarNode(I->getOperand(1));
      VarNode *source3 = addVarNode(I->getOperand(2));

      // Create the operation using the intersect to constrain sink's interval.
      BasicInterval *BI = new BasicInterval(getLLVM_range(I,modulePass,DL));
      TernaryOp *TOp =
            new TernaryOp(BI, sink, I, source1, source2, source3, I->getOpcode());

      // Insert the operation in the graph.
      this->oprs.insert(TOp);

      // Insert this definition in defmap
      this->defMap[sink->getValue()] = TOp;

      // Inserts the sources of the operation in the use map list.
      this->useMap.find(source1->getValue())->second.insert(TOp);
      this->useMap.find(source2->getValue())->second.insert(TOp);
      this->useMap.find(source3->getValue())->second.insert(TOp);
   }

   /// Add a phi node (actual phi, does not include sigmas)
   void ConstraintGraph::addPhiOp(PHINode *Phi, ModulePass *modulePass, const llvm::DataLayout* DL)
   {
      // Create the sink.
      VarNode *sink = addVarNode(Phi);
      PhiOp *phiOp = new PhiOp(new BasicInterval(getLLVM_range(Phi,modulePass,DL)), sink, Phi);

      // Insert the operation in the graph.
      this->oprs.insert(phiOp);

      // Insert this definition in defmap
      this->defMap[sink->getValue()] = phiOp;

      // Create the sources.
      for (const Value *operand : Phi->operands())
      {
         VarNode *source = addVarNode(operand);
         phiOp->addSource(source);
         // Inserts the sources of the operation in the use map list.
         this->useMap.find(source->getValue())->second.insert(phiOp);
      }
   }

   void ConstraintGraph::addSigmaOp(PHINode *Sigma, ModulePass *modulePass, const llvm::DataLayout* DL)
   {
      assert(Sigma->getNumOperands() == 1U);
      // Create the sink.
      VarNode *sink = addVarNode(Sigma);
      BasicInterval *BItv = nullptr;
      SigmaOp *sigmaOp = nullptr;

      const BasicBlock *thisbb = Sigma->getParent();
      for (const Value *operand : Sigma->operands())
      {
         VarNode *source = addVarNode(operand);

         // Create the operation (two cases from: branch or switch)
         ValuesBranchMap::iterator vbmit = this->valuesBranchMap.find(operand);

         // Branch case
         if (vbmit != this->valuesBranchMap.end())
         {
            const ValueBranchMap &VBM = vbmit->second;
            if (thisbb == VBM.getBBTrue())
            {
               BItv = VBM.getItvT();
            }
            else
            {
               if (thisbb == VBM.getBBFalse())
               {
                  BItv = VBM.getItvF();
               }
            }
         }
         else
         {
            // Switch case
            ValuesSwitchMap::iterator vsmit = this->valuesSwitchMap.find(operand);

            if (vsmit == this->valuesSwitchMap.end())
            {
               continue;
            }

            const ValueSwitchMap &VSM = vsmit->second;
            // Find out which case are we dealing with
            for (unsigned idx = 0, e = VSM.getNumOfCases(); idx < e; ++idx)
            {
               const BasicBlock *bb = VSM.getBB(idx);
               if (bb == thisbb)
               {
                  BItv = VSM.getItv(idx);
                  break;
               }
            }
         }

         if (BItv == nullptr)
            sigmaOp = new SigmaOp(new BasicInterval(getLLVM_range(Sigma,modulePass,DL)), sink, Sigma, source, Sigma->getOpcode());
         else
            sigmaOp = new SigmaOp(BItv, sink, Sigma, source, Sigma->getOpcode());

         // Insert the operation in the graph.
         this->oprs.insert(sigmaOp);

         // Insert this definition in defmap
         this->defMap[sink->getValue()] = sigmaOp;

         // Inserts the sources of the operation in the use map list.
         this->useMap.find(source->getValue())->second.insert(sigmaOp);
      }
   }



   namespace {
      // LLVM's Instructions.h doesn't provide a function like this, so I made one.
      bool isTernaryOp(const Instruction *I) {
         return isa<SelectInst>(I);
      }
   }

   void ConstraintGraph::buildOperations(Instruction *I, ModulePass *modulePass, const llvm::DataLayout* DL)
   {
      if (I->isBinaryOp()) {
         addBinaryOp(I,modulePass,DL);
      } else if (isTernaryOp(I)) {
         addTernaryOp(I,modulePass,DL);
      } else {
         if(isa<ICmpInst>(I))
         {
            addBinaryOp(I,modulePass,DL);
         }
         // Handle Phi functions.
         else if (auto Phi = dyn_cast<PHINode>(I)) {
            if (Phi->getNumOperands()==1) {
               addSigmaOp(Phi,modulePass,DL);
            } else {
               addPhiOp(Phi,modulePass,DL);
            }
         } else {
            // We have an unary instruction to handle.
            addUnaryOp(I,modulePass,DL);
         }
      }
   }

   void ConstraintGraph::buildValueSwitchMap(const SwitchInst *sw)
   {
      const Value *condition = sw->getCondition();
      // Verify conditions
      const Type *opType = sw->getCondition()->getType();
      if (!opType->isIntegerTy())
         return;

      // Create VarNode for switch condition explicitly (need to do this when
      // inlining is used!)
      addVarNode(condition);
      unsigned bw = condition->getType()->getPrimitiveSizeInBits();

      SmallVector<std::pair<BasicInterval *, const BasicBlock *>, 4> BBsuccs;

      // Treat when condition of switch is a cast of the real condition (same thing
      // as in buildValueBranchMap)
      const CastInst *castinst = nullptr;
      const Value *Op0_0 = nullptr;
      if ((castinst = dyn_cast<CastInst>(condition)) != nullptr)
         Op0_0 = castinst->getOperand(0);

      // Handle 'default', if there is any
      BasicBlock *succ = sw->getDefaultDest();
      if (succ != nullptr)
      {
         auto antidefaultRange = Range(Empty,bw,Min,Max);
         for(auto case_expr: sw->cases())
         {
            const ConstantInt *constant = case_expr.getCaseValue();
            APInt cval = constant->getValue();
            if (cval.getBitWidth() < MAX_BIT_INT)
               cval = cval.sext(MAX_BIT_INT);
            antidefaultRange = antidefaultRange.unionWith(Range(Regular,bw,cval,cval));
         }

         APInt sigMin = antidefaultRange.getLower();
         APInt sigMax = antidefaultRange.getUpper();
         Range Values = Range(Anti,bw,sigMin,sigMax);
         // Create the interval using the intersection in the branch.
         BasicInterval *BI = new BasicInterval(Values);
         BBsuccs.push_back(std::make_pair(BI, succ));
      }

      // Handle the rest of cases
      for(auto case_expr: sw->cases())
      {
         const ConstantInt *constant = case_expr.getCaseValue();

         APInt cval = constant->getValue();
         if (cval.getBitWidth() < MAX_BIT_INT)
            cval = cval.sext(MAX_BIT_INT);
         Range Values = Range(Regular,bw,cval,cval);
         // Create the interval using the intersection in the branch.
         BasicInterval *BI = new BasicInterval(Values);
         BBsuccs.push_back(std::make_pair(BI, succ));
      }

      ValueSwitchMap VSM(condition, BBsuccs);
      valuesSwitchMap.insert(std::make_pair(condition, VSM));

      if (Op0_0 != nullptr) {
         ValueSwitchMap VSM_0(Op0_0, BBsuccs);
         valuesSwitchMap.insert(std::make_pair(Op0_0, VSM_0));
      }
   }

   void ConstraintGraph::buildValueBranchMap(const BranchInst *br)
   {
      // Verify conditions
      if (!br->isConditional())
         return;
      ICmpInst *ici = dyn_cast<ICmpInst>(br->getCondition());
      if (ici == nullptr)
         return;

      const Type *op0Type = ici->getOperand(0)->getType();
      const Type *op1Type = ici->getOperand(1)->getType();
      if (!op0Type->isIntegerTy() || !op1Type->isIntegerTy())
         return;

      // Create VarNodes for comparison operands explicitly (need to do this when
      // inlining is used!)
      addVarNode(ici->getOperand(0));
      addVarNode(ici->getOperand(1));

      // Gets the successors of the current basic block.
      const BasicBlock *TBlock = br->getSuccessor(0);
      const BasicBlock *FBlock = br->getSuccessor(1);

      // We have a Variable-Constant comparison.
      const Value *Op0 = ici->getOperand(0);
      const Value *Op1 = ici->getOperand(1);
      const ConstantInt *constant = nullptr;
      const Value *variable = nullptr;

      // If both operands are constants, nothing to do here
      if (isa<ConstantInt>(Op0) and isa<ConstantInt>(Op1))
         return;

      // Then there are two cases: variable being compared to a constant,
      // or variable being compared to another variable

      // Op0 is constant, Op1 is variable
      if ((constant = dyn_cast<ConstantInt>(Op0)) != nullptr)
         variable = Op1;
      // Op0 is variable, Op1 is constant
      else if ((constant = dyn_cast<ConstantInt>(Op1)) != nullptr)
         variable = Op0;
      // Both are variables
      // which means constant == 0 and variable == 0

      if (constant != nullptr)
      {
         // Calculate the range of values that would satisfy the comparison.
         ConstantRange CR(constant->getValue(), constant->getValue() + 1);
         CmpInst::Predicate pred = ici->getPredicate();
         CmpInst::Predicate swappred = ici->getSwappedPredicate();

         // If the comparison is in the form (var pred const), we use the actual
         // predicate to build the constant range. Otherwise, (const pred var),
         // we use the swapped predicate
         ConstantRange tmpT =
               (variable == Op0)
               ? ConstantRange::makeSatisfyingICmpRegion(pred, CR)
               : ConstantRange::makeSatisfyingICmpRegion(swappred, CR);

         APInt sigMin = tmpT.getSignedMin();
         APInt sigMax = tmpT.getSignedMax();
         if (sigMin.getBitWidth() < MAX_BIT_INT)
            sigMin = sigMin.sext(MAX_BIT_INT);
         if (sigMax.getBitWidth() < MAX_BIT_INT)
            sigMax = sigMax.sext(MAX_BIT_INT);


         assert(sigMax.sge(sigMin));

         auto bw = variable->getType()->getPrimitiveSizeInBits();
         Range TValues = Range(Regular,bw, sigMin, sigMax);

         Range FValues = Range(Anti,bw, sigMin, sigMax);

         // Create the interval using the intersection in the branch.
         BasicInterval *BT = new BasicInterval(TValues);
         BasicInterval *BF = new BasicInterval(FValues);

         ValueBranchMap VBM(variable, TBlock, FBlock, BT, BF);
         valuesBranchMap.insert(std::make_pair(variable, VBM));

         // Do the same for the operand of variable (if variable is a cast
         // instruction)
         const CastInst *castinst = nullptr;
         if ((castinst = dyn_cast<CastInst>(variable)) != nullptr)
         {
            const Value *variable_0 = castinst->getOperand(0);

            BasicInterval *BT = new BasicInterval(TValues);
            BasicInterval *BF = new BasicInterval(FValues);

            ValueBranchMap VBM(variable_0, TBlock, FBlock, BT, BF);
            valuesBranchMap.insert(std::make_pair(variable_0, VBM));
         }
      }
      // Both operands of the Compare Instruction are variables
      else
      {
         // Create the interval using the intersection in the branch.
         CmpInst::Predicate pred = ici->getPredicate();
         CmpInst::Predicate invPred = ici->getInversePredicate();
         assert(Op0->getType()->getPrimitiveSizeInBits()==Op1->getType()->getPrimitiveSizeInBits());
         Range CR(Unknown,Op0->getType()->getPrimitiveSizeInBits(),Min,Max);

         // Symbolic intervals for op0
         SymbInterval *STOp0 = new SymbInterval(CR, Op1, pred);
         SymbInterval *SFOp0 = new SymbInterval(CR, Op1, invPred);

         ValueBranchMap VBMOp0(Op0, TBlock, FBlock, STOp0, SFOp0);
         valuesBranchMap.insert(std::make_pair(Op0, VBMOp0));

         // Symbolic intervals for operand of op0 (if op0 is a cast instruction)
         const CastInst *castinst = nullptr;
         if ((castinst = dyn_cast<CastInst>(Op0)) != nullptr)
         {
            const Value *Op0_0 = castinst->getOperand(0);

            SymbInterval *STOp1_1 = new SymbInterval(CR, Op1, pred);
            SymbInterval *SFOp1_1 = new SymbInterval(CR, Op1, invPred);

            ValueBranchMap VBMOp1_1(Op0_0, TBlock, FBlock, STOp1_1, SFOp1_1);
            valuesBranchMap.insert(std::make_pair(Op0_0, VBMOp1_1));
         }

         // Symbolic intervals for op1
         SymbInterval *STOp1 = new SymbInterval(CR, Op0, invPred);
         SymbInterval *SFOp1 = new SymbInterval(CR, Op0, pred);
         ValueBranchMap VBMOp1(Op1, TBlock, FBlock, STOp1, SFOp1);
         valuesBranchMap.insert(std::make_pair(Op1, VBMOp1));

         // Symbolic intervals for operand of op1 (if op1 is a cast instruction)
         castinst = nullptr;
         if ((castinst = dyn_cast<CastInst>(Op1)) != nullptr)
         {
            const Value *Op0_0 = castinst->getOperand(0);

            SymbInterval *STOp1_1 = new SymbInterval(CR, Op1, pred);
            SymbInterval *SFOp1_1 = new SymbInterval(CR, Op1, invPred);

            ValueBranchMap VBMOp1_1(Op0_0, TBlock, FBlock, STOp1_1, SFOp1_1);
            valuesBranchMap.insert(std::make_pair(Op0_0, VBMOp1_1));
         }
      }
   }

   void ConstraintGraph::buildValueMaps(const Function &F)
   {
      for (const BasicBlock &BB : F) {
         const TerminatorInst *ti = BB.getTerminator();
         const BranchInst *br = dyn_cast<BranchInst>(ti);
         const SwitchInst *sw = dyn_cast<SwitchInst>(ti);

         if (br != nullptr) {
            buildValueBranchMap(br);
         } else if (sw != nullptr) {
            buildValueSwitchMap(sw);
         }
      }
   }


   /*
 * Used to insert constant in the right position
 */
   void ConstraintGraph::insertConstantIntoVector(APInt constantval)
   {
      if (constantval.getBitWidth() < MAX_BIT_INT)
         constantval = constantval.sext(MAX_BIT_INT);
      constantvector.push_back(constantval);
   }

   /*
 * Get the first constant from vector greater than val
 */
   APInt getFirstGreaterFromVector(const SmallVector<APInt, 2> &constantvector,
                                   const APInt &val)
   {
      for (const APInt &vapint : constantvector)
      {
         if (vapint.sge(val))
            return vapint;
      }
      return Max;
   }

   /*
 * Get the first constant from vector less than val
 */
   APInt getFirstLessFromVector(const SmallVector<APInt, 2> &constantvector,
                                const APInt &val)
   {
      for (auto vit = constantvector.rbegin(), vend = constantvector.rend(); vit != vend; ++vit)
      {
         const APInt &vapint = *vit;
         if (vapint.sle(val))
            return vapint;
      }
      return Min;
   }

   /*
 * Create a vector containing all constants related to the component
 * They include:
 *   - Constants inside component
 *   - Constants that are source of an edge to an entry point
 *   - Constants from intersections generated by sigmas
 */
   void ConstraintGraph::buildConstantVector(
         const SmallPtrSet<VarNode *, 32> &component, const UseMap &compusemap)
   {
      // Remove all elements from the vector
      constantvector.clear();

      // Get constants inside component (TODO: may not be necessary, since
      // components with more than 1 node may
      // never have a constant inside them)
      for (VarNode *varNode : component)
      {
         const Value *V = varNode->getValue();
         const ConstantInt *ci = nullptr;

         if ((ci = dyn_cast<ConstantInt>(V)) != nullptr) {
            insertConstantIntoVector(ci->getValue());
         }
      }

      // Get constants that are sources of operations whose sink belong to the
      // component
      for (VarNode *varNode : component)
      {
         const Value *V = varNode->getValue();
         DefMap::iterator dfit = defMap.find(V);
         if (dfit == defMap.end())
            continue;

         // Handle BinaryOp case
         const BinaryOp *bop = dyn_cast<BinaryOp>(dfit->second);
         const PhiOp *pop = dyn_cast<PhiOp>(dfit->second);
         if (bop != nullptr)
         {
            const VarNode *source1 = bop->getSource1();
            const Value *sourceval1 = source1->getValue();
            const VarNode *source2 = bop->getSource2();
            const Value *sourceval2 = source2->getValue();

            const ConstantInt *const1, *const2;

            if ((const1 = dyn_cast<ConstantInt>(sourceval1)) != nullptr)
               insertConstantIntoVector(const1->getValue());
            if ((const2 = dyn_cast<ConstantInt>(sourceval2)) != nullptr)
               insertConstantIntoVector(const2->getValue());
         }
         // Handle PhiOp case
         else if (pop != nullptr)
         {
            for (unsigned i = 0, e = pop->getNumSources(); i < e; ++i)
            {
               const VarNode *source = pop->getSource(i);
               const Value *sourceval = source->getValue();
               const ConstantInt *consti;
               if ((consti = dyn_cast<ConstantInt>(sourceval)) != nullptr)
                  insertConstantIntoVector(consti->getValue());
            }
         }
      }

      // Get constants used in intersections generated for sigmas
      for (auto &pair : compusemap)
      {
         for (BasicOp *op : pair.second)
         {
            const SigmaOp *sigma = dyn_cast<SigmaOp>(op);
            if (sigma != nullptr)
            {
               // Symbolic intervals are discarded, as they don't have fixed values yet
               if (isa<SymbInterval>(sigma->getIntersect()))
                  continue;
               Range rintersect = sigma->getIntersect()->getRange();
               const APInt &lb = rintersect.getLower();
               const APInt &ub = rintersect.getUpper();
               if (lb.ne(Min) && lb.ne(Max))
                  insertConstantIntoVector(lb);
               if (ub.ne(Min) && ub.ne(Max))
                  insertConstantIntoVector(ub);
            }
         }
      }

      // Sort vector in ascending order and remove duplicates
      std::sort(constantvector.begin(), constantvector.end(),
                [](const APInt &i1, const APInt &i2) { return i1.slt(i2); });

      // std::unique doesn't remove duplicate elements, only
      // move them to the end
      // This is why erase is necessary. To remove these duplicates
      // that will be now at the end.
      SmallVector<APInt, 2>::iterator last =
            std::unique(constantvector.begin(), constantvector.end());

      constantvector.erase(last, constantvector.end());
   }

   /// Iterates through all instructions in the function and builds the graph.
   void ConstraintGraph::buildGraph(Function &F, ModulePass *modulePass, const llvm::DataLayout* DL)
   {
      this->func = &F;
      buildValueMaps(F);
      for (auto &I : instructions(F))
      {
         const Type *ty = I.getType();
         // Only integers are dealt with
         if (!ty->isIntegerTy())
            continue;
         if (!isValidInstruction(&I))
            continue;
         buildOperations(&I, modulePass,DL);
      }
   }

   void ConstraintGraph::buildVarNodes()
   {
      // Initializes the nodes and the use map structure.
      for (auto &pair : vars)
         pair.second->init(this->defMap.count(pair.first) == 0u);
   }

   // FIXME: do it just for component
   void CropDFS::storeAbstractStates(const SmallPtrSet<VarNode *, 32> &component)
   {
      for (VarNode *varNode : component)
         varNode->storeAbstractState();
   }

   bool Meet::fixed(BasicOp *op,
                    const SmallVector<APInt, 2> * /*constantvector*/)
   {
      Range oldInterval = op->getSink()->getRange();
      Range newInterval = op->eval();

      op->getSink()->setRange(newInterval);
      llvm::ModuleSlotTracker MST(op->getInstruction()->getFunction()->getParent());
      MST.incorporateFunction(*op->getInstruction()->getFunction());
      auto instID = MST.getLocalSlot(op->getInstruction());
      LOG_TRANSACTION("FIXED::%" << instID << ": "
                      << oldInterval << " -> " << newInterval);
      return oldInterval != newInterval;
   }

   /// This is the meet operator of the growth analysis. The growth analysis
   /// will change the bounds of each variable, if necessary. Initially, each
   /// variable is bound to either the undefined interval, e.g. [., .], or to
   /// a constant interval, e.g., [3, 15]. After this analysis runs, there will
   /// be no undefined interval. Each variable will be either bound to a
   /// constant interval, or to [-, c], or to [c, +], or to [-, +].
   bool Meet::widen(BasicOp *op, const SmallVector<APInt, 2> *constantvector)
   {
      assert(constantvector != nullptr && "Invalid pointer to constant vector");

      Range oldInterval = op->getSink()->getRange();
      unsigned bw = oldInterval.getBitWidth();
      Range newInterval = op->eval();

      const APInt &oldLower = oldInterval.getLower();
      const APInt &oldUpper = oldInterval.getUpper();
      const APInt &newLower = newInterval.getLower();
      const APInt &newUpper = newInterval.getUpper();

      // Jump-set
      APInt nlconstant = getFirstLessFromVector(*constantvector, newLower);
      APInt nuconstant = getFirstGreaterFromVector(*constantvector, newUpper);

      if (oldInterval.isUnknown())
      {
         op->getSink()->setRange(newInterval);
      }
      else
      {
         if (newLower.slt(oldLower) && newUpper.sgt(oldUpper))
            op->getSink()->setRange(Range(Regular,bw,nlconstant,nuconstant));
         else
         {
            if (newLower.slt(oldLower))
               op->getSink()->setRange(Range(Regular,bw,nlconstant,oldUpper));
            else
            {
               if (newUpper.sgt(oldUpper))
                  op->getSink()->setRange(Range(Regular,bw,oldLower,nuconstant));
            }
         }
      }
      llvm::ModuleSlotTracker MST(op->getInstruction()->getFunction()->getParent());
      MST.incorporateFunction(*op->getInstruction()->getFunction());
      auto instID = MST.getLocalSlot(op->getInstruction());
      Range sinkInterval = oldInterval;
      LOG_TRANSACTION("WIDEN::%" << instID << ": "
                      << oldInterval << " -> " << sinkInterval);
      return oldInterval != sinkInterval;
   }

   bool Meet::growth(BasicOp *op,
                     const SmallVector<APInt, 2> * /*constantvector*/)
   {
      Range oldInterval = op->getSink()->getRange();
      Range newInterval = op->eval();
      if (oldInterval.isUnknown())
         op->getSink()->setRange(newInterval);
      else
      {
         unsigned bw = oldInterval.getBitWidth();
         const APInt &oldLower = oldInterval.getLower();
         const APInt &oldUpper = oldInterval.getUpper();
         const APInt &newLower = newInterval.getLower();
         const APInt &newUpper = newInterval.getUpper();
         if (newLower.slt(oldLower))
         {
            if (newUpper.sgt(oldUpper))
               op->getSink()->setRange(Range(Regular,bw));
            else
               op->getSink()->setRange(Range(Regular,bw,Min,oldUpper));
         }
         else if (newUpper.sgt(oldUpper))
            op->getSink()->setRange(Range(Regular,bw,oldLower,Max));
      }
      Range sinkInterval = op->getSink()->getRange();
      llvm::ModuleSlotTracker MST(op->getInstruction()->getFunction()->getParent());
      MST.incorporateFunction(*op->getInstruction()->getFunction());
      auto instID = MST.getLocalSlot(op->getInstruction());
      LOG_TRANSACTION("GROWTH::%" << instID << ": "
                      << oldInterval << " -> " << sinkInterval);
      return oldInterval != sinkInterval;
   }

   /// This is the meet operator of the cropping analysis. Whereas the growth
   /// analysis expands the bounds of each variable, regardless of intersections
   /// in the constraint graph, the cropping analysis shrinks these bounds back
   /// to ranges that respect the intersections.
   bool Meet::narrow(BasicOp *op,
                     const SmallVector<APInt, 2> * /*constantvector*/)
   {
      const auto& olderInterval = op->getSink()->getRange();
      unsigned bw = olderInterval.getBitWidth();
      APInt oLower = olderInterval.getLower();
      APInt oUpper = olderInterval.getUpper();

      Range newInterval = op->eval();

      const APInt &nLower = newInterval.getLower();
      const APInt &nUpper = newInterval.getUpper();

      bool hasChanged = false;

      if (oLower.eq(Min) && nLower.ne(Min))
      {
         op->getSink()->setRange(Range(Regular,bw, nLower, oUpper));
         hasChanged = true;
      }
      else
      {
         const APInt &smin = APIntOps::smin(oLower, nLower);
         if (oLower.ne(smin))
         {
            op->getSink()->setRange(Range(Regular,bw, smin, oUpper));
            hasChanged = true;
         }
      }

      if (oUpper.eq(Max) && nUpper.ne(Max))
      {
         op->getSink()->setRange(
                  Range(Regular,bw, olderInterval.getLower(), nUpper));
         hasChanged = true;
      }
      else
      {
         const APInt &smax = APIntOps::smax(oUpper, nUpper);
         if (oUpper.ne(smax))
         {
            op->getSink()->setRange(
                     Range(Regular,bw, olderInterval.getLower(), smax));
            hasChanged = true;
         }
      }
      llvm::ModuleSlotTracker MST(op->getInstruction()->getFunction()->getParent());
      MST.incorporateFunction(*op->getInstruction()->getFunction());
      auto instID = MST.getLocalSlot(op->getInstruction());
      LOG_TRANSACTION("NARROW::%" << instID << ": "
                      << Range(Regular,bw, oLower, oUpper) << " -> "
                      << olderInterval);
      return hasChanged;
   }

   bool Meet::crop(BasicOp *op, const SmallVector<APInt, 2> * /*constantvector*/)
   {
      Range oldInterval = op->getSink()->getRange();
      unsigned bw = oldInterval.getBitWidth();
      Range newInterval = op->eval();

      bool hasChanged = false;
      char abstractState = op->getSink()->getAbstractState();

      if ((abstractState == '-' || abstractState == '?') &&
          newInterval.getLower().sgt(oldInterval.getLower()))
      {
         op->getSink()->setRange(
                  Range(Regular,bw, newInterval.getLower(), oldInterval.getUpper()));
         hasChanged = true;
      }

      if ((abstractState == '+' || abstractState == '?') &&
          newInterval.getUpper().slt(oldInterval.getUpper()))
      {
         op->getSink()->setRange(
                  Range(Regular,bw, oldInterval.getLower(), newInterval.getUpper()));
         hasChanged = true;
      }
      llvm::ModuleSlotTracker MST(op->getInstruction()->getFunction()->getParent());
      MST.incorporateFunction(*op->getInstruction()->getFunction());
      auto instID = MST.getLocalSlot(op->getInstruction());
      LOG_TRANSACTION("CROP::%" << instID << ": "
                      << oldInterval << " -> "
                      << oldInterval);
      return hasChanged;
   }

   void Cousot::preUpdate(const UseMap &compUseMap,
                          SmallPtrSet<const Value *, 6> &entryPoints)
   {
      update(compUseMap, entryPoints, Meet::widen);
   }

   void Cousot::posUpdate(const UseMap &compUseMap,
                          SmallPtrSet<const Value *, 6> &entryPoints,
                          const SmallPtrSet<VarNode *, 32> * /*component*/)
   {
      update(compUseMap, entryPoints, Meet::narrow);
   }

   void CropDFS::preUpdate(const UseMap &compUseMap,
                           SmallPtrSet<const Value *, 6> &entryPoints)
   {
      update(compUseMap, entryPoints, Meet::growth);
   }

   void CropDFS::posUpdate(const UseMap &compUseMap,
                           SmallPtrSet<const Value *, 6> & /*entryPoints*/,
                           const SmallPtrSet<VarNode *, 32> *component)
   {
      storeAbstractStates(*component);
      GenOprs::iterator obgn = oprs.begin(), oend = oprs.end();
      for (; obgn != oend; ++obgn)
      {
         BasicOp *op = *obgn;

         if (component->count(op->getSink()) != 0u)
         {
            // int_op
            if (isa<UnaryOp>(op) &&
                (op->getSink()->getRange().getLower().ne(Min) ||
                 op->getSink()->getRange().getUpper().ne(Max)))
            {
               crop(compUseMap, op);
            }
         }
      }
   }

   void CropDFS::crop(const UseMap &compUseMap, BasicOp *op)
   {
      SmallPtrSet<BasicOp *, 8> activeOps;
      SmallPtrSet<const VarNode *, 8> visitedOps;

      // init the activeOps only with the op received
      activeOps.insert(op);

      while (!activeOps.empty())
      {
         BasicOp *V = *activeOps.begin();
         activeOps.erase(V);
         const VarNode *sink = V->getSink();

         // if the sink has been visited go to the next activeOps
         if (visitedOps.count(sink) != 0u)
            continue;

         Meet::crop(V, nullptr);
         visitedOps.insert(sink);

         // The use list.of sink
         const SmallPtrSet<BasicOp *, 8> &L =
               compUseMap.find(sink->getValue())->second;
         for (BasicOp *op : L)
            activeOps.insert(op);
      }
   }

   void ConstraintGraph::update(
         const UseMap &compUseMap, SmallPtrSet<const Value *, 6> &actv,
         bool (*meet)(BasicOp *op, const SmallVector<APInt, 2> *constantvector))
   {
      while (!actv.empty())
      {
         const Value *V = *actv.begin();
         actv.erase(V);

#ifdef STATS
         // Updates Fermap
         if (meet == Meet::narrow) {
            FerMap[V]++;
         }
#endif

         // The use list.
         const SmallPtrSet<BasicOp *, 8> &L = compUseMap.find(V)->second;

         for (BasicOp *op : L)
         {
            if (meet(op, &constantvector))
            {
               // I want to use it as a set, but I also want
               // keep an order or insertions and removals.
               actv.insert(op->getSink()->getValue());
            }
         }
      }
   }

   void ConstraintGraph::update(unsigned nIterations, const UseMap &compUseMap,
                                SmallPtrSet<const Value *, 6> &actv)
   {
      while (!actv.empty())
      {
         const Value *V = *actv.begin();
         actv.erase(V);
         // The use list.
         const SmallPtrSet<BasicOp *, 8> &L = compUseMap.find(V)->second;
         for (BasicOp *op : L)
         {
            if (nIterations == 0)
            {
               actv.clear();
               return;
            }
            --nIterations;
            if (Meet::fixed(op, nullptr))
               actv.insert(op->getSink()->getValue());
         }
      }
   }

   /// Finds the intervals of the variables in the graph.
   void ConstraintGraph::findIntervals()
   {
      //	clearValueMaps();

      // Builds symbMap
#ifdef STATS
      Timer *timer = prof.registerNewTimer(
                        "Nuutila", "Nuutila's algorithm for strongly connected components");
      timer->startTimer();
#endif
      buildSymbolicIntersectMap();

      // List of SCCs
      Nuutila sccList(&vars, &useMap, &symbMap);
#ifdef STATS
      timer->stopTimer();
      prof.addTimeRecord(timer);
      // delete timer;
#endif
      // STATS
      numSCCs += sccList.worklist.size();
#ifdef SCC_DEBUG
      unsigned numberOfSCCs = numSCCs;
#endif

      // For each SCC in graph, do the following
#ifdef STATS
      timer = prof.registerNewTimer("ConstraintSolving", "Constraint solving");
      timer->startTimer();
#endif

      for (Nuutila::iterator nit = sccList.begin(), nend = sccList.end();
           nit != nend; ++nit)
      {
         SmallPtrSet<VarNode *, 32> &component = *sccList.components[*nit];
#ifdef SCC_DEBUG
         --numberOfSCCs;
#endif

         if (component.size() == 1)
         {
            ++numAloneSCCs;
            fixIntersects(component);
            VarNode *var = *component.begin();
            if (var->getRange().isUnknown())
               var->setRange(Range(Regular,var->getValue()->getType()->getPrimitiveSizeInBits(),Min,Max));
         }
         else
         {
            if (component.size() > sizeMaxSCC)
               sizeMaxSCC = component.size();

            UseMap compUseMap = buildUseMap(component);

            // Get the entry points of the SCC
            SmallPtrSet<const Value *, 6> entryPoints;

#ifdef JUMPSET
            // Create vector of constants inside component
            // Comment this line below to deactivate jump-set
            buildConstantVector(component, compUseMap);
#endif

            // generateEntryPoints(component, entryPoints);
            // iterate a fixed number of time before widening
            // update(component.size()*2 /*| NUMBER_FIXED_ITERATIONS*/, compUseMap,
            // entryPoints);

#ifdef DEBUG_RA
            if (func != nullptr)
            {
               printToFile(*func, "/tmp/" + func->getName().str() + "cgfixed.dot");
            }
#endif

            generateEntryPoints(component, entryPoints);
            // First iterate till fix point
            preUpdate(compUseMap, entryPoints);
            fixIntersects(component);

            for (VarNode *varNode : component)
            {
               if (varNode->getRange().isUnknown())
                  varNode->setRange(Range(Regular, varNode->getRange().getBitWidth(), Min, Max));
            }

            // printResultIntervals();
#ifdef DEBUG_RA
            if (func != nullptr)
            {
               printToFile(*func, "/tmp/" + func->getName().str() + "cgint.dot");
            }
#endif

            // Second iterate till fix point
            SmallPtrSet<const Value *, 6> activeVars;
            generateActivesVars(component, activeVars);
            posUpdate(compUseMap, activeVars, &component);
         }
         propagateToNextSCC(component);
      }

#ifdef STATS
      timer->stopTimer();
      prof.addTimeRecord(timer);
#endif

#ifdef SCC_DEBUG
      ASSERT(numberOfSCCs == 0, "Not all SCCs have been visited");
#endif

#ifdef STATS
            timer = prof.registerNewTimer("ComputeStats", "Compute statistics");
      timer->startTimer();

      computeStats();

      timer->stopTimer();
      prof.addTimeRecord(timer);
#endif
   }

   void ConstraintGraph::generateEntryPoints(
         const SmallPtrSet<VarNode *, 32> &component,
         SmallPtrSet<const Value *, 6> &entryPoints)
   {
      // Iterate over the varnodes in the component
      for (VarNode *varNode : component)
      {
         const Value *V = varNode->getValue();

         if (dyn_cast<PHINode>(V) && dyn_cast<PHINode>(V)->getNumOperands()==1)
         {
            DefMap::iterator dit = this->defMap.find(V);
            if (dit != this->defMap.end())
            {
               BasicOp *bop = dit->second;
               SigmaOp *defop = dyn_cast<SigmaOp>(bop);

               if ((defop != nullptr) && defop->isUnresolved())
               {
                  defop->getSink()->setRange(bop->eval());
                  defop->markResolved();
               }
            }
         }
         if (!varNode->getRange().isUnknown())
            entryPoints.insert(V);
      }
   }

   void ConstraintGraph::fixIntersects(const SmallPtrSet<VarNode *, 32> &component)
   {
      // Iterate again over the varnodes in the component
      for (VarNode *varNode : component)
      {
         const Value *V = varNode->getValue();
         SymbMap::iterator sit = symbMap.find(V);
         if (sit != symbMap.end())
         {
            for (BasicOp *op : sit->second)
               op->fixIntersects(varNode);
         }
      }
   }

   void ConstraintGraph::generateActivesVars(
         const SmallPtrSet<VarNode *, 32> &component,
         SmallPtrSet<const Value *, 6> &activeVars)
   {
      for (VarNode *varNode : component)
      {
         const Value *V = varNode->getValue();
         const ConstantInt *CI = dyn_cast<ConstantInt>(V);
         if (CI != nullptr)
            continue;
         activeVars.insert(V);
      }
   }

   // TODO(vhscampos):
   /// Releases the memory used by the graph.
   void ConstraintGraph::clear() {}

   /// Prints the content of the graph in dot format. For more informations
   /// about the dot format, see: http://www.graphviz.org/pdf/dotguide.pdf
   void ConstraintGraph::print(const Function &F, raw_ostream &OS) const
   {
      const char *quot = R"(")";
      // Print the header of the .dot file.
      OS << "digraph dotgraph {\n";
      OS << R"(label="Constraint Graph for ')";
      OS << F.getName() << "\' function\";\n";
      OS << "node [shape=record,fontname=\"Times-Roman\",fontsize=14];\n";

      // Print the body of the .dot file.
      for (auto &pair : vars)
      {
         if (const ConstantInt *C = dyn_cast<ConstantInt>(pair.first))
         {
            OS << " " << C->getValue();
         }
         else
         {
            OS << quot;
            printVarName(pair.first, OS);
            OS << quot;
         }
         OS << R"( [label=")" << pair.second << "\"]\n";
      }

      for (BasicOp *op : oprs)
      {
         op->print(OS);
         OS << '\n';
      }
      OS << pseudoEdgesString.str();
      // Print the footer of the .dot file.
      OS << "}\n";
   }

   void ConstraintGraph::printToFile(const Function &F,
                                     const std::string &FileName)
   {
      std::error_code ErrorInfo;
      raw_fd_ostream file(FileName, ErrorInfo, sys::fs::F_Text);

      if (!file.has_error())
      {
         print(F, file);
         file.close();
      }
      else
      {
         errs() << "ERROR: file " << FileName.c_str() << " can't be opened!\n";
         abort();
      }
   }

   void ConstraintGraph::printResultIntervals()
   {
      for (auto &pair : vars)
      {
         if (const ConstantInt *C = dyn_cast<ConstantInt>(pair.first))
            errs() << C->getValue() << " ";
         else
            printVarName(pair.first, errs());
         pair.second->getRange().print(errs());
         errs() << '\n';
      }
      errs() << '\n';
   }

   void ConstraintGraph::computeStats()
   {
      for (const auto &pair : vars)
      {
         // We only count the instructions that have uses.
         if (pair.first->getNumUses() == 0)
         {
            ++numZeroUses;
            // continue;
         }

         // ConstantInts must NOT be counted!!
         if (isa<ConstantInt>(pair.first))
         {
            ++numConstants;
            continue;
         }

         // Variables that are not IntegerTy are ignored
         if (!pair.first->getType()->isIntegerTy())
         {
            ++numNotInt;
            continue;
         }

         // Count original (used) bits
         unsigned total = pair.first->getType()->getPrimitiveSizeInBits();
         usedBits += total;
         Range CR = pair.second->getRange();

         // If range is unknown, we have total needed bits
         if (CR.isUnknown())
         {
            ++numUnknown;
            needBits += total;
            continue;
         }
         // If range is empty, we have 0 needed bits
         if (CR.isEmpty())
         {
            ++numEmpty;
            continue;
         }
         if (CR.getLower().eq(Min))
         {
            if (CR.getUpper().eq(Max))
               ++numMaxRange;
            else
               ++numMinInfC;
         }
         else if (CR.getUpper().eq(Max))
            ++numCPlusInf;
         else
            ++numCC;


         unsigned ub, lb;

         if (CR.getLower().isNegative())
         {
            APInt abs = CR.getLower().abs();
            lb = abs.getActiveBits() + 1;
         }
         else
            lb = CR.getLower().getActiveBits() + 1;


         if (CR.getUpper().isNegative())
         {
            APInt abs = CR.getUpper().abs();
            ub = abs.getActiveBits() + 1;
         }
         else
            ub = CR.getUpper().getActiveBits() + 1;


         unsigned nBits = lb > ub ? lb : ub;

         // If both bounds are positive, decrement needed bits by 1
         if (!CR.getLower().isNegative() && !CR.getUpper().isNegative())
            --nBits;
         if (nBits < total)
            needBits += nBits;
         else
            needBits += total;
      }

      double totalB = usedBits;
      double needB = needBits;
      double reduction = (totalB - needB) * 100 / totalB;
      percentReduction = static_cast<unsigned int>(reduction);

      numVars += this->vars.size();
      numOps += this->oprs.size();
   }


   /*
 *	This method builds a map that binds each variable label to the
 *operations
 *  where this variable is used.
 */
   UseMap
   ConstraintGraph::buildUseMap(const SmallPtrSet<VarNode *, 32> &component)
   {
      UseMap compUseMap;
      for (SmallPtrSetIterator<VarNode *> vit = component.begin(),
           vend = component.end();
           vit != vend; ++vit)
      {
         const VarNode *var = *vit;
         const Value *V = var->getValue();
         // Get the component's use list for V (it does not exist until we try to get
         // it)
         SmallPtrSet<BasicOp *, 8> &list = compUseMap[V];
         // Get the use list of the variable in component
         UseMap::iterator p = this->useMap.find(V);
         // For each operation in the list, verify if its sink is in the component
         for (BasicOp *opit : p->second)
         {
            VarNode *sink = opit->getSink();
            // If it is, add op to the component's use map
            if (component.count(sink) != 0u)
               list.insert(opit);
         }
      }
      return compUseMap;
   }

   /*
 *	This method builds a map of variables to the lists of operations where
 *  these variables are used as futures. Its C++ type should be something like
 *  map<VarNode, List<Operation>>.
 */
   void ConstraintGraph::buildSymbolicIntersectMap()
   {
      // Creates the symbolic intervals map
      // FIXME: Why this map is beeing recreated?
      symbMap = SymbMap();

      // Iterate over the operations set
      for (BasicOp *op : oprs)
      {
         // If the operation is unary and its interval is symbolic
         UnaryOp *uop = dyn_cast<UnaryOp>(op);
         if ((uop != nullptr) && isa<SymbInterval>(uop->getIntersect()))
         {
            SymbInterval *symbi = cast<SymbInterval>(uop->getIntersect());
            const Value *V = symbi->getBound();
            SymbMap::iterator p = symbMap.find(V);
            if (p != symbMap.end())
               p->second.insert(uop);
            else
            {
               SmallPtrSet<BasicOp *, 8> l;
               l.insert(uop);
               symbMap.insert(std::make_pair(V, l));
            }
         }
      }
   }

   /*
 *	This method evaluates once each operation that uses a variable in
 *  component, so that the next SCCs after component will have entry
 *  points to kick start the range analysis algorithm.
 */
   void ConstraintGraph::propagateToNextSCC(
         const SmallPtrSet<VarNode *, 32> &component)
   {
      for (auto var : component)
      {
         const Value *V = var->getValue();
         auto p = this->useMap.find(V);
         for (BasicOp *op : p->second)
         {
            SigmaOp *sigmaop = dyn_cast<SigmaOp>(op);
            op->getSink()->setRange(op->eval());
            if ((sigmaop != nullptr) && sigmaop->getIntersect()->getRange().isUnknown())
               sigmaop->markUnresolved();
         }
      }
   }

   /*
 *	Adds the edges that ensure that we solve a future before fixing its
 *  interval. I have created a new class: ControlDep edges, to represent
 *  the control dependences. In this way, in order to delete these edges,
 *  one just need to go over the map of uses removing every instance of the
 *  ControlDep class.
 */
   void Nuutila::addControlDependenceEdges(SymbMap *symbMap, UseMap *useMap,
                                           VarNodes *vars)
   {
      for (auto sit = symbMap->begin(), send = symbMap->end(); sit != send; ++sit)
      {
         for (auto opit = sit->second.begin(), opend = sit->second.end(); opit != opend; ++opit)
         {
            // Cria uma operação pseudo-aresta
            VarNodes::iterator source_value = vars->find(sit->first);
            VarNode *source = source_value->second;
            //			if (source_value != vars.end()) {
            //				source = vars.find(sit->first)->second;
            //			}

            //			if (source == nullptr) {
            //				continue;
            //			}
            BasicOp *cdedge = new ControlDep((*opit)->getSink(), source);
            //			BasicOp *cdedge = new
            // ControlDep((cast<UnaryOp>(*opit))->getSource(), source);
            //(*useMap)[(*opit)->getSink()->getValue()].insert(cdedge);
            (*useMap)[sit->first].insert(cdedge);
         }
      }
   }

   /*
 *	Removes the control dependence edges from the constraint graph.
 */
   void Nuutila::delControlDependenceEdges(UseMap *useMap)
   {
      for (UseMap::iterator it = useMap->begin(), end = useMap->end(); it != end;
           ++it)
      {

         std::deque<ControlDep *> ops;

         for (BasicOp *sit : it->second)
         {
            ControlDep *op = nullptr;
            if ((op = dyn_cast<ControlDep>(sit)) != nullptr)
               ops.push_back(op);
         }

         for (ControlDep *op : ops) {
            // Add pseudo edge to the string
            const Value *V = op->getSource()->getValue();
            if (const ConstantInt *C = dyn_cast<ConstantInt>(V))
               pseudoEdgesString << " " << C->getValue() << " -> ";
            else
            {
               pseudoEdgesString << " " << '"';
               printVarName(V, pseudoEdgesString);
               pseudoEdgesString << '"' << " -> ";
            }
            const Value *VS = op->getSink()->getValue();
            pseudoEdgesString << '"';
            printVarName(VS, pseudoEdgesString);
            pseudoEdgesString << '"';
            pseudoEdgesString << " [style=dashed]\n";
            // Remove pseudo edge from the map
            it->second.erase(op);
         }
      }
   }

   /*
 *	Finds SCCs using Nuutila's algorithm. This algorithm is divided in
 *  two parts. The first calls the recursive visit procedure on every node
 *  in the constraint graph. The second phase revisits these nodes,
 *  grouping them in components.
 */
   void Nuutila::visit(Value *V, std::stack<Value *> &stack, UseMap *useMap)
   {
      dfs[V] = index;
      ++index;
      root[V] = V;

      // Visit every node defined in an instruction that uses V
      for (SmallPtrSetIterator<BasicOp *> sit = (*useMap)[V].begin(),
           send = (*useMap)[V].end();
           sit != send; ++sit)
      {
         BasicOp *op = *sit;
         Value *name = const_cast<Value *>(op->getSink()->getValue());
         if (dfs[name] < 0)
            visit(name, stack, useMap);
         if ((!static_cast<bool>(inComponent.count(name))) &&
             (dfs[root[V]] >= dfs[root[name]]))
            root[V] = root[name];
      }

      // The second phase of the algorithm assigns components to stacked nodes
      if (root[V] == V)
      {
         // Neither the worklist nor the map of components is part of Nuutila's
         // original algorithm. We are using these data structures to get a
         // topological ordering of the SCCs without having to go over the root
         // list once more.
         worklist.push_back(V);

         SmallPtrSet<VarNode *, 32> *SCC = new SmallPtrSet<VarNode *, 32>;

         SCC->insert((*variables)[V]);

         inComponent.insert(V);

         while (!stack.empty() && (dfs[stack.top()] > dfs[V]))
         {
            Value *node = stack.top();
            stack.pop();
            inComponent.insert(node);
            SCC->insert((*variables)[node]);
         }
         components[V] = SCC;
      }
      else
         stack.push(V);
   }

   /*
 *	Finds the strongly connected components in the constraint graph formed
 *by
 *	Variables and UseMap. The class receives the map of futures to insert
 *the
 *  control dependence edges in the contraint graph. These edges are removed
 *  after the class is done computing the SCCs.
 */
   Nuutila::Nuutila(VarNodes *varNodes, UseMap *useMap, SymbMap *symbMap,
                    bool single)
   {
      if (single)
      {
         /* FERNANDO */
         SmallPtrSet<VarNode *, 32> *SCC = new SmallPtrSet<VarNode *, 32>;
         for (VarNodes::iterator vit = varNodes->begin(), vend = varNodes->end(); vit != vend; ++vit)
            SCC->insert(vit->second);

         for (VarNodes::iterator vit = varNodes->begin(), vend = varNodes->end(); vit != vend; ++vit)
         {
            Value *V = const_cast<Value *>(vit->first);
            components[V] = SCC;
         }
         if (!varNodes->empty())
            this->worklist.push_back(const_cast<Value *>(varNodes->begin()->first));
      }
      else
      {
         // Copy structures
         this->variables = varNodes;
         this->index = 0;

         // Iterate over all varnodes of the constraint graph
         for (VarNodes::iterator vit = varNodes->begin(), vend = varNodes->end(); vit != vend; ++vit)
         {
            // Initialize DFS control variable for each Value in the graph
            Value *V = const_cast<Value *>(vit->first);
            dfs[V] = -1;
         }
         addControlDependenceEdges(symbMap, useMap, varNodes);
         // Iterate again over all varnodes of the constraint graph
         for (VarNodes::iterator vit = varNodes->begin(), vend = varNodes->end(); vit != vend; ++vit)
         {
            Value *V = const_cast<Value *>(vit->first);
            // If the Value has not been visited yet, call visit for him
            if (dfs[V] < 0)
            {
               std::stack<Value *> pilha;
               visit(V, pilha, useMap);
            }
         }
         delControlDependenceEdges(useMap);
      }

#ifdef SCC_DEBUG
      ASSERT(checkWorklist(), "an inconsistency in SCC worklist have been found");
      ASSERT(checkComponents(), "a component has been used more than once");
      ASSERT(checkTopologicalSort(useMap), "topological sort is incorrect");
#endif
   }

   Nuutila::~Nuutila()
   {
      for (auto mit = components.begin(), mend = components.end(); mit != mend; ++mit)
         delete mit->second;
   }

#ifdef SCC_DEBUG
   bool Nuutila::checkWorklist()
   {
      bool consistent = true;
      for (Nuutila::iterator nit = this->begin(), nend = this->end(); nit != nend;
           ++nit) {
         Value *v = *nit;
         for (Nuutila::iterator nit2 = this->begin(), nend2 = this->end();
              nit2 != nend2; ++nit2) {
            Value *v2 = *nit2;
            if (v == v2 && nit != nit2) {
               errs() << "[Nuutila::checkWorklist] Duplicated entry in worklist\n";
               v->dump();
               consistent = false;
            }
         }
      }
      return consistent;
   }

   bool Nuutila::checkComponents() {
      bool isConsistent = true;
      for (Nuutila::iterator nit = this->begin(), nend = this->end(); nit != nend;
           ++nit) {
         SmallPtrSet<VarNode *, 32> *component = this->components[*nit];
         for (Nuutila::iterator nit2 = this->begin(), nend2 = this->end();
              nit2 != nend2; ++nit2) {
            SmallPtrSet<VarNode *, 32> *component2 = this->components[*nit2];
            if (component == component2 && nit != nit2) {
               errs() << "[Nuutila::checkComponent] Component [" << component << ", "
                      << component->size() << "]\n";
               isConsistent = false;
            }
         }
      }
      return isConsistent;
   }

   /**
 * Check if a component has an edge to another component
 */
   bool Nuutila::hasEdge(SmallPtrSet<VarNode *, 32> *componentFrom,
                         SmallPtrSet<VarNode *, 32> *componentTo, UseMap *useMap) {
      for (SmallPtrSetIterator<VarNode *> vit = componentFrom->begin(),
           vend = componentFrom->end();
           vit != vend; ++vit) {
         const Value *source = (*vit)->getValue();
         for (SmallPtrSetIterator<BasicOp *> sit = (*useMap)[source].begin(),
              send = (*useMap)[source].end();
              sit != send; ++sit) {
            BasicOp *op = *sit;
            if (componentTo->count(op->getSink())) {
               return true;
            }
         }
      }
      return false;
   }

   bool Nuutila::checkTopologicalSort(UseMap *useMap) {
      bool isConsistent = true;
      DenseMap<SmallPtrSet<VarNode *, 32> *, bool> visited;
      for (Nuutila::iterator nit = this->begin(), nend = this->end(); nit != nend;
           ++nit) {
         SmallPtrSet<VarNode *, 32> *component = this->components[*nit];
         visited[component] = false;
      }

      for (Nuutila::iterator nit = this->begin(), nend = this->end(); nit != nend;
           ++nit) {
         SmallPtrSet<VarNode *, 32> *component = this->components[*nit];

         if (!visited[component]) {
            visited[component] = true;
            // check if this component points to another component that has already
            // been visited
            for (Nuutila::iterator nit2 = this->begin(), nend2 = this->end();
                 nit2 != nend2; ++nit2) {
               SmallPtrSet<VarNode *, 32> *component2 = this->components[*nit2];
               if (nit != nit2 && visited[component2] &&
                   hasEdge(component, component2, useMap)) {
                  isConsistent = false;
               }
            }
         } else {
            errs() << "[Nuutila::checkTopologicalSort] Component visited more than "
                      "once time\n";
         }
      }

      return isConsistent;
   }

#endif

   InterProceduralRACropDFSHelper::~InterProceduralRACropDFSHelper()
   {
#ifdef STATS
      // TODO(vhscampos): Deprecated by the usage of llvm::Timer
      //  prof.printTime("BuildGraph");
      //  prof.printTime("Nuutila");
      //  prof.printTime("SCCs resolution");
      //  prof.printTime("ComputeStats");
      prof.printMemoryUsage();

      std::ostringstream formated;
      formated << 100 * (1.0 - (static_cast<double>(needBits) / usedBits));
      errs() << formated.str() << "\t - "
             << " Percentage of reduction\n";

      // max visit computation
      unsigned maxtimes = 0;
      for (auto &pair : FerMap) {
         unsigned times = pair.second;
         if (times > maxtimes) {
            maxtimes = times;
         }
      }
      maxVisit = maxtimes;
      errs() << "Initial number of bits: " << usedBits << "\n";
      errs() << "Needed bits: " << needBits << "\n";
      errs() << "Percentage of reduction of the number of bits: " << percentReduction << "\n";
      errs() << "Number of strongly connected components: " << numSCCs << "\n";
      errs() << "Number of SCCs containing only one node: " << numAloneSCCs << "\n";
      errs() << "Size of largest SCC: " << sizeMaxSCC << "\n";
      errs() << "Number of variables: " << numVars << "\n";
      errs() << "Number of unknown variables: " << numUnknown << "\n";
      errs() << "Number of empty-set variables: " << numEmpty << "\n";
      errs() << "Number of variables [c, +inf]: " << numCPlusInf << "\n";
      errs() << "Number of variables [c, c]: " <<numCC  << "\n";
      errs() << "Number of variables [-inf, c]: " << numMinInfC << "\n";
      errs() << "Number of variables [-inf, +inf]: " << numMaxRange << "\n";
      errs() << "Number of constants: " << numConstants << "\n";
      errs() << "Number of variables without any use: " << numZeroUses << "\n";
      errs() << "Number of variables that are not Integer: " << numNotInt << "\n";
      errs() << "Number of operations: " << numOps << "\n";
      errs() << "Max number of times a value has been visited: " << maxVisit << "\n";
      errs() << "MAX_BIT_INT: " << MAX_BIT_INT << "\n";

#endif
   }

   bool InterProceduralRACropDFSHelper::runOnModule(Module &M, ModulePass *modulePass)
   {
      // Constraint Graph
      //	if(CG) delete CG;
      CG = new CropDFS();

      MAX_BIT_INT = getMaxBitWidth(M);
      updateConstantIntegers(MAX_BIT_INT);

      // Build the Constraint Graph by running on each function
#ifdef STATS
      Timer *timer = prof.registerNewTimer("BuildGraph", "Build constraint graph");
      timer->startTimer();
#endif
      for (Function &F : M.functions()) {
         // If the function is only a declaration, or if it has variable number of
         // arguments, do not match
         if (F.isDeclaration() || F.isVarArg()) {
            continue;
         }

         CG->buildGraph(F, modulePass, &M.getDataLayout());
         MatchParametersAndReturnValues(F, *CG);
      }
      CG->buildVarNodes();

#ifdef STATS
      timer->stopTimer();
      prof.addTimeRecord(timer);

      prof.registerMemoryUsage();
#endif
#ifdef DEBUG_RA
      std::string moduleIdentifier = M.getModuleIdentifier();
      int pos = moduleIdentifier.rfind('/');
      std::string mIdentifier =
            pos > 0 ? moduleIdentifier.substr(pos) : moduleIdentifier;
      CG->printToFile(*(M.begin()), "/tmp/" + mIdentifier + ".cgpre.dot");
#endif
      CG->findIntervals();
#ifdef DEBUG_RA
      CG->printToFile(*(M.begin()), "/tmp/" + mIdentifier + ".cgpos.dot");
#endif

      return false;
   }

   unsigned InterProceduralRACropDFSHelper::getMaxBitWidth(Module &M)
   {
      unsigned max = 0U;
      // Search through the functions for the max int bitwidth
      for (const Function &F : M.functions()) {
         if (!F.isDeclaration()) {
            unsigned bitwidth = RangeAnalysis::getMaxBitWidth(F);

            if (bitwidth > max) {
               max = bitwidth;
            }
         }
      }
      return max+1;
   }

   void InterProceduralRACropDFSHelper::MatchParametersAndReturnValues(
         Function &F, ConstraintGraph &G) {
      // Only do the matching if F has any use
      if (!F.hasNUsesOrMore(1)) {
         return;
      }

      // Data structure which contains the matches between formal and real
      // parameters
      // First: formal parameter
      // Second: real parameter
      SmallVector<std::pair<Value *, Value *>, 4> parameters(F.arg_size());

      // Fetch the function arguments (formal parameters) into the data structure
      Function::arg_iterator argptr;
      Function::arg_iterator e;
      unsigned i;

      for (i = 0, argptr = F.arg_begin(), e = F.arg_end(); argptr != e;
           ++i, ++argptr) {
         parameters[i].first = &*argptr;
      }

      // Check if the function returns a supported value type. If not, no return
      // value matching is done
      bool noReturn = F.getReturnType()->isVoidTy();

      // Creates the data structure which receives the return values of the
      // function, if there is any
      SmallPtrSet<Value *, 4> returnValues;

      if (!noReturn) {
         // Iterate over the basic blocks to fetch all possible return values
         for (BasicBlock &BB : F) {
            // Get the terminator instruction of the basic block and check if it's
            // a return instruction: if it's not, continue to next basic block
            Instruction *terminator = BB.getTerminator();

            ReturnInst *RI = dyn_cast<ReturnInst>(terminator);

            if (RI == nullptr) {
               continue;
            }

            // Get the return value and insert in the data structure
            returnValues.insert(RI->getReturnValue());
         }
      }

      // For each use of F, get the real parameters and the caller instruction to do
      // the matching
      SmallVector<PhiOp *, 4> matchers(F.arg_size(), nullptr);

      for (unsigned i = 0, e = parameters.size(); i < e; ++i)
      {
         VarNode *sink = G.addVarNode(parameters[i].first);

         matchers[i] = new PhiOp(new BasicInterval(), sink, nullptr);

         // Insert the operation in the graph.
         G.getOprs()->insert(matchers[i]);

         // Insert this definition in defmap
         (*G.getDefMap())[sink->getValue()] = matchers[i];
      }

      // For each return value, create a node
      SmallVector<VarNode *, 4> returnVars;

      for (Value *returnValue : returnValues) {
         // Add VarNode to the CG
         VarNode *from = G.addVarNode(returnValue);

         returnVars.push_back(from);
      }

      for (Use &U : F.uses()) {
         User *Us = U.getUser();

         // Ignore blockaddress uses
         if (isa<BlockAddress>(Us)) {
            continue;
         }

         // Used by a non-instruction, or not the callee of a function, do not
         // match.
         if (!isa<CallInst>(Us) && !isa<InvokeInst>(Us)) {
            continue;
         }

         Instruction *caller = cast<Instruction>(Us);

         CallSite CS(caller);

         if (!CS.isCallee(&U)) {
            continue;
         }

         // Iterate over the real parameters and put them in the data structure
         CallSite::arg_iterator AI;
         CallSite::arg_iterator EI;

         for (i = 0, AI = CS.arg_begin(), EI = CS.arg_end(); AI != EI; ++i, ++AI) {
            parameters[i].second = *AI;
         }

         // // Do the interprocedural construction of CG
         VarNode *to = nullptr;
         VarNode *from = nullptr;

         // Match formal and real parameters
         for (i = 0; i < parameters.size(); ++i) {
            // Add real parameter to the CG
            from = G.addVarNode(parameters[i].second);

            // Connect nodes
            matchers[i]->addSource(from);

            // Inserts the sources of the operation in the use map list.
            G.getUseMap()->find(from->getValue())->second.insert(matchers[i]);
         }

         // Match return values
         if (!noReturn) {
            // Add caller instruction to the CG (it receives the return value)
            to = G.addVarNode(caller);

            PhiOp *phiOp = new PhiOp(new BasicInterval(), to, nullptr);

            // Insert the operation in the graph.
            G.getOprs()->insert(phiOp);

            // Insert this definition in defmap
            (*G.getDefMap())[to->getValue()] = phiOp;

            for (VarNode *var : returnVars) {
               phiOp->addSource(var);

               // Inserts the sources of the operation in the use map list.
               G.getUseMap()->find(var->getValue())->second.insert(phiOp);
            }
         }

         // Real parameters are cleaned before moving to the next use (for safety's
         // sake)
         for (auto &pair : parameters) {
            pair.second = nullptr;
         }
      }
   }

   APInt InterProceduralRACropDFSHelper::getMin() { return Min; }

   APInt InterProceduralRACropDFSHelper::getMax() { return Max; }

   Range InterProceduralRACropDFSHelper::getRange(const Value *v) {
      return CG->getRange(v);
   }


} // namespace RangeAnalysis
