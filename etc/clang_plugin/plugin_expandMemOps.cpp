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
 *              Copyright (C) 2018-2024 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
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
 * @file plugin_expandMemOps.cpp
 * @brief This pass expand memcpy, memset and memmove.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "ExpandMemOpsPass.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/InitializePasses.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Utils/LoopUtils.h>
#if __clang_major__ != 4
#include <llvm/Transforms/Utils/LowerMemIntrinsics.h>
#endif
#if __clang_major__ >= 13
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/IPO/ArgumentPromotion.h>
#include <llvm/Transforms/IPO/GlobalOpt.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#if __clang_major__ >= 16
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Transforms/Scalar/LowerAtomicPass.h>
#else
#include <llvm/Transforms/Scalar/LowerAtomic.h>
#endif
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>
#endif

#include "debug_print.hpp"

#include <cxxabi.h>

#define PEEL_THRESHOLD 16

#define DEBUG_TYPE "expand-mem-ops"

char llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::ID = 0;

bool llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::addrIsOfIntArrayType(llvm::Value* dst_addr, unsigned& Align,
                                                                            const llvm::DataLayout* DL)
{
   llvm::Type* srcType = nullptr;
   if(llvm::dyn_cast<llvm::BitCastInst>(dst_addr))
   {
      srcType = llvm::cast<llvm::BitCastInst>(dst_addr)->getSrcTy();
   }
   else if(llvm::dyn_cast<llvm::ConstantExpr>(dst_addr) &&
           cast<llvm::ConstantExpr>(dst_addr)->getOpcode() == llvm::Instruction::BitCast)
   {
      srcType = cast<llvm::ConstantExpr>(dst_addr)->getOperand(0)->getType();
   }
   if(srcType && srcType->isPointerTy()
#if __clang_major__ >= 13
      && !srcType->isOpaquePointerTy()
#endif
   )
   {
#if __clang_major__ < 16
      const auto pointee = llvm::cast<llvm::PointerType>(srcType)->getElementType();
#else
      const auto pointee = srcType->getNonOpaquePointerElementType();
#endif
      if(pointee->isArrayTy())
      {
         const auto elType = llvm::cast<llvm::ArrayType>(pointee)->getArrayElementType();
         const auto size = elType->isSized() ? DL->getTypeAllocSizeInBits(elType) : 8ULL;
         if(size <= Align * 8)
         {
            Align = size / 8;
            return elType->isIntegerTy();
         }
      }
      else if(pointee->isStructTy())
      {
         const auto st = cast<llvm::StructType>(pointee);
         if(st->getNumElements() == 1 && st->getElementType(0)->isArrayTy())
         {
            const auto elType = llvm::cast<llvm::ArrayType>(st->getElementType(0))->getArrayElementType();
            const auto size = elType->isSized() ? DL->getTypeAllocSizeInBits(elType) : 8ULL;
            if(size <= Align * 8)
            {
               Align = size / 8;
               return elType->isIntegerTy();
            }
         }
      }
   }

   return false;
}

unsigned llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::getLoopOperandSizeInBytesLocal(llvm::Type* Type)
{
   if(auto* VTy = dyn_cast<llvm::VectorType>(Type))
   {
#if __clang_major__ >= 12
      return (VTy->getElementCount().getFixedValue() * VTy->getElementType()->getPrimitiveSizeInBits()) / 8;
#else
      return (VTy->getNumElements() * VTy->getElementType()->getPrimitiveSizeInBits()) / 8;
#endif
   }
   return Type->getPrimitiveSizeInBits() / 8;
}

