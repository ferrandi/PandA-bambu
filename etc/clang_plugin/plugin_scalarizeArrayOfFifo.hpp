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