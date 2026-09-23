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
/**
 * @file loops.cpp
 * @brief Loops data structure specialization.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "loops.hpp"

#include "profiling_information.hpp"

namespace detail
{
   template <>
   inline void appendProfilingInfo<BBGraph>(std::ostream& os,
                                            const refcount<const ProfilingInformation>& profiling_information,
                                            const LoopConstRef& loop)
   {
#if HAVE_HOST_PROFILING_BUILT
      if(profiling_information)
      {
         os << "\\nAvg. Iterations=" << profiling_information->GetLoopAvgIterations(loop)
            << "- Max Iterations=" << profiling_information->GetLoopMaxIterations(loop->getLoopId());
      }
#endif
   }
} // namespace detail

template class LoopsTemplate<BBGraph, BBGraphTraits>;