llvm::Type* llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::getMemcpyLoopLoweringTypeLocal(
    llvm::LLVMContext& Context, llvm::ConstantInt* Length, unsigned src_align, unsigned dst_align,
    llvm::Value* src_addr, llvm::Value* dst_addr, const llvm::DataLayout* DL, bool is_volatile, bool& Optimize)
{
   if(!is_volatile)
   {
      if(src_align == dst_align && dst_align == Length->getZExtValue() && dst_align <= 8)
      {
         LLVM_DEBUG(llvm::dbgs() << "memcpy can be optimized\n");
         LLVM_DEBUG(llvm::dbgs() << "Align=" << src_align << "\n");
         return llvm::Type::getIntNTy(Context, src_align * 8);
      }
      unsigned localsrc_align = src_align;
      const auto srcCheck = addrIsOfIntArrayType(src_addr, localsrc_align, DL);
      if(srcCheck)
      {
         unsigned localDstAlign = dst_align;
         const auto dstCheck = addrIsOfIntArrayType(dst_addr, localDstAlign, DL);
         if(dstCheck && localsrc_align == localDstAlign)
         {
            Optimize = true;
            LLVM_DEBUG(llvm::dbgs() << "memcpy can be optimized\n");
            LLVM_DEBUG(llvm::dbgs() << "Align=" << src_align << "\n");
            return llvm::Type::getIntNTy(Context, src_align * 8);
         }
      }
   }
   return llvm::Type::getInt8Ty(Context);
}

void llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::getMemcpyLoopResidualLoweringTypeLocal(
    llvm::SmallVectorImpl<llvm::Type*>& OpsOut, llvm::LLVMContext& Context, unsigned RemainingBytes, unsigned src_align,
    unsigned dst_align)
{
   for(unsigned i = 0; i != RemainingBytes; ++i)
   {
      OpsOut.push_back(llvm::Type::getInt8Ty(Context));
   }
}

void llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::createMemCpyLoopKnownSizeLocal(
    llvm::Instruction* InsertBefore, llvm::Value* src_addr, llvm::Value* dst_addr, llvm::ConstantInt* CopyLen,
    unsigned src_align, unsigned dst_align, bool src_volatile, bool dst_volatile, const llvm::DataLayout* DL)
{
   LLVM_DEBUG(llvm::dbgs() << "src: align: " << src_align << ", volatile: " << src_volatile << "\n");
   LLVM_DEBUG(llvm::dbgs() << "dst: align: " << dst_align << ", volatile: " << dst_volatile << "\n");
   // No need to expand zero length copies.
   if(CopyLen->isZero())
   {
      return;
   }

   llvm::BasicBlock* PreLoopBB = InsertBefore->getParent();
   llvm::BasicBlock* PostLoopBB = nullptr;
   llvm::Function* ParentFunc = PreLoopBB->getParent();
   llvm::LLVMContext& Ctx = PreLoopBB->getContext();

   bool PeelCandidate = false;
   const auto TypeOfCopyLen = CopyLen->getType();
   const auto LoopOpType = getMemcpyLoopLoweringTypeLocal(Ctx, CopyLen, src_align, dst_align, src_addr, dst_addr, DL,
                                                          src_volatile || dst_volatile, PeelCandidate);

   const auto LoopOpSize = getLoopOperandSizeInBytesLocal(LoopOpType);
   const auto LoopEndCount = CopyLen->getZExtValue() / LoopOpSize;

   const auto SrcAS = cast<llvm::PointerType>(src_addr->getType())->getAddressSpace();
   const auto DstAS = cast<llvm::PointerType>(dst_addr->getType())->getAddressSpace();

   auto BytesCopied = LoopEndCount * LoopOpSize;
   const auto RemainingBytes = CopyLen->getZExtValue() - BytesCopied;

   LLVM_DEBUG(llvm::dbgs() << "  size: " << LoopOpSize << ", count: " << LoopEndCount << ", left: " << RemainingBytes
                           << "\n");

   bool do_unrolling;
#if __clang_major__ == 7
   do_unrolling = false;
#else
   do_unrolling = true;
#endif
   bool srcIsAlloca = false;
   bool srcIsGlobal = false;
   if(llvm::dyn_cast<llvm::BitCastInst>(src_addr) &&
      dyn_cast<llvm::AllocaInst>(llvm::dyn_cast<llvm::BitCastInst>(src_addr)->getOperand(0)))
   {
      srcIsAlloca = true;
      do_unrolling &= (LoopEndCount <= PEEL_THRESHOLD);
   }
   else if(llvm::dyn_cast<llvm::ConstantExpr>(src_addr) &&
           cast<llvm::ConstantExpr>(src_addr)->getOpcode() == llvm::Instruction::BitCast &&
           dyn_cast<llvm::GlobalVariable>(cast<llvm::ConstantExpr>(src_addr)->getOperand(0)))
   {
      srcIsGlobal = true;
      do_unrolling &=
          (dyn_cast<llvm::GlobalVariable>(cast<llvm::ConstantExpr>(src_addr)->getOperand(0))->isConstant() ||
           (LoopEndCount <= PEEL_THRESHOLD));
   }
   else if(LoopEndCount == 1)
   {
      do_unrolling = true;
   }

   if(do_unrolling && !src_volatile && !dst_volatile && (srcIsAlloca || srcIsGlobal) &&
      llvm::dyn_cast<llvm::BitCastInst>(dst_addr) && PeelCandidate)
   {
      llvm::PointerType* SrcOpType = llvm::PointerType::get(LoopOpType, SrcAS);
      llvm::PointerType* DstOpType = llvm::PointerType::get(LoopOpType, DstAS);
      llvm::IRBuilder<> Builder(InsertBefore);
      auto srcAddress = Builder.CreateBitCast(srcIsAlloca ? llvm::dyn_cast<llvm::BitCastInst>(src_addr)->getOperand(0) :
                                                            cast<llvm::ConstantExpr>(src_addr)->getOperand(0),
                                              SrcOpType);
      auto dstAddress = Builder.CreateBitCast(llvm::dyn_cast<llvm::BitCastInst>(dst_addr)->getOperand(0), DstOpType);
      for(auto LI = 0u; LI < LoopEndCount; ++LI)
      {
         llvm::Value* SrcGEP =
             Builder.CreateInBoundsGEP(LoopOpType, srcAddress, llvm::ConstantInt::get(TypeOfCopyLen, LI));
#if __clang_major__ <= 11
         llvm::Value* Load = Builder.CreateLoad(SrcGEP, src_volatile);
#else
         llvm::Value* Load = Builder.CreateLoad(SrcGEP->getType()->getPointerElementType(), SrcGEP, src_volatile);
#endif
         llvm::Value* DstGEP =
             Builder.CreateInBoundsGEP(LoopOpType, dstAddress, llvm::ConstantInt::get(TypeOfCopyLen, LI));
         Builder.CreateStore(Load, DstGEP, dst_volatile);
      }
   }
   else if(LoopEndCount != 0)
   {
      // Split
      PostLoopBB = PreLoopBB->splitBasicBlock(InsertBefore, "memcpy-split");
      llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(Ctx, "load-store-loop", ParentFunc, PostLoopBB);
      PreLoopBB->getTerminator()->setSuccessor(0, LoopBB);

      llvm::IRBuilder<> PLBuilder(PreLoopBB->getTerminator());

      // Cast the Src and Dst pointers to pointers to the loop operand type (if
      // needed).
      llvm::PointerType* SrcOpType = llvm::PointerType::get(LoopOpType, SrcAS);
      llvm::PointerType* DstOpType = llvm::PointerType::get(LoopOpType, DstAS);
      if(src_addr->getType() != SrcOpType)
      {
         src_addr = PLBuilder.CreateBitCast(src_addr, SrcOpType);
      }
      if(dst_addr->getType() != DstOpType)
      {
         dst_addr = PLBuilder.CreateBitCast(dst_addr, DstOpType);
      }

      llvm::IRBuilder<> LoopBuilder(LoopBB);
      llvm::PHINode* LoopIndex = LoopBuilder.CreatePHI(TypeOfCopyLen, 2, "loop-index");
      LoopIndex->addIncoming(llvm::ConstantInt::get(TypeOfCopyLen, 0U), PreLoopBB);
      // Loop Body
      llvm::Value* SrcGEP = LoopBuilder.CreateInBoundsGEP(LoopOpType, src_addr, LoopIndex);
#if __clang_major__ <= 11
      llvm::Value* Load = LoopBuilder.CreateLoad(SrcGEP, src_volatile);
#else
      llvm::Value* Load = LoopBuilder.CreateLoad(SrcGEP->getType()->getPointerElementType(), SrcGEP, src_volatile);
#endif
      llvm::Value* DstGEP = LoopBuilder.CreateInBoundsGEP(LoopOpType, dst_addr, LoopIndex);
      LoopBuilder.CreateStore(Load, DstGEP, dst_volatile);

      llvm::Value* NewIndex = LoopBuilder.CreateAdd(LoopIndex, llvm::ConstantInt::get(TypeOfCopyLen, 1U));
      LoopIndex->addIncoming(NewIndex, LoopBB);

      // Create the loop branch condition.
      llvm::Constant* LoopEndCI = llvm::ConstantInt::get(TypeOfCopyLen, LoopEndCount);
      LoopBuilder.CreateCondBr(LoopBuilder.CreateICmpULT(NewIndex, LoopEndCI), LoopBB, PostLoopBB);
   }

   if(RemainingBytes)
   {
      llvm::IRBuilder<> RBuilder(PostLoopBB ? PostLoopBB->getFirstNonPHI() : InsertBefore);

      // Update the alignment based on the copy size used in the loop body.
      src_align = std::min(src_align, LoopOpSize);
      dst_align = std::min(dst_align, LoopOpSize);

      llvm::SmallVector<llvm::Type*, 5> RemainingOps;
      getMemcpyLoopResidualLoweringTypeLocal(RemainingOps, Ctx, RemainingBytes, src_align, dst_align);

      for(auto OpTy : RemainingOps)
      {
         // Calculate the new index
         unsigned OperandSize = getLoopOperandSizeInBytesLocal(OpTy);
         uint64_t GepIndex = BytesCopied / OperandSize;
         assert(GepIndex * OperandSize == BytesCopied && "Division should have no Remainder!");
         // Cast source to operand type and load
         llvm::PointerType* SrcPtrType = llvm::PointerType::get(OpTy, SrcAS);
         llvm::Value* CastedSrc =
             src_addr->getType() == SrcPtrType ? src_addr : RBuilder.CreateBitCast(src_addr, SrcPtrType);
         llvm::Value* SrcGEP =
             RBuilder.CreateInBoundsGEP(OpTy, CastedSrc, llvm::ConstantInt::get(TypeOfCopyLen, GepIndex));
#if __clang_major__ <= 11
         llvm::Value* Load = RBuilder.CreateLoad(SrcGEP, src_volatile);
#else
         llvm::Value* Load = RBuilder.CreateLoad(SrcGEP->getType()->getPointerElementType(), SrcGEP, src_volatile);
#endif
         // Cast destination to operand type and store.
         llvm::PointerType* DstPtrType = llvm::PointerType::get(OpTy, DstAS);
         llvm::Value* CastedDst =
             dst_addr->getType() == DstPtrType ? dst_addr : RBuilder.CreateBitCast(dst_addr, DstPtrType);
         llvm::Value* DstGEP =
             RBuilder.CreateInBoundsGEP(OpTy, CastedDst, llvm::ConstantInt::get(TypeOfCopyLen, GepIndex));
         RBuilder.CreateStore(Load, DstGEP, dst_volatile);

         BytesCopied += OperandSize;
      }
   }
   assert(BytesCopied == CopyLen->getZExtValue() && "Bytes copied should match size in the call!");
}

void llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::expandMemSetAsLoopLocal(llvm::MemSetInst* Memset,
                                                                               const llvm::DataLayout* DL)
{
   llvm::Instruction* InsertBefore = Memset;
   llvm::Value* dst_addr = Memset->getRawDest();
   llvm::Value* CopyLen = Memset->getLength();
   llvm::Value* SetValue = Memset->getValue();
   unsigned Align =
#if __clang_major__ <= 6
       Memset->getAlignment();
#elif __clang_major__ < 16
       Memset->getDestAlignment();
#else
       dst_addr->getPointerAlignment(*DL).value();
#endif
   bool IsVolatile = Memset->isVolatile();
   llvm::Type* TypeOfCopyLen = CopyLen->getType();
   llvm::BasicBlock* OrigBB = InsertBefore->getParent();
   llvm::Function* F = OrigBB->getParent();
   llvm::BasicBlock* NewBB = OrigBB->splitBasicBlock(InsertBefore, "split");
   llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(F->getContext(), "loadstoreloop", F, NewBB);

   addrIsOfIntArrayType(dst_addr, Align, DL);

   bool AlignCanBeUsed = false;
   if(isa<llvm::ConstantInt>(CopyLen) && isa<llvm::Constant>(SetValue) &&
      cast<llvm::Constant>(SetValue)->isNullValue() && Align > 1 && Align <= 8 && SetValue->getType()->isIntegerTy())
   {
      AlignCanBeUsed = true;
   }
   if(AlignCanBeUsed)
   {
      LLVM_DEBUG(llvm::dbgs() << "memset can be optimized\n");
      LLVM_DEBUG(llvm::dbgs() << "Align=" << Align << "\n");
      SetValue = llvm::ConstantInt::get(llvm::Type::getIntNTy(F->getContext(), Align * 8), 0);
   }
   else
   {
      LLVM_DEBUG(llvm::dbgs() << "memset cannot be optimized\n");
   }
   llvm::IRBuilder<> Builder(OrigBB->getTerminator());

   auto ActualCopyLen =
       AlignCanBeUsed ?
           llvm::ConstantInt::get(TypeOfCopyLen, cast<llvm::ConstantInt>(CopyLen)->getValue().udiv(
                                                     llvm::APInt(TypeOfCopyLen->getIntegerBitWidth(), Align))) :
           CopyLen;

   // Cast pointer to the type of value getting stored
   unsigned dstAS = cast<llvm::PointerType>(dst_addr->getType())->getAddressSpace();
   dst_addr = Builder.CreateBitCast(dst_addr, llvm::PointerType::get(SetValue->getType(), dstAS));

   Builder.CreateCondBr(Builder.CreateICmpEQ(llvm::ConstantInt::get(TypeOfCopyLen, 0), CopyLen), NewBB, LoopBB);
   OrigBB->getTerminator()->eraseFromParent();

   llvm::IRBuilder<> LoopBuilder(LoopBB);
   llvm::PHINode* LoopIndex = LoopBuilder.CreatePHI(TypeOfCopyLen, 0);
   LoopIndex->addIncoming(llvm::ConstantInt::get(TypeOfCopyLen, 0), OrigBB);

   if(AlignCanBeUsed)
   {
#if __clang_major__ >= 11
      LoopBuilder.CreateAlignedStore(SetValue, LoopBuilder.CreateInBoundsGEP(SetValue->getType(), dst_addr, LoopIndex),
                                     llvm::MaybeAlign(Align), IsVolatile);
#else
      LoopBuilder.CreateAlignedStore(SetValue, LoopBuilder.CreateInBoundsGEP(SetValue->getType(), dst_addr, LoopIndex),
                                     Align, IsVolatile);
#endif
   }
   else
   {
      LoopBuilder.CreateStore(SetValue, LoopBuilder.CreateInBoundsGEP(SetValue->getType(), dst_addr, LoopIndex),
                              IsVolatile);
   }

   llvm::Value* NewIndex = LoopBuilder.CreateAdd(LoopIndex, llvm::ConstantInt::get(TypeOfCopyLen, 1));
   LoopIndex->addIncoming(NewIndex, LoopBB);

   LoopBuilder.CreateCondBr(LoopBuilder.CreateICmpULT(NewIndex, ActualCopyLen), LoopBB, NewBB);
}

llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)() : ModulePass(ID)
{
   initializeTargetTransformInfoWrapperPassPass(*PassRegistry::getPassRegistry());
   initializeLoopPassPass(*PassRegistry::getPassRegistry());
}

#if __clang_major__ >= 13
llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)(
    const CLANG_VERSION_SYMBOL(_plugin_expandMemOps) &)
    : CLANG_VERSION_SYMBOL(_plugin_expandMemOps)()
{
}
#endif

bool llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::exec(
    Module& M, llvm::function_ref<llvm::TargetTransformInfo&(llvm::Function&)> GetTTI
#if __clang_major__ >= 16
    ,
    llvm::function_ref<llvm::ScalarEvolution&(llvm::Function&)> GetSE
#endif
)
{
#if __clang_major__ < 13
   if(skipModule(M))
   {
      return false;
   }
#endif
   LLVM_DEBUG(llvm::dbgs() << "Running mem ops expansion on " << M.getFunctionList().size() << " functions\n");

   auto DL = &M.getDataLayout();
   auto res = false;
   auto currFuncIterator = M.getFunctionList().begin();
   while(currFuncIterator != M.getFunctionList().end())
   {
      auto& F = *currFuncIterator;
      LLVM_DEBUG(llvm::dbgs() << "  Function " << F.getName().str() << "\n");
      llvm::SmallVector<llvm::MemIntrinsic*, 4> MemCalls;
      for(auto& BI : F)
      {
         for(auto& I : BI)
         {
            if(auto InstrCall = dyn_cast<llvm::MemIntrinsic>(&I))
            {
               LLVM_DEBUG({
                  llvm::dbgs() << "    Found mem intrinsic: ";
                  InstrCall->print(llvm::dbgs(), true);
                  llvm::dbgs() << "\n";
               });
               MemCalls.push_back(InstrCall);
               LLVM_DEBUG({
                  if(auto* Memcpy = dyn_cast<llvm::MemCpyInst>(InstrCall))
                  {
                     if(dyn_cast<llvm::ConstantInt>(Memcpy->getLength()))
                     {
                        llvm::dbgs() << "    Found a memcpy with a constant number of iterations\n";
                     }
                     else
                     {
                        llvm::dbgs() << "    Found a memcpy with an unknown number of iterations\n";
                     }
                  }
                  else if(dyn_cast<llvm::MemSetInst>(InstrCall))
                  {
                     llvm::dbgs() << "    Found a memset intrinsic\n";
                  }
               });
            }
         }
      }
      // Transform mem* intrinsic calls.
      for(llvm::MemIntrinsic* MemCall : MemCalls)
      {
         bool do_erase;
         do_erase = false;
         if(auto Memcpy = dyn_cast<llvm::MemCpyInst>(MemCall))
         {
            auto CI = dyn_cast<llvm::ConstantInt>(Memcpy->getLength());
            if(__clang_major__ < 16 && CI)
            {
               LLVM_DEBUG(llvm::dbgs() << "Expanding memcpy constant\n");
               createMemCpyLoopKnownSizeLocal(Memcpy, Memcpy->getRawSource(), Memcpy->getRawDest(), CI,
#if __clang_major__ <= 6
                                              Memcpy->getAlignment(), Memcpy->getAlignment(),
#elif __clang_major__ < 16
                                              Memcpy->getSourceAlignment(), Memcpy->getDestAlignment(),
#else
                                              Memcpy->getRawSource()->getPointerAlignment(*DL).value(),
                                              Memcpy->getRawDest()->getPointerAlignment(*DL).value(),
#endif
                                              Memcpy->isVolatile(), Memcpy->isVolatile(), DL);
               do_erase = true;
               res = true;
            }
#if __clang_major__ != 4
            else
            {
               LLVM_DEBUG(llvm::dbgs() << "Expanding memcpy as loop\n");
               const auto& TTI = GetTTI(F);
#if __clang_major__ >= 16
               auto& SE = GetSE(F);
#endif
               llvm::expandMemCpyAsLoop(Memcpy, TTI
#if __clang_major__ >= 16
                                        ,
                                        &SE
#endif
               );
               do_erase = true;
               res = true;
            }
#endif
         }
         else if(auto Memmove = dyn_cast<llvm::MemMoveInst>(MemCall))
         {
            LLVM_DEBUG(llvm::dbgs() << "Expanding memmove\n");
#if __clang_major__ != 4
            llvm::expandMemMoveAsLoop(Memmove);
            do_erase = true;
            res = true;
#endif
         }
         else if(auto Memset = dyn_cast<llvm::MemSetInst>(MemCall))
         {
            LLVM_DEBUG(llvm::dbgs() << "Expanding memset\n");
#if __clang_major__ >= 16
            expandMemSetAsLoop(Memset);
#else
            expandMemSetAsLoopLocal(Memset, DL);
#endif
            do_erase = true;
            res = true;
         }
         if(do_erase)
         {
            MemCall->eraseFromParent();
         }
      }
      ++currFuncIterator;
   }
   return res;
}

