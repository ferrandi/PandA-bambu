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
#ifndef BAMBU_SCALARIZE_FIFO_ARRAY_PASS_HPP
#define BAMBU_SCALARIZE_FIFO_ARRAY_PASS_HPP

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm
{
   /// Replaces every  alloca [1 x hls::stream<T>]  with a plain
   /// alloca %hls::stream<T>  and removes the corresponding no-op GEPs,
   /// both on the alloca itself and on function parameters that were
   /// updated at their call sites.
   ///
   /// Requires LLVM 15+ (opaque pointers).
   struct ScalarizeFifoArrayPass : public ModulePass, public PassInfoMixin<ScalarizeFifoArrayPass>
   {
    public:
      static char ID;

      ScalarizeFifoArrayPass() : ModulePass(ID)
      {
      }

      ScalarizeFifoArrayPass(const ScalarizeFifoArrayPass&) : ScalarizeFifoArrayPass()
      {
      }

      /// Shared implementation called by both PM entry-points.
      bool exec(Module& M);

      /// New Pass Manager entry-point (LLVM 15+).
      PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);

      /// Legacy Pass Manager entry-point.
      bool runOnModule(Module& M) override;

      StringRef getPassName() const override;

      void getAnalysisUsage(AnalysisUsage& AU) const override;
   };

} // namespace llvm

#endif // BAMBU_SCALARIZE_FIFO_ARRAY_PASS_HPP