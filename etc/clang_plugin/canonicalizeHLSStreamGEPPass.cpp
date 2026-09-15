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
/*
 * @author Tommaso Fellegara <tommaso.fellegara@polimi.it>
 *
 */
#include "canonicalizeHLSStreamGEPPass.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

using namespace llvm;

char CanonicalizeHLSStreamGEPPass::ID = 0;

namespace
{
   static bool isRedundantHLSStreamGEP(GetElementPtrInst* GEP)
   {
      Type* SourceTy = GEP->getSourceElementType();

      auto* ST = dyn_cast<StructType>(SourceTy);

      if(!ST)
         return false;

      if(!ST->hasName())
         return false;

      if(!ST->getName().contains("hls::stream"))
         return false;

      for(Value* Idx : GEP->indices())
      {
         auto* CI = dyn_cast<ConstantInt>(Idx);

         if(!CI)
            return false;

         if(!CI->isZero())
            return false;
      }

      return true;
   }

} // anonymous namespace

bool CanonicalizeHLSStreamGEPPass::exec(Module& M)
{
   std::vector<GetElementPtrInst*> ToRemove;

   for(Function& F : M)
   {
      for(BasicBlock& BB : F)
      {
         for(Instruction& I : BB)
         {
            auto* GEP = dyn_cast<GetElementPtrInst>(&I);

            if(!GEP)
               continue;

            if(!isRedundantHLSStreamGEP(GEP))
               continue;

            ToRemove.push_back(GEP);
         }
      }
   }

   for(GetElementPtrInst* GEP : ToRemove)
   {
      Value* BasePtr = GEP->getPointerOperand();

      GEP->replaceAllUsesWith(BasePtr);
      GEP->eraseFromParent();
   }

   return !ToRemove.empty();
}

PreservedAnalyses CanonicalizeHLSStreamGEPPass::run(Module& M, ModuleAnalysisManager&)
{
   bool Changed = exec(M);

   return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool CanonicalizeHLSStreamGEPPass::runOnModule(Module& M)
{
   return exec(M);
}

StringRef CanonicalizeHLSStreamGEPPass::getPassName() const
{
   return "Canonicalize HLS Stream GEP";
}

void CanonicalizeHLSStreamGEPPass::getAnalysisUsage(AnalysisUsage& AU) const
{
   AU.setPreservesCFG();
}