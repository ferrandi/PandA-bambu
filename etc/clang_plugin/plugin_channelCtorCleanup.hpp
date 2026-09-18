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
#ifndef BAMBU_CHANNEL_CTOR_CLEANUP_PASS_HPP
#define BAMBU_CHANNEL_CTOR_CLEANUP_PASS_HPP

#include "panda_clang_compat.hpp"

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm
{
   /// Removes the software construction of ac_channel<T> / hls::stream<T> objects from the kernel IR.
   ///
   /// The unified ac_channel.h holds a std::deque<T> unconditionally, so a channel constructed inside
   /// the synthesised call graph drags operator new / operator delete - and the deque implementation
   /// behind them - into the design, where bambu has no functional unit to map them onto. The object
   /// itself is left alone: that is what lets the very same .ll run on the CPU. Only the construction
   /// is stripped, and only in bambu's own pipeline.
   ///
   /// Must run before the inliner, where a constructor is still a single call rather than a spread of
   /// GEPs and stores.
   struct ChannelCtorCleanupPass : public ModulePass, public PassInfoMixin<ChannelCtorCleanupPass>
   {
    public:
      static char ID;

      ChannelCtorCleanupPass() : ModulePass(ID)
      {
      }

      ChannelCtorCleanupPass(const ChannelCtorCleanupPass&) : ChannelCtorCleanupPass()
      {
      }

      /// Shared implementation called by both PM entry-points.
      bool exec(Module& M);

      /// New Pass Manager entry-point.
      PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);

      /// Legacy Pass Manager entry-point.
      bool runOnModule(Module& M) override;

      StringRef getPassName() const override;
   };
} // namespace llvm

#endif // BAMBU_CHANNEL_CTOR_CLEANUP_PASS_HPP
