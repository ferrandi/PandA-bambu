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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file simulation_evaluation.cpp
 * @brief .
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "simulation_evaluation.hpp"

///. include
#include "Parameter.hpp"

/// HLS include
#include "hls_manager.hpp"

// include from HLS/simulation
#include "SimulationInformation.hpp"

/// STD include
#include <string>

/// STL include
#include "custom_set.hpp"
#include <tuple>
#include <vector>

/// technology/physical_library/models includes
#include "time_model.hpp"

// include from wrapper/simulation
#include "SimulationTool.hpp"

/// utility includes
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "utility.hpp"

SimulationEvaluation::SimulationEvaluation(const ParameterConstRef _Param, const HLS_managerRef _hls_mgr, const DesignFlowManagerConstRef _design_flow_manager)
    : EvaluationBaseStep(_Param, _hls_mgr, 0, _design_flow_manager, HLSFlowStep_Type::SIMULATION_EVALUATION), already_executed(false)
{
   debug_level = _Param->get_class_debug_level(GET_CLASS(*this));
}

SimulationEvaluation::~SimulationEvaluation() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> SimulationEvaluation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_HDL, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_SIMULATION_SCRIPT, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::TESTBENCH_GENERATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

DesignFlowStep_Status SimulationEvaluation::InternalExec()
{
   THROW_ASSERT(not already_executed, "simulation cannot be executed multiple times!");

   HLSMgr->RSim->sim_tool->CheckExecution();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Executing simulation");
   HLSMgr->RSim->avg_n_cycles = HLSMgr->RSim->sim_tool->Simulate(HLSMgr->RSim->tot_n_cycles, HLSMgr->RSim->n_testcases);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Executed simulation");
   if(not parameters->isOption(OPT_no_clean) and not parameters->getOption<bool>(OPT_no_clean))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Cleaning up simulation files");
      HLSMgr->RSim->sim_tool->Clean();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Cleaned up simulation files");
   }
   std::string objective_string = parameters->getOption<std::string>(OPT_evaluation_objectives);
   std::vector<std::string> objective_vector = convert_string_to_vector<std::string>(objective_string, ",");
   for(const auto& objective : objective_vector)
   {
      if(objective == "CYCLES" or objective == "TIME" or objective == "TOTAL_CYCLES" or objective == "TOTAL_TIME" or objective == "TIMExAREA")
      {
         unsigned long long int tot_cycles = HLSMgr->RSim->tot_n_cycles;
         unsigned long long int avg_cycles = HLSMgr->RSim->avg_n_cycles;
         const auto num_executions = HLSMgr->RSim->n_testcases;
         HLSMgr->evaluations["TOTAL_CYCLES"] = std::vector<double>(1, static_cast<double>(tot_cycles));
         HLSMgr->evaluations["CYCLES"] = std::vector<double>(1, static_cast<double>(avg_cycles));
         HLSMgr->evaluations["NUM_EXECUTIONS"] = std::vector<double>(1, static_cast<double>(num_executions));
      }
   }

   return DesignFlowStep_Status::SUCCESS;
}
