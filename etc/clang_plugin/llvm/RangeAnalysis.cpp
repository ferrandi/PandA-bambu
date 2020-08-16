/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2018-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
  * The implementation started from code available at this link:https://code.google.com/archive/p/range-analysis/.
  * The original code went through a deep revision and it has been changed to be compatible to a recent version of LLVM.
  * Some extensions have been also added:
  * - Added anti range support
  * - Redesigned many Range operations to take into account wrapping and to
  *   improve the reductions performed
  * - Integrated the LLVM lazy value range analysis.
  * - Added support to range value propagation of load from constant arrays.
  * - Added support to range value propagation of load and store from generic arrays.
  *
  * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
  *
//===-------------------------- RangeAnalysis.cpp -------------------------===//
//===-----        Performs the Range analysis of the variables        -----===//
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
*/

#include "RangeAnalysis.hpp"

#include <cassert>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/ilist_iterator.h"
#include "llvm/ADT/iterator.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LazyValueInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/PassSupport.h"
#include "llvm/Support/FileSystem.h"

#if __clang_major__ == 4
#include "llvm/Transforms/Utils/MemorySSA.h"
#else
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Support/KnownBits.h"
#endif

#if HAVE_LIBBDD
#include "HardekopfLin_AA.hpp"
#endif

#define DEBUG_TYPE "range-analysis"

//************************** Log Transactions ********************************//
#ifdef LOG_TRANSACTIONS
std::error_code _log_ErrorInfo;
llvm::Twine _log_fileName = "/tmp/ratransactions";
llvm::raw_fd_ostream _log_file(_log_fileName.str().c_str(), _log_ErrorInfo, llvm::sys::fs::F_Text);
#define LOG_TRANSACTION(str) _log_file << str << "\n"
#define FINISH_LOG _log_file.close();
#else
#define LOG_TRANSACTION(str)
#define FINISH_LOG
#endif

using namespace llvm;

namespace RangeAnalysis
{
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
   STATISTIC(numAnti, "Number of variables )c, c(.");
   STATISTIC(numMinInfC, "Number of variables [-inf, c].");
   STATISTIC(numMaxRange, "Number of variables [-inf, +inf].");
   STATISTIC(numConstants, "Number of constants.");
   STATISTIC(numZeroUses, "Number of variables without any use.");
   STATISTIC(numNotInt, "Number of variables that are not Integer.");
   STATISTIC(numOps, "Number of operations");

   namespace
   {
      // The number of bits needed to store the largest variable of the function (APInt).
      unsigned MAX_BIT_INT = 1;
      unsigned POINTER_BITSIZE = 32;

      // ========================================================================== //
      // Static global functions and definitions
      // ========================================================================== //

      APInt Min, Max, Zero, One;

      // Used to print pseudo-edges in the Constraint Graph dot
      std::string pestring;
      raw_string_ostream pseudoEdgesString(pestring);

#ifdef STATS
      // Used to profile
      Profile prof;
#endif

      // Print name of variable according to its type
      void printVarName(const Value* V, raw_ostream& OS)
      {
         const Argument* A = nullptr;
         const Instruction* I = nullptr;

         if((A = dyn_cast<Argument>(V)) != nullptr)
         {
            const llvm::Function* currentFunction = A->getParent();
            llvm::ModuleSlotTracker MST(currentFunction->getParent());
            MST.incorporateFunction(*currentFunction);
            auto argID = MST.getLocalSlot(A);
            OS << A->getParent()->getName() << ".%" << argID;
         }
         else if((I = dyn_cast<Instruction>(V)) != nullptr)
         {
            llvm::ModuleSlotTracker MST(I->getParent()->getParent()->getParent());
            MST.incorporateFunction(*I->getParent()->getParent());
            auto instID = MST.getLocalSlot(I);
            auto BBID = MST.getLocalSlot(I->getParent());

            OS << I->getFunction()->getName() << ".BB" << BBID;
            if(instID < 0)
            {
               I->print(OS);
            }
            else
            {
               OS << ".%" << instID;
            }
         }
         else
         {
            OS << V->getName();
         }
      }

      /// Selects the instructions that we are going to evaluate.
      bool isValidInstruction(const Instruction* I)
      {
         switch(I->getOpcode())
         {
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
            case Instruction::Load:
            case Instruction::Store:
               return true;
            default:
               return false;
         }
      }

      Range CR2R(const ConstantRange& CR, unsigned bw)
      {
         assert(!CR.isFullSet());
         const auto& l = CR.getLower();
         const auto& u = CR.getUpper();
         if(CR.isEmptySet())
         {
            return Range(Empty, bw);
         }
         if(CR.isSignWrappedSet())
         {
            return Range(Anti, bw, u.sext(MAX_BIT_INT), (l - 1).sext(MAX_BIT_INT));
         }
         if(!CR.isWrappedSet())
         {
            return Range(Regular, bw, l.sext(MAX_BIT_INT), (u - 1).sext(MAX_BIT_INT));
         }
         if(!CR.isSignWrappedSet())
         {
            return Range(Regular, bw, l.sext(MAX_BIT_INT), (u - 1).sext(MAX_BIT_INT));
         }

         llvm_unreachable("unexpected case");
      }

      Range getLLVM_range(const Instruction* I, ModulePass* modulePass, const llvm::DataLayout* DL)
      {
         auto bw = I->getType()->getPrimitiveSizeInBits();
         if(bw == 1)
         {
            return Range(Regular, bw, Zero, One);
         }
         if(modulePass == nullptr || DL == nullptr)
         {
            return Range(Regular, bw);
         }
         auto Fun = const_cast<llvm::Function*>(I->getFunction());
         assert(modulePass);
         unsigned long long int zeroMask = 0;
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
         auto& LVI = modulePass->getAnalysis<llvm::LazyValueInfoWrapperPass>(*Fun).getLVI();
         const Value* VI = I;
         llvm::ConstantRange range = LVI.getConstantRange(const_cast<llvm::Value*>(VI), const_cast<llvm::BasicBlock*>(I->getParent()));
         assert(!range.isEmptySet());
         auto ty = I->getType();
         auto obj_size = ty->isSized() ? DL->getTypeAllocSizeInBits(ty) : 8ULL;
         auto active_size = ty->isSized() ? DL->getTypeSizeInBits(ty) : 8ULL;
         auto i = active_size;
         for(; i > 1; --i)
         {
            if((zeroMask & (1ULL << (i - 1))) == 0)
            {
               break;
            }
         }
         if(i != active_size)
         {
            ++i;
            if(i < active_size)
            {
               active_size = i;
            }
         }
         if(obj_size != active_size)
         {
            assert(active_size < 64);

            APInt l(MAX_BIT_INT, -(1LL << (active_size - 1)), true);
            APInt u(MAX_BIT_INT, (1LL << (active_size - 1)) - 1, true);
            if(range.isEmptySet())
            {
               return Range(Regular, bw, l, u);
            }
            if(range.getSignedMin().sext(MAX_BIT_INT).sgt(l))
            {
               l = range.getSignedMin().sext(MAX_BIT_INT);
            }
            if(range.getSignedMax().sext(MAX_BIT_INT).slt(u))
            {
               u = range.getSignedMax().sext(MAX_BIT_INT);
            }
            return Range(Regular, bw, l, u);
         }
         if(!range.isFullSet())
         {
            return CR2R(range, bw);
         }

         return Range(Regular, bw);
      }

      Range getLLVM_range(const Constant* CV)
      {
         auto vid = CV->getValueID();
         switch(vid)
         {
            case llvm::Value::ConstantAggregateZeroVal:
            {
               auto type = CV->getType();
               if(dyn_cast<llvm::ArrayType>(type) || dyn_cast<llvm::VectorType>(type))
               {
                  auto elmtType = cast<llvm::SequentialType>(type)->getElementType();
                  assert(elmtType->isIntegerTy());
                  auto bw = elmtType->getPrimitiveSizeInBits();
                  return Range(Regular, bw, Zero, Zero);
               }

               llvm_unreachable("unexpected case");
            }
            case llvm::Value::ConstantArrayVal:
            {
               const auto* val = cast<const llvm::ConstantArray>(CV);
               auto bw = val->getType()->getArrayElementType()->getPrimitiveSizeInBits();
               Range res(Empty, bw);
               for(unsigned index = 0; index < val->getNumOperands(); ++index)
               {
                  auto elmnt = val->getOperand(index);
                  assert(dyn_cast<llvm::ConstantInt>(elmnt));
                  auto ci = cast<llvm::ConstantInt>(elmnt)->getValue();
                  res = res.unionWith(Range(Regular, bw, ci.sext(MAX_BIT_INT), ci.sext(MAX_BIT_INT)));
               }
               return res;
            }
            case llvm::Value::ConstantDataArrayVal:
            case llvm::Value::ConstantDataVectorVal:
            {
               const auto* val = cast<const llvm::ConstantDataSequential>(CV);
               auto bw = val->getElementType()->getPrimitiveSizeInBits();
               Range res(Empty, bw);
               for(unsigned index = 0; index < val->getNumElements(); ++index)
               {
                  auto elmnt = val->getElementAsConstant(index);
                  assert(dyn_cast<llvm::ConstantInt>(elmnt));
                  auto ci = cast<llvm::ConstantInt>(elmnt)->getValue();
                  res = res.unionWith(Range(Regular, bw, ci.sext(MAX_BIT_INT), ci.sext(MAX_BIT_INT)));
               }
               return res;
            }
            case llvm::Value::ConstantIntVal:
            {
               const auto* val = cast<const llvm::ConstantInt>(CV);
               auto bw = CV->getType()->getPrimitiveSizeInBits();
               return Range(Regular, bw, val->getValue().sext(MAX_BIT_INT), val->getValue().sext(MAX_BIT_INT));
            }

            default:
               llvm::errs() << "getLLVM_range kind not supported: ";
               CV->print(llvm::errs());
               llvm::errs() << "\n";
               llvm_unreachable("Unexpected case");
         }
         llvm_unreachable("unexpected condition");
      }
      unsigned ObjectBW(const llvm::Value* varValue)
      {
         assert(varValue);
         auto vid = varValue->getValueID();
         assert(vid == llvm::Value::InstructionVal + llvm::Instruction::Alloca || vid == llvm::Value::GlobalVariableVal);
         unsigned int elmtSize = 0;
         if(auto AI = llvm::dyn_cast<const llvm::AllocaInst>(varValue))
         {
            if(AI->getAllocatedType()->isArrayTy())
            {
               if(cast<llvm::ArrayType>(AI->getAllocatedType())->getArrayElementType()->isPointerTy())
               {
                  elmtSize = POINTER_BITSIZE;
               }
               else
               {
                  elmtSize = cast<llvm::ArrayType>(AI->getAllocatedType())->getArrayElementType()->getPrimitiveSizeInBits();
               }
            }
            else if(AI->getAllocatedType()->isPointerTy())
            {
               elmtSize = POINTER_BITSIZE;
            }
            else
            {
               elmtSize = AI->getAllocatedType()->getPrimitiveSizeInBits();
            }
         }
         else if(auto GV = llvm::dyn_cast<const llvm::GlobalVariable>(varValue))
         {
            auto VGT = GV->getValueType();
            if(VGT->isAggregateType())
            {
               if(VGT->isArrayTy())
               {
                  if(cast<llvm::ArrayType>(VGT)->getElementType()->isPointerTy())
                  {
                     elmtSize = POINTER_BITSIZE;
                  }
                  else
                  {
                     elmtSize = cast<llvm::ArrayType>(VGT)->getElementType()->getPrimitiveSizeInBits();
                  }
               }
               else
               {
                  return 0;
               }
            }
            else if(VGT->isPointerTy())
            {
               elmtSize = POINTER_BITSIZE;
            }
            else
            {
               elmtSize = VGT->getPrimitiveSizeInBits();
            }
         }
         else
         {
            varValue->print(llvm::errs());
            llvm_unreachable("unexpected condition");
         }
         return elmtSize;
      }
      unsigned LoadStoreOperationBW(const Value* V, const Value* GV, const llvm::DataLayout* DL)
      {
         if(auto ST = dyn_cast<StoreInst>(V))
         {
            return ST->getValueOperand()->getType()->isPointerTy() ? POINTER_BITSIZE : ST->getValueOperand()->getType()->getPrimitiveSizeInBits();
         }
         if(dyn_cast<LoadInst>(V))
         {
            return V->getType()->isPointerTy() ? POINTER_BITSIZE : V->getType()->getPrimitiveSizeInBits();
         }
         if(GV)
         {
            return ObjectBW(GV);
         }
         if(V->getType()->isPointerTy())
         {
            return POINTER_BITSIZE;
         }
         if(auto bw = V->getType()->getPrimitiveSizeInBits())
         {
            return bw;
         }
         if(auto IV = dyn_cast<InsertValueInst>(V))
         {
            return IV->getInsertedValueOperand()->getType()->isPointerTy() ? POINTER_BITSIZE : IV->getInsertedValueOperand()->getType()->getPrimitiveSizeInBits();
         }
         if(V->getType()->isSized())
         {
            return DL->getTypeAllocSizeInBits(V->getType());
         }

         V->getType()->print(llvm::errs());
         llvm::errs() << "\n";
         if(V->getType()->isVectorTy())
         {
            llvm::errs() << "Is a vector type\n";
         }
         if(V->getType()->isStructTy())
         {
            llvm::errs() << "Is a struct type\n";
         }

         V->print(llvm::errs());
         llvm::errs() << "\n";
         if(GV)
         {
            GV->print(llvm::errs());
            llvm::errs() << "\n";
         }
         llvm_unreachable("unexpected condition");
      }

   } // end anonymous namespace

   // ========================================================================== //
   // RangeAnalysis
   // ========================================================================== //
   unsigned RangeAnalysis::getMaxBitWidth(const Function& F)
   {
      unsigned int InstBitSize = 0, opBitSize = 0, max = 0;

      // Obtains the maximum bit width of the instructions of the function.
      for(const Instruction& I : instructions(F))
      {
         InstBitSize = I.getType()->getPrimitiveSizeInBits();
         if(I.getType()->isIntegerTy() && InstBitSize > max)
         {
            max = InstBitSize;
         }
         // Obtains the maximum bit width of the operands of the instruction.
         for(const Value* operand : I.operands())
         {
            opBitSize = operand->getType()->getPrimitiveSizeInBits();
            if(operand->getType()->isIntegerTy() && opBitSize > max)
            {
               max = opBitSize;
            }
         }
      }
      // Bit-width equal to 0 is not valid, so we increment to 1
      if(max == 0)
      {
         ++max;
      }
      return max;
   }

   void RangeAnalysis::updateConstantIntegers(unsigned maxBitWidth)
   {
      // Updates the Min and Max values.
      Min = APInt::getSignedMinValue(maxBitWidth);
      Max = APInt::getSignedMaxValue(maxBitWidth);
      Zero = APInt(MAX_BIT_INT, 0UL, true);
      One = APInt(MAX_BIT_INT, 1UL, true);
   }

   // ========================================================================== //
   // Range
   // ========================================================================== //
   Range_base::Range_base(RangeType rType, unsigned rbw) : l(Min), u(Max), bw(rbw), type(rType)
   {
      assert(rbw);
   }

   void Range_base::normalizeRange(const APInt& lb, const APInt& ub, RangeType rType)
   {
      if(rType == Empty || rType == Unknown)
      {
         l = Min;
         u = Max;
      }
      else if(rType == Anti)
      {
         if(lb.sgt(ub))
         {
            type = Regular;
            l = Min;
            u = Max;
         }
         else
         {
            if(lb.eq(Min) && ub.eq(Max))
            {
               type = Empty;
            }
            else if(lb.eq(Min))
            {
               type = Regular;
               l = ub + 1;
               u = Max;
            }
            else if(ub.eq(Max))
            {
               type = Regular;
               l = Min;
               u = lb - 1;
            }
            else
            {
               assert(ub.sge(lb));
               auto maxS = APInt::getSignedMaxValue(bw);
               maxS = maxS.sext(MAX_BIT_INT);
               auto minS = APInt::getSignedMinValue(bw);
               minS = minS.sext(MAX_BIT_INT);
               bool lbgt = lb.sgt(maxS);
               bool ubgt = ub.sgt(maxS);
               bool lblt = lb.slt(minS);
               bool ublt = ub.slt(minS);
               if(lbgt && ubgt)
               {
                  l = lb.trunc(bw).sext(MAX_BIT_INT);
                  u = ub.trunc(bw).sext(MAX_BIT_INT);
               }
               else if(lblt && ublt)
               {
                  l = ub.trunc(bw).sext(MAX_BIT_INT);
                  u = lb.trunc(bw).sext(MAX_BIT_INT);
               }
               else if(!lblt && ubgt)
               {
                  auto ubnew = ub.trunc(bw).sext(MAX_BIT_INT);
                  if(ubnew.sge(lb - 1))
                  {
                     l = Min;
                     u = Max;
                  }
                  else
                  {
                     type = Regular;
                     l = ubnew + 1;
                     u = lb - 1;
                  }
               }
               else if(lblt && !ubgt)
               {
                  auto lbnew = lb.trunc(bw).sext(MAX_BIT_INT);
                  if(lbnew.sle(ub + 1))
                  {
                     l = Min;
                     u = Max;
                  }
                  else
                  {
                     type = Regular;
                     l = ub + 1;
                     u = lbnew - 1;
                  }
               }
               else if(!lblt && !ubgt)
               {
                  l = lb;
                  u = ub;
               }
               else
               {
                  llvm_unreachable("unexpected condition");
               }
            }
         }
      }
      else if((lb - 1).eq(ub))
      {
         type = Regular;
         l = Min;
         u = Max;
      }
      else if(lb.sgt(ub))
      {
         normalizeRange(ub + 1, lb - 1, Anti);
      }
      else
      {
         assert(ub.sge(lb));
         auto maxS = APInt::getSignedMaxValue(bw);
         maxS = maxS.sext(MAX_BIT_INT);
         auto minS = APInt::getSignedMinValue(bw);
         minS = minS.sext(MAX_BIT_INT);

         bool lbgt = lb.sgt(maxS);
         bool ubgt = ub.sgt(maxS);
         bool lblt = lb.slt(minS);
         bool ublt = ub.slt(minS);
         if(ubgt && lblt)
         {
            l = Min;
            u = Max;
         }
         else if(lbgt && ubgt)
         {
            l = lb.trunc(bw).sext(MAX_BIT_INT);
            u = ub.trunc(bw).sext(MAX_BIT_INT);
         }
         else if(lblt && ublt)
         {
            l = ub.trunc(bw).sext(MAX_BIT_INT);
            u = lb.trunc(bw).sext(MAX_BIT_INT);
         }
         else if(!lblt && ubgt)
         {
            auto ubnew = ub.trunc(bw).sext(MAX_BIT_INT);
            if(ubnew.sge(lb - 1))
            {
               l = Min;
               u = Max;
            }
            else
            {
               type = Anti;
               l = ubnew + 1;
               u = lb - 1;
            }
         }
         else if(lblt && !ubgt)
         {
            auto lbnew = lb.trunc(bw).sext(MAX_BIT_INT);
            if(lbnew.sle(ub + 1))
            {
               l = Min;
               u = Max;
            }
            else
            {
               type = Anti;
               l = ub + 1;
               u = lbnew - 1;
            }
         }
         else if(!lblt && !ubgt)
         {
            l = lb;
            u = ub;
         }
         else
         {
            llvm_unreachable("unexpected condition");
         }
      }
      if(!u.sge(l))
      {
         l = Min;
         u = Max;
      }
   }

   unsigned Range_base::neededBits(const llvm::APInt& a, const llvm::APInt& b, bool sign)
   {
      if(sign)
      {
         return std::max(a.getMinSignedBits(), b.getMinSignedBits());
      }

      return std::max(a.getActiveBits(), b.getActiveBits());
   }

   Range_base::Range_base(RangeType rType, unsigned rbw, const APInt& lb, const APInt& ub) : l(lb), u(ub), bw(rbw), type(rType)
   {
      assert(rbw);
      normalizeRange(lb, ub, rType);
   }

   Range_base Range_base::getAnti(const Range_base& o)
   {
      if(o.type == Anti)
      {
         return Range_base(Regular, o.bw, o.l, o.u);
      }
      if(o.type == Regular)
      {
         return Range_base(Anti, o.bw, o.l, o.u);
      }
      if(o.type == Empty)
      {
         return Range_base(Regular, o.bw, Min, Max);
      }
      if(o.type == Unknown)
      {
         return o;
      }

      llvm_unreachable("unexpected condition");
   }

   unsigned int Range_base::getBitWidth() const
   {
      return bw;
   }

   const APInt Range_base::getLower() const
   {
      assert(type != Anti);
      return l;
   }

   const APInt Range_base::getUpper() const
   {
      assert(type != Anti);
      return u;
   }

   const APInt Range_base::getSignedMax() const
   {
      assert(type != Unknown && type != Empty);
      if(type == Regular)
      {
         auto maxS = APInt::getSignedMaxValue(bw);
         if(u.sgt(maxS.sext(MAX_BIT_INT)))
         {
            return maxS;
         }

         return u.trunc(bw);
      }

      return APInt::getSignedMaxValue(bw);
   }
   const APInt Range_base::getSignedMin() const
   {
      assert(type != Unknown && type != Empty);
      if(type != Anti)
      {
         auto minS = APInt::getSignedMinValue(bw);
         if(l.slt(minS.sext(MAX_BIT_INT)))
         {
            return minS;
         }

         return l.trunc(bw);
      }

      return APInt::getSignedMinValue(bw);
   }
   const APInt Range_base::getUnsignedMax() const
   {
      assert(type != Unknown && type != Empty);
      if(type == Regular)
      {
         if((l.isNegative()) && (u.isNegative()))
         {
            return u.trunc(bw);
         }
         if(l.slt(Zero))
         {
            return APInt::getMaxValue(bw);
         }

         return u.trunc(bw);
      }

      auto lb = l.sextOrTrunc(bw) - 1;
      auto ub = u.sextOrTrunc(bw) + 1;
      auto bwZero = APInt(bw, 0, false);

      if(lb.sge(bwZero) || ub.slt(bwZero))
      {
         return APInt::getMaxValue(bw);
      }

      return lb;
   }
   const APInt Range_base::getUnsignedMin() const
   {
      assert(type != Unknown && type != Empty);
      if(type == Regular)
      {
         if((l.isNegative()) && (u.isNegative()))
         {
            return l.trunc(bw);
         }
         if(l.slt(Zero))
         {
            return APInt(bw, 0, false);
         }

         return l.trunc(bw);
      }

      auto lb = l.sextOrTrunc(bw) - 1;
      auto ub = u.sextOrTrunc(bw) + 1;
      auto bwZero = APInt(bw, 0, false);

      if(lb.sge(bwZero) || ub.slt(bwZero))
      {
         return APInt(bw, 0, false);
      }

      return ub;
   }

   bool Range::isMaxRange() const
   {
      if(isAnti())
      {
         return false;
      }
      return this->getLower().eq(Min) && this->getUpper().eq(Max);
   }
   bool Range::isConstant() const
   {
      if(isAnti())
      {
         return false;
      }
      return this->getLower().eq(this->getUpper());
   }

   /// Add and Mul are commutative. So, they are a little different
   /// than the other operations.
   /// Many Range reductions are done by exploiting ConstantRange code
   Range Range::add(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown() || other.isMaxRange())
      {
         return other;
      }
      if(this->isAnti() && other.isConstant())
      {
         auto antiThis = getAnti(*this);
         auto l = antiThis.getLower();
         auto u = antiThis.getUpper();
         auto ol = other.getLower();
         if(ol.sge(Max - u))
         {
            return Range(Regular, getBitWidth());
         }

         return Range(Anti, getBitWidth(), l + ol, u + ol);
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }
      if(other.isConstant())
      {
         auto l = this->getLower();
         auto u = this->getUpper();
         auto ol = other.getLower();
         if(l.eq(Min))
         {
            assert(u.ne(Max));
            return Range(Regular, getBitWidth(), l, u + ol);
         }
         if(u.eq(Max))
         {
            assert(l.ne(Min));
            return Range(Regular, getBitWidth(), l + ol, u);
         }

         return Range(Regular, getBitWidth(), l + ol, u + ol);
      }