bool llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::runOnModule(Module& M)
{
   auto GetTTI = [&](llvm::Function& F) -> llvm::TargetTransformInfo& {
      return getAnalysis<llvm::TargetTransformInfoWrapperPass>().getTTI(F);
   };
#if __clang_major__ >= 16
   auto GetSE = [&](llvm::Function& F) -> llvm::ScalarEvolution& {
      return getAnalysis<llvm::ScalarEvolutionWrapperPass>(F).getSE();
   };
#endif
   return exec(M, GetTTI
#if __clang_major__ >= 16
               ,
               GetSE
#endif
   );
}

llvm::StringRef llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::getPassName() const
{
   return CLANG_VERSION_STRING(_plugin_expandMemOps);
}

void llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::getAnalysisUsage(AnalysisUsage& AU) const
{
   getLoopAnalysisUsage(AU);
   AU.addRequired<TargetTransformInfoWrapperPass>();
#if __clang_major__ >= 16
   AU.addRequired<ScalarEvolutionWrapperPass>();
#endif
}

#if __clang_major__ >= 13
llvm::PreservedAnalyses llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)::run(llvm::Module& M,
                                                                              llvm::ModuleAnalysisManager& MAM)
{
   LLVM_DEBUG(llvm::dbgs() << "Running mem ops expansion\n");
   auto& FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
   auto GetTTI = [&](llvm::Function& F) -> llvm::TargetTransformInfo& {
      return FAM.getResult<llvm::TargetIRAnalysis>(F);
   };
#if __clang_major__ >= 16
   auto GetSE = [&](llvm::Function& F) -> llvm::ScalarEvolution& { return FAM.getResult<ScalarEvolutionAnalysis>(F); };
#endif
   const auto changed = exec(M, GetTTI
#if __clang_major__ >= 16
                             ,
                             GetSE
#endif
   );
   return (changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
}
#endif

#if !defined(_WIN32)
#if CPP_LANGUAGE
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)>
    XPass(CLANG_VERSION_STRING(_plugin_expandMemOpsCpp), "Make all private/static but the top function",
          false /* Only looks at CFG */, false /* Analysis Pass */);
