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
 *              Copyright (C) 2015-2024 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
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
/**
 * @file basic_blocks_profiling.cpp
 * @brief Analysis step performing a dynamic profiling of basic blocks execution
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "basic_blocks_profiling.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "c_backend.hpp"
#include "c_backend_information.hpp"
#include "c_backend_step_factory.hpp"
#include "call_graph_manager.hpp"
#include "compiler_wrapper.hpp"
#include "custom_set.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "fileIO.hpp"
#include "function_behavior.hpp"
#include "hash_helper.hpp"
#include "hls_step.hpp"
#include "host_profiling_constants.hpp"
#include "profiling_information.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"

#include <cerrno>
#include <string>
#include <utility>
#include <vector>

BasicBlocksProfiling::BasicBlocksProfiling(const application_managerRef _AppM,
                                           const DesignFlowManagerConstRef _design_flow_manager,
                                           const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, BASIC_BLOCKS_PROFILING, _design_flow_manager, _parameters),
      profiling_source_file(parameters->getOption<std::string>(OPT_output_temporary_directory) + "/host_profiling.c")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
BasicBlocksProfiling::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   return relationships;
}

void BasicBlocksProfiling::ComputeRelationships(DesignFlowStepSet& relationship,
                                                const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == DEPENDENCE_RELATIONSHIP)
   {
      const auto c_backend_factory = GetPointer<const CBackendStepFactory>(
          design_flow_manager.lock()->CGetDesignFlowStepFactory(DesignFlowStep::C_BACKEND));
      relationship.insert(c_backend_factory->CreateCBackendStep(
          CBackendInformationConstRef(new CBackendInformation(CBackendInformation::CB_BBP, profiling_source_file))));
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

void BasicBlocksProfiling::Initialize()
{
   const auto functions = AppM->CGetCallGraphManager()->GetReachedBodyFunctions();
   for(const auto function : functions)
   {
      AppM->GetFunctionBehavior(function)->profiling_information->Clear();
   }
}

DesignFlowStep_Status BasicBlocksProfiling::Exec()
{
   const auto temporary_path = parameters->getOption<std::filesystem::path>(OPT_output_temporary_directory);

   const auto run_name = temporary_path / "run.tmp";
   const auto profile_data_name = temporary_path / STR_CST_host_profiling_data;

   const CompilerWrapperConstRef compiler_wrapper(
       new CompilerWrapper(parameters, parameters->getOption<CompilerWrapper_CompilerTarget>(OPT_host_compiler),
                           CompilerWrapper_OptimizationSet::O1));
   CustomSet<std::string> tp_files;
   tp_files.insert(profiling_source_file);
   compiler_wrapper->CreateExecutable(tp_files, run_name.string(), "");
   std::string change_directory;
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Starting dynamic profiling");
   if(parameters->isOption(OPT_path))
   {
      change_directory = "cd \"" + parameters->getOption<std::string>(OPT_path) + "\" && ";
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---Changing working directory to " + parameters->getOption<std::string>(OPT_path));
   }
   const auto exec_argvs = parameters->getOption<CustomSet<std::string>>(OPT_exec_argv);
   for(const auto& exec_argv : exec_argvs)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Running with parameters: " + exec_argv);
      // The argument
      std::filesystem::remove(profile_data_name);

      const auto command = change_directory + "\"" + run_name.string() + "\" " + exec_argv + " ";
      const auto ret = PandaSystem(parameters, command, false, temporary_path / STR_CST_host_profiling_output);
      if(IsError(ret))
      {
         if(errno && !parameters->getOption<bool>(OPT_no_return_zero))
         {
            THROW_ERROR_CODE(PROFILING_EC, "Error " + std::string(strerror(errno)) + " during dynamic profiling");
         }
      }

      std::ifstream profilefile(profile_data_name);
      if(profilefile.is_open())
      {
         std::string line;
         ProfilingInformationRef profiling_information;
         decltype(BBGraphInfo::bb_index_map) bb_index_map;
         while(!profilefile.eof())
         {
            getline(profilefile, line);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Read " + line);
            if(line.size())
            {
               const auto splitted = string_to_container<std::vector<std::string>>(line, " ");
               THROW_ASSERT(splitted.size() == 2, line);
               if(line.find("Function") != std::string::npos)
               {
                  const auto function_behavior =
                      AppM->CGetFunctionBehavior(static_cast<unsigned>(std::stoul(splitted[1])));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Found data of function " +
                                     function_behavior->CGetBehavioralHelper()->get_function_name());
                  profiling_information = function_behavior->profiling_information;
                  bb_index_map = function_behavior->CGetBBGraph(FunctionBehavior::FBB)->CGetBBGraphInfo()->bb_index_map;
               }
               else
               {
                  const auto bb_index = static_cast<unsigned>(std::stoul(splitted[0]));
                  if(bb_index_map.find(bb_index) != bb_index_map.end())
                  {
                     const auto bb_vertex = bb_index_map.find(bb_index)->second;
                     profiling_information->bb_executions[bb_vertex] = std::stoull(splitted[1]);
                  }
                  else
                  {
                     THROW_ASSERT(splitted[1] == "0", splitted[1]);
                  }
               }
            }
         }
         profilefile.close();
      }
      else
      {
         THROW_ERROR_CODE(PROFILING_EC, "Error during opening of profile data file " + profile_data_name.string());
      }
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--Ended dynamic profiling");

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      const auto functions = AppM->CGetCallGraphManager()->GetReachedBodyFunctions();
      for(const auto function : functions)
      {
         AppM->CGetFunctionBehavior(function)->CGetBBGraph(FunctionBehavior::FBB)->WriteDot("BB_profiling.dot");
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}