      auto this_min = getSignedMin();
      auto this_max = getSignedMax();
      auto other_min = other.getSignedMin();
      auto other_max = other.getSignedMax();
      ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      ConstantRange otherR = (other_min == other_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(other_min, other_max + 1);

      auto thisU_min = getUnsignedMin();
      auto thisU_max = getUnsignedMax();
      auto otherU_min = other.getUnsignedMin();
      auto otherU_max = other.getUnsignedMax();
      ConstantRange thisUR = (thisU_min == thisU_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(thisU_min, thisU_max + 1);
      ConstantRange otherUR = (otherU_min == otherU_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(otherU_min, otherU_max + 1);

      auto AddU = thisUR.add(otherUR);
      auto AddS = thisR.add(otherR);
      if(AddU.isFullSet() && AddS.isFullSet())
      {
         return Range(Regular, getBitWidth());
      }
      if(AddU.isFullSet())
      {
         return CR2R(AddS, getBitWidth());
      }
      if(AddS.isFullSet())
      {
         return CR2R(AddU, getBitWidth());
      }

      return BestRange(AddU, AddS, getBitWidth());
   }

   Range Range::sub(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown() || other.isMaxRange())
      {
         return other;
      }
      if(this->isAnti() && other.isConstant())
      {
         auto antiThis = getAnti(*this);
         auto l = antiThis.getLower();
         auto u = antiThis.getUpper();
         auto ol = other.getLower();
         if(l.sle(Min + ol))
         {
            return Range(Regular, getBitWidth());
         }

         return Range(Anti, getBitWidth(), l - ol, u - ol);
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }
      if(other.isConstant())
      {
         auto l = this->getLower();
         auto u = this->getUpper();
         auto ol = other.getLower();
         if(l.eq(Min))
         {
            assert(u.ne(Max));
            auto bw = getBitWidth();
            auto minValue = APInt::getSignedMinValue(bw).sext(MAX_BIT_INT);
            auto upper = u - ol;
            if(minValue.sgt(upper))
            {
               return Range(Regular, bw);
            }

            return Range(Regular, bw, l, upper);
         }
         if(u.eq(Max))
         {
            assert(l.ne(Min));
            auto bw = getBitWidth();
            auto maxValue = APInt::getSignedMaxValue(bw).sext(MAX_BIT_INT);
            auto lower = l - ol;
            if(maxValue.slt(lower))
            {
               return Range(Regular, bw);
            }

            return Range(Regular, getBitWidth(), l - ol, u);
         }

         auto bw = getBitWidth();
         auto lower = (l - ol).sextOrTrunc(bw).sext(MAX_BIT_INT);
         auto upper = (u - ol).sextOrTrunc(bw).sext(MAX_BIT_INT);
         if(lower.sle(upper))
         {
            return Range(Regular, bw, lower, upper);
         }

         return Range(Anti, bw, upper + 1, lower - 1);
      }

      auto this_min = getSignedMin();
      auto this_max = getSignedMax();
      auto other_min = other.getSignedMin();
      auto other_max = other.getSignedMax();
      ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      ConstantRange otherR = (other_min == other_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(other_min, other_max + 1);

      auto thisU_min = getUnsignedMin();
      auto thisU_max = getUnsignedMax();
      auto otherU_min = other.getUnsignedMin();
      auto otherU_max = other.getUnsignedMax();
      ConstantRange thisUR = (thisU_min == thisU_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(thisU_min, thisU_max + 1);
      ConstantRange otherUR = (otherU_min == otherU_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(otherU_min, otherU_max + 1);

      auto SubU = thisUR.sub(otherUR);
      auto SubS = thisR.sub(otherR);
      if(SubU.isFullSet() && SubS.isFullSet())
      {
         return Range(Regular, getBitWidth());
      }
      if(SubU.isFullSet())
      {
         return CR2R(SubS, getBitWidth());
      }
      if(SubS.isFullSet())
      {
         return CR2R(SubU, getBitWidth());
      }

      return BestRange(SubU, SubS, getBitWidth());
   }

   Range Range::mul(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown() || other.isMaxRange())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }

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

      ConstantRange Result_zext = ConstantRange(this_min * Other_min, this_max * Other_max + 1);
      ConstantRange UR = Result_zext.truncate(getBitWidth());

      // Now the signed range. Because we could be dealing with negative numbers
      // here, the lower bound is the smallest of the Cartesian product of the
      // lower and upper ranges; for example:
      //   [-1,4) * [-2,3) = min(-1*-2, -1*2, 3*-2, 3*2) = -6.
      // Similarly for the upper bound, swapping min for max.

      this_min = getSignedMin().sext(getBitWidth() * 2);
      this_max = getSignedMax().sext(getBitWidth() * 2);
      Other_min = other.getSignedMin().sext(getBitWidth() * 2);
      Other_max = other.getSignedMax().sext(getBitWidth() * 2);

      auto L = {this_min * Other_min, this_min * Other_max, this_max * Other_min, this_max * Other_max};
      auto Compare = [](const APInt& A, const APInt& B) { return A.slt(B); };
      ConstantRange Result_sext(std::min(L, Compare), std::max(L, Compare) + 1);
      ConstantRange SR = Result_sext.truncate(getBitWidth());
      return BestRange(UR, SR, getBitWidth());
   }

#define DIV_HELPER(OP, x, y) (x).eq(Max) ? ((y).slt(Zero) ? Min : ((y).eq(Zero) ? Zero : Max)) : ((y).eq(Max) ? Zero : ((x).eq(Min) ? ((y).slt(Zero) ? Max : ((y).eq(Zero) ? Zero : Min)) : ((y).eq(Min) ? Zero : ((x).OP((y))))))

   Range Range::udiv(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      //      llvm::errs() << "this:";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "other:";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";
      APInt a = getUnsignedMin().zext(MAX_BIT_INT);
      APInt b = getUnsignedMax().zext(MAX_BIT_INT);
      APInt c = other.getUnsignedMin().zext(MAX_BIT_INT);
      APInt d = other.getUnsignedMax().zext(MAX_BIT_INT);

      // Deal with division by 0 exception
      if(c.eq(Zero) && d.eq(Zero))
      {
         return Range(Regular, getBitWidth(), Min, Max);
      }
      if(c.eq(Zero))
      {
         c = One;
      }

      APInt candidates[4];
      candidates[0] = DIV_HELPER(udiv, a, c);
      candidates[1] = DIV_HELPER(udiv, a, d);
      candidates[2] = DIV_HELPER(udiv, b, c);
      candidates[3] = DIV_HELPER(udiv, b, d);
      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt* min = &candidates[0];
      APInt* max = &candidates[0];
      for(unsigned i = 1; i < 4; ++i)
      {
         if(candidates[i].sgt(*max))
         {
            max = &candidates[i];
         }
         else if(candidates[i].slt(*min))
         {
            min = &candidates[i];
         }
      }
      return Range(Regular, getBitWidth(), *min, *max);
   }

   Range Range::sdiv(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti())
      {
         return Range(Regular, getBitWidth());
      }

      const APInt& a = this->getLower();
      const APInt& b = this->getUpper();
      APInt c1, d1, c2, d2;
      bool is_zero_in = false;
      if(other.isAnti())
      {
         auto antiRange = getAnti(other);
         c1 = Min;
         d1 = antiRange.getLower() - 1;
         if(d1.eq(Zero))
         {
            d1 = -One;
         }
         else if(d1.sgt(Zero))
         {
            return Range(Regular, getBitWidth()); /// could be improved
         }
         c2 = antiRange.getUpper() + 1;
         if(c2.eq(Zero))
         {
            c2 = One;
         }
         else if(c2.slt(Zero))
         {
            return Range(Regular, getBitWidth()); /// could be improved
         }
         d2 = Max;
      }
      else
      {
         c1 = other.getLower();
         d1 = other.getUpper();
         // Deal with division by 0 exception
         if(c1.eq(Zero) && d1.eq(Zero))
         {
            return Range(Regular, getBitWidth(), Min, Max);
         }
         is_zero_in = c1.slt(Zero) && d1.sgt(Zero);
         if(is_zero_in)
         {
            d1 = -One;
            c2 = One;
         }
         else
         {
            c2 = other.getLower();
            if(c2.eq(Zero))
            {
               c1 = c2 = One;
            }
         }
         d2 = other.getUpper();
         if(d2.eq(Zero))
         {
            d1 = d2 = -One;
         }
      }
      auto n_iters = (is_zero_in || other.isAnti()) ? 8u : 4u;

      APInt candidates[8];
      candidates[0] = DIV_HELPER(sdiv, a, c1);
      candidates[1] = DIV_HELPER(sdiv, a, d1);
      candidates[2] = DIV_HELPER(sdiv, b, c1);
      candidates[3] = DIV_HELPER(sdiv, b, d1);
      if(n_iters == 8)
      {
         candidates[4] = DIV_HELPER(sdiv, a, c2);
         candidates[5] = DIV_HELPER(sdiv, a, d2);
         candidates[6] = DIV_HELPER(sdiv, b, c2);
         candidates[7] = DIV_HELPER(sdiv, b, d2);
      }
      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt* min = &candidates[0];
      APInt* max = &candidates[0];
      for(unsigned i = 1; i < n_iters; ++i)
      {
         if(candidates[i].sgt(*max))
         {
            max = &candidates[i];
         }
         else if(candidates[i].slt(*min))
         {
            min = &candidates[i];
         }
      }
      return Range(Regular, getBitWidth(), *min, *max);
      ;
   }

   Range Range::urem(const Range& other) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }

      const APInt& a = this->getUnsignedMin().zext(MAX_BIT_INT);
      const APInt& b = this->getUnsignedMax().zext(MAX_BIT_INT);
      APInt c = other.getUnsignedMin().zext(MAX_BIT_INT);
      const APInt& d = other.getUnsignedMax().zext(MAX_BIT_INT);

      // Deal with mod 0 exception
      if(c.eq(Zero) && d.eq(Zero))
      {
         return Range(Regular, getBitWidth());
      }
      if(c.eq(Zero))
      {
         c = APInt(MAX_BIT_INT, 1);
      }
      //      llvm::errs() << "this-urem:";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "other-urem:";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";

      APInt candidates[8];

      candidates[0] = a.slt(c) ? a : Zero;
      candidates[1] = a.slt(d) ? a : Zero;
      candidates[2] = b.slt(c) ? b : Zero;
      candidates[3] = b.slt(d) ? b : Zero;
      candidates[4] = a.slt(c) ? a : c - 1;
      candidates[5] = a.slt(d) ? a : d - 1;
      candidates[6] = b.slt(c) ? b : c - 1;
      candidates[7] = b.slt(d) ? b : d - 1;

      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt* min = &candidates[0];
      APInt* max = &candidates[0];
      for(unsigned i = 1; i < 8; ++i)
      {
         if(candidates[i].sgt(*max))
         {
            max = &candidates[i];
         }
         else if(candidates[i].slt(*min))
         {
            min = &candidates[i];
         }
      }
      auto res = Range(Regular, getBitWidth(), *min, *max);
      //      llvm::errs() << "res-urem:";
      //      res.print(llvm::errs());
      //      llvm::errs() << "\n";
      return res;
   }

   Range Range::srem(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }
      if(other == Range(Regular, other.getBitWidth(), Zero, Zero))
      {
         return Range(Empty, getBitWidth());
      }

      const APInt& a = this->getLower();
      const APInt& b = this->getUpper();
      APInt c = other.getLower();
      const APInt& d = other.getUpper();

      // Deal with mod 0 exception
      if(c.eq(Zero) && d.eq(Zero))
      {
         return Range(Regular, getBitWidth(), Min, Max);
      }
      if(c.eq(Zero))
      {
         c = APInt(MAX_BIT_INT, 1);
      }

      //      llvm::errs() << "this-rem:";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "other-rem:";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";

      APInt candidates[4];
      candidates[0] = Min;
      candidates[1] = Min;
      candidates[2] = Max;
      candidates[3] = Max;
      if(a.ne(Min) && c.ne(Min))
      {
         candidates[0] = a.srem(c); // lower lower
      }
      if(a.ne(Min) && d.ne(Max))
      {
         candidates[1] = a.srem(d); // lower upper
      }
      if(b.ne(Max) && c.ne(Min))
      {
         candidates[2] = b.srem(c); // upper lower
      }
      if(b.ne(Max) && d.ne(Max))
      {
         candidates[3] = b.srem(d); // upper upper
      }
      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt* min = &candidates[0];
      APInt* max = &candidates[0];
      for(unsigned i = 1; i < 4; ++i)
      {
         if(candidates[i].sgt(*max))
         {
            max = &candidates[i];
         }
         else if(candidates[i].slt(*min))
         {
            min = &candidates[i];
         }
      }
      auto res = Range(Regular, getBitWidth(), *min, *max);
      //      llvm::errs() << "res-rem:";
      //      res.print(llvm::errs());
      //      llvm::errs() << "\n";

      return res;
   }

   // Logic has been borrowed from ConstantRange
   Range Range::shl(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown() || other.isMaxRange())
      {
         return other;
      }
      auto bw = getBitWidth();
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, bw);
      }

      //      llvm::errs() << "this-shl:";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "other-shl:";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";

      const APInt& a = this->getLower();
      const APInt& b = this->getUpper();
      const APInt c = other.getUnsignedMin().zext(MAX_BIT_INT);
      const APInt d = other.getUnsignedMax().zext(MAX_BIT_INT);
      APInt Prec(MAX_BIT_INT, bw);

      if(c.uge(Prec))
      {
         return Range(Regular, bw);
      }
      if(d.uge(Prec))
      {
         return Range(Regular, bw);
      }
      if(a.eq(Min) || b.eq(Max))
      {
         return Range(Regular, bw);
      }
      if(a.eq(b) && c.eq(d)) // constant case
      {
         auto minmax = a.trunc(bw).shl(c.trunc(bw));
         minmax = minmax.sextOrTrunc(MAX_BIT_INT);
         return Range(Regular, bw, minmax, minmax);
      }
      if(a.isNegative() && b.isNegative())
      {
         auto clOnes = a.countLeadingOnes() - (MAX_BIT_INT - getBitWidth());
         if(d.ugt(clOnes))
         { // overflow
            return Range(Regular, bw);
         }

         return Range(Regular, getBitWidth(), a.shl(d), b.shl(c));
      }
      if(a.isNegative() && b.isNonNegative())
      {
         auto clOnes = a.countLeadingOnes() - (MAX_BIT_INT - getBitWidth());
         auto clZeros = b.countLeadingZeros() - (MAX_BIT_INT - getBitWidth());
         if(d.ugt(clOnes) || d.ugt(clZeros))
         { // overflow
            return Range(Regular, bw);
         }

         return Range(Regular, getBitWidth(), a.shl(d), b.shl(d));
      }

      auto clZeros = b.countLeadingZeros() - (MAX_BIT_INT - getBitWidth());
      if(d.ugt(clZeros))
      { // overflow
         return Range(Regular, bw);
      }

      return Range(Regular, getBitWidth(), a.shl(c), b.shl(d));
   }

   Range Range::lshr(const Range& other) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      APInt this_min = getUnsignedMin();
      APInt this_max = getUnsignedMax();
      APInt other_min = other.getUnsignedMin();
      APInt other_max = other.getUnsignedMax();
      const ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      const ConstantRange otherR = (other_min == other_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(other_min, other_max + 1);
      auto lshrU = thisR.lshr(otherR);
      if(lshrU.isFullSet())
      {
         return Range(Regular, getBitWidth());
      }

      return CR2R(lshrU, getBitWidth());
   }

#if __clang_major__ < 6

   static ConstantRange local_ashr(const ConstantRange& lthis, const ConstantRange& Other)
   {
      if(lthis.isEmptySet() || Other.isEmptySet())
      {
         return ConstantRange(lthis.getBitWidth(), /*isFullSet=*/false);
      }

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
      if(lthis.getSignedMin().isNonNegative())
      {
         // Upper and Lower of LHS are non-negative.
         min = PosMin;
         max = PosMax;
      }
      else if(lthis.getSignedMax().isNegative())
      {
         // Upper and Lower of LHS are negative.
         min = NegMin;
         max = NegMax;
      }
      else
      {
         // Upper is non-negative and Lower is negative.
         min = NegMin;
         max = PosMax;
      }
      if(min == max)
      {
         return ConstantRange(lthis.getBitWidth(), /*isFullSet=*/true);
      }

      return ConstantRange(std::move(min), std::move(max));
   }
