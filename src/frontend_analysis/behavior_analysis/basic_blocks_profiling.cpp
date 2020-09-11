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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
/// Header include
#include "basic_blocks_profiling.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "c_backend.hpp"
#include "c_backend_step_factory.hpp"
#include "call_graph_manager.hpp"
#include "custom_set.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "fileIO.hpp"
#include "function_behavior.hpp"
#include "gcc_wrapper.hpp"
#include "hash_helper.hpp"
#include "host_profiling_constants.hpp"
#include "profiling_information.hpp"
#include "string_manipulation.hpp"

/// STD include
#include <string>
#include <utility>
#include <vector>

BasicBlocksProfiling::BasicBlocksProfiling(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, BASIC_BLOCKS_PROFILING, _design_flow_manager, _parameters), profiling_source_file(parameters->getOption<std::string>(OPT_output_temporary_directory) + "/host_profiling.c")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

BasicBlocksProfiling::~BasicBlocksProfiling() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> BasicBlocksProfiling::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   return relationships;
}

DesignFlowStep_Status BasicBlocksProfiling::Exec()
{
   boost::filesystem::path temporary_path(parameters->getOption<std::string>(OPT_output_temporary_directory));

   boost::filesystem::path run_name = temporary_path / ("run.tmp");
   boost::filesystem::path profile_data_name = temporary_path / STR_CST_host_profiling_data;

   const GccWrapperConstRef gcc_wrapper(new GccWrapper(this->parameters, parameters->getOption<GccWrapper_CompilerTarget>(OPT_host_compiler), GccWrapper_OptimizationSet::O1));
   CustomSet<std::string> tp_files;
   tp_files.insert(profiling_source_file);
   gcc_wrapper->CreateExecutable(tp_files, run_name.string(), "");
   std::string change_directory;
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Starting dynamic profiling");
   if(parameters->isOption(OPT_path))
   {
      change_directory = "cd \"" + parameters->getOption<std::string>(OPT_path) + "\" && ";
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Changing working directory to " + parameters->getOption<std::string>(OPT_path));
   }
   const auto exec_argvs = parameters->getOption<const CustomSet<std::string>>(OPT_exec_argv);
   for(const auto& exec_argv : exec_argvs)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Running with parameters: " + exec_argv);
      // The argument
      boost::filesystem::remove(profile_data_name);

      const auto command = change_directory + "\"" + run_name.string() + "\" " + exec_argv + " ";
      const auto ret = PandaSystem(parameters, command, temporary_path.string() + STR_CST_host_profiling_output);
      if(IsError(ret))
      {
         if(errno and not parameters->getOption<bool>(OPT_no_return_zero))
         {
            THROW_ERROR_CODE(PROFILING_EC, "Error " + boost::lexical_cast<std::string>(errno) + " during dynamic profiling");
         }
      }

      std::ifstream profilefile(profile_data_name.string().c_str());
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
               std::vector<std::string> splitted = SplitString(line, " ");
               THROW_ASSERT(splitted.size() == 2, line);
               if(line.find("Function") != std::string::npos)
               {
                  const auto function_behavior = AppM->CGetFunctionBehavior(boost::lexical_cast<unsigned int>(splitted[1]));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found data of function " + function_behavior->CGetBehavioralHelper()->get_function_name());
                  profiling_information = function_behavior->profiling_information;
                  bb_index_map = function_behavior->CGetBBGraph(FunctionBehavior::FBB)->CGetBBGraphInfo()->bb_index_map;
               }
               else
               {
                  const auto bb_index = boost::lexical_cast<unsigned int>(splitted[0]);
                  if(bb_index_map.find(bb_index) != bb_index_map.end())
                  {
                     const auto bb_vertex = bb_index_map.find(bb_index)->second;
                     profiling_information->bb_executions[bb_vertex] = boost::lexical_cast<unsigned long long int>(splitted[1]);
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
      CustomOrderedSet<unsigned int> functions = AppM->CGetCallGraphManager()->GetReachedBodyFunctions();
      for(const auto function : functions)
      {
         AppM->CGetFunctionBehavior(function)->CGetBBGraph(FunctionBehavior::FBB)->WriteDot("BB_profiling.dot");
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}

void BasicBlocksProfiling::Initialize()
{
   CustomOrderedSet<unsigned int> functions = AppM->CGetCallGraphManager()->GetReachedBodyFunctions();
   for(auto function : functions)
   {
      AppM->GetFunctionBehavior(function)->profiling_information->Clear();
   }
}

void BasicBlocksProfiling::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == DEPENDENCE_RELATIONSHIP)
   {
      vertex backend_step = design_flow_manager.lock()->GetDesignFlowStep(CBackend::ComputeSignature(CBackend::CB_BBP));
      const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
      const auto design_flow_step = backend_step != NULL_VERTEX ?
                                        design_flow_graph->CGetDesignFlowStepInfo(backend_step)->design_flow_step :
                                        GetPointer<const CBackendStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("CBackend"))->CreateCBackendStep(CBackend::CB_BBP, profiling_source_file, CBackendInformationConstRef());
      relationship.insert(design_flow_step);
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}
