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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 */
/**
 * @file host_profiling.cpp
 * @brief Analysis step performing profiling of loops, paths or both
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "host_profiling.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "hash_helper.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "profiling_information.hpp"
#include "string_manipulation.hpp"

#include <cerrno>
#include <filesystem>
#include <unistd.h>

HostProfiling_Method operator&(const HostProfiling_Method first, const HostProfiling_Method second)
{
   return static_cast<HostProfiling_Method>(static_cast<int>(first) | static_cast<int>(second));
}

HostProfiling::HostProfiling(const application_managerRef _AppM, const DesignFlowManager& _design_flow_manager,
                             const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, HOST_PROFILING, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
HostProfiling::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         const HostProfiling_Method profiling_method =
             parameters->getOption<HostProfiling_Method>(OPT_profiling_method);
         THROW_ASSERT(profiling_method != HostProfiling_Method::PM_NONE,
                      "Host profiilng required but algorithm has not been selected");
         if(static_cast<int>(profiling_method) & static_cast<int>(HostProfiling_Method::PM_BBP))
         {
            relationships.insert(std::make_pair(BASIC_BLOCKS_PROFILING, WHOLE_APPLICATION));
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status HostProfiling::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}

void HostProfiling::normalize(
    const application_managerRef AppM,
    const CustomUnorderedMap<unsigned int, CustomUnorderedMapStable<unsigned int, unsigned long long>>& loop_instances,
    const ParameterConstRef parameters)
{
#ifndef NDEBUG
   const int debug_level = parameters->get_class_debug_level("HostProfiling");
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Normalizing loop iteration");
   // Iterating over all functions with body
   for(const auto f : AppM->get_functions_with_body())
   {
      // Normalizing basic block execution time
      // First computing number of execution of the function
      // number of function execution
      const FunctionBehaviorRef FB = AppM->GetFunctionBehavior(f);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Function: " + FB->CGetBehavioralHelper()->GetFunctionName());

      // Normalizing loop number of iteration and frequency
      const auto& loops = FB->getConstLoops()->getList();
      for(const auto& loop : loops)
      {
         unsigned int loop_id = loop->getLoopId();
         /// FIXME: zero loop
         if(loop_id == 0)
         {
            continue;
         }
         long double avg_number = 0.0L;
         long double abs_execution = 0.0L;
         PathProfilingInformation& path_profiling = FB->profiling_information->path_profiling;
         if(path_profiling.find(loop_id) == path_profiling.end())
         {
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Loop: " + std::to_string(loop_id));
         const auto& elements = path_profiling.find(loop_id)->second;
         for(const auto& element : elements)
         {
            abs_execution += element.second;
         }
         if(abs_execution != 0.0L)
         {
            if(loop_instances.find(f) == loop_instances.end())
            {
               THROW_ERROR_CODE(PROFILING_EC,
                                "Function " + FB->CGetBehavioralHelper()->GetFunctionName() + " exited abnormally");
            }
            THROW_ASSERT(loop_instances.at(f).find(loop_id) != loop_instances.at(f).end(),
                         "Loop " + std::to_string(f) + " is no executed");
            THROW_ASSERT(loop_instances.at(f).at(loop_id) != 0,
                         "Loop " + std::to_string(loop_id) + " of function " + std::to_string(f) +
                             " is executed but does not exist an external path with its header");
            avg_number = abs_execution / loop_instances.at(f).at(loop_id);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Avg. Number Executions: " + std::to_string(avg_number));
         FB->profiling_information->avg_iterations[loop->getLoopId()] = avg_number;
         FB->profiling_information->abs_iterations[loop->getLoopId()] =
             static_cast<unsigned long long int>(llroundl(abs_execution));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Factor: " + std::to_string(abs_execution));
         for(auto& k : path_profiling.at(loop_id))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->New path");
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Absolute path: " + std::to_string(k.second));
            k.second /= abs_execution;
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Relative path: " + std::to_string(k.second));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      if(parameters->getOption<bool>(OPT_print_dot))
      {
         FB->getConstLoops()->writeDot(FB->GetDotPath() / "LF.dot", FB->CGetProfilingInformation());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Normalized loop iteration");
}
