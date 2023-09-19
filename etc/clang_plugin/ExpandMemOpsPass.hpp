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
 *              Copyright (C) 2018-2023 Politecnico di Milano
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
 * @file ExpandMemOpsPass.hpp
 * @brief This pass expand memcpy, memset and memmove.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 */

#ifndef EXPANDMEMOPSPASS_HPP
#define EXPANDMEMOPSPASS_HPP

#include "clang_version_symbol.hpp"

#include <llvm/ADT/STLExtras.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/Pass.h>

#if __clang_major__ != 4
#include <llvm/Transforms/Utils/LowerMemIntrinsics.h>
#endif
#if __clang_major__ >= 13
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#endif

#ifndef __clang_major__
#define __clang_major__ 4
#endif

namespace llvm
{
   class CLANG_VERSION_SYMBOL(_plugin_expandMemOps)
       : public ModulePass
#if __clang_major__ >= 13
         ,
         public PassInfoMixin<CLANG_VERSION_SYMBOL(_plugin_expandMemOps)>
#endif
   {
    private:
      bool addrIsOfIntArrayType(llvm::Value* DstAddr, unsigned& Align, const llvm::DataLayout* DL);

      unsigned getLoopOperandSizeInBytesLocal(llvm::Type* Type);

      llvm::Type* getMemcpyLoopLoweringTypeLocal(llvm::LLVMContext& Context, llvm::ConstantInt* Length,
                                                 unsigned SrcAlign, unsigned DestAlign, llvm::Value* SrcAddr,
                                                 llvm::Value* DstAddr, const llvm::DataLayout* DL, bool isVolatile,
                                                 bool& Optimize);

      void getMemcpyLoopResidualLoweringTypeLocal(llvm::SmallVectorImpl<llvm::Type*>& OpsOut,
                                                  llvm::LLVMContext& Context, unsigned RemainingBytes,
                                                  unsigned SrcAlign, unsigned DestAlign);

      void createMemCpyLoopKnownSizeLocal(llvm::Instruction* InsertBefore, llvm::Value* SrcAddr, llvm::Value* DstAddr,
                                          llvm::ConstantInt* CopyLen, unsigned SrcAlign, unsigned DestAlign,
                                          bool SrcIsVolatile, bool DstIsVolatile, const llvm::DataLayout* DL);

      void expandMemSetAsLoopLocal(llvm::MemSetInst* Memset, const llvm::DataLayout* DL);

    public:
      static char ID;

      CLANG_VERSION_SYMBOL(_plugin_expandMemOps)();

#if __clang_major__ >= 13
      CLANG_VERSION_SYMBOL(_plugin_expandMemOps)(const CLANG_VERSION_SYMBOL(_plugin_expandMemOps) &);
#endif

      bool exec(Module& M, llvm::function_ref<llvm::TargetTransformInfo&(llvm::Function&)> GetTTI
#if __clang_major__ >= 16
                ,
                llvm::function_ref<llvm::ScalarEvolution&(llvm::Function&)> GetSE
#endif
      );

      bool runOnModule(Module& M) override;

      StringRef getPassName() const override;

      void getAnalysisUsage(AnalysisUsage& AU) const override;

#if __clang_major__ >= 13
      llvm::PreservedAnalyses run(llvm::Module& M, llvm::ModuleAnalysisManager& MAM);
#endif
   };

} // namespace llvm

#endif // SCALAR_REPLACEMENT_OF_AGGREGATES_EXPANDMEMOPSPASS_HPP
