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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file generate_simulation_scripts.cpp
 * @brief Wrapper used to generate simulation scripts
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "generate_simulation_scripts.hpp"

#include "BackendFlow.hpp"
#include "Parameter.hpp"
#include "SimulationInformation.hpp"
#include "SimulationTool.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "structural_manager.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"
#include "utility.hpp"

#include <list>
#include <string>
#include <tuple>

GenerateSimulationScripts::GenerateSimulationScripts(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                                     const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::GENERATE_SIMULATION_SCRIPT)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

GenerateSimulationScripts::~GenerateSimulationScripts() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
GenerateSimulationScripts::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_HDL, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::TESTBENCH_GENERATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
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

DesignFlowStep_Status GenerateSimulationScripts::Exec()
{
   const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = HLSMgr->get_tree_manager()->GetFunction(top_symbols.front());
   const auto top_hls = HLSMgr->get_HLS(GET_INDEX_CONST_NODE(top_fnode));
   const auto suffix = "_beh";
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Generating simulation scripts");
   std::list<std::string> full_list;
   std::copy(HLSMgr->aux_files.begin(), HLSMgr->aux_files.end(), std::back_inserter(full_list));
   std::copy(HLSMgr->hdl_files.begin(), HLSMgr->hdl_files.end(), std::back_inserter(full_list));
   std::string inc_dirs;
   if(BackendFlow::DetermineBackendFlowType(HLSMgr->get_HLS_device(), parameters) == BackendFlow::LATTICE_FPGA)
   {
      inc_dirs = parameters->getOption<std::string>(OPT_lattice_inc_dirs);
   }
   THROW_ASSERT(HLSMgr->RSim->filename_bench != "", "Testbench not yet set");
   full_list.push_back(HLSMgr->RSim->filename_bench);

   HLSMgr->RSim->sim_tool = SimulationTool::CreateSimulationTool(
       SimulationTool::to_sim_type(parameters->getOption<std::string>(OPT_simulator)), parameters, suffix,
       HLSMgr->CGetFunctionBehavior(GET_INDEX_CONST_NODE(top_fnode))->CGetBehavioralHelper()->GetMangledFunctionName(),
       inc_dirs);

   HLSMgr->RSim->sim_tool->GenerateSimulationScript(top_hls->top->get_circ()->get_id(), full_list);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Generated simulation scripts");
   return DesignFlowStep_Status::SUCCESS;
}

bool GenerateSimulationScripts::HasToBeExecuted() const
{
   return true;
}
