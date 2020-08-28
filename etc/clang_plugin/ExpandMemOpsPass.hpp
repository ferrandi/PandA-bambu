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
/*
 * The implementation performs scalar replacement of aggregates.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef SCALAR_REPLACEMENT_OF_AGGREGATES_EXPANDMEMOPSPASS_HPP
#define SCALAR_REPLACEMENT_OF_AGGREGATES_EXPANDMEMOPSPASS_HPP

#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Pass.h>
#if __clang_major__ != 4
#include "llvm/Transforms/Utils/LowerMemIntrinsics.h"
#endif

#ifndef __clang_major__
#define __clang_major__ 4
#endif
#define PEEL_THRESHOLD 16

namespace llvm
{
   class ExpandMemOpsPass : public ModulePass
   {
    private:
      bool addrIsOfIntArrayType(llvm::Value* DstAddr, unsigned& Align, const llvm::DataLayout* DL)
      {
         llvm::Type* srcType = nullptr;
         if(llvm::dyn_cast<llvm::BitCastInst>(DstAddr))
         {
            srcType = llvm::cast<llvm::BitCastInst>(DstAddr)->getSrcTy();
         }
         else if(llvm::dyn_cast<llvm::ConstantExpr>(DstAddr) && cast<llvm::ConstantExpr>(DstAddr)->getOpcode() == llvm::Instruction::BitCast)
         {
            srcType = cast<llvm::ConstantExpr>(DstAddr)->getOperand(0)->getType();
         }
         if(srcType)
         {
            if(srcType->isPointerTy())
            {
               auto pointee = llvm::cast<llvm::PointerType>(srcType)->getElementType();
               if(pointee->isArrayTy())
               {
                  auto elType = llvm::cast<llvm::ArrayType>(pointee)->getArrayElementType();
                  auto size = elType->isSized() ? DL->getTypeAllocSizeInBits(elType) : 8ULL;
                  if(size <= Align * 8)
                  {
                     Align = size / 8;
                     return elType->isIntegerTy();
                  }
               }
               else if(pointee->isStructTy())
               {
                  auto st = cast<llvm::StructType>(pointee);
                  if(st->getNumElements() == 1 && st->getElementType(0)->isArrayTy())
                  {
                     auto elType = llvm::cast<llvm::ArrayType>(st->getElementType(0))->getArrayElementType();
                     auto size = elType->isSized() ? DL->getTypeAllocSizeInBits(elType) : 8ULL;
                     if(size <= Align * 8)
                     {
                        Align = size / 8;
                        return elType->isIntegerTy();
                     }
                  }
               }
            }
         }

         return false;
      }

      unsigned getLoopOperandSizeInBytesLocal(llvm::Type* Type)
      {
         if(llvm::VectorType* VTy = dyn_cast<llvm::VectorType>(Type))
         {
            return VTy->getBitWidth() / 8;
         }
         return Type->getPrimitiveSizeInBits() / 8;
      }

      llvm::Type* getMemcpyLoopLoweringTypeLocal(llvm::LLVMContext& Context, llvm::ConstantInt* Length, unsigned SrcAlign, unsigned DestAlign, llvm::Value* SrcAddr, llvm::Value* DstAddr, const llvm::DataLayout* DL, bool isVolatile, bool& Optimize)
      {
         if(!isVolatile)
         {
            if(SrcAlign == DestAlign && DestAlign == Length->getZExtValue() && DestAlign <= 8)
            {
#if PRINT_DBG_MSG
               llvm::errs() << "memcpy can be optimized\n";
               llvm::errs() << "Align=" << SrcAlign << "\n";
#endif
               return llvm::Type::getIntNTy(Context, SrcAlign * 8);
            }
            unsigned localSrcAlign = SrcAlign;
            auto srcCheck = addrIsOfIntArrayType(SrcAddr, localSrcAlign, DL);
            if(srcCheck)
            {
               unsigned localDstAlign = DestAlign;
               auto dstCheck = addrIsOfIntArrayType(DstAddr, localDstAlign, DL);
               if(dstCheck && localSrcAlign == localDstAlign)
               {
                  Optimize = true;
#if PRINT_DBG_MSG
                  llvm::errs() << "memcpy can be optimized\n";
                  llvm::errs() << "Align=" << SrcAlign << "\n";
#endif
                  return llvm::Type::getIntNTy(Context, SrcAlign * 8);
               }
            }
         }
         return llvm::Type::getInt8Ty(Context);
      }

      void getMemcpyLoopResidualLoweringTypeLocal(llvm::SmallVectorImpl<llvm::Type*>& OpsOut, llvm::LLVMContext& Context, unsigned RemainingBytes, unsigned SrcAlign, unsigned DestAlign)
      {
         for(unsigned i = 0; i != RemainingBytes; ++i)
            OpsOut.push_back(llvm::Type::getInt8Ty(Context));
      }

      void createMemCpyLoopKnownSizeLocal(llvm::Instruction* InsertBefore, llvm::Value* SrcAddr, llvm::Value* DstAddr, llvm::ConstantInt* CopyLen, unsigned SrcAlign, unsigned DestAlign, bool SrcIsVolatile, bool DstIsVolatile, const llvm::DataLayout* DL)
      {
         // No need to expand zero length copies.
         if(CopyLen->isZero())
            return;

         llvm::BasicBlock* PreLoopBB = InsertBefore->getParent();
         llvm::BasicBlock* PostLoopBB = nullptr;
         llvm::Function* ParentFunc = PreLoopBB->getParent();
         llvm::LLVMContext& Ctx = PreLoopBB->getContext();

         bool PeelCandidate = false;
         llvm::Type* TypeOfCopyLen = CopyLen->getType();
         llvm::Type* LoopOpType = getMemcpyLoopLoweringTypeLocal(Ctx, CopyLen, SrcAlign, DestAlign, SrcAddr, DstAddr, DL, SrcIsVolatile || DstIsVolatile, PeelCandidate);

         unsigned LoopOpSize = getLoopOperandSizeInBytesLocal(LoopOpType);
         uint64_t LoopEndCount = CopyLen->getZExtValue() / LoopOpSize;

         unsigned SrcAS = cast<llvm::PointerType>(SrcAddr->getType())->getAddressSpace();
         unsigned DstAS = cast<llvm::PointerType>(DstAddr->getType())->getAddressSpace();

         uint64_t BytesCopied = LoopEndCount * LoopOpSize;
         uint64_t RemainingBytes = CopyLen->getZExtValue() - BytesCopied;

         bool do_unrolling;
#if __clang_major__ == 7
         do_unrolling = false;
#else
         do_unrolling = true;
#endif
         bool srcIsAlloca = false;
         bool srcIsGlobal = false;
         if(llvm::dyn_cast<llvm::BitCastInst>(SrcAddr) && dyn_cast<llvm::AllocaInst>(llvm::dyn_cast<llvm::BitCastInst>(SrcAddr)->getOperand(0)))
         {
            srcIsAlloca = true;
            do_unrolling = do_unrolling && (LoopEndCount <= PEEL_THRESHOLD);
         }
         else if(llvm::dyn_cast<llvm::ConstantExpr>(SrcAddr) && cast<llvm::ConstantExpr>(SrcAddr)->getOpcode() == llvm::Instruction::BitCast && dyn_cast<llvm::GlobalVariable>(cast<llvm::ConstantExpr>(SrcAddr)->getOperand(0)))
         {
            srcIsGlobal = true;
            do_unrolling = do_unrolling && (dyn_cast<llvm::GlobalVariable>(cast<llvm::ConstantExpr>(SrcAddr)->getOperand(0))->isConstant() || (LoopEndCount <= PEEL_THRESHOLD));
         }
         else if(LoopEndCount == 1)
            do_unrolling = true;
         if(do_unrolling && !SrcIsVolatile && !DstIsVolatile && (srcIsAlloca || srcIsGlobal) && llvm::dyn_cast<llvm::BitCastInst>(DstAddr) && PeelCandidate)
         {
            llvm::PointerType* SrcOpType = llvm::PointerType::get(LoopOpType, SrcAS);
            llvm::PointerType* DstOpType = llvm::PointerType::get(LoopOpType, DstAS);
            llvm::IRBuilder<> Builder(InsertBefore);
            auto srcAddress = Builder.CreateBitCast(srcIsAlloca ? llvm::dyn_cast<llvm::BitCastInst>(SrcAddr)->getOperand(0) : cast<llvm::ConstantExpr>(SrcAddr)->getOperand(0), SrcOpType);
            auto dstAddress = Builder.CreateBitCast(llvm::dyn_cast<llvm::BitCastInst>(DstAddr)->getOperand(0), DstOpType);
            for(auto LI = 0u; LI < LoopEndCount; ++LI)
            {
               llvm::Value* SrcGEP = Builder.CreateInBoundsGEP(LoopOpType, srcAddress, llvm::ConstantInt::get(TypeOfCopyLen, LI));
               llvm::Value* Load = Builder.CreateLoad(SrcGEP, SrcIsVolatile);
               llvm::Value* DstGEP = Builder.CreateInBoundsGEP(LoopOpType, dstAddress, llvm::ConstantInt::get(TypeOfCopyLen, LI));
               Builder.CreateStore(Load, DstGEP, DstIsVolatile);
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
            if(SrcAddr->getType() != SrcOpType)
            {
               SrcAddr = PLBuilder.CreateBitCast(SrcAddr, SrcOpType);
            }
            if(DstAddr->getType() != DstOpType)
            {
               DstAddr = PLBuilder.CreateBitCast(DstAddr, DstOpType);
            }

            llvm::IRBuilder<> LoopBuilder(LoopBB);
            llvm::PHINode* LoopIndex = LoopBuilder.CreatePHI(TypeOfCopyLen, 2, "loop-index");
            LoopIndex->addIncoming(llvm::ConstantInt::get(TypeOfCopyLen, 0U), PreLoopBB);
            // Loop Body
            llvm::Value* SrcGEP = LoopBuilder.CreateInBoundsGEP(LoopOpType, SrcAddr, LoopIndex);
            llvm::Value* Load = LoopBuilder.CreateLoad(SrcGEP, SrcIsVolatile);
            llvm::Value* DstGEP = LoopBuilder.CreateInBoundsGEP(LoopOpType, DstAddr, LoopIndex);
            LoopBuilder.CreateStore(Load, DstGEP, DstIsVolatile);

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
            SrcAlign = std::min(SrcAlign, LoopOpSize);
            DestAlign = std::min(DestAlign, LoopOpSize);

            llvm::SmallVector<llvm::Type*, 5> RemainingOps;
            getMemcpyLoopResidualLoweringTypeLocal(RemainingOps, Ctx, RemainingBytes, SrcAlign, DestAlign);

            for(auto OpTy : RemainingOps)
            {
               // Calculate the new index
               unsigned OperandSize = getLoopOperandSizeInBytesLocal(OpTy);
               uint64_t GepIndex = BytesCopied / OperandSize;
               assert(GepIndex * OperandSize == BytesCopied && "Division should have no Remainder!");
               // Cast source to operand type and load
               llvm::PointerType* SrcPtrType = llvm::PointerType::get(OpTy, SrcAS);
               llvm::Value* CastedSrc = SrcAddr->getType() == SrcPtrType ? SrcAddr : RBuilder.CreateBitCast(SrcAddr, SrcPtrType);
               llvm::Value* SrcGEP = RBuilder.CreateInBoundsGEP(OpTy, CastedSrc, llvm::ConstantInt::get(TypeOfCopyLen, GepIndex));
               llvm::Value* Load = RBuilder.CreateLoad(SrcGEP, SrcIsVolatile);

               // Cast destination to operand type and store.
               llvm::PointerType* DstPtrType = llvm::PointerType::get(OpTy, DstAS);
               llvm::Value* CastedDst = DstAddr->getType() == DstPtrType ? DstAddr : RBuilder.CreateBitCast(DstAddr, DstPtrType);
               llvm::Value* DstGEP = RBuilder.CreateInBoundsGEP(OpTy, CastedDst, llvm::ConstantInt::get(TypeOfCopyLen, GepIndex));
               RBuilder.CreateStore(Load, DstGEP, DstIsVolatile);

               BytesCopied += OperandSize;
            }
         }
         assert(BytesCopied == CopyLen->getZExtValue() && "Bytes copied should match size in the call!");
      }

      void expandMemSetAsLoopLocal(llvm::MemSetInst* Memset, const llvm::DataLayout* DL)
      {
         llvm::Instruction* InsertBefore = Memset;
         llvm::Value* DstAddr = Memset->getRawDest();
         llvm::Value* CopyLen = Memset->getLength();
         llvm::Value* SetValue = Memset->getValue();
#if __clang_major__ >= 7
         unsigned Align = Memset->getDestAlignment();
#else
         unsigned Align = Memset->getAlignment();
#endif
         bool IsVolatile = Memset->isVolatile();
         llvm::Type* TypeOfCopyLen = CopyLen->getType();
         llvm::BasicBlock* OrigBB = InsertBefore->getParent();
         llvm::Function* F = OrigBB->getParent();
         llvm::BasicBlock* NewBB = OrigBB->splitBasicBlock(InsertBefore, "split");
         llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(F->getContext(), "loadstoreloop", F, NewBB);

         addrIsOfIntArrayType(DstAddr, Align, DL);

         bool AlignCanBeUsed = false;
         if(isa<llvm::ConstantInt>(CopyLen) && isa<llvm::Constant>(SetValue) && cast<llvm::Constant>(SetValue)->isNullValue() && Align > 1 && Align <= 8 && SetValue->getType()->isIntegerTy())
            AlignCanBeUsed = true;
         if(AlignCanBeUsed)
         {
#if PRINT_DBG_MSG
            llvm::errs() << "memset can be optimized\n";
            llvm::errs() << "Align=" << Align << "\n";
#endif
            SetValue = llvm::ConstantInt::get(llvm::Type::getIntNTy(F->getContext(), Align * 8), 0);
         }
#if PRINT_DBG_MSG
         else
            llvm::errs() << "memset cannot be optimized\n";
#endif
         llvm::IRBuilder<> Builder(OrigBB->getTerminator());

         auto ActualCopyLen = AlignCanBeUsed ? llvm::ConstantInt::get(TypeOfCopyLen, cast<llvm::ConstantInt>(CopyLen)->getValue().udiv(llvm::APInt(TypeOfCopyLen->getIntegerBitWidth(), Align))) : CopyLen;

         // Cast pointer to the type of value getting stored
         unsigned dstAS = cast<llvm::PointerType>(DstAddr->getType())->getAddressSpace();
         DstAddr = Builder.CreateBitCast(DstAddr, llvm::PointerType::get(SetValue->getType(), dstAS));

         Builder.CreateCondBr(Builder.CreateICmpEQ(llvm::ConstantInt::get(TypeOfCopyLen, 0), CopyLen), NewBB, LoopBB);
         OrigBB->getTerminator()->eraseFromParent();

         llvm::IRBuilder<> LoopBuilder(LoopBB);
         llvm::PHINode* LoopIndex = LoopBuilder.CreatePHI(TypeOfCopyLen, 0);
         LoopIndex->addIncoming(llvm::ConstantInt::get(TypeOfCopyLen, 0), OrigBB);

         if(AlignCanBeUsed)
            LoopBuilder.CreateAlignedStore(SetValue, LoopBuilder.CreateInBoundsGEP(SetValue->getType(), DstAddr, LoopIndex), Align, IsVolatile);
         else
            LoopBuilder.CreateStore(SetValue, LoopBuilder.CreateInBoundsGEP(SetValue->getType(), DstAddr, LoopIndex), IsVolatile);

         llvm::Value* NewIndex = LoopBuilder.CreateAdd(LoopIndex, llvm::ConstantInt::get(TypeOfCopyLen, 1));
         LoopIndex->addIncoming(NewIndex, LoopBB);

         LoopBuilder.CreateCondBr(LoopBuilder.CreateICmpULT(NewIndex, ActualCopyLen), LoopBB, NewBB);
      }

    public:
      static char ID;

      ExpandMemOpsPass() : ModulePass(ID)
      {
         initializeTargetTransformInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeLoopPassPass(*PassRegistry::getPassRegistry());
      }

      bool runOnModule(Module& M) override
      {
         auto DL = &M.getDataLayout();
         auto res = false;
         auto currFuncIterator = M.getFunctionList().begin();
         while(currFuncIterator != M.getFunctionList().end())
         {
            auto& F = *currFuncIterator;
            const llvm::TargetTransformInfo& TTI = getAnalysis<llvm::TargetTransformInfoWrapperPass>().getTTI(F);
            auto fname = std::string(currFuncIterator->getName());
            llvm::SmallVector<llvm::MemIntrinsic*, 4> MemCalls;
            for(llvm::Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI)
            {
               for(llvm::BasicBlock::iterator II = BI->begin(), IE = BI->end(); II != IE; ++II)
               {
                  if(llvm::MemIntrinsic* InstrCall = dyn_cast<llvm::MemIntrinsic>(II))
                  {
#if PRINT_DBG_MSG
                     llvm::errs() << "Found a memIntrinsic Call\n";
#endif
                     MemCalls.push_back(InstrCall);
#if PRINT_DBG_MSG
                     if(llvm::MemCpyInst* Memcpy = dyn_cast<llvm::MemCpyInst>(InstrCall))
                     {
                        if(llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(Memcpy->getLength()))
                           llvm::errs() << "Found a memcpy with a constant number of iterations\n";
                        else
                           llvm::errs() << "Found a memcpy with an unknown number of iterations\n";
                     }
                     else if(llvm::MemSetInst* Memset = dyn_cast<llvm::MemSetInst>(InstrCall))
                     {
                        llvm::errs() << "Found a memset intrinsic\n";
                     }
#endif
                  }
               }
            }
            // Transform mem* intrinsic calls.
            for(llvm::MemIntrinsic* MemCall : MemCalls)
            {
               bool do_erase;
               do_erase = false;
               if(llvm::MemCpyInst* Memcpy = dyn_cast<llvm::MemCpyInst>(MemCall))
               {
                  if(llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(Memcpy->getLength()))
                  {
                     createMemCpyLoopKnownSizeLocal(Memcpy, Memcpy->getRawSource(), Memcpy->getRawDest(), CI,
#if __clang_major__ > 6
                                                    Memcpy->getSourceAlignment(), Memcpy->getDestAlignment(),
#else
                                                    Memcpy->getAlignment(), Memcpy->getAlignment(),
#endif
                                                    Memcpy->isVolatile(), Memcpy->isVolatile(), DL);
                     do_erase = true;
                  }
#if __clang_major__ != 4
                  else
                  {
                     llvm::expandMemCpyAsLoop(Memcpy, TTI);
                     do_erase = true;
                  }
#endif
               }
               else if(llvm::MemMoveInst* Memmove = dyn_cast<llvm::MemMoveInst>(MemCall))
               {
#if __clang_major__ != 4
                  llvm::expandMemMoveAsLoop(Memmove);
                  do_erase = true;
#endif
               }
               else if(llvm::MemSetInst* Memset = dyn_cast<llvm::MemSetInst>(MemCall))
               {
                  expandMemSetAsLoopLocal(Memset, DL);
                  do_erase = true;
               }
               if(do_erase)
                  MemCall->eraseFromParent();
            }
            ++currFuncIterator;
         }
         return res;
      }

      StringRef getPassName() const override
      {
         return "ExpMemOpsPass";
      }

      void getAnalysisUsage(AnalysisUsage& AU) const override
      {
         AU.addRequired<TargetTransformInfoWrapperPass>();
      }
   };

   llvm::Pass* createExpandMemOpsPass()
   {
      return new ExpandMemOpsPass();
   }

   char ExpandMemOpsPass::ID = 0;

} // namespace llvm

#endif // SCALAR_REPLACEMENT_OF_AGGREGATES_EXPANDMEMOPSPASS_HPP
