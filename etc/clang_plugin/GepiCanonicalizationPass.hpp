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
 *              Copyright (C) 2019 Politecnico di Milano
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
//
// Created by Marco Siracusa on 7/27/19.
//

#ifndef SCALAR_REPLACEMENT_OF_AGGREGATES_GEPICANONICALIZATIONPASS_HPP
#define SCALAR_REPLACEMENT_OF_AGGREGATES_GEPICANONICALIZATIONPASS_HPP

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Pass.h>

enum SROA_optimizations
{
   SROA_cleanLCSSA,
   SROA_gepiExplicitation,
   SROA_codeSimplification,
   SROA_ptrIteratorSimplification,
   SROA_chunkOperationsLowering,
   SROA_bitcastVectorRemoval,
   SROA_removeLifetime,
   SROA_selectLowering,
   SROA_canonicalIdxs,
   SROA_removeMeta
};

static const char* optimization_names[] = {"LCSSA cleanup",
                                           "GEPOP to GEPI",
                                           "Force unroll",
                                           "Pointer Iterator Simplification"
                                           "Chunk operations lowering",
                                           "Bitcast vector removal",
                                           "Remove lifetime intrinsic",
                                           "Select lowering",
                                           "Canonical indexes",
                                           "Remove metadata"};

class GepiCanonicalizationPass : public llvm::FunctionPass
{
 public:
   char ID = 0;

 private:
   unsigned short optimization_selection = 0;

 public:
   explicit GepiCanonicalizationPass(char& _ID, unsigned short optimization_selection) : llvm::FunctionPass(_ID), optimization_selection(optimization_selection)
   {
   }
   explicit GepiCanonicalizationPass(unsigned short optimization_selection) : llvm::FunctionPass(ID), optimization_selection(optimization_selection)
   {
   }

   bool runOnFunction(llvm::Function& function) override;

   void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
   {
      AU.setPreservesCFG();
      AU.addRequiredTransitive<llvm::LoopInfoWrapperPass>();
      AU.addRequiredTransitive<llvm::ScalarEvolutionWrapperPass>();
   }
};

GepiCanonicalizationPass* createCleanLCSSA();

GepiCanonicalizationPass* createGepiExplicitation();

GepiCanonicalizationPass* createCodeSimplificationPass();

GepiCanonicalizationPass* createPtrIteratorSimplificationPass();

GepiCanonicalizationPass* createChunkOperationsLoweringPass();

GepiCanonicalizationPass* createBitcastVectorRemovalPass();

GepiCanonicalizationPass* createRemoveIntrinsicPass();

GepiCanonicalizationPass* createSelectLoweringPass();

GepiCanonicalizationPass* createGepiCanonicalIdxsPass();

GepiCanonicalizationPass* createRemoveMetaPass();

#endif // SCALAR_REPLACEMENT_OF_AGGREGATES_GEPICANONICALIZATIONPASS_HPP