#else
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)>
    XPass(CLANG_VERSION_STRING(_plugin_expandMemOps), "Make all private/static but the top function",
          false /* Only looks at CFG */, false /* Analysis Pass */);
#endif
#endif

#if __clang_major__ >= 13
llvm::PassPluginLibraryInfo CLANG_PLUGIN_INFO(_plugin_expandMemOps)()
{
   return {LLVM_PLUGIN_API_VERSION, CLANG_VERSION_STRING(_plugin_expandMemOps), "v0.12", [](llvm::PassBuilder& PB) {
              const auto load = [](llvm::ModulePassManager& MPM) {
                 llvm::FunctionPassManager FPM;
                 FPM.addPass(llvm::InstCombinePass());
                 FPM.addPass(llvm::UnifyFunctionExitNodesPass());
                 MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));
                 MPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)());
                 return true;
              };
              PB.registerPipelineParsingCallback([&](llvm::StringRef Name, llvm::ModulePassManager& MPM,
                                                     llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                 if(Name == CLANG_VERSION_STRING(_plugin_expandMemOps))
                 {
                    return load(MPM);
                 }
                 return false;
              });
              PB.registerOptimizerLastEPCallback([&](llvm::ModulePassManager& MPM,
#if __clang_major__ < 16
                                                     llvm::PassBuilder::OptimizationLevel
#else
                                                     llvm::OptimizationLevel
#endif
                                                 ) { return load(MPM); });
           }};
}

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo()
{
   return CLANG_PLUGIN_INFO(_plugin_expandMemOps)();
}
#else
#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_expandMemOps)());
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses
    CLANG_VERSION_SYMBOL(_plugin_expandMemOps_Ox)(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);
#endif
#endif

// using namespace llvm;
//
// namespace llvm
// {
//    void CLANG_PLUGIN_INIT(_plugin_expandMemOps)(PassRegistry&);
// } // namespace llvm
//
// INITIALIZE_PASS_BEGIN(CLANG_VERSION_SYMBOL(_plugin_expandMemOps), CLANG_VERSION_STRING(_plugin_expandMemOps),
//                       "expand all memset,memcpy and memmove", false, false)
// INITIALIZE_PASS_END(CLANG_VERSION_SYMBOL(_plugin_expandMemOps), CLANG_VERSION_STRING(_plugin_expandMemOps),
//                     "expand all memset,memcpy and memmove", false, false)
