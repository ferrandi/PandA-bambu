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