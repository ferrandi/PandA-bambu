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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
 * @file basic_blocks_profiling.cpp
 * @brief Analysis step performing a dynamic profiling of basic blocks execution
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "basic_blocks_profiling.hpp"

#include "BackendWrapper.hpp"
#include "CompilerWrapper.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "custom_set.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "fileIO.hpp"
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "hls_device.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "host_profiling_constants.hpp"
#include "profiling_information.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"

#include <string>
#include <utility>
#include <vector>

BasicBlocksProfiling::BasicBlocksProfiling(const application_managerRef _AppM,
                                           const DesignFlowManager& _design_flow_manager,
                                           const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, BASIC_BLOCKS_PROFILING, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
BasicBlocksProfiling::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType) const
{
   return {};
}

void BasicBlocksProfiling::ComputeRelationships(DesignFlowStepSet& relationship,
                                                const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == DEPENDENCE_RELATIONSHIP)
   {
      const auto hls_factory =
          GetPointer<const HLSFlowStepFactory>(design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::HLS));
      relationship.insert(hls_factory->CreateHLSFlowStep(HLSFlowStep_Type::TESTBENCH_GENERATION));
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

void BasicBlocksProfiling::Initialize()
{
   const auto functions = AppM->CGetCallGraphManager().GetReachedBodyFunctions();
   for(const auto function : functions)
   {
      AppM->GetFunctionBehavior(function)->profiling_information->Clear();
   }
}

DesignFlowStep_Status BasicBlocksProfiling::Exec()
{
   const auto HLSMgr = std::dynamic_pointer_cast<HLS_manager>(AppM);
   BackendWrapper sim(parameters, HLSMgr->get_HLS_device(),
                      {"simulation/" + boost::to_lower_copy(parameters->getOption<std::string>(OPT_simulator))});
   sim.init(HLSMgr);
   sim.run();

   const auto bb_profile_filename =
       parameters->getOption<std::filesystem::path>(OPT_output_temporary_directory) / STR_CST_host_profiling_data;

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Parse profiling data");
   std::ifstream profilefile(bb_profile_filename);
   if(profilefile.is_open())
   {
      std::string line;
      while(!profilefile.eof())
      {
         getline(profilefile, line);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Read " + line);
         auto tokens = string_to_container<std::vector<std::string>>(line, " ");
         if(tokens.front() == "RUN")
         {
            // Nothing to do
         }
         else if(tokens.front() == "FUNC")
         {
            const auto fid = static_cast<unsigned>(std::stoull(tokens[1]));
            const auto function_behavior = AppM->CGetFunctionBehavior(fid);
            const auto& bb_index_map =
                function_behavior->GetBBGraph(FunctionBehavior::FBB).CGetGraphInfo().bb_index_map;
            auto& bb_executions = function_behavior->profiling_information->bb_executions;
            unsigned bbi = 0;
            for(auto it = std::next(tokens.begin(), 2); it != tokens.end(); ++it)
            {
               const auto run_execs = std::stoull(*it);
               const auto bbv_it = bb_index_map.find(bbi++);
               if(bbv_it != bb_index_map.end())
               {
                  auto bb_execs_it = bb_executions.find(bbv_it->second);
                  if(bb_execs_it != bb_executions.end())
                  {
                     bb_execs_it->second += run_execs;
                  }
                  else
                  {
                     bb_executions.emplace(bbv_it->second, run_execs);
                  }
               }
               else
               {
                  THROW_ASSERT(run_execs == 0, "Executions profiling data on unknown basic block for " +
                                                   function_behavior->CGetBehavioralHelper()->GetFunctionName());
               }
            }
         }
         else
         {
            THROW_ERROR("Unknown profiling data class: " + tokens.front());
         }
      }
      profilefile.close();
   }
   else
   {
      THROW_ERROR_CODE(PROFILING_EC, "Error during opening of profile data file " + bb_profile_filename.string());
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--Parse profiling data completed");

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      const auto functions = AppM->CGetCallGraphManager().GetReachedBodyFunctions();
      for(const auto function : functions)
      {
         const auto FB = AppM->CGetFunctionBehavior(function);
         FB->GetBBGraph(FunctionBehavior::FBB).writeDot(FB->GetDotPath() / "BB_profiling.dot");
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}
