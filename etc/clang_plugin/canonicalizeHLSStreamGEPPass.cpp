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
 *                Copyright (C) 2025-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
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