#endif

   Range Range::ashr(const Range& other) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }

      auto this_min = getSignedMin();
      auto this_max = getSignedMax();
      auto other_min = other.getUnsignedMin();
      auto other_max = other.getUnsignedMax();
      const ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      const ConstantRange otherR = (other_min == other_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(other_min, other_max + 1);
#if __clang_major__ < 6
      auto AshrU = local_ashr(thisR, otherR);
#else
      auto AshrU = thisR.ashr(otherR);
#endif
      auto bw = getBitWidth();
      if(AshrU.isFullSet())
      {
         return Range(Regular, bw);
      }
      auto res = CR2R(AshrU, bw);
      return res;
   }

   /*
    * 	This and operation is coded following Hacker's Delight algorithm.
    * 	According to the author, it provides tight results.
    */
   Range Range::And(const Range& other) const
   {
      //      llvm::errs() << "And-a: ";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "And-b: ";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      APInt a = this->isAnti() ? Min : this->getLower();
      APInt b = this->isAnti() ? Max : this->getUpper();
      APInt c = other.isAnti() ? Min : other.getLower();
      APInt d = other.isAnti() ? Max : other.getUpper();

      // negate everybody
      APInt negA = APInt(a);
      negA.flipAllBits();
      APInt negB = APInt(b);
      negB.flipAllBits();
      APInt negC = APInt(c);
      negC.flipAllBits();
      APInt negD = APInt(d);
      negD.flipAllBits();
      auto bw = getBitWidth();

      Range inv1 = Range(Regular, bw, negB, negA);
      Range inv2 = Range(Regular, bw, negD, negC);
      Range invres = inv1.Or(inv2);

      // negate the result of the 'or'
      APInt invLower = invres.getUpper();
      invLower.flipAllBits();
      APInt invUpper = invres.getLower();
      invUpper.flipAllBits();
      auto res = Range(Regular, bw, invLower, invUpper);
      //      llvm::errs() << "And-res: ";
      //      res.print(llvm::errs());
      //      llvm::errs() << "\n";
      return res;
   }

   namespace
   {
      APInt minOR(APInt a, const APInt& b, APInt c, const APInt& d)
      {
         APInt m, temp;
         m = APInt(MAX_BIT_INT, 1) << (MAX_BIT_INT - 1);
         while(m.ne(Zero))
         {
            if((~a & c & m).ne(Zero))
            {
               temp = (a | m) & -m;
               if(temp.ule(b))
               {
                  a = temp;
                  break;
               }
            }
            else if((a & ~c & m).ne(Zero))
            {
               temp = (c | m) & -m;
               if(temp.ule(d))
               {
                  c = temp;
                  break;
               }
            }
            m = m.lshr(1);
         }
         return a | c;
      }

      APInt maxOR(const APInt& a, APInt b, const APInt& c, APInt d)
      {
         APInt m, temp;
         m = APInt(MAX_BIT_INT, 1) << (MAX_BIT_INT - 1);
         while(m.ne(Zero))
         {
            if((b & d & m).ne(Zero))
            {
               temp = (b - m) | (m - One);
               if(temp.uge(a))
               {
                  b = temp;
                  break;
               }
               temp = (d - m) | (m - One);
               if(temp.uge(c))
               {
                  d = temp;
                  break;
               }
            }
            m = m.lshr(1);
         }
         return b | d;
      }

      APInt minXOR(APInt a, const APInt& b, APInt c, const APInt& d)
      {
         APInt m, temp;
         m = APInt(MAX_BIT_INT, 1) << (MAX_BIT_INT - 1);
         while(m.ne(Zero))
         {
            if((~a & c & m).ne(Zero))
            {
               temp = (a | m) & -m;
               if(temp.ule(b))
               {
                  a = temp;
               }
            }
            else if((a & ~c & m).ne(Zero))
            {
               temp = (c | m) & -m;
               if(temp.ule(d))
               {
                  c = temp;
               }
            }
            m = m.lshr(1);
         }
         return a ^ c;
      }

      APInt maxXOR(const APInt& a, APInt b, const APInt& c, APInt d)
      {
         APInt m, temp;
         m = APInt(MAX_BIT_INT, 1) << (MAX_BIT_INT - 1);
         while(m.ne(Zero))
         {
            if((b & d & m).ne(Zero))
            {
               temp = (b - m) | (m - 1);
               if(temp.uge(a))
               {
                  b = temp;
               }
               else
               {
                  temp = (d - m) | (m - 1);
                  if(temp.uge(c))
                  {
                     d = temp;
                  }
               }
            }
            m = m.lshr(1);
         }
         return b ^ d;
      }

   } // namespace

   /**
    * Or operation coded following Hacker's Delight algorithm.
    */
   Range Range::Or(const Range& other) const
   {
      //      llvm::errs() << "Or-a: ";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "Or-b: ";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }

      const APInt a = this->getLower();
      const APInt b = this->getUpper();
      const APInt c = other.getLower();
      const APInt d = other.getUpper();

      //      if (a.eq(Min) || b.eq(Max) || c.eq(Min) || d.eq(Max))
      //         return Range(Regular,getBitWidth());

      unsigned char switchval = 0;
      switchval += (a.isNonNegative() ? 1 : 0);
      switchval <<= 1;
      switchval += (b.isNonNegative() ? 1 : 0);
      switchval <<= 1;
      switchval += (c.isNonNegative() ? 1 : 0);
      switchval <<= 1;
      switchval += (d.isNonNegative() ? 1 : 0);

      APInt l = Min, u = Max;

      // llvm::errs() << "switchval " << (unsigned)switchval << "\n";

      switch(switchval)
      {
         case 0:
            l = minOR(a, b, c, d);
            u = maxOR(a, b, c, d);
            break;
         case 1:
            l = a;
            u = APInt::getAllOnesValue(MAX_BIT_INT);
            break;
         case 3:
            l = minOR(a, b, c, d);
            u = maxOR(a, b, c, d);
            break;
         case 4:
            l = c;
            u = APInt::getAllOnesValue(MAX_BIT_INT);
            break;
         case 5:
            l = (a.slt(c) ? a : c);
            u = maxOR(Zero, b, Zero, d);
            break;
         case 7:
            l = minOR(a, APInt::getAllOnesValue(MAX_BIT_INT), c, d);
            u = maxOR(Zero, b, c, d);
            break;
         case 12:
            l = minOR(a, b, c, d);
            u = maxOR(a, b, c, d);
            break;
         case 13:
            l = minOR(a, b, c, APInt::getAllOnesValue(MAX_BIT_INT));
            u = maxOR(a, b, Zero, d);
            break;
         case 15:
            l = minOR(a, b, c, d);
            u = maxOR(a, b, c, d);
            break;
      }
      if(l.eq(Min) || u.eq(Max))
      {
         return Range(Regular, getBitWidth());
      }
      auto res = Range(Regular, getBitWidth(), l, u);
      //      llvm::errs() << "Or-res: ";
      //      res.print(llvm::errs());
      //      llvm::errs() << "\n";
      return res;
   }

   Range Range::Xor(const Range& other) const
   {
      //      llvm::errs() << "Xor-a: ";
      //      this->print(llvm::errs());
      //      llvm::errs() << "\n";
      //      llvm::errs() << "Xor-b: ";
      //      other.print(llvm::errs());
      //      llvm::errs() << "\n";
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }
      const APInt a = this->getLower();
      const APInt b = this->getUpper();
      const APInt c = other.getLower();
      const APInt d = other.getUpper();
      if(a.isNonNegative() && b.isNonNegative() && c.isNonNegative() && d.isNonNegative())
      {
         APInt l = minXOR(a, b, c, d);
         APInt u = maxXOR(a, b, c, d);
         auto res = Range(Regular, getBitWidth(), l, u);
         //         llvm::errs() << "Xor-res: ";
         //         res.print(llvm::errs());
         //         llvm::errs() << "\n";
         //         return res;
      }
      else if(c.eq(-One) && d.eq(-One) && a.isNonNegative() && b.isNonNegative())
      {
         auto res = other.sub(*this);
         //         llvm::errs() << "Xor-res-1: ";
         //         res.print(llvm::errs());
         //         llvm::errs() << "\n";
         return res;
      }
      return Range(Regular, getBitWidth());
   }

   Range Range::Eq(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(!isAnti() && !other.isAnti())
      {
         if(getLower().eq(Min) || getUpper().eq(Max) || other.getLower().eq(Min) || other.getUpper().eq(Max))
         {
            return Range(Regular, bw, Zero, One);
         }
      }
      bool areTheyEqual = !this->intersectWith(other).isEmpty();
      auto AntiThis = getAnti(*this);
      APInt a = isAnti() ? AntiThis.getLower() : this->getLower();
      APInt b = isAnti() ? AntiThis.getUpper() : this->getUpper();
      bool areTheyDifferent = !(a.eq(b) && *this == other);

      if(areTheyEqual && areTheyDifferent)
      {
         return Range(Regular, bw, Zero, One);
      }
      if(areTheyEqual && !areTheyDifferent)
      {
         return Range(Regular, bw, One, One);
      }
      if(!areTheyEqual && areTheyDifferent)
      {
         return Range(Regular, bw, Zero, Zero);
      }

      llvm_unreachable("condition unexpected");
   }
   Range Range::Ne(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(!isAnti() && !other.isAnti())
      {
         if(getLower().eq(Min) || getUpper().eq(Max) || other.getLower().eq(Min) || other.getUpper().eq(Max))
         {
            return Range(Regular, bw, Zero, One);
         }
      }
      bool areTheyEqual = !this->intersectWith(other).isEmpty();
      auto AntiThis = getAnti(*this);
      APInt a = isAnti() ? AntiThis.getLower() : this->getLower();
      APInt b = isAnti() ? AntiThis.getUpper() : this->getUpper();
      bool areTheyDifferent = !(a.eq(b) && *this == other);
      if(areTheyEqual && areTheyDifferent)
      {
         return Range(Regular, bw, Zero, One);
      }
      if(areTheyEqual && !areTheyDifferent)
      {
         return Range(Regular, bw, Zero, Zero);
      }
      if(!areTheyEqual && areTheyDifferent)
      {
         return Range(Regular, bw, One, One);
      }

      llvm_unreachable("condition unexpected");
   }
   Range Range::Ugt(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, Zero, One);
      }

      APInt a = this->getUnsignedMin();
      APInt b = this->getUnsignedMax();
      APInt c = other.getUnsignedMin();
      APInt d = other.getUnsignedMax();
      if(a.ugt(d))
      {
         return Range(Regular, bw, One, One);
      }
      if(c.uge(b))
      {
         return Range(Regular, bw, Zero, Zero);
      }

      return Range(Regular, bw, Zero, One);
   }
   Range Range::Uge(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, Zero, One);
      }

      APInt a = this->getUnsignedMin();
      APInt b = this->getUnsignedMax();
      APInt c = other.getUnsignedMin();
      APInt d = other.getUnsignedMax();
      if(a.uge(d))
      {
         return Range(Regular, bw, One, One);
      }
      if(c.ugt(b))
      {
         return Range(Regular, bw, Zero, Zero);
      }

      return Range(Regular, bw, Zero, One);
   }
   Range Range::Ult(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, Zero, One);
      }

      APInt a = this->getUnsignedMin();
      APInt b = this->getUnsignedMax();
      APInt c = other.getUnsignedMin();
      APInt d = other.getUnsignedMax();
      if(b.ult(c))
      {
         return Range(Regular, bw, One, One);
      }
      if(d.ule(a))
      {
         return Range(Regular, bw, Zero, Zero);
      }

      return Range(Regular, bw, Zero, One);
   }
   Range Range::Ule(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, Zero, One);
      }

      APInt a = this->getUnsignedMin();
      APInt b = this->getUnsignedMax();
      APInt c = other.getUnsignedMin();
      APInt d = other.getUnsignedMax();
      if(b.ule(c))
      {
         return Range(Regular, bw, One, One);
      }
      if(d.ult(a))
      {
         return Range(Regular, bw, Zero, Zero);
      }

      return Range(Regular, bw, Zero, One);
   }
   Range Range::Sgt(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, Zero, One);
      }

      APInt a = this->getSignedMin();
      APInt b = this->getSignedMax();
      APInt c = other.getSignedMin();
      APInt d = other.getSignedMax();
      if(a.sgt(d))
      {
         return Range(Regular, bw, One, One);
      }
      if(c.sge(b))
      {
         return Range(Regular, bw, Zero, Zero);
      }

      return Range(Regular, bw, Zero, One);
   }
   Range Range::Sge(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, Zero, One);
      }

      APInt a = this->getSignedMin();
      APInt b = this->getSignedMax();
      APInt c = other.getSignedMin();
      APInt d = other.getSignedMax();
      if(a.sge(d))
      {
         return Range(Regular, bw, One, One);
      }
      if(c.sgt(b))
      {
         return Range(Regular, bw, Zero, Zero);
      }

      return Range(Regular, bw, Zero, One);
   }
   Range Range::Slt(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, Zero, One);
      }

      APInt a = this->getSignedMin();
      APInt b = this->getSignedMax();
      APInt c = other.getSignedMin();
      APInt d = other.getSignedMax();
      if(b.slt(c))
      {
         return Range(Regular, bw, One, One);
      }
      if(d.sle(a))
      {
         return Range(Regular, bw, Zero, Zero);
      }

      return Range(Regular, bw, Zero, One);
   }
   Range Range::Sle(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, Zero, One);
      }

      APInt a = this->getSignedMin();
      APInt b = this->getSignedMax();
      APInt c = other.getSignedMin();
      APInt d = other.getSignedMax();
      if(b.sle(c))
      {
         return Range(Regular, bw, One, One);
      }
      if(d.slt(a))
      {
         return Range(Regular, bw, Zero, Zero);
      }

      return Range(Regular, bw, Zero, One);
   }

   // Truncate
   // - if the source range is entirely inside max bit range, it is the result
   // - else, the result is the max bit range
   Range Range::truncate(unsigned bitwidth) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }

      APInt maxupper = APInt::getSignedMaxValue(bitwidth);
      APInt maxlower = APInt::getSignedMinValue(bitwidth);

      if(bitwidth < MAX_BIT_INT)
      {
         maxupper = maxupper.sext(MAX_BIT_INT);
         maxlower = maxlower.sext(MAX_BIT_INT);
      }

      // Check if source range is contained by max bit range
      if(!this->isAnti() && this->getLower().sge(maxlower) && this->getUpper().sle(maxupper))
      {
         return Range(Regular, bitwidth, this->getLower(), this->getUpper());
      }

      return Range(Regular, bitwidth, maxlower, maxupper);
   }

   Range Range::sextOrTrunc(unsigned bitwidth) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      auto from_bw = getBitWidth();
      auto this_min = from_bw == 1 ? getUnsignedMin() : getSignedMin();
      auto this_max = from_bw == 1 ? getUnsignedMax() : getSignedMax();
      const ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      auto sextRes = thisR.sextOrTrunc(bitwidth);
      if(sextRes.isFullSet())
      {
         return Range(Regular, bitwidth);
      }

      return CR2R(sextRes, bitwidth); // Range(Regular,bitwidth,sextRes.getSignedMin().sext(MAX_BIT_INT),sextRes.getSignedMax().sext(MAX_BIT_INT));
   }

   Range Range::zextOrTrunc(unsigned bitwidth) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      auto this_min = getUnsignedMin();
      auto this_max = getUnsignedMax();
      const ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      auto zextRes = thisR.zextOrTrunc(bitwidth);
      if(zextRes.isFullSet())
      {
         return Range(Regular, bitwidth);
      }

      return CR2R(zextRes, bitwidth); // Range(Regular,bitwidth,zextRes.getUnsignedMin().zext(MAX_BIT_INT),zextRes.getUnsignedMax().zext(MAX_BIT_INT));
   }

   Range Range::intersectWith(const Range& other) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
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
         if(u.slt(l))
         {
            return Range(Empty, getBitWidth());
         }

         return Range(Regular, getBitWidth(), l, u);
      }
      if(this->isAnti() && !other.isAnti())
      {
         auto antiRange = getAnti(*this);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         if(antil.sle(other.getLower()))
         {
            if(other.getUpper().sle(antiu))
            {
               return Range(Empty, getBitWidth());
            }
            APInt l = other.getLower().sgt(antiu) ? other.getLower() : antiu + 1;
            APInt u = other.getUpper();
            return Range(Regular, getBitWidth(), l, u);
         }
         if(antiu.sge(other.getUpper()))
         {
            assert(!other.getLower().sge(antil));
            APInt l = other.getLower();
            APInt u = other.getUpper().slt(antil) ? other.getUpper() : antil - 1;
            return Range(Regular, getBitWidth(), l, u);
         }
         if(other.getLower().eq(Min) && other.getUpper().eq(Max))
         {
            return *this;
         }
         if(antil.sgt(other.getUpper()) || antiu.slt(other.getLower()))
         {
            return other;
         }

         // we approximate to the range of other
         return other;
      }
      if(!this->isAnti() && other.isAnti())
      {
         auto antiRange = getAnti(other);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         if(antil.sle(this->getLower()))
         {
            if(this->getUpper().sle(antiu))
            {
               return Range(Empty, getBitWidth());
            }
            APInt l = this->getLower().sgt(antiu) ? this->getLower() : antiu + 1;
            APInt u = this->getUpper();
            return Range(Regular, getBitWidth(), l, u);
         }
         if(antiu.sge(this->getUpper()))
         {
            assert(!this->getLower().sge(antil));
            APInt l = this->getLower();
            APInt u = this->getUpper().slt(antil) ? this->getUpper() : antil - 1;
            return Range(Regular, getBitWidth(), l, u);
         }
         if(this->getLower().eq(Min) && this->getUpper().eq(Max))
         {
            return other;
         }
         if(antil.sgt(this->getUpper()) || antiu.slt(this->getLower()))
         {
            return *this;
         }

         // we approximate to the range of this
         return *this;
      }

      auto antiRange_a = getAnti(*this);
      auto antiRange_b = getAnti(other);
      auto antil_a = antiRange_a.getLower();
      auto antiu_a = antiRange_a.getUpper();
      auto antil_b = antiRange_b.getLower();
      auto antiu_b = antiRange_b.getUpper();
      if(antil_a.sgt(antil_b))
      {
         std::swap(antil_a, antil_b);
         std::swap(antiu_a, antiu_b);
      }
      if(antil_b.sgt(antiu_a + 1))
      {
         return Range(Anti, getBitWidth(), antil_a, antiu_a);
      }
      APInt l = antil_a;
      APInt u = antiu_a.sgt(antiu_b) ? antiu_a : antiu_b;
      if(l.eq(Min) && u.eq(Max))
      {
         return Range(Empty, getBitWidth());
      }

      return Range(Anti, getBitWidth(), l, u);
   }

   Range Range::unionWith(const Range& other) const
   {
      if(this->isEmpty() || this->isUnknown())
      {
         return other;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return *this;
      }
      if(!this->isAnti() && !other.isAnti())
      {
         APInt l = getLower().slt(other.getLower()) ? getLower() : other.getLower();
         APInt u = getUpper().sgt(other.getUpper()) ? getUpper() : other.getUpper();
         return Range(Regular, getBitWidth(), l, u);
      }
      if(this->isAnti() && !other.isAnti())
      {
         auto antiRange = getAnti(*this);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         assert(antil.ne(Min));
         assert(antiu.ne(Max));
         if(antil.sgt(other.getUpper()) || antiu.slt(other.getLower()))
         {
            return *this;
         }
         if(antil.sgt(other.getLower()) && antiu.slt(other.getUpper()))
         {
            return Range(Regular, getBitWidth());
         }
         if(antil.sge(other.getLower()) && antiu.sgt(other.getUpper()))
         {
            return Range(Anti, getBitWidth(), other.getUpper() + 1, antiu);
         }
         if(antil.slt(other.getLower()) && antiu.sle(other.getUpper()))
         {
            return Range(Anti, getBitWidth(), antil, other.getLower() - 1);
         }

         return Range(Regular, getBitWidth()); // approximate to the full set
      }
      if(!this->isAnti() && other.isAnti())
      {
         auto antiRange = getAnti(other);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         assert(antil.ne(Min));
         assert(antiu.ne(Max));
         if(antil.sgt(this->getUpper()) || antiu.slt(this->getLower()))
         {
            return other;
         }
         if(antil.sgt(this->getLower()) && antiu.slt(this->getUpper()))
         {
            return Range(Regular, getBitWidth());
         }
         if(antil.sge(this->getLower()) && antiu.sgt(this->getUpper()))
         {
            return Range(Anti, getBitWidth(), this->getUpper() + 1, antiu);
         }
         if(antil.slt(this->getLower()) && antiu.sle(this->getUpper()))
         {
            return Range(Anti, getBitWidth(), antil, this->getLower() - 1);
         }

         return Range(Regular, getBitWidth()); // approximate to the full set
      }

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
      {
         return Range(Regular, getBitWidth());
      }
      if(antil_a.sgt(antil_b) && antiu_a.slt(antiu_b))
      {
         return *this;
      }
      if(antil_b.sgt(antil_a) && antiu_b.slt(antiu_a))
      {
         return *this;
      }
      if(antil_a.sge(antil_b) && antiu_b.sle(antiu_a))
      {
         return Range(Anti, getBitWidth(), antil_a, antiu_b);
      }
      if(antil_b.sge(antil_a) && antiu_a.sle(antiu_b))
      {
         return Range(Anti, getBitWidth(), antil_b, antiu_a);
      }

      llvm_unreachable("unsupported condition");
   }

   Range Range::BestRange(const llvm::ConstantRange& UR, const llvm::ConstantRange& SR, unsigned bw) const
   {
      if(UR.isFullSet() && SR.isFullSet())
      {
         return Range(Regular, bw);
      }
      if(UR.isFullSet())
      {
         return CR2R(SR, bw);
      }
      if(SR.isFullSet())
      {
         return CR2R(UR, bw);
      }
      auto nbitU = neededBits(UR.getUnsignedMin(), UR.getUnsignedMax(), false);
      auto nbitS = neededBits(SR.getSignedMin(), SR.getSignedMax(), true);
      if(nbitU < nbitS)
      {
         return CR2R(UR, bw);
      }

      return CR2R(SR, bw);
   }

   bool Range::operator==(const Range& other) const
   {
      return getBitWidth() == other.getBitWidth() && Range::isSameType(*this, other) && Range::isSameRange(*this, other);
   }

   bool Range::operator!=(const Range& other) const
   {
      return getBitWidth() != other.getBitWidth() || !Range::isSameType(*this, other) || !Range::isSameRange(*this, other);
   }

   void Range::print(raw_ostream& OS) const
   {
      if(this->isUnknown())
      {
         OS << "Unknown";
         return;
      }
      if(this->isEmpty())
      {
         OS << "Empty";
         return;
      }
      if(this->isAnti())
      {
         auto antiObj = getAnti(*this);
         if(antiObj.getLower().eq(Min))
         {
            OS << ")-inf,";
         }
         else
         {
            OS << ")" << antiObj.getLower() << ",";
         }
         OS << getBitWidth() << ",";
         if(antiObj.getUpper().eq(Max))
         {
            OS << "+inf(";
         }
         else
         {
            OS << antiObj.getUpper() << "(";
         }
      }
      else
      {
         if(getLower().eq(Min))
         {
            OS << "[-inf,";
         }
         else
         {
            OS << "[" << getLower() << ",";
         }
         OS << getBitWidth() << ",";
         if(getUpper().eq(Max))
         {
            OS << "+inf]";
         }
         else
         {
            OS << getUpper() << "]";
         }
      }
   }

   raw_ostream& operator<<(raw_ostream& OS, const Range& R)
   {
      R.print(OS);
      return OS;
   }

   // ========================================================================== //
   // BasicInterval
   // ========================================================================== //

   BasicInterval::BasicInterval(Range range) : range(std::move(range))
   {
   }

   BasicInterval::BasicInterval() : range(Range(Regular, MAX_BIT_INT))
   {
   }

   // This is a base class, its dtor must be virtual.
   BasicInterval::~BasicInterval() = default;

   /// Pretty print.
   void BasicInterval::print(raw_ostream& OS) const
   {
      this->getRange().print(OS);
   }

   // ========================================================================== //
   // SymbInterval
   // ========================================================================== //

   SymbInterval::SymbInterval(const Range& range, const Value* bound, CmpInst::Predicate pred) : BasicInterval(range), bound(bound), pred(pred)
   {
   }

   SymbInterval::~SymbInterval() = default;

   Range SymbInterval::fixIntersects(VarNode* bound, VarNode* sink)
   {
      // Get the lower and the upper bound of the
      // node which bounds this intersection.
      const auto boundRange = bound->getRange();
      const auto sinkRange = sink->getRange();
      assert(!boundRange.isEmpty());
      assert(!sinkRange.isEmpty());

      auto IsAnti = bound->getRange().isAnti() || sinkRange.isAnti();
      APInt l = IsAnti ? (boundRange.isUnknown() ? Min : boundRange.getUnsignedMin().zext(MAX_BIT_INT)) : boundRange.getLower();
      APInt u = IsAnti ? (boundRange.isUnknown() ? Max : boundRange.getUnsignedMax().zext(MAX_BIT_INT)) : boundRange.getUpper();

      // Get the lower and upper bound of the interval of this operation
      APInt lower = IsAnti ? (sinkRange.isUnknown() ? Min : sinkRange.getUnsignedMin().zext(MAX_BIT_INT)) : sinkRange.getLower();
      APInt upper = IsAnti ? (sinkRange.isUnknown() ? Max : sinkRange.getUnsignedMax().zext(MAX_BIT_INT)) : sinkRange.getUpper();

      auto bw = getRange().getBitWidth();
      switch(this->getOperation())
      {
         case ICmpInst::ICMP_EQ: // equal
            return Range(Regular, bw, l, u);
         case ICmpInst::ICMP_SLE: // signed less or equal
            if(lower.sgt(u))
            {
               return Range(Empty, bw);
            }
            else
            {
               return Range(Regular, bw, lower, u);
            }
         case ICmpInst::ICMP_SLT: // signed less than
            if(u != Max)
            {
               if(lower.sgt(u - 1))
               {
                  return Range(Empty, bw);
               }

               return Range(Regular, bw, lower, u - 1);
            }
            else
            {
               if(lower.sgt(u))
               {
                  return Range(Empty, bw);
               }

               return Range(Regular, bw, lower, u);
            }
         case ICmpInst::ICMP_SGE: // signed greater or equal
            if(l.sgt(upper))
            {
               return Range(Empty, bw);
            }
            else
            {
               return Range(Regular, bw, l, upper);
            }
         case ICmpInst::ICMP_SGT: // signed greater than
            if(l != Min)
            {
               if((l + 1).sgt(upper))
               {
                  return Range(Empty, bw);
               }

               return Range(Regular, bw, l + 1, upper);
            }
            else
            {
               if((l).sgt(upper))
               {
                  return Range(Empty, bw);
               }

               return Range(Regular, bw, l, upper);
            }
         default:
            return Range(Regular, bw);
      }
      llvm_unreachable("unexpected condition");
   }

   /// Pretty print.
   void SymbInterval::print(raw_ostream& OS) const
   {
      auto bnd = getBound();
      switch(this->getOperation())
      {
         case ICmpInst::ICMP_EQ: // equal
            OS << "[lb(";
            if(bnd.second)
            {
               printVarName(bnd.second, OS);
            }
            else
            {
               printVarName(bnd.first, OS);
            }
            OS << "), ub(";
            if(bnd.second)
            {
               printVarName(bnd.second, OS);
            }
            else
            {
               printVarName(bnd.first, OS);
            }
            OS << ")]";
            break;
         case ICmpInst::ICMP_SLE: // sign less or equal
            OS << "[-inf, ub(";
            if(bnd.second)
            {
               printVarName(bnd.second, OS);
            }
            else
            {
               printVarName(bnd.first, OS);
            }
            OS << ")]";
            break;
         case ICmpInst::ICMP_SLT: // sign less than
            OS << "[-inf, ub(";
            if(bnd.second)
            {
               printVarName(bnd.second, OS);
            }
            else
            {
               printVarName(bnd.first, OS);
            }
            OS << ") - 1]";
            break;
         case ICmpInst::ICMP_SGE: // sign greater or equal
            OS << "[lb(";
            if(bnd.second)
            {
               printVarName(bnd.second, OS);
            }
            else
            {
               printVarName(bnd.first, OS);
            }
            OS << "), +inf]";
            break;
         case ICmpInst::ICMP_SGT: // sign greater than
            OS << "[lb(";
            if(bnd.second)
            {
               printVarName(bnd.second, OS);
            }
            else
            {
               printVarName(bnd.first, OS);
            }
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
   VarNode::VarNode(const Value* _V, const Value* _GV, const llvm::DataLayout* DL) : V(_V), GV(_GV), interval(Unknown, LoadStoreOperationBW(_V, _GV, DL), Min, Max), abstractState(0)
   {
      if(dyn_cast<FPToSIInst>(_V))
      {
         auto bw = _V->getType()->getPrimitiveSizeInBits();
         interval = Range(Regular, bw, APInt::getSignedMinValue(bw).sext(MAX_BIT_INT), APInt::getSignedMaxValue(bw).sext(MAX_BIT_INT));
      }
      else if(dyn_cast<FPToUIInst>(_V))
      {
         auto bw = _V->getType()->getPrimitiveSizeInBits();
         interval = Range(Regular, bw, APInt::getMinValue(bw).sext(MAX_BIT_INT), APInt::getMaxValue(bw).sext(MAX_BIT_INT));
      }
   }

   /// The dtor.
   VarNode::~VarNode() = default;

   /// Initializes the value of the node.
   void VarNode::init(bool outside, const llvm::DataLayout* DL)
   {
      auto bw = LoadStoreOperationBW(V, GV, DL);
      assert(bw);
      if(const auto* CI = dyn_cast<ConstantInt>(V))
      {
         APInt tmp = CI->getValue();
         if(tmp.getBitWidth() < MAX_BIT_INT)
         {
            tmp = tmp.sext(MAX_BIT_INT);
         }
         this->setRange(Range(Regular, bw, tmp, tmp));
      }
      else
      {
         if(!outside)
         {
            // Initialize with a basic, unknown, interval.
            this->setRange(Range(Unknown, bw));
         }
         else
         {
            this->setRange(Range(Regular, bw));
         }
      }
   }

   /// Pretty print.
   void VarNode::print(raw_ostream& OS) const
   {
      if(const auto* C = dyn_cast<ConstantInt>(V))
      {
         OS << C->getValue();
      }
      else if(GV)
      {
         printVarName(GV, OS);
         OS << "=";
         printVarName(V, OS);
      }
      else
      {
         printVarName(V, OS);
      }
      OS << " ";
      this->getRange().print(OS);
   }

   void VarNode::storeAbstractState()
   {
      ASSERT(!this->interval.isUnknown(), "storeAbstractState doesn't handle empty set");

      if(this->interval.getLower().eq(Min))
      {
         if(this->interval.getUpper().eq(Max))
         {
            this->abstractState = '?';
         }
         else
         {
            this->abstractState = '-';
         }
      }
      else if(this->interval.getUpper().eq(Max))
      {
         this->abstractState = '+';
      }
      else
      {
         this->abstractState = '0';
      }
   }

   raw_ostream& operator<<(raw_ostream& OS, const VarNode* VN)
   {
      VN->print(OS);
      return OS;
   }

   // ========================================================================== //
   // BasicOp
   // ========================================================================== //

   /// We can not want people creating objects of this class,
   /// but we want to inherit of it.
   BasicOp::BasicOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const Instruction* inst) : intersect(std::move(intersect)), sink(sink), inst(inst)
   {
   }

   /// We can not want people creating objects of this class,
   /// but we want to inherit of it.
   BasicOp::~BasicOp() = default;

   /// Replace symbolic intervals with hard-wired constants.
   void BasicOp::fixIntersects(VarNode* V)
   {
      if(auto* SI = dyn_cast<SymbInterval>(getIntersect().get()))
      {
         this->setIntersect(SI->fixIntersects(V, getSink()));
      }
   }

   // ========================================================================== //
   // ControlDep
   // ========================================================================== //

   ControlDep::ControlDep(VarNode* sink, VarNode* source) : BasicOp(std::make_shared<BasicInterval>(), sink, nullptr), source(source)
   {
   }

   ControlDep::~ControlDep() = default;

   Range ControlDep::eval()
   {
      return Range(Regular, MAX_BIT_INT);
   }

   void ControlDep::print(raw_ostream& /*OS*/) const
   {
   }

   // ========================================================================== //
   // LoadOp
   // ========================================================================== //

   LoadOp::LoadOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const Instruction* inst) : BasicOp(std::move(intersect), sink, inst)
   {
   }

   LoadOp::~LoadOp() = default;

   Range LoadOp::eval()
   {
      unsigned bw = getSink()->getBitWidth();
      Range result(Unknown, bw, Min, Max);
      //      getSink()->getValue().first->print(llvm::errs());
      //      llvm::errs()<< "\n";
      if(getNumSources() == 0)
      {
         assert(bw == getIntersect()->getRange().getBitWidth());
         return getIntersect()->getRange();
      }

      result = this->getSource(0)->getRange();
      // Iterate over the sources of the load
      for(const VarNode* varNode : sources)
      {
         result = result.unionWith(varNode->getRange());
      }

      //      llvm::errs()<< "=";
      //      result.print(llvm::errs());
      //      llvm::errs()<< "\n";
      bool test = this->getIntersect()->getRange().isMaxRange();
      if(!test)
      {
         Range aux = this->getIntersect()->getRange();
         Range intersect = result.intersectWith(aux);
         if(!intersect.isEmpty())
         {
            result = intersect;
         }
      }
      return result;
   }

   void LoadOp::print(raw_ostream& OS) const
   {
      const char* quot = R"(")";
      OS << " " << quot << this << quot << R"( [label=")";
      OS << "LoadOp\"]\n";

      for(auto src : sources)
      {
         const auto V = src->getValue();
         if(const auto* C = dyn_cast<ConstantInt>(V.first))
         {
            OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
         }
         else
         {
            OS << " " << quot;
            if(V.second)
            {
               printVarName(V.second, OS);
               OS << "." << V.first;
            }
            else
            {
               printVarName(V.first, OS);
            }
            OS << quot << " -> " << quot << this << quot << "\n";
         }
      }

      const auto VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      if(VS.second)
      {
         printVarName(VS.second, OS);
         OS << "." << VS.first;
      }
      else
      {
         printVarName(VS.first, OS);
      }
      OS << quot << "\n";
   }

   // ========================================================================== //
   // StoreOp
   // ========================================================================== //

   StoreOp::StoreOp(VarNode* sink, const Instruction* inst, Range _init) : BasicOp(std::make_shared<BasicInterval>(), sink, inst), init(std::move(_init))
   {
   }

   StoreOp::~StoreOp() = default;

   Range StoreOp::eval()
   {
      Range result = init;
      // Iterate over the sources of the Store
      for(const VarNode* varNode : sources)
      {
         result = result.unionWith(varNode->getRange());
      }
      return result;
   }

   void StoreOp::print(raw_ostream& OS) const
   {
      const char* quot = R"(")";
      OS << " " << quot << this << quot << R"( [label=")";
      OS << "StoreOp\"]\n";

      for(auto src : sources)
      {
         const auto V = src->getValue();
         if(const auto* C = dyn_cast<ConstantInt>(V.first))
         {
            OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
         }
         else
         {
            OS << " " << quot;
            if(V.second)
            {
               printVarName(V.second, OS);
               OS << "." << V.first;
            }
            else
            {
               printVarName(V.first, OS);
            }
            OS << quot << " -> " << quot << this << quot << "\n";
         }
      }

      const auto VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      if(VS.second)
      {
         printVarName(VS.second, OS);
         OS << "." << VS.first;
      }
      else
      {
         printVarName(VS.first, OS);
      }
      OS << quot << "\n";
   }

   // ========================================================================== //
   // UnaryOp
   // ========================================================================== //

   UnaryOp::UnaryOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const Instruction* inst, VarNode* source, unsigned int opcode) : BasicOp(std::move(intersect), sink, inst), source(source), opcode(opcode)
   {
   }

   // The dtor.
   UnaryOp::~UnaryOp() = default;

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   Range UnaryOp::eval()
   {
      unsigned bw = getSink()->getBitWidth();
      Range oprnd = source->getRange();
      Range result(Unknown, bw, Min, Max);

      if(oprnd.isRegular() || oprnd.isAnti())
      {
         //         getSink()->getValue().first->print(llvm::errs());
         //         llvm::errs()<< "\n";
         //         oprnd.print(llvm::errs());
         switch(this->getOpcode())
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
         //         llvm::errs()<< "=";
         //         result.print(llvm::errs());
         //         llvm::errs()<< "\n";
      }
      else if(oprnd.isEmpty())
      {
         result = Range(Empty, bw);
      }

      auto test = this->getIntersect()->getRange().isMaxRange();
      if(!test)
      {
         Range aux = this->getIntersect()->getRange();
         Range intersect = result.intersectWith(aux);
         if(!intersect.isEmpty())
         {
            result = intersect;
         }
         //         llvm::errs() << "intersection: ";
         //         result.print(llvm::errs());
         //         llvm::errs() << "\n";
      }
      return result;
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void UnaryOp::print(raw_ostream& OS) const
   {
      const char* quot = R"(")";
      OS << " " << quot << this << quot << R"( [label=")";

      // Instruction bitwidth
      unsigned bw = getSink()->getBitWidth();

      switch(this->opcode)
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

      const auto V = this->getSource()->getValue();
      if(const auto* C = dyn_cast<ConstantInt>(V.first))
      {
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V.second)
         {
            printVarName(V.second, OS);
            OS << "." << V.first;
         }
         else
         {
            printVarName(V.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }

      const auto VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      if(VS.second)
      {
         printVarName(VS.second, OS);
         OS << "." << VS.first;
      }
      else
      {
         printVarName(VS.first, OS);
      }
      OS << quot << "\n";
   }

   // ========================================================================== //
   // SigmaOp
   // ========================================================================== //

   SigmaOp::SigmaOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const Instruction* inst, VarNode* source, VarNode* _SymbolicSource, unsigned int opcode)
       : UnaryOp(std::move(intersect), sink, inst, source, opcode), SymbolicSource(_SymbolicSource), unresolved(false)
   {
   }

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   Range SigmaOp::eval()
   {
      Range result = this->getSource()->getRange();
      //      llvm::errs() << "SigmaOp: ";
      //      getSink()->print(llvm::errs());
      //      llvm::errs()<<" src=";
      //      result.print(llvm::errs());
      Range aux = this->getIntersect()->getRange();
      //      llvm::errs() << " aux = ";
      //      aux.print(llvm::errs());
      //      llvm::errs()<<"\n";
      if(!aux.isUnknown())
      {
         Range intersect = result.intersectWith(aux);
         if(!intersect.isEmpty())
         {
            result = intersect;
         }
      }
      //      llvm::errs() << " = ";
      //      result.print(llvm::errs());
      //      llvm::errs()<<"\n";
      return result;
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void SigmaOp::print(raw_ostream& OS) const
   {
      const char* quot = R"(")";
      OS << " " << quot << this << quot << R"( [label=")"
         << "SigmnaOp:";
      this->getIntersect()->print(OS);
      OS << "\"]\n";
      const auto V = this->getSource()->getValue();
      if(const auto* C = dyn_cast<ConstantInt>(V.first))
      {
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V.second)
         {
            printVarName(V.second, OS);
            OS << "." << V.first;
         }
         else
         {
            printVarName(V.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      if(SymbolicSource)
      {
         const auto V = SymbolicSource->getValue();
         if(const auto* C = dyn_cast<ConstantInt>(V.first))
         {
            OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
         }
         else
         {
            OS << " " << quot;
            if(V.second)
            {
               printVarName(V.second, OS);
               OS << "." << V.first;
            }
            else
            {
               printVarName(V.first, OS);
            }
            OS << quot << " -> " << quot << this << quot << "\n";
         }
      }

      const auto VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      if(VS.second)
      {
         printVarName(VS.second, OS);
         OS << "." << VS.first;
      }
      else
      {
         printVarName(VS.first, OS);
      }
      OS << quot << "\n";
   }

   // ========================================================================== //
   // BinaryOp
   // ========================================================================== //

   // The ctor.
   BinaryOp::BinaryOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const Instruction* inst, VarNode* source1, VarNode* source2, unsigned int opcode) : BasicOp(std::move(intersect), sink, inst), source1(source1), source2(source2), opcode(opcode)
   {
   }

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   /// Basically, this function performs the operation indicated in its opcode
   /// taking as its operands the source1 and the source2.
   Range BinaryOp::eval()
   {
      Range op1 = this->getSource1()->getRange();
      Range op2 = this->getSource2()->getRange();
      // Instruction bitwidth
      unsigned bw = getSink()->getBitWidth();
      Range result(Unknown, bw);

      // only evaluate if all operands are Regular
      if((op1.isRegular() || op1.isAnti()) && (op2.isRegular() || op2.isAnti()))
      {
         //         getSink()->getValue().first->print(llvm::errs());
         //         llvm::errs()<< "\n";
         //         op1.print(llvm::errs());
         //         llvm::errs()<< ",";
         //         op2.print(llvm::errs());
         assert(op1.getBitWidth() == bw || this->getOpcode() == llvm::Instruction::ICmp);
         assert(op2.getBitWidth() == bw || this->getOpcode() == llvm::Instruction::ICmp);
         switch(this->getOpcode())
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
               result = op1.Or(op2);
               break;
            case Instruction::Xor:
               result = op1.Xor(op2);
               break;
            case llvm::Instruction::ICmp:
            {
               auto pred = cast<llvm::CmpInst>(getSink()->getValue().first)->getPredicate();
               if(pred == llvm::ICmpInst::ICMP_EQ)
               {
                  result = op1.Eq(op2, bw);
               }
               else if(pred == llvm::ICmpInst::ICMP_NE)
               {
                  result = op1.Ne(op2, bw);
               }
               else if(pred == llvm::ICmpInst::ICMP_UGT)
               {
                  result = op1.Ugt(op2, bw);
               }
               else if(pred == llvm::ICmpInst::ICMP_UGE)
               {
                  result = op1.Uge(op2, bw);
               }
               else if(pred == llvm::ICmpInst::ICMP_ULT)
               {
                  result = op1.Ult(op2, bw);
               }
               else if(pred == llvm::ICmpInst::ICMP_ULE)
               {
                  result = op1.Ule(op2, bw);
               }
               else if(pred == llvm::ICmpInst::ICMP_SGT)
               {
                  result = op1.Sgt(op2, bw);
               }
               else if(pred == llvm::ICmpInst::ICMP_SGE)
               {
                  result = op1.Sge(op2, bw);
               }
               else if(pred == llvm::ICmpInst::ICMP_SLT)
               {
                  result = op1.Slt(op2, bw);
               }
               else if(pred == llvm::ICmpInst::ICMP_SLE)
               {
                  result = op1.Sle(op2, bw);
               }
               else
               {
                  result = Range(Regular, bw, Zero, One);
               }
               break;
            }
            default:
               break;
         }
         //         llvm::errs()<< "=";
         //         result.print(llvm::errs());
         //         llvm::errs()<< "\n";
         bool test = this->getIntersect()->getRange().isMaxRange();
         if(!test)
         {
            Range aux = this->getIntersect()->getRange();
            //            llvm::errs()<< "  aux=";
            //            aux.print(llvm::errs());
            //            llvm::errs()<< "\n";
            Range intersect = result.intersectWith(aux);
            if(!intersect.isEmpty())
            {
               result = intersect;
            }
         }
      }
      else
      {
         if(op1.isEmpty() || op2.isEmpty())
         {
            result = Range(Empty, bw);
         }
      }
      return result;
   }

   /// Pretty print.
   void BinaryOp::print(raw_ostream& OS) const
   {
      const char* quot = R"(")";
      const char* opcodeName = Instruction::getOpcodeName(this->getOpcode());
      OS << " " << quot << this << quot << R"( [label=")" << opcodeName << "\"]\n";
      const auto V1 = this->getSource1()->getValue();
      if(const auto* C = dyn_cast<ConstantInt>(V1.first))
      {
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V1.second)
         {
            printVarName(V1.second, OS);
            OS << "." << V1.first;
         }
         else
         {
            printVarName(V1.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      const auto V2 = this->getSource2()->getValue();
      if(const auto* C = dyn_cast<ConstantInt>(V2.first))
      {
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V2.second)
         {
            printVarName(V2.second, OS);
            OS << "." << V2.first;
         }
         else
         {
            printVarName(V2.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      const auto VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      if(VS.second)
      {
         printVarName(VS.second, OS);
         OS << "." << VS.first;
      }
      else
      {
         printVarName(VS.first, OS);
      }
      OS << quot << "\n";
   }

   // ========================================================================== //
   // TernaryOp
   // ========================================================================== //

   // The ctor.
   TernaryOp::TernaryOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const Instruction* inst, VarNode* source1, VarNode* source2, VarNode* source3, unsigned int opcode)
       : BasicOp(std::move(intersect), sink, inst), source1(source1), source2(source2), source3(source3), opcode(opcode)
   {
   }

   Range TernaryOp::eval()
   {
      Range op1 = this->getSource1()->getRange();
      Range op2 = this->getSource2()->getRange();
      Range op3 = this->getSource3()->getRange();
      // Instruction bitwidth
      unsigned bw = getSink()->getBitWidth();
      assert(bw == op2.getBitWidth());
      assert(bw == op3.getBitWidth());
      Range result(Unknown, bw, Min, Max);

      // only evaluate if all operands are Regular
      if((op1.isRegular() || op1.isAnti()) && (op2.isRegular() || op2.isAnti()) && (op3.isRegular() || op3.isAnti()))
      {
         //         getSink()->getValue().first->print(llvm::errs());
         //         llvm::errs()<< "\n";
         //         op1.print(llvm::errs());
         //         llvm::errs()<< "?";
         //         op2.print(llvm::errs());
         //         llvm::errs()<< ":";
         //         op3.print(llvm::errs());
         switch(this->getOpcode())
         {
            case Instruction::Select:
            {
               // Source1 is the selector
               if(op1 == Range(Regular, op1.getBitWidth(), One, One))
               {
                  result = op2;
               }
               else if(op1 == Range(Regular, op1.getBitWidth(), Zero, Zero))
               {
                  result = op3;
               }
               else
               {
                  auto I = getInstruction();
                  auto opV0 = I->getOperand(0);

                  if(isa<ICmpInst>(opV0))
                  {
                     const Value* CondOp0 = cast<ICmpInst>(opV0)->getOperand(0);
                     const Value* CondOp1 = cast<ICmpInst>(opV0)->getOperand(1);
                     if(isa<ConstantInt>(CondOp0) || isa<ConstantInt>(CondOp1))
                     {
                        auto variable = isa<ConstantInt>(CondOp0) ? CondOp1 : CondOp0;
                        auto constant = isa<ConstantInt>(CondOp0) ? cast<ConstantInt>(CondOp0) : cast<ConstantInt>(CondOp1);
                        auto opV1 = I->getOperand(1);
                        auto opV2 = I->getOperand(2);
                        if(variable == opV1 || variable == opV2)
                        {
                           ConstantRange CR(constant->getValue(), constant->getValue() + 1);
                           CmpInst::Predicate pred = cast<ICmpInst>(opV0)->getPredicate();
                           CmpInst::Predicate swappred = cast<ICmpInst>(opV0)->getSwappedPredicate();

                           ConstantRange tmpT = (variable == CondOp0) ? ConstantRange::makeSatisfyingICmpRegion(pred, CR) : ConstantRange::makeSatisfyingICmpRegion(swappred, CR);
                           assert(!tmpT.isFullSet());
                           auto bw = op2.getBitWidth();
                           assert(bw == op3.getBitWidth());
                           auto tmpT_Range = CR2R(tmpT, bw);

                           if(variable == opV2)
                           {
                              Range FValues = Range(Range_base::getAnti(tmpT_Range));
                              op3 = op3.intersectWith(FValues);
                           }
                           else
                           {
                              const Range& TValues = tmpT_Range;
                              op2 = op2.intersectWith(TValues);
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
         //         llvm::errs()<< "=";
         //         result.print(llvm::errs());
         //         llvm::errs()<< "\n";
         bool test = this->getIntersect()->getRange().isMaxRange();
         if(!test)
         {
            Range aux = this->getIntersect()->getRange();
            Range intersect = result.intersectWith(aux);
            if(!intersect.isEmpty())
            {
               result = intersect;
            }
         }
      }
      else
      {
         if(op1.isEmpty() || op2.isEmpty() || op3.isEmpty())
         {
            result = Range(Empty, bw);
         }
      }
      return result;
   }

   /// Pretty print.
   void TernaryOp::print(raw_ostream& OS) const
   {
      const char* quot = R"(")";
      const char* opcodeName = Instruction::getOpcodeName(this->getOpcode());
      OS << " " << quot << this << quot << R"( [label=")" << opcodeName << "\"]\n";

      const auto V1 = this->getSource1()->getValue();
      if(const auto* C = dyn_cast<ConstantInt>(V1.first))
      {
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V1.second)
         {
            printVarName(V1.second, OS);
            OS << "." << V1.first;
         }
         else
         {
            printVarName(V1.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      const auto V2 = this->getSource2()->getValue();
      if(const auto* C = dyn_cast<ConstantInt>(V2.first))
      {
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V2.second)
         {
            printVarName(V2.second, OS);
            OS << "." << V2.first;
         }
         else
         {
            printVarName(V2.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }

      const auto V3 = this->getSource3()->getValue();
      if(const auto* C = dyn_cast<ConstantInt>(V3.first))
      {
         OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V3.second)
         {
            printVarName(V3.second, OS);
            OS << "." << V3.first;
         }
         else
         {
            printVarName(V3.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      const auto VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      if(VS.second)
      {
         printVarName(VS.second, OS);
         OS << "." << VS.first;
      }
      else
      {
         printVarName(VS.first, OS);
      }
      OS << quot << "\n";
   }

   // ========================================================================== //
   // PhiOp
   // ========================================================================== //

   // The ctor.
   PhiOp::PhiOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const Instruction* inst) : BasicOp(std::move(intersect), sink, inst)
   {
   }

   /// Computes the interval of the sink based on the interval of the sources.
   /// The result of evaluating a phi-function is the union of the ranges of
   /// every variable used in the phi.
   Range PhiOp::eval()
   {
      assert(sources.size() > 0);
      Range result = this->getSource(0)->getRange();
      //      getSink()->print(llvm::errs());
      //      llvm::errs() << "\n";
      // Iterate over the sources of the phiop
      for(const VarNode* varNode : sources)
      {
         //         llvm::errs() << " ->";
         //         varNode->getRange().print(llvm::errs());
         //         llvm::errs() << "\n";
         result = result.unionWith(varNode->getRange());
      }
      //      llvm::errs() << "=";
      //      result.print(llvm::errs());
      //      llvm::errs() << "\n";
      bool test = this->getIntersect()->getRange().isMaxRange();
      if(!test)
      {
         Range aux = this->getIntersect()->getRange();
         //         llvm::errs() << " aux= ";
         //         aux.print(llvm::errs());
         Range intersect = result.intersectWith(aux);
         if(!intersect.isEmpty())
         {
            result = intersect;
         }
      }
      //      llvm::errs() << " res= ";
      //      result.print(llvm::errs());
      //      llvm::errs() << "\n";
      return result;
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void PhiOp::print(raw_ostream& OS) const
   {
      const char* quot = R"(")";
      OS << " " << quot << this << quot << R"( [label=")";
      OS << "phi";
      OS << "\"]\n";
      for(const VarNode* varNode : sources)
      {
         const auto V = varNode->getValue();
         if(const auto* C = dyn_cast<ConstantInt>(V.first))
         {
            OS << " " << C->getValue() << " -> " << quot << this << quot << "\n";
         }
         else
         {
            OS << " " << quot;
            if(V.second)
            {
               printVarName(V.second, OS);
               OS << "." << V.first;
            }
            else
            {
               printVarName(V.first, OS);
            }
            OS << quot << " -> " << quot << this << quot << "\n";
         }
      }
      const auto VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      if(VS.second)
      {
         printVarName(VS.second, OS);
         OS << "." << VS.first;
      }
      else
      {
         printVarName(VS.first, OS);
      }
      OS << quot << "\n";
   }

   // ========================================================================== //
   // ConstraintGraph
   // ========================================================================== //

   /// The dtor.
   ConstraintGraph::~ConstraintGraph()
   {
      for(auto& pair : vars)
      {
         delete pair.second;
      }
      for(BasicOp* op : oprs)
      {
         delete op;
      }
   }

   Range ConstraintGraph::getRange(eValue v, const llvm::DataLayout* DL)
   {
      auto vit = this->vars.find(v);
      if(vit == this->vars.end())
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
         auto bw = LoadStoreOperationBW(v.first, v.second, DL);
         assert(bw);
         const auto* ci = dyn_cast<ConstantInt>(v.first);
         if(ci == nullptr)
         {
            return Range(Unknown, bw);
         }
         APInt tmp = ci->getValue();
         if(tmp.getBitWidth() < MAX_BIT_INT)
         {
            tmp = tmp.sext(MAX_BIT_INT);
         }
         return Range(Regular, bw, tmp, tmp);
      }
      return vit->second->getRange();
   }

   /// Adds a VarNode to the graph.
   VarNode* ConstraintGraph::addVarNode(const Value* V, const Value* GV, const llvm::DataLayout* DL)
   {
      eValue ev(V, GV);
      auto vit = this->vars.find(ev);
      if(vit != this->vars.end())
      {
         return vit->second;
      }

      auto* node = new VarNode(V, GV, DL);
      this->vars.insert(std::make_pair(ev, node));

      // Inserts the node in the use map list.
      SmallPtrSet<BasicOp*, 8> useList;
      this->useMap.insert(std::make_pair(ev, useList));
      return node;
   }

   /// Adds an UnaryOp in the graph.
   void ConstraintGraph::addUnaryOp(const Instruction* I, ModulePass* modulePass, const llvm::DataLayout* DL)
   {
      assert(I->getNumOperands() == 1U);
      // Create the sink.
      VarNode* sink = addVarNode(I, nullptr, DL);
      // Create the source.
      VarNode* source = nullptr;

      switch(I->getOpcode())
      {
         case Instruction::Trunc:
         case Instruction::ZExt:
         case Instruction::SExt:
            source = addVarNode(I->getOperand(0), nullptr, DL);
            break;
         default:
            return;
      }
      UnaryOp* UOp = nullptr;
      UOp = new UnaryOp(std::make_shared<BasicInterval>(getLLVM_range(I, modulePass, DL)), sink, I, source, I->getOpcode());
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
   void ConstraintGraph::addBinaryOp(const Instruction* I, ModulePass* modulePass, const llvm::DataLayout* DL)
   {
      assert(I->getNumOperands() == 2U);
      // Create the sink.
      VarNode* sink = addVarNode(I, nullptr, DL);

      // Create the sources.
      VarNode* source1 = addVarNode(I->getOperand(0), nullptr, DL);
      VarNode* source2 = addVarNode(I->getOperand(1), nullptr, DL);

      // Create the operation using the intersect to constrain sink's interval.
      std::shared_ptr<BasicInterval> BI = std::make_shared<BasicInterval>(getLLVM_range(I, modulePass, DL));
      BinaryOp* BOp = new BinaryOp(BI, sink, I, source1, source2, I->getOpcode());

      // Insert the operation in the graph.
      this->oprs.insert(BOp);

      // Insert this definition in defmap
      this->defMap[sink->getValue()] = BOp;

      // Inserts the sources of the operation in the use map list.
      this->useMap.find(source1->getValue())->second.insert(BOp);
      this->useMap.find(source2->getValue())->second.insert(BOp);
   }

   void ConstraintGraph::addTernaryOp(const Instruction* I, ModulePass* modulePass, const llvm::DataLayout* DL)
   {
      assert(I->getNumOperands() == 3U);
      // Create the sink.
      VarNode* sink = addVarNode(I, nullptr, DL);

      // Create the sources.
      VarNode* source1 = addVarNode(I->getOperand(0), nullptr, DL);
      VarNode* source2 = addVarNode(I->getOperand(1), nullptr, DL);
      VarNode* source3 = addVarNode(I->getOperand(2), nullptr, DL);

      // Create the operation using the intersect to constrain sink's interval.
      std::shared_ptr<BasicInterval> BI = std::make_shared<BasicInterval>(getLLVM_range(I, modulePass, DL));
      TernaryOp* TOp = new TernaryOp(BI, sink, I, source1, source2, source3, I->getOpcode());

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
   void ConstraintGraph::addPhiOp(const PHINode* Phi, ModulePass* modulePass, const llvm::DataLayout* DL)
   {
      // Create the sink.
      VarNode* sink = addVarNode(Phi, nullptr, DL);
      std::shared_ptr<BasicInterval> BI = std::make_shared<BasicInterval>(getLLVM_range(Phi, modulePass, DL));
      PhiOp* phiOp = new PhiOp(BI, sink, Phi);

      // Insert the operation in the graph.
      this->oprs.insert(phiOp);

      // Insert this definition in defmap
      this->defMap[sink->getValue()] = phiOp;

      // Create the sources.
      for(const Value* operand : Phi->operands())
      {
         VarNode* source = addVarNode(operand, nullptr, DL);
         phiOp->addSource(source);
         // Inserts the sources of the operation in the use map list.
         this->useMap.find(source->getValue())->second.insert(phiOp);
      }
   }

   void ConstraintGraph::addSigmaOp(const PHINode* Sigma, ModulePass* modulePass, const llvm::DataLayout* DL)
   {
      assert(Sigma->getNumOperands() == 1U);
      // Create the sink.
      VarNode* sink = addVarNode(Sigma, nullptr, DL);
      std::shared_ptr<BasicInterval> BItv;
      SigmaOp* sigmaOp = nullptr;

      const BasicBlock* thisbb = Sigma->getParent();
      for(const Value* operand : Sigma->operands())
      {
         VarNode* source = addVarNode(operand, nullptr, DL);

         // Create the operation (two cases from: branch or switch)
         auto vbmit = this->valuesBranchMap.find(operand);

         // Branch case
         if(vbmit != this->valuesBranchMap.end())
         {
            const ValueBranchMap& VBM = vbmit->second;
            if(thisbb == VBM.getBBTrue())
            {
               BItv = VBM.getItvT();
            }
            else
            {
               if(thisbb == VBM.getBBFalse())
               {
                  BItv = VBM.getItvF();
               }
            }
         }
         else
         {
            // Switch case
            auto vsmit = this->valuesSwitchMap.find(operand);

            if(vsmit == this->valuesSwitchMap.end())
            {
               continue;
            }

            const ValueSwitchMap& VSM = vsmit->second;
            // Find out which case are we dealing with
            for(unsigned idx = 0, e = VSM.getNumOfCases(); idx < e; ++idx)
            {
               const BasicBlock* bb = VSM.getBB(idx);
               if(bb == thisbb)
               {
                  BItv = VSM.getItv(idx);
                  break;
               }
            }
         }

         if(BItv == nullptr)
         {
            sigmaOp = new SigmaOp(std::make_shared<BasicInterval>(getLLVM_range(Sigma, modulePass, DL)), sink, Sigma, source, nullptr, Sigma->getOpcode());
         }
         else
         {
            //            llvm::errs() << "Add SigmaOp: ";
            //            BItv->print(llvm::errs());
            //            llvm::errs()<<"\n";
            //            BItv->getRange().print(llvm::errs());
            //            llvm::errs()<<"\n";
            VarNode* SymbSrc = nullptr;
            if(auto symb = dyn_cast<SymbInterval>(BItv.get()))
            {
               auto bound = symb->getBound();
               SymbSrc = addVarNode(bound.first, bound.second, DL);
            }
            sigmaOp = new SigmaOp(BItv, sink, Sigma, source, SymbSrc, Sigma->getOpcode());
            if(SymbSrc)
            {
               this->useMap.find(SymbSrc->getValue())->second.insert(sigmaOp);
            }
         }

         // Insert the operation in the graph.
         this->oprs.insert(sigmaOp);

         // Insert this definition in defmap
         this->defMap[sink->getValue()] = sigmaOp;

         // Inserts the sources of the operation in the use map list.
         this->useMap.find(source->getValue())->second.insert(sigmaOp);
      }
   }

   void ConstraintGraph::addLoadOp(const llvm::LoadInst* LI, Andersen_AA* PtoSets_AA, bool arePointersResolved, llvm::ModulePass* modulePass, const llvm::DataLayout* DL,
                                   llvm::DenseMap<const llvm::Function*, llvm::SmallPtrSet<const llvm::Instruction*, 6>>& Function2Store)
   {
      auto bw = LI->getType()->getPrimitiveSizeInBits();
      Range intersection(Regular, bw, Min, Max);
      if(LI->isSimple())
      {
         auto PO = LI->getPointerOperand();
         bool pointToConstant = false;
#if HAVE_LIBBDD
         if(PtoSets_AA)
         {
            auto PTS = PtoSets_AA->PE(PO) == NOVAR_ID ? nullptr : PtoSets_AA->pointsToSet(PO);
            if(PTS && !PTS->empty())
            {
               pointToConstant = true;
               for(auto var : *PTS)
               {
                  auto varValue = PtoSets_AA->getValue(var);
                  if(!varValue)
                  {
                     pointToConstant = false;
                     break;
                  }
                  if(!isa<llvm::GlobalVariable>(varValue) || !cast<llvm::GlobalVariable>(varValue)->isConstant() || cast<llvm::GlobalVariable>(varValue)->getValueType()->isStructTy() ||
                     (cast<llvm::GlobalVariable>(varValue)->getValueType()->isArrayTy() && !cast<llvm::ArrayType>(cast<llvm::GlobalVariable>(varValue)->getValueType())->getElementType()->isIntegerTy()) ||
                     (cast<llvm::GlobalVariable>(varValue)->getValueType()->isArrayTy() && cast<llvm::ArrayType>(cast<llvm::GlobalVariable>(varValue)->getValueType())->getElementType()->getPrimitiveSizeInBits() != bw) ||
                     (cast<llvm::GlobalVariable>(varValue)->getValueType()->isSingleValueType() && cast<llvm::GlobalVariable>(varValue)->getValueType()->getPrimitiveSizeInBits() != bw))
                  {
                     pointToConstant = false;
                     break;
                  }
               }
            }
         }
#endif
         if(pointToConstant)
         {
#if HAVE_LIBBDD
            Range res(Empty, bw);
            for(auto var : *PtoSets_AA->pointsToSet(PO))
            {
               auto varValue = PtoSets_AA->getValue(var);
               assert(varValue);
               assert(isa<llvm::GlobalVariable>(varValue) && cast<llvm::GlobalVariable>(varValue)->isConstant());
               auto ROM = cast<llvm::GlobalVariable>(varValue);
               auto init_value = ROM->getInitializer();
               auto ivid = init_value->getValueID();
               if(ivid == llvm::Value::ConstantArrayVal || ivid == llvm::Value::ConstantDataArrayVal || ivid == llvm::Value::ConstantDataVectorVal || ivid == llvm::Value::ConstantAggregateZeroVal || ivid == llvm::Value::ConstantIntVal)
               {
                  res = res.unionWith(getLLVM_range(init_value));
               }
               else
               {
                  init_value->print(llvm::errs(), true);
                  llvm_unreachable("unexpected condition");
               }
            }
            intersection = res;
#endif
         }
         else
         {
            intersection = getLLVM_range(LI, modulePass, DL);
         }
      }
      else
      {
         intersection = getLLVM_range(LI, modulePass, DL);
      }
      VarNode* sink = addVarNode(LI, nullptr, DL);
      LoadOp* loadOp = new LoadOp(std::make_shared<BasicInterval>(intersection), sink, LI);
      // Insert the operation in the graph.
      this->oprs.insert(loadOp);
      // Insert this definition in defmap
      this->defMap[sink->getValue()] = loadOp;
#if HAVE_LIBBDD
      if(arePointersResolved)
      {
         assert(PtoSets_AA);
         auto PO = LI->getPointerOperand();
         if(PtoSets_AA->PE(PO) != NOVAR_ID)
         {
            for(auto var : *PtoSets_AA->pointsToSet(PO))
            {
               auto varValue = PtoSets_AA->getValue(var);
               assert(varValue);
               //               llvm::errs() << "LoadedVar: ";
               //               varValue->print(llvm::errs());
               //               llvm::errs() << "\n";

               for(const Value* operand : ComputeConflictingStores(nullptr, varValue, LI, PtoSets_AA, Function2Store, modulePass))
               {
                  //                  llvm::errs() << "  source: ";
                  //                  operand->print(llvm::errs());
                  //                  llvm::errs() << "\n";
                  VarNode* source = addVarNode(operand, varValue, DL);
                  loadOp->addSource(source);
                  this->useMap.find(source->getValue())->second.insert(loadOp);
               }
            }
         }
      }
#endif
   }

   static bool recurseComputeConflictingStores(llvm::SmallPtrSet<const llvm::Value*, 6>& visited, const Instruction* mInstr, const llvm::Value* GV, llvm::SmallPtrSet<const llvm::Value*, 6>& res, Andersen_AA* PtoSets_AA,
                                               llvm::DenseMap<const Function*, SmallPtrSet<const Instruction*, 6>>& Function2Store)
   {
      assert(mInstr);
      bool GVfound = false;
      if(visited.find(mInstr) != visited.end())
      {
         return GVfound;
      }
      visited.insert(mInstr);
      if(auto ld = dyn_cast<LoadInst>(mInstr))
      {
         assert(ld->isVolatile());
         res.insert(mInstr);
         GVfound = true;
      }
      else if(auto st = dyn_cast<StoreInst>(mInstr))
      {
#if HAVE_LIBBDD
         auto PO = st->getPointerOperand();
         for(auto var : *PtoSets_AA->pointsToSet(PO))
         {
            auto varValue = PtoSets_AA->getValue(var);
            assert(varValue);
            if(varValue == GV)
            {
               GVfound = true;
               break;
            }
         }
#else
         GVfound = true;

#endif
         if(GVfound)
         {
            //            llvm::errs() << "      added0 ";
            //            mInstr->print(llvm::errs());
            //            llvm::errs() << "\n";
            res.insert(mInstr);
         }
      }
      else if(auto CI = dyn_cast<CallInst>(mInstr))
      {
         auto CF = CI->getCalledFunction();
         assert(CF);
         if(CF->isDeclaration() || CF->isIntrinsic())
         {
            GVfound = true;
            //            llvm::errs() << "      added1 ";
            //            mInstr->print(llvm::errs());
            //            llvm::errs() << "\n";
            res.insert(mInstr);
         }
         else
         {
            if(Function2Store.find(CF) != Function2Store.end()) // relevant only in case CF has stores
            {
               bool morethanoneBB = false;
               for(auto si : Function2Store.find(CF)->second)
               {
                  auto gvFOUND = recurseComputeConflictingStores(visited, si, GV, res, PtoSets_AA, Function2Store);
                  GVfound = (!morethanoneBB) && (GVfound || gvFOUND);
                  unsigned bbGT1 = 0;
                  for(auto& BB : *CF)
                  {
                     if(bbGT1 > 0)
                     {
                        GVfound = false;
                        morethanoneBB = true;
                        break;
                     }
                     ++bbGT1;
                  }
               }
            }
         }
      }
      else if(auto II = dyn_cast<llvm::InvokeInst>(mInstr))
      {
         auto CF = II->getCalledFunction();
         assert(CF);
         if(CF->isDeclaration() || CF->isIntrinsic())
         {
            GVfound = true;
            //            llvm::errs() << "      added2 ";
            //            mInstr->print(llvm::errs());
            //            llvm::errs() << "\n";
            res.insert(mInstr);
         }
         else
         {
            if(Function2Store.find(CF) != Function2Store.end()) // relevant only in case CF has stores
            {
               bool morethanoneBB = false;
               for(auto si : Function2Store.find(CF)->second)
               {
                  auto gvFOUND = recurseComputeConflictingStores(visited, si, GV, res, PtoSets_AA, Function2Store);
                  GVfound = (!morethanoneBB) && (GVfound || gvFOUND);
                  if(CF->size() > 1)
                  {
                     GVfound = false;
                     morethanoneBB = true;
                     break;
                  }
               }
            }
         }
      }
      else
      {
         llvm_unreachable("unexpected condition");
      }
      return GVfound;
   }
   static void addAllClobberingStmts(llvm::DenseSet<std::pair<const llvm::Instruction*, const llvm::Function*>>& toBeAnalyzed, const llvm::MemorySSA& MSSA, llvm::MemoryAccess* ma)
   {
      llvm::SmallPtrSet<llvm::MemoryAccess*, 6> Analyzed;
      llvm::SmallPtrSet<llvm::MemoryAccess*, 6> MAtoBeAnalyzed;
      MAtoBeAnalyzed.insert(ma);

      while(!MAtoBeAnalyzed.empty())
      {
         auto currMA = *MAtoBeAnalyzed.begin();
         MAtoBeAnalyzed.erase(currMA);
         if(Analyzed.find(currMA) == Analyzed.end())
         {
            Analyzed.insert(currMA);
            if(auto mp = dyn_cast<llvm::MemoryPhi>(currMA))
            {
               //               llvm::errs() << "    addAllClobberingStmts-phi\n";
               for(auto index = 0u; index < mp->getNumIncomingValues(); ++index)
               {
                  auto val = mp->getIncomingValue(index);
                  assert(val->getValueID() == llvm::Value::MemoryDefVal || val->getValueID() == llvm::Value::MemoryPhiVal);
                  if(!MSSA.isLiveOnEntryDef(val))
                  {
                     MAtoBeAnalyzed.insert(val);
                  }
                  else
                  {
                     //                     llvm::errs() << "    addAllClobberingStmts-function: " << mp->getBlock()->getParent()->getName() << "\n";
                     auto el = std::pair<llvm::Instruction*, llvm::Function*>(nullptr, mp->getBlock()->getParent());
                     toBeAnalyzed.insert(el);
                  }
               }
            }
            else
            {
               //               llvm::errs() << "    addAllClobberingStmts-mud\n";
               auto mud = dyn_cast<llvm::MemoryUseOrDef>(currMA);
               assert(mud);
               toBeAnalyzed.insert(std::pair<llvm::Instruction*, llvm::Function*>(mud->getMemoryInst(), nullptr));
            }
         }
      }
   }
   static void recurseUpMemoryAccess(const llvm::Function* fun, llvm::DenseSet<std::pair<const llvm::Instruction*, const llvm::Function*>>& toBeAnalyzedInstr, llvm::ModulePass* modulePass)
   {
      for(auto fuse : fun->users())
      {
         if(auto instr = dyn_cast<llvm::Instruction>(fuse))
         {
            //            llvm::errs() << "    recurseUpMemoryAccess-instr-" << instr->getFunction()->getName() << ": ";
            //            instr->print(llvm::errs());
            //            llvm::errs() << "\n";
            const auto& MSSA = modulePass->getAnalysis<llvm::MemorySSAWrapperPass>(*const_cast<llvm::Function*>(instr->getFunction())).getMSSA();
            auto funMA = MSSA.getMemoryAccess(instr);
            if(!funMA)
            {
               continue;
            }
            auto funImmDefAcc = funMA->getDefiningAccess();
            if(!MSSA.isLiveOnEntryDef(funImmDefAcc))
            {
               //               llvm::errs() << "    add stmts\n";
               if(auto mud = dyn_cast<llvm::MemoryUseOrDef>(funImmDefAcc))
               {
                  toBeAnalyzedInstr.insert(std::pair<const llvm::Instruction*, const llvm::Function*>(mud->getMemoryInst(), nullptr));
               }
               else
               {
                  //                  llvm::errs() << "    phisUp\n";
                  addAllClobberingStmts(toBeAnalyzedInstr, MSSA, funImmDefAcc);
               }
            }
            else
            {
               //               llvm::errs() << "    add upfunction " << instr->getFunction() << "\n";
               recurseUpMemoryAccess(instr->getFunction(), toBeAnalyzedInstr, modulePass);
            }
         }
      }
   }

   llvm::SmallPtrSet<const llvm::Value*, 6> ConstraintGraph::ComputeConflictingStores(const llvm::StoreInst* SI, const llvm::Value* GV, const llvm::Instruction* instr0, Andersen_AA* PtoSets_AA,
                                                                                      llvm::DenseMap<const Function*, SmallPtrSet<const Instruction*, 6>>& Function2Store, llvm::ModulePass* modulePass)
   {
      llvm::SmallPtrSet<const llvm::Value*, 6> res;
      llvm::DenseSet<std::pair<const llvm::Instruction*, const llvm::Function*>> toBeAnalyzed;
      llvm::DenseSet<std::pair<const llvm::Instruction*, const llvm::Function*>> Analyzed;
      const auto& MSSA0 = modulePass->getAnalysis<llvm::MemorySSAWrapperPass>(*const_cast<llvm::Function*>(instr0->getFunction())).getMSSA();
      const llvm::MemoryUseOrDef* ma0 = MSSA0.getMemoryAccess(instr0);
      auto immDefAcc0 = ma0->getDefiningAccess();
      if(!MSSA0.isLiveOnEntryDef(immDefAcc0))
      {
         if(auto mud = dyn_cast<llvm::MemoryUseOrDef>(immDefAcc0))
         {
            toBeAnalyzed.insert(std::pair<llvm::Instruction*, llvm::Function*>(mud->getMemoryInst(), nullptr));
         }
         else
         {
            //            llvm::errs() << "    phis0\n";
            addAllClobberingStmts(toBeAnalyzed, MSSA0, immDefAcc0);
         }
      }
      else
      {
         auto mInstr = ma0->getMemoryInst();
         auto fun = mInstr->getFunction();
         recurseUpMemoryAccess(fun, toBeAnalyzed, modulePass);
      }

      while(!toBeAnalyzed.empty())
      {
         auto currInstrFun = *toBeAnalyzed.begin();
         toBeAnalyzed.erase(currInstrFun);
         if(Analyzed.find(currInstrFun) == Analyzed.end())
         {
            Analyzed.insert(currInstrFun);
            if(currInstrFun.first)
            {
               auto mInstr = currInstrFun.first;
               if(mInstr != SI) /// self loop could be removed
               {
                  llvm::SmallPtrSet<const llvm::Value*, 6> visited;
                  bool GVfound = recurseComputeConflictingStores(visited, mInstr, GV, res, PtoSets_AA, Function2Store);
                  if(!GVfound)
                  {
                     const auto& MSSAm = modulePass->getAnalysis<llvm::MemorySSAWrapperPass>(*const_cast<llvm::Function*>(mInstr->getFunction())).getMSSA();
                     const llvm::MemoryUseOrDef* mam = MSSAm.getMemoryAccess(mInstr);
                     auto immDefAccm = mam->getDefiningAccess();
                     if(!MSSAm.isLiveOnEntryDef(immDefAccm))
                     {
                        if(auto mud = dyn_cast<llvm::MemoryUseOrDef>(immDefAccm))
                        {
                           toBeAnalyzed.insert(std::pair<llvm::Instruction*, llvm::Function*>(mud->getMemoryInst(), nullptr));
                        }
                        else
                        {
                           //                           llvm::errs() << "    phis\n";
                           addAllClobberingStmts(toBeAnalyzed, MSSAm, immDefAccm);
                        }
                     }
                     else
                     {
                        auto mInstrm = mam->getMemoryInst();
                        auto fun = mInstrm->getFunction();
                        recurseUpMemoryAccess(fun, toBeAnalyzed, modulePass);
                     }
                  }
               }
            }
            else if(currInstrFun.second)
            {
               //               llvm::errs() << "Upfunction: " << currInstrFun.second->getName() << "\n";
               recurseUpMemoryAccess(currInstrFun.second, toBeAnalyzed, modulePass);
            }
            else
            {
               llvm_unreachable("unexpected condition");
            }
         }
      }
      return res;
   }

   static bool isAggregateValue(const llvm::Value* varValue)
   {
      auto vid = varValue->getValueID();
      if(auto AI = llvm::dyn_cast<const llvm::AllocaInst>(varValue))
      {
         return AI->getAllocatedType()->isAggregateType();
      }
      if(auto GV = llvm::dyn_cast<const llvm::GlobalVariable>(varValue))
      {
         auto VGT = GV->getValueType();
         return VGT->isAggregateType();
      }
      if(vid == llvm::Value::FunctionVal)
      {
         return false;
      }

      varValue->print(llvm::errs());
      llvm_unreachable("unexpected condition");
   }
   void ConstraintGraph::addStoreOp(const llvm::StoreInst* SI, Andersen_AA* PtoSets_AA, bool arePointersResolved, llvm::ModulePass* modulePass, llvm::DenseMap<const Function*, SmallPtrSet<const Instruction*, 6>>& Function2Store, const llvm::DataLayout* DL)
   {
#if HAVE_LIBBDD
      if(arePointersResolved)
      {
         assert(PtoSets_AA);
         auto PO = SI->getPointerOperand();
         assert(PtoSets_AA->PE(PO) != NOVAR_ID);
         auto bw = SI->getValueOperand()->getType()->getPrimitiveSizeInBits();
         Range intersection(Regular, bw, Min, Max);

         for(auto var : *PtoSets_AA->pointsToSet(PO))
         {
            auto varValue = PtoSets_AA->getValue(var);
            assert(varValue);
            VarNode* sink = addVarNode(SI, varValue, DL);
            StoreOp* storeOp;
            if(dyn_cast<llvm::GlobalVariable>(varValue) && dyn_cast<llvm::GlobalVariable>(varValue)->hasInitializer())
            {
               storeOp = new StoreOp(sink, SI, getLLVM_range(dyn_cast<llvm::GlobalVariable>(varValue)->getInitializer()));
            }
            else
            {
               storeOp = new StoreOp(sink, SI, Range(Empty, bw));
            }
            this->oprs.insert(storeOp);
            this->defMap[sink->getValue()] = storeOp;
            VarNode* source = addVarNode(SI->getValueOperand(), nullptr, DL);
            storeOp->addSource(source);
            this->useMap.find(source->getValue())->second.insert(storeOp);

            if(isAggregateValue(varValue))
            {
               //               llvm::errs() << "SI: ";
               //               SI->print(llvm::errs());
               //               llvm::errs() << "\n";
               //               llvm::errs() << "    isAggregateValue: ";
               //               varValue->print(llvm::errs());
               //               llvm::errs() << "\n";
               for(const Value* operand : ComputeConflictingStores(SI, varValue, SI, PtoSets_AA, Function2Store, modulePass))
               {
                  //                  llvm::errs() << "  source: ";
                  //                  operand->print(llvm::errs());
                  //                  llvm::errs() << "\n";
                  VarNode* source = addVarNode(operand, varValue, DL);
                  storeOp->addSource(source);
                  this->useMap.find(source->getValue())->second.insert(storeOp);
               }
            }
         }
      }
#endif
   }

   namespace
   {
      // LLVM's Instructions.h doesn't provide a function like this, so I made one.
      bool isTernaryOp(const Instruction* I)
      {
         return isa<SelectInst>(I);
      }
   } // namespace

   void ConstraintGraph::buildOperations(const Instruction* I, ModulePass* modulePass, const llvm::DataLayout* DL, Andersen_AA* PtoSets_AA, bool arePointersResolved, llvm::DenseMap<const Function*, SmallPtrSet<const Instruction*, 6>>& Function2Store)
   {
      if(auto LI = dyn_cast<LoadInst>(I))
      {
         addLoadOp(LI, PtoSets_AA, arePointersResolved, modulePass, DL, Function2Store);
      }
      else if(auto SI = dyn_cast<StoreInst>(I))
      {
         addStoreOp(SI, PtoSets_AA, arePointersResolved, modulePass, Function2Store, DL);
      }
      else if(I->isBinaryOp())
      {
         addBinaryOp(I, modulePass, DL);
      }
      else if(isTernaryOp(I))
      {
         addTernaryOp(I, modulePass, DL);
      }
      else
      {
         if(isa<ICmpInst>(I))
         {
            addBinaryOp(I, modulePass, DL);
            // Handle Phi functions.
         }
         else if(auto Phi = dyn_cast<PHINode>(I))
         {
            if(Phi->getNumOperands() == 1)
            {
               addSigmaOp(Phi, modulePass, DL);
            }
            else
            {
               addPhiOp(Phi, modulePass, DL);
            }
         }
         else
         {
            // We have an unary instruction to handle.
            addUnaryOp(I, modulePass, DL);
         }
      }
   }

   void ConstraintGraph::buildValueSwitchMap(const SwitchInst* sw, const llvm::DataLayout* DL)
   {
      const Value* condition = sw->getCondition();
      // Verify conditions
      const Type* opType = sw->getCondition()->getType();
      if(!opType->isIntegerTy())
      {
         return;
      }

      // Create VarNode for switch condition explicitly
      addVarNode(condition, nullptr, DL);
      unsigned bw = condition->getType()->getPrimitiveSizeInBits();

      SmallVector<std::pair<std::shared_ptr<BasicInterval>, const BasicBlock*>, 4> BBsuccs;

      // Treat when condition of switch is a cast of the real condition (same thing
      // as in buildValueBranchMap)
      const CastInst* castinst = nullptr;
      const Value* Op0_0 = nullptr;
      if((castinst = dyn_cast<CastInst>(condition)) != nullptr)
      {
         Op0_0 = castinst->getOperand(0);
      }

      // Handle 'default', if there is any
      BasicBlock* succ = sw->getDefaultDest();
      if(succ != nullptr)
      {
         auto antidefaultRange = Range(Empty, bw);
         for(auto case_expr : sw->cases())
         {
            const ConstantInt* constant = case_expr.getCaseValue();
            APInt cval = constant->getValue();
            if(cval.getBitWidth() < MAX_BIT_INT)
            {
               cval = cval.sext(MAX_BIT_INT);
            }
            antidefaultRange = antidefaultRange.unionWith(Range(Regular, bw, cval, cval));
         }

         APInt sigMin = antidefaultRange.getLower();
         APInt sigMax = antidefaultRange.getUpper();
         Range Values = Range(Anti, bw, sigMin, sigMax);
         // Create the interval using the intersection in the branch.
         std::shared_ptr<BasicInterval> BI = std::make_shared<BasicInterval>(Values);
         BBsuccs.push_back(std::make_pair(BI, succ));
      }

      // Handle the rest of cases
      for(auto case_expr : sw->cases())
      {
         const ConstantInt* constant = case_expr.getCaseValue();

         APInt cval = constant->getValue();
         if(cval.getBitWidth() < MAX_BIT_INT)
         {
            cval = cval.sext(MAX_BIT_INT);
         }
         Range Values = Range(Regular, bw, cval, cval);
         // Create the interval using the intersection in the branch.
         std::shared_ptr<BasicInterval> BI = std::make_shared<BasicInterval>(Values);
         BBsuccs.push_back(std::make_pair(BI, succ));
      }

      ValueSwitchMap VSM(condition, BBsuccs);
      valuesSwitchMap.insert(std::make_pair(condition, VSM));

      if(Op0_0 != nullptr)
      {
         ValueSwitchMap VSM_0(Op0_0, BBsuccs);
         valuesSwitchMap.insert(std::make_pair(Op0_0, VSM_0));
      }
   }

   void ConstraintGraph::buildValueBranchMap(const BranchInst* br, const llvm::DataLayout* DL)
   {
      // Verify conditions
      if(!br->isConditional())
      {
         return;
      }
      auto* ici = dyn_cast<ICmpInst>(br->getCondition());
      if(ici == nullptr)
      {
         return;
      }

      const Type* op0Type = ici->getOperand(0)->getType();
      const Type* op1Type = ici->getOperand(1)->getType();
      if(!op0Type->isIntegerTy() || !op1Type->isIntegerTy())
      {
         return;
      }

      // Create VarNodes for comparison operands explicitly
      addVarNode(ici->getOperand(0), nullptr, DL);
      addVarNode(ici->getOperand(1), nullptr, DL);

      // Gets the successors of the current basic block.
      const BasicBlock* TBlock = br->getSuccessor(0);
      const BasicBlock* FBlock = br->getSuccessor(1);

      // We have a Variable-Constant comparison.
      const Value* Op0 = ici->getOperand(0);
      const Value* Op1 = ici->getOperand(1);
      const ConstantInt* constant = nullptr;
      const Value* variable = nullptr;

      // If both operands are constants, nothing to do here
      if(isa<ConstantInt>(Op0) and isa<ConstantInt>(Op1))
      {
         return;
      }

      // Then there are two cases: variable being compared to a constant,
      // or variable being compared to another variable

      // Op0 is constant, Op1 is variable
      if((constant = dyn_cast<ConstantInt>(Op0)) != nullptr)
      {
         variable = Op1;
         // Op0 is variable, Op1 is constant
      }
      else if((constant = dyn_cast<ConstantInt>(Op1)) != nullptr)
      {
         variable = Op0;
      }
      // Both are variables
      // which means constant == 0 and variable == 0

      if(constant != nullptr)
      {
         // Calculate the range of values that would satisfy the comparison.
         ConstantRange CR(constant->getValue(), constant->getValue() + 1);
         CmpInst::Predicate pred = ici->getPredicate();
         CmpInst::Predicate swappred = ici->getSwappedPredicate();

         // If the comparison is in the form (var pred const), we use the actual
         // predicate to build the constant range. Otherwise, (const pred var),
         // we use the swapped predicate
         ConstantRange tmpT = (variable == Op0) ? ConstantRange::makeSatisfyingICmpRegion(pred, CR) : ConstantRange::makeSatisfyingICmpRegion(swappred, CR);

         auto bw = variable->getType()->getPrimitiveSizeInBits();
         Range TValues = tmpT.isFullSet() ? Range(Regular, bw) : CR2R(tmpT, bw);
         Range FValues = tmpT.isFullSet() ? Range(Empty, bw) : Range(Range_base::getAnti(TValues));

         // Create the interval using the intersection in the branch.
         std::shared_ptr<BasicInterval> BT = std::make_shared<BasicInterval>(TValues);
         std::shared_ptr<BasicInterval> BF = std::make_shared<BasicInterval>(FValues);

         ValueBranchMap VBM(variable, TBlock, FBlock, BT, BF);
         valuesBranchMap.insert(std::make_pair(variable, VBM));

         // Do the same for the operand of variable (if variable is a cast
         // instruction)
         const CastInst* castinst = nullptr;
         if((castinst = dyn_cast<CastInst>(variable)) != nullptr)
         {
            const Value* variable_0 = castinst->getOperand(0);

            std::shared_ptr<BasicInterval> BT = std::make_shared<BasicInterval>(TValues);
            std::shared_ptr<BasicInterval> BF = std::make_shared<BasicInterval>(FValues);

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
         CmpInst::Predicate swapPred = ici->getSwappedPredicate();
         CmpInst::Predicate invSwapPred = CmpInst::getInversePredicate(swapPred);
         assert(Op0->getType()->getPrimitiveSizeInBits() == Op1->getType()->getPrimitiveSizeInBits());
         Range CR(Unknown, Op0->getType()->getPrimitiveSizeInBits());

         // Symbolic intervals for op0
         std::shared_ptr<BasicInterval> STOp0 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, Op1, pred));
         std::shared_ptr<BasicInterval> SFOp0 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, Op1, invPred));

         ValueBranchMap VBMOp0(Op0, TBlock, FBlock, STOp0, SFOp0);
         valuesBranchMap.insert(std::make_pair(Op0, VBMOp0));

         // Symbolic intervals for operand of op0 (if op0 is a cast instruction)
         const CastInst* castinst = nullptr;
         if((castinst = dyn_cast<CastInst>(Op0)) != nullptr)
         {
            const Value* Op0_0 = castinst->getOperand(0);

            std::shared_ptr<BasicInterval> STOp0_0 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, Op1, pred));
            std::shared_ptr<BasicInterval> SFOp0_0 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, Op1, invPred));

            ValueBranchMap VBMOp0_0(Op0_0, TBlock, FBlock, STOp0_0, SFOp0_0);
            valuesBranchMap.insert(std::make_pair(Op0_0, VBMOp0_0));
         }

         // Symbolic intervals for op1
         std::shared_ptr<BasicInterval> STOp1 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, Op0, swapPred));
         std::shared_ptr<BasicInterval> SFOp1 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, Op0, invSwapPred));
         ValueBranchMap VBMOp1(Op1, TBlock, FBlock, STOp1, SFOp1);
         valuesBranchMap.insert(std::make_pair(Op1, VBMOp1));

         // Symbolic intervals for operand of op1 (if op1 is a cast instruction)
         castinst = nullptr;
         if((castinst = dyn_cast<CastInst>(Op1)) != nullptr)
         {
            const Value* Op1_1 = castinst->getOperand(0);

            std::shared_ptr<BasicInterval> STOp1_1 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, Op0, swapPred));
            std::shared_ptr<BasicInterval> SFOp1_1 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, Op0, invSwapPred));

            ValueBranchMap VBMOp1_1(Op1_1, TBlock, FBlock, STOp1_1, SFOp1_1);
            valuesBranchMap.insert(std::make_pair(Op1_1, VBMOp1_1));
         }
      }
   }

   void ConstraintGraph::buildValueMaps(const Function& F, const llvm::DataLayout* DL)
   {
      for(const BasicBlock& BB : F)
      {
         const auto* ti = BB.getTerminator();
         const auto* br = dyn_cast<BranchInst>(ti);
         const auto* sw = dyn_cast<SwitchInst>(ti);

         if(br != nullptr)
         {
            buildValueBranchMap(br, DL);
         }
         else if(sw != nullptr)
         {
            buildValueSwitchMap(sw, DL);
         }
      }
   }

   /*
    * Used to insert constant in the right position
    */
   void ConstraintGraph::insertConstantIntoVector(APInt constantval)
   {
      if(constantval.getBitWidth() < MAX_BIT_INT)
      {
         constantval = constantval.sext(MAX_BIT_INT);
      }
      constantvector.push_back(constantval);
   }

   /*
    * Get the first constant from vector greater than val
    */
   APInt getFirstGreaterFromVector(const SmallVector<APInt, 2>& constantvector, const APInt& val)
   {
      for(const APInt& vapint : constantvector)
      {
         if(vapint.sge(val))
         {
            return vapint;
         }
      }
      return Max;
   }

   /*
    * Get the first constant from vector less than val
    */
   APInt getFirstLessFromVector(const SmallVector<APInt, 2>& constantvector, const APInt& val)
   {
      for(auto vit = constantvector.rbegin(), vend = constantvector.rend(); vit != vend; ++vit)
      {
         const APInt& vapint = *vit;
         if(vapint.sle(val))
         {
            return vapint;
         }
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
   void ConstraintGraph::buildConstantVector(const llvm::SmallPtrSet<VarNode*, 32>& component, const UseMap& compusemap)
   {
      // Remove all elements from the vector
      constantvector.clear();

      // Get constants inside component (TODO: may not be necessary, since
      // components with more than 1 node may
      // never have a constant inside them)
      for(VarNode* varNode : component)
      {
         const auto V = varNode->getValue();
         const ConstantInt* ci = nullptr;

         if((ci = dyn_cast<ConstantInt>(V.first)) != nullptr)
         {
            insertConstantIntoVector(ci->getValue());
         }
      }

      // Get constants that are sources of operations whose sink belong to the
      // component
      for(VarNode* varNode : component)
      {
         const auto V = varNode->getValue();
         auto dfit = defMap.find(V);
         if(dfit == defMap.end())
         {
            continue;
         }

         // Handle BinaryOp case
         const BinaryOp* bop = dyn_cast<BinaryOp>(dfit->second);
         const PhiOp* pop = dyn_cast<PhiOp>(dfit->second);
         if(bop != nullptr)
         {
            const VarNode* source1 = bop->getSource1();
            const auto sourceval1 = source1->getValue();
            const VarNode* source2 = bop->getSource2();
            const auto sourceval2 = source2->getValue();

            const ConstantInt *const1, *const2;
            auto opcode = bop->getOpcode();

            if((const1 = dyn_cast<ConstantInt>(sourceval1.first)) != nullptr)
            {
               const auto& cnstVal = const1->getValue();
               if(opcode == llvm::Instruction::ICmp)
               {
                  auto pred = cast<llvm::CmpInst>(bop->getSink()->getValue().first)->getPredicate();
                  if(pred == llvm::ICmpInst::ICMP_EQ || pred == llvm::ICmpInst::ICMP_NE)
                  {
                     insertConstantIntoVector(cnstVal);
                     insertConstantIntoVector(cnstVal - 1);
                     insertConstantIntoVector(cnstVal + 1);
                  }
                  else if(pred == llvm::ICmpInst::ICMP_SGT || pred == llvm::ICmpInst::ICMP_SLE)
                  {
                     insertConstantIntoVector(cnstVal);
                     insertConstantIntoVector(cnstVal + 1);
                  }
                  else if(pred == llvm::ICmpInst::ICMP_SGE || pred == llvm::ICmpInst::ICMP_SLT)
                  {
                     insertConstantIntoVector(cnstVal);
                     insertConstantIntoVector(cnstVal - 1);
                  }
                  else if(pred == llvm::ICmpInst::ICMP_UGT || pred == llvm::ICmpInst::ICMP_ULE)
                  {
                     auto bw = source1->getBitWidth();
                     auto cnstValU = const1->getValue().zextOrTrunc(bw).zext(MAX_BIT_INT);
                     insertConstantIntoVector(cnstValU);
                     insertConstantIntoVector(cnstValU + 1);
                  }
                  else if(pred == llvm::ICmpInst::ICMP_UGE || pred == llvm::ICmpInst::ICMP_ULT)
                  {
                     auto bw = source1->getBitWidth();
                     auto cnstValU = const1->getValue().zextOrTrunc(bw).zext(MAX_BIT_INT);
                     insertConstantIntoVector(cnstValU);
                     insertConstantIntoVector(cnstValU - 1);
                  }
                  else
                  {
                     llvm_unreachable("unexpected condition");
                  }
               }
               else
               {
                  insertConstantIntoVector(cnstVal);
               }
            }
            if((const2 = dyn_cast<ConstantInt>(sourceval2.first)) != nullptr)
            {
               const auto& cnstVal = const2->getValue();
               if(opcode == llvm::Instruction::ICmp)
               {
                  auto pred = cast<llvm::CmpInst>(bop->getSink()->getValue().first)->getPredicate();
                  if(pred == llvm::ICmpInst::ICMP_EQ || pred == llvm::ICmpInst::ICMP_NE)
                  {
                     insertConstantIntoVector(cnstVal);
                     insertConstantIntoVector(cnstVal - 1);
                     insertConstantIntoVector(cnstVal + 1);
                  }
                  else if(pred == llvm::ICmpInst::ICMP_SGT || pred == llvm::ICmpInst::ICMP_SLE)
                  {
                     insertConstantIntoVector(cnstVal);
                     insertConstantIntoVector(cnstVal + 1);
                  }
                  else if(pred == llvm::ICmpInst::ICMP_SGE || pred == llvm::ICmpInst::ICMP_SLT)
                  {
                     insertConstantIntoVector(cnstVal);
                     insertConstantIntoVector(cnstVal - 1);
                  }
                  else if(pred == llvm::ICmpInst::ICMP_UGT || pred == llvm::ICmpInst::ICMP_ULE)
                  {
                     auto bw = source2->getBitWidth();
                     auto cnstValU = const2->getValue().zextOrTrunc(bw).zext(MAX_BIT_INT);
                     insertConstantIntoVector(cnstValU);
                     insertConstantIntoVector(cnstValU + 1);
                  }
                  else if(pred == llvm::ICmpInst::ICMP_UGE || pred == llvm::ICmpInst::ICMP_ULT)
                  {
                     auto bw = source2->getBitWidth();
                     auto cnstValU = const2->getValue().zextOrTrunc(bw).zext(MAX_BIT_INT);
                     insertConstantIntoVector(cnstValU);
                     insertConstantIntoVector(cnstValU - 1);
                  }
                  else
                  {
                     llvm_unreachable("unexpected condition");
                  }
               }
               else
               {
                  insertConstantIntoVector(cnstVal);
               }
            }
         }
         // Handle PhiOp case
         else if(pop != nullptr)
         {
            for(unsigned i = 0, e = pop->getNumSources(); i < e; ++i)
            {
               const VarNode* source = pop->getSource(i);
               const auto sourceval = source->getValue();
               const ConstantInt* consti;
               if((consti = dyn_cast<ConstantInt>(sourceval.first)) != nullptr)
               {
                  insertConstantIntoVector(consti->getValue());
               }
            }
         }
      }

      // Get constants used in intersections
      for(auto& pair : compusemap)
      {
         for(BasicOp* op : pair.second)
         {
            const SigmaOp* sigma = dyn_cast<SigmaOp>(op);
            // Symbolic intervals are discarded, as they don't have fixed values yet
            if(sigma == nullptr || isa<SymbInterval>(sigma->getIntersect().get()))
            {
               continue;
            }
            Range rintersect = op->getIntersect()->getRange();
            if(rintersect.isAnti())
            {
               auto anti = Range(Range_base::getAnti(rintersect));
               const APInt lb = anti.getLower();
               const APInt ub = anti.getUpper();
               if(lb.ne(Min) && lb.ne(Max))
               {
                  insertConstantIntoVector(lb - 1);
                  insertConstantIntoVector(lb);
               }
               if(ub.ne(Min) && ub.ne(Max))
               {
                  insertConstantIntoVector(ub);
                  insertConstantIntoVector(ub + 1);
               }
            }
            else
            {
               const APInt& lb = rintersect.getLower();
               const APInt& ub = rintersect.getUpper();
               if(lb.ne(Min) && lb.ne(Max))
               {
                  insertConstantIntoVector(lb - 1);
                  insertConstantIntoVector(lb);
               }
               if(ub.ne(Min) && ub.ne(Max))
               {
                  insertConstantIntoVector(ub);
                  insertConstantIntoVector(ub + 1);
               }
            }
         }
      }

      // Sort vector in ascending order and remove duplicates
      std::sort(constantvector.begin(), constantvector.end(), [](const APInt& i1, const APInt& i2) { return i1.slt(i2); });

      // std::unique doesn't remove duplicate elements, only
      // move them to the end
      // This is why erase is necessary. To remove these duplicates
      // that will be now at the end.
      auto last = std::unique(constantvector.begin(), constantvector.end());

      constantvector.erase(last, constantvector.end());
   }

   /// Iterates through all instructions in the function and builds the graph.
   void ConstraintGraph::buildGraph(const Function& F, ModulePass* modulePass, const llvm::DataLayout* DL, Andersen_AA* PtoSets_AA, bool arePointersResolved, llvm::DenseMap<const Function*, SmallPtrSet<const Instruction*, 6>>& Function2Store)
   {
      this->func = &F;
      buildValueMaps(F, DL);
      for(auto& I : instructions(F))
      {
         // Only integers are dealt with
         if((!dyn_cast<llvm::StoreInst>(&I) && !I.getType()->isIntegerTy()) || (dyn_cast<llvm::StoreInst>(&I) && !dyn_cast<llvm::StoreInst>(&I)->getValueOperand()->getType()->isIntegerTy()))
         {
            continue;
         }
         if(!isValidInstruction(&I))
         {
            continue;
         }
         buildOperations(&I, modulePass, DL, PtoSets_AA, arePointersResolved, Function2Store);
      }
   }

   void ConstraintGraph::buildVarNodes(const llvm::DataLayout* DL)
   {
      // Initializes the nodes and the use map structure.
      for(auto& pair : vars)
      {
         pair.second->init(this->defMap.count(pair.first) == 0u, DL);
      }
   }

   // FIXME: do it just for component
   void CropDFS::storeAbstractStates(const llvm::SmallPtrSet<VarNode*, 32>& component)
   {
      for(auto varNode : component)
      {
         varNode->storeAbstractState();
      }
   }

   bool Meet::fixed(BasicOp* op)
   {
      const auto oldInterval = op->getSink()->getRange();
      Range newInterval = op->eval();

      op->getSink()->setRange(newInterval);
#ifdef LOG_TRANSACTIONS
      if(op->getInstruction())
      {
         llvm::ModuleSlotTracker MST(op->getInstruction()->getFunction()->getParent());
         MST.incorporateFunction(*op->getInstruction()->getFunction());
         auto instID = MST.getLocalSlot(op->getInstruction());
         LOG_TRANSACTION("FIXED::%" << instID << ": " << oldInterval << " -> " << newInterval);
      }
      else
      {
         LOG_TRANSACTION("FIXED::%"
                         << "artificial phi "
                         << ": " << oldInterval << " -> " << newInterval);
      }
#endif
      return oldInterval != newInterval;
   }

   /// This is the meet operator of the growth analysis. The growth analysis
   /// will change the bounds of each variable, if necessary. Initially, each
   /// variable is bound to either the undefined interval, e.g. [., .], or to
   /// a constant interval, e.g., [3, 15]. After this analysis runs, there will
   /// be no undefined interval. Each variable will be either bound to a
   /// constant interval, or to [-, c], or to [c, +], or to [-, +].
   bool Meet::widen(BasicOp* op, const SmallVector<APInt, 2>* constantvector)
   {
      assert(constantvector != nullptr && "Invalid pointer to constant vector");
      const auto oldInterval = op->getSink()->getRange();
      Range newInterval = op->eval();

      unsigned bw = oldInterval.getBitWidth();
      if(oldInterval.isUnknown() || oldInterval.isEmpty() || oldInterval.isAnti() || newInterval.isEmpty() || newInterval.isAnti())
      {
         if(oldInterval.isAnti() && newInterval.isAnti() && newInterval != oldInterval)
         {
            auto oldAnti = Range::getAnti(oldInterval);
            auto newAnti = Range::getAnti(newInterval);
            const APInt oldLower = oldAnti.getLower();
            const APInt oldUpper = oldAnti.getUpper();
            const APInt newLower = newAnti.getLower();
            const APInt newUpper = newAnti.getUpper();
            APInt nlconstant = getFirstGreaterFromVector(*constantvector, newLower);
            APInt nuconstant = getFirstLessFromVector(*constantvector, newUpper);

            if(newLower.sgt(oldLower) && newUpper.slt(oldUpper))
            {
               op->getSink()->setRange(Range(Anti, bw, nlconstant, nuconstant));
            }
            else
            {
               if(newLower.sgt(oldLower))
               {
                  op->getSink()->setRange(Range(Anti, bw, nlconstant, oldUpper));
               }
               else if(newUpper.slt(oldUpper))
               {
                  op->getSink()->setRange(Range(Anti, bw, oldLower, nuconstant));
               }
            }
         }
         else
         {
            op->getSink()->setRange(newInterval);
         }
      }
      else
      {
         const APInt& oldLower = oldInterval.getLower();
         const APInt& oldUpper = oldInterval.getUpper();
         const APInt& newLower = newInterval.getLower();
         const APInt& newUpper = newInterval.getUpper();

         // Jump-set
         APInt nlconstant = getFirstLessFromVector(*constantvector, newLower);
         APInt nuconstant = getFirstGreaterFromVector(*constantvector, newUpper);
         if(newLower.slt(oldLower) && newUpper.sgt(oldUpper))
         {
            op->getSink()->setRange(Range(Regular, bw, nlconstant, nuconstant));
         }
         else
         {
            if(newLower.slt(oldLower))
            {
               op->getSink()->setRange(Range(Regular, bw, nlconstant, oldUpper));
            }
            else if(newUpper.sgt(oldUpper))
            {
               op->getSink()->setRange(Range(Regular, bw, oldLower, nuconstant));
            }
         }
      }
      const auto& sinkInterval = op->getSink()->getRange();
#ifdef LOG_TRANSACTIONS
      if(op->getInstruction())
      {
         llvm::ModuleSlotTracker MST(op->getInstruction()->getFunction()->getParent());
         MST.incorporateFunction(*op->getInstruction()->getFunction());
         auto instID = MST.getLocalSlot(op->getInstruction());
         LOG_TRANSACTION("WIDEN::%" << instID << ": " << oldInterval << " -> " << newInterval << " -> " << sinkInterval);
         llvm::errs() << "WIDEN::%" << instID << ": " << oldInterval << " -> " << newInterval << " -> " << sinkInterval << "\n";
      }
      else
      {
         LOG_TRANSACTION("WIDEN::%"
                         << "artificial phi "
                         << ": " << oldInterval << " -> " << newInterval << " -> " << sinkInterval);
         llvm::errs() << "WIDEN::%"
                      << "artificial phi "
                      << ": " << oldInterval << " -> " << newInterval << " -> " << sinkInterval << "\n";
      }

#endif
      return oldInterval != sinkInterval;
   }

   bool Meet::growth(BasicOp* op, const SmallVector<APInt, 2>* /*constantvector*/)
   {
      const auto oldInterval = op->getSink()->getRange();
      Range newInterval = op->eval();
      if(oldInterval.isUnknown() || oldInterval.isEmpty() || oldInterval.isAnti() || newInterval.isEmpty() || newInterval.isAnti())
      {
         op->getSink()->setRange(newInterval);
      }
      else
      {
         unsigned bw = oldInterval.getBitWidth();
         const APInt& oldLower = oldInterval.getLower();
         const APInt& oldUpper = oldInterval.getUpper();
         const APInt& newLower = newInterval.getLower();
         const APInt& newUpper = newInterval.getUpper();
         if(newLower.slt(oldLower))
         {
            if(newUpper.sgt(oldUpper))
            {
               op->getSink()->setRange(Range(Regular, bw));
            }
            else
            {
               op->getSink()->setRange(Range(Regular, bw, Min, oldUpper));
            }
         }
         else if(newUpper.sgt(oldUpper))
         {
            op->getSink()->setRange(Range(Regular, bw, oldLower, Max));
         }
      }
      const auto& sinkInterval = op->getSink()->getRange();
#ifdef LOG_TRANSACTIONS
      if(op->getInstruction())
      {
         llvm::ModuleSlotTracker MST(op->getInstruction()->getFunction()->getParent());
         MST.incorporateFunction(*op->getInstruction()->getFunction());
         auto instID = MST.getLocalSlot(op->getInstruction());
         LOG_TRANSACTION("GROWTH::%" << instID << ": " << oldInterval << " -> " << sinkInterval);
      }
      else
      {
         LOG_TRANSACTION("GROWTH::%"
                         << "artificial phi "
                         << ": " << oldInterval << " -> " << sinkInterval);
         llvm::errs() << "GROWTH::%"
                      << "artificial phi "
                      << ": " << oldInterval << " -> " << sinkInterval << "\n";
      }
#endif
      return oldInterval != sinkInterval;
   }

   /// This is the meet operator of the cropping analysis. Whereas the growth
   /// analysis expands the bounds of each variable, regardless of intersections
   /// in the constraint graph, the cropping analysis shrinks these bounds back
   /// to ranges that respect the intersections.
   bool Meet::narrow(BasicOp* op, const SmallVector<APInt, 2>* constantvector)
   {
      const auto oldInterval = op->getSink()->getRange();
      Range newInterval = op->eval();
      unsigned bw = oldInterval.getBitWidth();

      if(oldInterval.isAnti() || newInterval.isAnti() || oldInterval.isEmpty() || newInterval.isEmpty())
      {
         if(oldInterval.isAnti() && newInterval.isAnti() && newInterval != oldInterval)
         {
            auto oldAnti = Range::getAnti(oldInterval);
            auto newAnti = Range::getAnti(newInterval);
            const APInt& oLower = oldAnti.getLower();
            const APInt& oUpper = oldAnti.getUpper();
            const APInt& nLower = newAnti.getLower();
            const APInt& nUpper = newAnti.getUpper();
            APInt nlconstant = getFirstGreaterFromVector(*constantvector, nLower);
            APInt nuconstant = getFirstLessFromVector(*constantvector, nUpper);
            assert(oLower.ne(Min));
            const APInt& smin = APIntOps::smax(oLower, nlconstant);
            if(oLower.ne(smin))
            {
               op->getSink()->setRange(Range(Anti, bw, smin, oUpper));
            }
            assert(oUpper.ne(Max));
            const APInt& smax = APIntOps::smin(oUpper, nuconstant);
            if(oUpper.ne(smax))
            {
               auto sinkRange = op->getSink()->getRange();
               if(sinkRange.isAnti())
               {
                  auto sinkAnti = Range::getAnti(sinkRange);
                  op->getSink()->setRange(Range(Anti, bw, sinkAnti.getLower(), smax));
               }
               else
               {
                  op->getSink()->setRange(Range(Anti, bw, sinkRange.getLower(), smax));
               }
            }
         }
         else
         {
            op->getSink()->setRange(newInterval);
         }
      }
      else
      {
         const APInt oLower = oldInterval.getLower();
         const APInt oUpper = oldInterval.getUpper();
         const APInt nLower = newInterval.getLower();
         const APInt nUpper = newInterval.getUpper();
         if(oLower.eq(Min) && nLower.ne(Min))
         {
            op->getSink()->setRange(Range(Regular, bw, nLower, oUpper));
         }
         else
         {
            const APInt& smin = APIntOps::smin(oLower, nLower);
            if(oLower.ne(smin))
            {
               op->getSink()->setRange(Range(Regular, bw, smin, oUpper));
            }
         }
         if(!op->getSink()->getRange().isAnti())
         {
            if(oUpper.eq(Max) && nUpper.ne(Max))
            {
               op->getSink()->setRange(Range(Regular, bw, op->getSink()->getRange().getLower(), nUpper));
            }
            else
            {
               const APInt& smax = APIntOps::smax(oUpper, nUpper);
               if(oUpper.ne(smax))
               {
                  op->getSink()->setRange(Range(Regular, bw, op->getSink()->getRange().getLower(), smax));
               }
            }
         }
      }
      const auto& sinkInterval = op->getSink()->getRange();
#ifdef LOG_TRANSACTIONS
      if(op->getInstruction())
      {
         llvm::ModuleSlotTracker MST(op->getInstruction()->getFunction()->getParent());
         MST.incorporateFunction(*op->getInstruction()->getFunction());
         auto instID = MST.getLocalSlot(op->getInstruction());
         LOG_TRANSACTION("NARROW::%" << instID << ": " << oldInterval << " -> " << sinkInterval);
         llvm::errs() << "NARROW::%" << instID << ": " << oldInterval << " -> " << sinkInterval << "\n";
      }
      else
      {
         LOG_TRANSACTION("NARROW::%"
                         << "artificial phi "
                         << ": " << oldInterval << " -> " << sinkInterval);
         llvm::errs() << "NARROW::%"
                      << "artificial phi "
                      << ": " << oldInterval << " -> " << sinkInterval << "\n";
      }
#endif
      return oldInterval != sinkInterval;
   }

   bool Meet::crop(BasicOp* op, const SmallVector<APInt, 2>* /*constantvector*/)
   {
      const auto oldInterval = op->getSink()->getRange();
      Range newInterval = op->eval();

      if(oldInterval.isAnti() || newInterval.isAnti() || oldInterval.isEmpty() || newInterval.isEmpty())
      {
         op->getSink()->setRange(newInterval);
      }
      else
      {
         unsigned bw = oldInterval.getBitWidth();
         char abstractState = op->getSink()->getAbstractState();
         if((abstractState == '-' || abstractState == '?') && newInterval.getLower().sgt(oldInterval.getLower()))
         {
            op->getSink()->setRange(Range(Regular, bw, newInterval.getLower(), oldInterval.getUpper()));
         }

         if((abstractState == '+' || abstractState == '?') && newInterval.getUpper().slt(oldInterval.getUpper()))
         {
            op->getSink()->setRange(Range(Regular, bw, oldInterval.getLower(), newInterval.getUpper()));
         }
      }
      const auto& sinkInterval = op->getSink()->getRange();
#ifdef LOG_TRANSACTIONS
      if(op->getInstruction())
      {
         llvm::ModuleSlotTracker MST(op->getInstruction()->getFunction()->getParent());
         MST.incorporateFunction(*op->getInstruction()->getFunction());
         auto instID = MST.getLocalSlot(op->getInstruction());
         LOG_TRANSACTION("CROP::%" << instID << ": " << oldInterval << " -> " << sinkInterval);
      }
      else
      {
         LOG_TRANSACTION("CROP::%"
                         << "artificial phi "
                         << ": " << oldInterval << " -> " << sinkInterval);
         llvm::errs() << "CROP::%"
                      << "artificial phi "
                      << ": " << oldInterval << " -> " << sinkInterval << "\n";
      }
#endif
      return oldInterval != sinkInterval;
   }

   void Cousot::preUpdate(const UseMap& compUseMap, DenseSet<eValue>& entryPoints)
   {
      update(compUseMap, entryPoints, Meet::widen);
   }

   void Cousot::posUpdate(const UseMap& compUseMap, DenseSet<eValue>& entryPoints, const llvm::SmallPtrSet<VarNode*, 32>* /*component*/)
   {
      update(compUseMap, entryPoints, Meet::narrow);
   }

   void CropDFS::preUpdate(const UseMap& compUseMap, DenseSet<eValue>& entryPoints)
   {
      update(compUseMap, entryPoints, Meet::growth);
   }

   void CropDFS::posUpdate(const UseMap& compUseMap, DenseSet<eValue>&, const llvm::SmallPtrSet<VarNode*, 32>* component)
   {
      storeAbstractStates(*component);
      auto obgn = oprs.begin(), oend = oprs.end();
      for(; obgn != oend; ++obgn)
      {
         BasicOp* op = *obgn;
         if(component->count(op->getSink()) != 0u)
         {
            crop(compUseMap, op);
         }
      }
   }

   void CropDFS::crop(const UseMap& compUseMap, BasicOp* op)
   {
      SmallPtrSet<BasicOp*, 8> activeOps;
      SmallPtrSet<const VarNode*, 8> visitedOps;

      // init the activeOps only with the op received
      activeOps.insert(op);

      while(!activeOps.empty())
      {
         BasicOp* V = *activeOps.begin();
         activeOps.erase(V);
         const VarNode* sink = V->getSink();

         // if the sink has been visited go to the next activeOps
         if(visitedOps.count(sink) != 0u)
         {
            continue;
         }

         Meet::crop(V, nullptr);
         visitedOps.insert(sink);

         // The use list.of sink
         const auto& L = compUseMap.find(sink->getValue())->second;
         for(BasicOp* op : L)
         {
            activeOps.insert(op);
         }
      }
   }

   void ConstraintGraph::update(const UseMap& compUseMap, DenseSet<eValue>& actv, bool (*meet)(BasicOp* op, const SmallVector<APInt, 2>* constantvector))
   {
      while(!actv.empty())
      {
         const auto V = *actv.begin();
         actv.erase(V);
         //         llvm::errs() << "-> update: ";
         //         V.first->print(llvm::errs());
         //         llvm::errs() << "\n";

         // The use list.
         const auto& L = compUseMap.find(V)->second;

         for(BasicOp* op : L)
         {
            //            llvm::errs() << "  > ";
            //            op->getSink()->print(llvm::errs());
            //            llvm::errs() <<  "\n";
            if(meet(op, &constantvector))
            {
               // I want to use it as a set, but I also want
               // keep an order or insertions and removals.
               auto val = op->getSink()->getValue();
               actv.insert(val);
            }
         }
      }
   }

   void ConstraintGraph::update(unsigned nIterations, const UseMap& compUseMap, llvm::DenseSet<eValue>& actv)
   {
      std::list<eValue> queue(actv.begin(), actv.end());
      actv.clear();
      while(!queue.empty())
      {
         const auto V = queue.front();
         queue.pop_front();
         // The use list.
         const auto& L = compUseMap.find(V)->second;
         for(auto op : L)
         {
            if(nIterations == 0)
            {
               return;
            }
            --nIterations;
            if(Meet::fixed(op))
            {
               auto next = op->getSink()->getValue();
               if(std::find(queue.begin(), queue.end(), next) == queue.end())
               {
                  queue.push_back(next);
               }
            }
         }
      }
   }

   /// Finds the intervals of the variables in the graph.
   void ConstraintGraph::findIntervals()
   {
      // Builds symbMap
#ifdef STATS
      Timer* timer = prof.registerNewTimer("Nuutila", "Nuutila's algorithm for strongly connected components");
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

      for(auto nit = sccList.begin(), nend = sccList.end(); nit != nend; ++nit)
      {
         auto& component = *sccList.components[*nit];
#ifdef SCC_DEBUG
         --numberOfSCCs;
#endif

#ifdef DEBUG_RA
         llvm::errs() << "Components:\n";
         for(auto var : component)
         {
            llvm::errs() << "  ";
            var->print(llvm::errs());
            llvm::errs() << "\n";
         }
         llvm::errs() << "-----------\n";
#endif
         if(component.size() == 1)
         {
            ++numAloneSCCs;
            VarNode* var = *component.begin();
            fixIntersectsSC(var);
            if(this->defMap.find(var->getValue()) != this->defMap.end())
            {
               BasicOp* op = this->defMap.find(var->getValue())->second;
               var->setRange(op->eval());
            }
            if(var->getRange().isUnknown())
            {
               var->setRange(Range(Regular, var->getBitWidth()));
            }
         }
         else
         {
            if(component.size() > sizeMaxSCC)
            {
               sizeMaxSCC = component.size();
            }

            UseMap compUseMap = buildUseMap(component);

            // Get the entry points of the SCC
            DenseSet<eValue> entryPoints;

#ifdef JUMPSET
            // Create vector of constants inside component
            // Comment this line below to deactivate jump-set
            buildConstantVector(component, compUseMap);
#endif
#ifdef DEBUG_RA
            for(auto cnst : constantvector)
               llvm::errs() << " " << cnst;
            if(!constantvector.empty())
               llvm::errs() << "\n";
#endif
            generateEntryPoints(component, entryPoints);
            // iterate a fixed number of time before widening
            update(component.size() * 16, compUseMap, entryPoints);
#ifdef DEBUG_RA
            if(func != nullptr)
            {
               printToFile(*func, "/tmp/" + func->getName().str() + "cgfixed.dot");
            }
#endif

            generateEntryPoints(component, entryPoints);
            //            llvm::errs() << "entryPoints:\n";
            //            for(auto el : entryPoints)
            //            {
            //               llvm::errs() << " ->";
            //               el.first->print(llvm::errs());
            //               llvm::errs() << "\n";
            //            }
            // First iterate till fix point
            preUpdate(compUseMap, entryPoints);
            //            llvm::errs() << "fixIntersects\n";
            fixIntersects(component);
            //            llvm::errs() << "--\n";

#ifdef DEBUG_RA
            if(func != nullptr)
            {
               printToFile(*func, "/tmp/" + func->getName().str() + "cgfixintersect.dot");
            }
#endif

            for(VarNode* varNode : component)
            {
               if(varNode->getRange().isUnknown())
               {
                  llvm::errs() << "initialize unknown: ";
                  varNode->getValue().first->print(llvm::errs());
                  llvm::errs() << "\n";
                  // llvm_unreachable("unexpected condition");
                  varNode->setRange(Range(Regular, varNode->getBitWidth(), Min, Max));
               }
            }

#ifdef DEBUG_RA
            if(func != nullptr)
            {
               printToFile(*func, "/tmp/" + func->getName().str() + "cgint.dot");
            }
#endif

            // Second iterate till fix point
            DenseSet<eValue> activeVars;
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

   void ConstraintGraph::generateEntryPoints(const llvm::SmallPtrSet<VarNode*, 32>& component, DenseSet<eValue>& entryPoints)
   {
      // Iterate over the varnodes in the component
      for(VarNode* varNode : component)
      {
         const auto V = varNode->getValue();

         if(dyn_cast<PHINode>(V.first) && dyn_cast<PHINode>(V.first)->getNumOperands() == 1)
         {
            auto dit = this->defMap.find(V);
            if(dit != this->defMap.end())
            {
               BasicOp* bop = dit->second;
               auto* defop = dyn_cast<SigmaOp>(bop);

               if((defop != nullptr) && defop->isUnresolved())
               {
                  defop->getSink()->setRange(bop->eval());
                  defop->markResolved();
               }
            }
         }
         if(!varNode->getRange().isUnknown())
         {
            entryPoints.insert(V);
         }
      }
   }

   void ConstraintGraph::fixIntersects(const llvm::SmallPtrSet<VarNode*, 32>& component)
   {
      // Iterate again over the varnodes in the component
      for(VarNode* varNode : component)
      {
         fixIntersectsSC(varNode);
      }
   }

   void ConstraintGraph::fixIntersectsSC(VarNode* varNode)
   {
      const auto V = varNode->getValue();
      auto sit = symbMap.find(V);
      if(sit != symbMap.end())
      {
         //         llvm::errs() << "fix intersects:\n";
         //         varNode->print(llvm::errs());
         //         llvm::errs() << "\n";
         for(BasicOp* op : sit->second)
         {
            //            llvm::errs() << "op intersects:\n";
            //            op->print(llvm::errs());
            //            llvm::errs() << "\n";
            op->fixIntersects(varNode);
            //            llvm::errs() << "sink:";
            //            op->getSink()->print(llvm::errs());
            //            llvm::errs() << "\n";
         }
      }
   }

   void ConstraintGraph::generateActivesVars(const llvm::SmallPtrSet<VarNode*, 32>& component, DenseSet<eValue>& activeVars)
   {
      for(VarNode* varNode : component)
      {
         const auto V = varNode->getValue();
         const auto* CI = dyn_cast<ConstantInt>(V.first);
         if(CI != nullptr)
         {
            continue;
         }
         activeVars.insert(V);
      }
   }

   /// Prints the content of the graph in dot format. For more information
   /// about the dot format, see: http://www.graphviz.org/pdf/dotguide.pdf
   void ConstraintGraph::print(const Function& F, raw_ostream& OS) const
   {
      const char* quot = R"(")";
      // Print the header of the .dot file.
      OS << "digraph dotgraph {\n";
      OS << R"(label="Constraint Graph for ')";
      OS << F.getName() << "\' function\";\n";
      OS << "node [shape=record,fontname=\"Times-Roman\",fontsize=14];\n";

      // Print the body of the .dot file.
      for(auto& pair : vars)
      {
         if(const auto* C = dyn_cast<ConstantInt>(pair.first.first))
         {
            OS << " " << C->getValue();
         }
         else
         {
            OS << quot;
            if(pair.first.second)
            {
               printVarName(pair.first.second, OS);
               OS << "." << pair.first.first;
            }
            else
            {
               printVarName(pair.first.first, OS);
            }
            OS << quot;
         }
         OS << R"( [label=")" << pair.second << "\"]\n";
      }

      for(BasicOp* op : oprs)
      {
         op->print(OS);
         OS << '\n';
      }
      OS << pseudoEdgesString.str();
      // Print the footer of the .dot file.
      OS << "}\n";
   }

   void ConstraintGraph::printToFile(const Function& F, const std::string& FileName)
   {
      std::error_code ErrorInfo;
      raw_fd_ostream file(FileName, ErrorInfo, sys::fs::F_Text);

      if(!file.has_error())
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

   void ConstraintGraph::computeStats()
   {
      for(const auto& pair : vars)
      {
         // We only count the instructions that have uses.
         if(pair.first.first->getNumUses() == 0)
         {
            ++numZeroUses;
            // continue;
         }

         // ConstantInts must NOT be counted!!
         if(isa<ConstantInt>(pair.first.first))
         {
            ++numConstants;
            continue;
         }

         // Variables that are not IntegerTy are ignored
         if(!(!pair.first.second && pair.first.first->getType()->isIntegerTy()) && !(pair.first.second && dyn_cast<StoreInst>(pair.first.first) && dyn_cast<StoreInst>(pair.first.first)->getValueOperand()->getType()->isIntegerTy()))
         {
            ++numNotInt;
            continue;
         }

         // Count original (used) bits
         unsigned total = dyn_cast<StoreInst>(pair.first.first) ? dyn_cast<StoreInst>(pair.first.first)->getValueOperand()->getType()->getPrimitiveSizeInBits() : pair.first.first->getType()->getPrimitiveSizeInBits();
         usedBits += total;
         Range CR = pair.second->getRange();

         // If range is unknown, we have total needed bits
         if(CR.isUnknown())
         {
            ++numUnknown;
            needBits += total;
            continue;
         }
         // If range is empty, we have 0 needed bits
         if(CR.isEmpty())
         {
            ++numEmpty;
            continue;
         }
         if(CR.isAnti())
         {
            ++numAnti;
         }
         else
         {
            if(CR.getLower().eq(Min))
            {
               if(CR.getUpper().eq(Max))
               {
                  ++numMaxRange;
               }
               else
               {
                  ++numMinInfC;
               }
            }
            else if(CR.getUpper().eq(Max))
            {
               ++numCPlusInf;
            }
            else
            {
               ++numCC;
            }

            unsigned ub, lb;

            if(CR.getLower().isNegative())
            {
               APInt abs = CR.getLower().abs();
               lb = abs.getActiveBits() + 1;
            }
            else
            {
               lb = CR.getLower().getActiveBits() + 1;
            }

            if(CR.getUpper().isNegative())
            {
               APInt abs = CR.getUpper().abs();
               ub = abs.getActiveBits() + 1;
            }
            else
            {
               ub = CR.getUpper().getActiveBits() + 1;
            }

            unsigned nBits = lb > ub ? lb : ub;

            // If both bounds are positive, decrement needed bits by 1
            if(!CR.getLower().isNegative() && !CR.getUpper().isNegative())
            {
               --nBits;
            }
            if(nBits < total)
            {
               needBits += nBits;
            }
            else
            {
               needBits += total;
            }
         }
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
   UseMap ConstraintGraph::buildUseMap(const llvm::SmallPtrSet<VarNode*, 32>& component)
   {
      UseMap compUseMap;
      for(auto vit = component.begin(), vend = component.end(); vit != vend; ++vit)
      {
         const VarNode* var = *vit;
         const auto V = var->getValue();
         // Get the component's use list for V (it does not exist until we try to get it)
         auto& list = compUseMap[V];
         // Get the use list of the variable in component
         auto p = this->useMap.find(V);
         // For each operation in the list, verify if its sink is in the component
         for(BasicOp* opit : p->second)
         {
            VarNode* sink = opit->getSink();
            // If it is, add op to the component's use map
            if(component.count(sink) != 0)
            {
               list.insert(opit);
            }
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
      symbMap = SymbMap();

      // Iterate over the operations set
      for(BasicOp* op : oprs)
      {
         // If the operation is unary and its interval is symbolic
         auto* uop = dyn_cast<UnaryOp>(op);
         if((uop != nullptr) && isa<SymbInterval>(uop->getIntersect().get()))
         {
            auto* symbi = cast<SymbInterval>(uop->getIntersect().get());
            const auto V = symbi->getBound();
            auto p = symbMap.find(V);
            if(p != symbMap.end())
            {
               p->second.insert(uop);
            }
            else
            {
               SmallPtrSet<BasicOp*, 8> l;
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
   void ConstraintGraph::propagateToNextSCC(const llvm::SmallPtrSet<VarNode*, 32>& component)
   {
      for(auto var : component)
      {
         const auto V = var->getValue();
         auto p = this->useMap.find(V);
         for(BasicOp* op : p->second)
         {
            /// VarNodes belonging to the current SCC must not be evaluated otherwise we break the fixed point previously computed
            if(component.find(op->getSink()) != component.end())
            {
               continue;
            }
            auto* sigmaop = dyn_cast<SigmaOp>(op);
            op->getSink()->setRange(op->eval());
            if((sigmaop != nullptr) && sigmaop->getIntersect()->getRange().isUnknown())
            {
               sigmaop->markUnresolved();
            }
         }
      }
   }

   /*
    *	Adds the edges that ensure that we solve a future before fixing its
    *  interval. I have created a new class: ControlDep edges, to represent
    *  the control dependencies. In this way, in order to delete these edges,
    *  one just need to go over the map of uses removing every instance of the
    *  ControlDep class.
    */
   void Nuutila::addControlDependenceEdges(SymbMap* symbMap, UseMap* useMap, VarNodes* vars)
   {
      for(auto sit = symbMap->begin(), send = symbMap->end(); sit != send; ++sit)
      {
         for(auto opit = sit->second.begin(), opend = sit->second.end(); opit != opend; ++opit)
         {
            auto source_value = vars->find(sit->first);
            VarNode* source = source_value->second;
            BasicOp* cdedge = new ControlDep((*opit)->getSink(), source);
            (*useMap)[sit->first].insert(cdedge);
         }
      }
   }

   /*
    *	Removes the control dependence edges from the constraint graph.
    */
   void Nuutila::delControlDependenceEdges(UseMap* useMap)
   {
      for(auto it = useMap->begin(), end = useMap->end(); it != end; ++it)
      {
         std::deque<ControlDep*> ops;
         for(auto sit : it->second)
         {
            ControlDep* op = nullptr;
            if((op = dyn_cast<ControlDep>(sit)) != nullptr)
            {
               ops.push_back(op);
            }
         }

         for(ControlDep* op : ops)
         {
            // Add pseudo edge to the string
            const auto V = op->getSource()->getValue();
            if(const auto* C = dyn_cast<ConstantInt>(V.first))
            {
               pseudoEdgesString << " " << C->getValue() << " -> ";
            }
            else
            {
               pseudoEdgesString << " " << '"';
               if(V.second)
               {
                  printVarName(V.second, pseudoEdgesString);
               }
               else
               {
                  printVarName(V.first, pseudoEdgesString);
               }
               pseudoEdgesString << '"' << " -> ";
            }
            const auto VS = op->getSink()->getValue();
            pseudoEdgesString << '"';
            if(VS.second)
            {
               printVarName(VS.second, pseudoEdgesString);
            }
            else
            {
               printVarName(VS.first, pseudoEdgesString);
            }
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
   void Nuutila::visit(eValue V, std::stack<eValue>& stack, UseMap* useMap)
   {
      dfs[V] = index;
      ++index;
      root[V] = V;

      // Visit every node defined in an instruction that uses V
      for(auto sit = (*useMap)[V].begin(), send = (*useMap)[V].end(); sit != send; ++sit)
      {
         auto op = *sit;
         auto name = op->getSink()->getValue();
         if(dfs[name] < 0)
         {
            visit(name, stack, useMap);
         }
         if((!static_cast<bool>(inComponent.count(name))) && (dfs[root[V]] >= dfs[root[name]]))
         {
            root[V] = root[name];
         }
      }

      // The second phase of the algorithm assigns components to stacked nodes
      if(root[V] == V)
      {
         // Neither the worklist nor the map of components is part of Nuutila's
         // original algorithm. We are using these data structures to get a
         // topological ordering of the SCCs without having to go over the root
         // list once more.
         worklist.push_back(V);
         auto SCC = new llvm::SmallPtrSet<VarNode*, 32>;
         SCC->insert((*variables)[V]);
         inComponent.insert(V);
         while(!stack.empty() && (dfs[stack.top()] > dfs[V]))
         {
            auto node = stack.top();
            stack.pop();
            inComponent.insert(node);
            SCC->insert((*variables)[node]);
         }
         components[V] = SCC;
      }
      else
      {
         stack.push(V);
      }
   }

   /*
    *	Finds the strongly connected components in the constraint graph formed
    *by
    *	Variables and UseMap. The class receives the map of futures to insert
    *the
    *  control dependence edges in the constraint graph. These edges are removed
    *  after the class is done computing the SCCs.
    */
   Nuutila::Nuutila(VarNodes* varNodes, UseMap* useMap, SymbMap* symbMap)
   {
      // Copy structures
      this->variables = varNodes;
      this->index = 0;

      // Iterate over all varnodes of the constraint graph
      for(auto vit = varNodes->begin(), vend = varNodes->end(); vit != vend; ++vit)
      {
         // Initialize DFS control variable for each Value in the graph
         auto V = vit->first;
         dfs[V] = -1;
      }
      addControlDependenceEdges(symbMap, useMap, varNodes);
      // Iterate again over all varnodes of the constraint graph
      for(auto vit = varNodes->begin(), vend = varNodes->end(); vit != vend; ++vit)
      {
         auto V = vit->first;
         // If the Value has not been visited yet, call visit for him
         if(dfs[V] < 0)
         {
            std::stack<eValue> pilha;
            visit(V, pilha, useMap);
         }
      }
      delControlDependenceEdges(useMap);

#ifdef SCC_DEBUG
      ASSERT(checkWorklist(), "an inconsistency in SCC worklist have been found");
      ASSERT(checkComponents(), "a component has been used more than once");
      ASSERT(checkTopologicalSort(useMap), "topological sort is incorrect");
#endif
   }

   Nuutila::~Nuutila()
   {
      for(auto mit = components.begin(), mend = components.end(); mit != mend; ++mit)
      {
         delete mit->second;
      }
   }

#ifdef SCC_DEBUG
   bool Nuutila::checkWorklist()
   {
      bool consistent = true;
      for(auto nit = this->begin(), nend = this->end(); nit != nend; ++nit)
      {
         auto v = *nit;
         for(auto nit2 = this->begin(), nend2 = this->end(); nit2 != nend2; ++nit2)
         {
            auto v2 = *nit2;
            if(v == v2 && nit != nit2)
            {
               errs() << "[Nuutila::checkWorklist] Duplicated entry in worklist\n";
               v.first->dump();
               consistent = false;
            }
         }
      }
      return consistent;
   }

   bool Nuutila::checkComponents()
   {
      bool isConsistent = true;
      for(auto nit = this->begin(), nend = this->end(); nit != nend; ++nit)
      {
         auto component = this->components[*nit];
         for(auto nit2 = this->begin(), nend2 = this->end(); nit2 != nend2; ++nit2)
         {
            auto component2 = this->components[*nit2];
            if(component == component2 && nit != nit2)
            {
               errs() << "[Nuutila::checkComponent] Component [" << component << ", " << component->size() << "]\n";
               isConsistent = false;
            }
         }
      }
      return isConsistent;
   }

   /**
    * Check if a component has an edge to another component
    */
   bool Nuutila::hasEdge(llvm::SmallPtrSet<VarNode*, 32>* componentFrom, llvm::SmallPtrSet<VarNode*, 32>* componentTo, UseMap* useMap)
   {
      for(auto vit = componentFrom->begin(), vend = componentFrom->end(); vit != vend; ++vit)
      {
         const auto source = (*vit)->getValue();
         for(auto sit = (*useMap)[source].begin(), send = (*useMap)[source].end(); sit != send; ++sit)
         {
            BasicOp* op = *sit;
            if(componentTo->count(op->getSink()) != 0)
               return true;
         }
      }
      return false;
   }

   bool Nuutila::checkTopologicalSort(UseMap* useMap)
   {
      bool isConsistent = true;
      DenseMap<llvm::SmallPtrSet<VarNode*, 32>*, bool> visited;
      for(auto nit = this->begin(), nend = this->end(); nit != nend; ++nit)
      {
         auto component = this->components[*nit];
         visited[component] = false;
      }

      for(auto nit = this->begin(), nend = this->end(); nit != nend; ++nit)
      {
         auto component = this->components[*nit];
         if(!visited[component])
         {
            visited[component] = true;
            // check if this component points to another component that has already
            // been visited
            for(auto nit2 = this->begin(), nend2 = this->end(); nit2 != nend2; ++nit2)
            {
               auto component2 = this->components[*nit2];
               if(nit != nit2 && visited[component2] && hasEdge(component, component2, useMap))
                  isConsistent = false;
            }
         }
         else
            llvm::errs() << "[Nuutila::checkTopologicalSort] Component visited more than once time\n";
      }
      return isConsistent;
   }

#endif

   InterProceduralRACropDFSHelper::~InterProceduralRACropDFSHelper()
   {
#ifdef STATS
      prof.printMemoryUsage();

      std::ostringstream formated;
      formated << 100 * (1.0 - (static_cast<double>(needBits) / usedBits));
      errs() << formated.str() << "\t - "
             << " Percentage of reduction\n";

      // max visit computation
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
      errs() << "Number of variables [c, c]: " << numCC << "\n";
      errs() << "Number of variables )c, c(: " << numAnti << "\n";
      errs() << "Number of variables [-inf, c]: " << numMinInfC << "\n";
      errs() << "Number of variables [-inf, +inf]: " << numMaxRange << "\n";
      errs() << "Number of constants: " << numConstants << "\n";
      errs() << "Number of variables without any use: " << numZeroUses << "\n";
      errs() << "Number of variables that are not Integer: " << numNotInt << "\n";
      errs() << "Number of operations: " << numOps << "\n";
      errs() << "MAX_BIT_INT: " << MAX_BIT_INT << "\n";

#endif
   }

   static bool checkStores(const Module& M, Andersen_AA* PtoSets_AA, llvm::DenseMap<const Function*, SmallPtrSet<const Instruction*, 6>>& Function2Store)
   {
      if(!PtoSets_AA)
      {
         return false;
      }
#if HAVE_LIBBDD
      bool res = true;
      for(auto& F : M.functions())
      {
         if(F.isDeclaration() || F.isVarArg())
         {
            continue;
         }
         for(auto& I : instructions(F))
         {
            if(auto SI = dyn_cast<StoreInst>(&I))
            {
               if(Function2Store.find(&F) == Function2Store.end())
               {
                  SmallPtrSet<const Instruction*, 6> storeList;
                  Function2Store.insert(std::make_pair(&F, storeList));
               }
               Function2Store.find(&F)->second.insert(SI);
               auto PO = SI->getPointerOperand();
               auto PTS = PtoSets_AA->pointsToSet(PO);
               if(PTS->empty())
               {
                  res = false;
                  break;
               }

               for(auto var : *PTS)
               {
                  auto varValue = PtoSets_AA->getValue(var);
                  if(!varValue)
                  {
                     res = false;
                     break;
                  }
                  auto vid = varValue->getValueID();
                  if(vid != llvm::Value::InstructionVal + llvm::Instruction::Alloca && vid != llvm::Value::GlobalVariableVal)
                  {
                     res = false;
                     break;
                  }
                  if(auto GV = dyn_cast<llvm::GlobalVariable>(varValue))
                  {
                     if(!GV->hasInternalLinkage())
                     {
                        res = false;
                        break;
                     }
                  }
               }
               if(!res)
               {
                  break;
               }
            }
            else if(auto CI = dyn_cast<CallInst>(&I))
            {
               if(!CI->getCalledFunction())
               {
                  res = false;
                  break;
               }
               if(!CI->onlyReadsMemory())
               {
                  if(Function2Store.find(&F) == Function2Store.end())
                  {
                     SmallPtrSet<const Instruction*, 6> storeList;
                     Function2Store.insert(std::make_pair(&F, storeList));
                  }
                  Function2Store.find(&F)->second.insert(&I);
               }
            }
            else if(auto II = dyn_cast<llvm::InvokeInst>(&I))
            {
               if(!II->getCalledFunction())
               {
                  res = false;
                  break;
               }
               if(!II->onlyReadsMemory())
               {
                  if(Function2Store.find(&F) == Function2Store.end())
                  {
                     SmallPtrSet<const Instruction*, 6> storeList;
                     Function2Store.insert(std::make_pair(&F, storeList));
                  }
                  Function2Store.find(&F)->second.insert(&I);
               }
            }
         }
         if(!res)
         {
            break;
         }
      }
      return res;
#else
      return false;
#endif
   }

   static bool checkSDS(const Module& M, Andersen_AA* PtoSets_AA)
   {
      llvm::DenseMap<const Value*, unsigned> SDS_map;
      if(!PtoSets_AA)
      {
         return false;
      }
#if HAVE_LIBBDD
      for(auto& F : M.functions())
      {
         if(F.isDeclaration() || F.isVarArg())
         {
            continue;
         }
         for(auto& I : instructions(F))
         {
            auto SI = dyn_cast<StoreInst>(&I);
            auto LI = dyn_cast<LoadInst>(&I);
            if(!(SI || LI))
            {
               continue;
            }
            const Value* PO;
            unsigned bw;
            if(SI)
            {
               PO = SI->getPointerOperand();
               auto writtenObj = SI->getValueOperand();
               bw = writtenObj->getType()->getPrimitiveSizeInBits();
               if(SI->getAlignment() && bw > (8 * SI->getAlignment()))
               {
                  return false;
               }
            }
            else
            {
               assert(LI);
               bw = LI->getType()->getPrimitiveSizeInBits();
               if(LI->getAlignment() && bw > (8 * LI->getAlignment()))
               {
                  return false;
               }
               PO = LI->getPointerOperand();
            }
            auto PTS = PtoSets_AA->pointsToSet(PO);
            if(PTS->empty())
            {
               return false;
            }
            for(auto var : *PTS)
            {
               auto varValue = PtoSets_AA->getValue(var);
               if(!varValue)
               {
                  return false;
               }
               if(llvm::dyn_cast<const llvm::CallInst>(varValue))
               {
                  return false;
               }
#ifndef NDEBUG
               auto vid = varValue->getValueID();
#endif
               assert(vid == llvm::Value::InstructionVal + llvm::Instruction::Alloca || vid == llvm::Value::GlobalVariableVal);
               unsigned int elmtSize = ObjectBW(varValue);
               if(elmtSize == 0)
               {
                  return false;
               }
               if(bw != elmtSize)
               {
                  return false;
               }
               if(SDS_map.find(varValue) == SDS_map.end())
               {
                  SDS_map.insert(std::make_pair(varValue, elmtSize));
               }
               else if(SDS_map.find(varValue)->second != elmtSize)
               {
                  return false;
               }
            }
         }
      }
      return true;
#else
      return false;
#endif
   }

   bool InterProceduralRACropDFSHelper::runOnModule(const Module& M, ModulePass* modulePass, Andersen_AA* PtoSets_AA)
   {
      // Constraint Graph
      // CG = new CropDFS();
      CG = new Cousot();

      MAX_BIT_INT = getMaxBitWidth(M);
      POINTER_BITSIZE = M.getDataLayout().getTypeAllocSizeInBits(llvm::Type::getInt32PtrTy(M.getContext()));
      updateConstantIntegers(MAX_BIT_INT);
      llvm::DenseMap<const Function*, SmallPtrSet<const Instruction*, 6>> Function2Store;
      auto arePointersResolved = checkStores(M, PtoSets_AA, Function2Store) && checkSDS(M, PtoSets_AA);
      if(arePointersResolved)
      {
#ifdef DEBUG_RA
         llvm::errs() << "Pointers are Resolved\n";
         M.print(llvm::errs(), nullptr);
#endif
      }
      else
      {
#ifdef DEBUG_RA
         llvm::errs() << "Pointers are not Resolved\n";
         M.print(llvm::errs(), nullptr);
#endif
      }

      // Build the Constraint Graph by running on each function
#ifdef STATS
      Timer* timer = prof.registerNewTimer("BuildGraph", "Build constraint graph");
      timer->startTimer();
#endif
      for(auto& F : M.functions())
      {
         // If the function is only a declaration, or if it has variable number of
         // arguments, do not match
         if(F.isDeclaration() || F.isVarArg())
         {
            continue;
         }

         CG->buildGraph(F, modulePass, &M.getDataLayout(), PtoSets_AA, arePointersResolved, Function2Store);
         MatchParametersAndReturnValues(F, *CG, &M.getDataLayout());
      }
      CG->buildVarNodes(&M.getDataLayout());

#ifdef STATS
      timer->stopTimer();
      prof.addTimeRecord(timer);

      prof.registerMemoryUsage();
#endif
#ifdef DEBUG_RA
      std::string moduleIdentifier = M.getModuleIdentifier();
      auto pos = moduleIdentifier.rfind('/');
      std::string mIdentifier = pos != std::string::npos ? moduleIdentifier.substr(pos) : moduleIdentifier;
      CG->printToFile(*(M.begin()), "/tmp/" + mIdentifier + ".cgpre.dot");
#endif
      CG->findIntervals();
#ifdef DEBUG_RA
      CG->printToFile(*(M.begin()), "/tmp/" + mIdentifier + ".cgpos.dot");
#endif

      finalizeRangeAnalysis(M);
      return false;
   }

   unsigned InterProceduralRACropDFSHelper::getMaxBitWidth(const Module& M)
   {
      unsigned max = 0U;
      // Search through the functions for the max int bitwidth
      for(const Function& F : M.functions())
      {
         if(!F.isDeclaration())
         {
            unsigned bitwidth = RangeAnalysis::getMaxBitWidth(F);
            if(bitwidth > max)
            {
               max = bitwidth;
            }
         }
      }
      return max + 1;
   }

   void InterProceduralRACropDFSHelper::MatchParametersAndReturnValues(const Function& F, ConstraintGraph& G, const llvm::DataLayout* DL)
   {
      // Only do the matching if F has any use
      unsigned int countUses = 0;
      for(auto& U : F.uses())
      {
         User* Us = U.getUser();
         if(!isa<CallInst>(Us) && !isa<InvokeInst>(Us))
         {
            continue;
         }

         auto* caller = cast<Instruction>(Us);
         CallSite CS(caller);
         if(!CS.isCallee(&U))
         {
            continue;
         }

         ++countUses;
      }
      if(countUses == 0)
      {
         return;
      }

      // Data structure which contains the matches between formal and real
      // parameters
      // First: formal parameter
      // Second: real parameter
      SmallVector<std::pair<const Value*, Value*>, 4> parameters(F.arg_size());

      // Fetch the function arguments (formal parameters) into the data structure
      unsigned i = 0;
      for(auto argptr = F.arg_begin(), e = F.arg_end(); argptr != e; ++i, ++argptr)
      {
         parameters[i].first = &*argptr;
      }

      // Check if the function returns a supported value type. If not, no return
      // value matching is done
      bool noReturn = F.getReturnType()->isVoidTy();

      // Creates the data structure which receives the return values of the
      // function, if there is any
      SmallPtrSet<Value*, 4> returnValues;

      if(!noReturn)
      {
         // Iterate over the basic blocks to fetch all possible return values
         for(auto& BB : F)
         {
            // Get the terminator instruction of the basic block and check if it's
            // a return instruction: if it's not, continue to next basic block
            auto terminator = BB.getTerminator();

            auto RI = dyn_cast<ReturnInst>(terminator);

            if(RI == nullptr)
            {
               continue;
            }

            // Get the return value and insert in the data structure
            returnValues.insert(RI->getReturnValue());
         }
      }
      if(returnValues.empty())
      {
         noReturn = true;
      }

      // For each use of F, get the real parameters and the caller instruction to do
      // the matching
      SmallVector<PhiOp*, 4> matchers(F.arg_size(), nullptr);

      for(unsigned long i = 0ul, e = parameters.size(); i < e; ++i)
      {
         VarNode* sink = G.addVarNode(parameters[i].first, nullptr, DL);
         sink->setRange(Range(Regular, sink->getBitWidth(), Min, Max));
         matchers[i] = new PhiOp(std::make_shared<BasicInterval>(), sink, nullptr);
         // Insert the operation in the graph.
         G.getOprs()->insert(matchers[i]);
         // Insert this definition in defmap
         (*G.getDefMap())[sink->getValue()] = matchers[i];
      }

      // For each return value, create a node
      SmallVector<VarNode*, 4> returnVars;

      for(Value* returnValue : returnValues)
      {
         // Add VarNode to the CG
         VarNode* from = G.addVarNode(returnValue, nullptr, DL);
         returnVars.push_back(from);
      }

      for(auto& U : F.uses())
      {
         User* Us = U.getUser();

         // Used by a non-instruction, or not the callee of a function, do not match.
         if(!isa<CallInst>(Us) && !isa<InvokeInst>(Us))
         {
            continue;
         }

         auto* caller = cast<Instruction>(Us);

         CallSite CS(caller);

         if(!CS.isCallee(&U))
         {
            continue;
         }

         // Iterate over the real parameters and put them in the data structure
         CallSite::arg_iterator AI;
         CallSite::arg_iterator EI;

         for(i = 0, AI = CS.arg_begin(), EI = CS.arg_end(); AI != EI; ++i, ++AI)
         {
            parameters[i].second = *AI;
         }

         // // Do the inter-procedural construction of CG
         VarNode* to = nullptr;
         VarNode* from = nullptr;

         // Match formal and real parameters
         for(i = 0; i < parameters.size(); ++i)
         {
            // Add real parameter to the CG
            from = G.addVarNode(parameters[i].second, nullptr, DL);

            // Connect nodes
            matchers[i]->addSource(from);

            // Inserts the sources of the operation in the use map list.
            G.getUseMap()->find(from->getValue())->second.insert(matchers[i]);
         }

         // Match return values
         if(!noReturn)
         {
            // Add caller instruction to the CG (it receives the return value)
            to = G.addVarNode(caller, nullptr, DL);
            to->setRange(Range(Regular, to->getBitWidth(), Min, Max));

            PhiOp* phiOp = new PhiOp(std::make_shared<BasicInterval>(), to, nullptr);

            // Insert the operation in the graph.
            G.getOprs()->insert(phiOp);

            // Insert this definition in defmap
            (*G.getDefMap())[to->getValue()] = phiOp;

            for(VarNode* var : returnVars)
            {
               phiOp->addSource(var);

               // Inserts the sources of the operation in the use map list.
               G.getUseMap()->find(var->getValue())->second.insert(phiOp);
            }
         }

         // Real parameters are cleaned before moving to the next use (for safety's
         // sake)
         for(auto& pair : parameters)
         {
            pair.second = nullptr;
         }
      }
   }

   const Range RangeAnalysis::getRange(const Value* v)
   {
      auto it = ranges.find(v);
      if(v->getType()->isVoidTy() || it == ranges.end())
      {
         return Range(Unknown, MAX_BIT_INT);
      }

      return it->second;
   }
   void RangeAnalysis::printRanges(llvm::Module&
#ifdef DEBUG_RA
                                       M
#endif
                                   ,
                                   llvm::raw_ostream&
#ifdef DEBUG_RA
                                       O
#endif
   )
   {
#ifdef DEBUG_RA
      for(llvm::Function& F : M)
      {
         O << "Analysis for function: " << F.getName() << "\n";
         for(llvm::BasicBlock& BB : F)
         {
            for(llvm::Instruction& I : BB)
            {
               const llvm::Value* V = &I;
               Range R = getRange(V);
               if(!R.isUnknown())
               {
                  R.print(O);
                  O << I << "\n";
               }
               else
               {
                  O << "unknown range ";
                  O << I << "\n";
               }
            }
         }
      }
#endif
   }

   void InterProceduralRACropDFSHelper::finalizeRangeAnalysis(const llvm::Module& M)
   {
      for(auto& F : M.functions())
      {
         if(F.isDeclaration() || F.isVarArg())
         {
            continue;
         }
         for(auto& I : instructions(F))
         {
            if(I.getType()->isVoidTy())
            {
               continue;
            }
            ranges.insert(std::make_pair(&I, CG->getRange(std::make_pair(&I, nullptr), &M.getDataLayout())));
         }
      }
      delete CG;
   }

} // namespace RangeAnalysis
