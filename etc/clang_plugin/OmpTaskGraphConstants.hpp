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
#include "panda_clang_compat.hpp"

/**
 * @file OmpTaskGraphConstants.hpp
 *
 * @author Kaixi Matteo Chen <kaiximatteo.chen@mail.polimi.it>
 *
 */

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#if PANDA_LLVM_CLANG_MAJOR >= 13

#include "OpenMPTaskPassConfig.hpp"

#include <cstdint>
#include <string>

#define NA_VALUE INT32_MAX

namespace Bambu
{
   namespace OpenMP
   {
      namespace Task
      {
         namespace Constants
         {
#ifdef SYSTEM_32_BIT
            // Bambu works with 32 bit
            const uint64_t structAlignment = 4U;
#else
            const uint64_t structAlignment = 8U;
#endif

            const std::string FAILURE_MSG = "[FAILURE]";
            const std::string INFO_MSG = "[INFO]";

            const std::string bambuHostReservedFnName = "bambu_host";
            const std::string llvmIndentName = "llvm.ident";
            const std::string llmvLoopUnrollMetadataNamePrefix = "llvm.loop.unroll";
            const std::string llmvLoopUnrollFullMetadata = "llvm.loop.unroll.full";
            const std::string llvmVersionPrefix = "clang version ";
            const std::string supportedLlvmVersion = "19";
            const std::string targetOmpTaskAllocName = "__kmpc_omp_task_alloc";
            const std::string targetOmpBarrierName = "__kmpc_barrier";
            const std::string targetOmpEndSingle = "__kmpc_end_single";
            const std::string targetOmpSingle = "__kmpc_single";
            const std::string targetOmpTaskSubmissionDepsName = "__kmpc_omp_task_with_deps";
            const std::string targetOmpTaskSubmissionName = "__kmpc_omp_task";
            const std::string targetOmpDepsStructName = "struct.kmp_depend_info";
            const std::string targetOmpAnonStructName = "struct.anon";
            const std::string targetOmpTaskTWithPrivatesStructName = "struct.kmp_task_t_with_privates";
            const std::string targetOmpOutlinedFnNamePrefix = ".omp_outlined";
            const std::string targetTaskEntryNamePrefix = ".omp_task_entry";
            const std::string targetTransformedTaskEntryNameSuffix = "transformed";
            const std::string targetOmpForkCall = "__kmpc_fork_call";
            const std::string outXMLFileName = "task_graph.xml";
            const std::string injectedTaskNamePreifx = "injected_task_";
            const std::string taskGraphName = "task_graph";

            constexpr uint8_t OPEN_FLAG = 0;
            constexpr uint8_t CLOSE_FLAG_WITH_DEPS = 1;
            constexpr uint8_t CLOSE_FLAG = 2;

         }; // namespace Constants

      } // namespace Task
   } // namespace OpenMP
} // namespace Bambu

#endif

#endif
