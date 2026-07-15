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
#ifndef CANONICALIZE_HLS_STREAM_GEP_PASS_HPP
#define CANONICALIZE_HLS_STREAM_GEP_PASS_HPP

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm
{
   struct CanonicalizeHLSStreamGEPPass : public ModulePass, public PassInfoMixin<CanonicalizeHLSStreamGEPPass>
   {
    public:
      static char ID;

      CanonicalizeHLSStreamGEPPass() : ModulePass(ID)
      {
      }

      CanonicalizeHLSStreamGEPPass(const CanonicalizeHLSStreamGEPPass&) : CanonicalizeHLSStreamGEPPass()
      {
      }

      /// Shared implementation.
      bool exec(Module& M);

      /// New PM entry-point.
      PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);

      /// Legacy PM entry-point.
      bool runOnModule(Module& M) override;

      StringRef getPassName() const override;

      void getAnalysisUsage(AnalysisUsage& AU) const override;
   };

} // namespace llvm

#endif // CANONICALIZE_HLS_STREAM_GEP_PASS_HPP