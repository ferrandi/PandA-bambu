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
 * @file generate_simulation_scripts.cpp
 * @brief Wrapper used to generate simulation scripts
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

// include class header
#include "generate_simulation_scripts.hpp"

// include Autoheaders
#include "config_headers/config_HAVE_LATTICE.hpp"
#if HAVE_LATTICE
#include "config_headers/config_LATTICE_PMI_MUL.hpp"
#include "config_headers/config_LATTICE_PMI_TDPBE.hpp"
#endif

// include from ./
#include "Parameter.hpp"

/// behavior include
#include "call_graph_manager.hpp"

/// circuit include
#include "structural_manager.hpp"

// include from HLS
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

// include from HLS/simulation
#include "SimulationInformation.hpp"

/// STD include
#include <string>

/// STL include
#include "custom_set.hpp"
#include <list>
#include <tuple>

// include from wrapper/synthesis
#include "BackendFlow.hpp"

// include from wrapper/simulation
#include "SimulationTool.hpp"

// include from tree
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "utility.hpp"

GenerateSimulationScripts::GenerateSimulationScripts(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::GENERATE_SIMULATION_SCRIPT)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

GenerateSimulationScripts::~GenerateSimulationScripts() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> GenerateSimulationScripts::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_HDL, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::TESTBENCH_GENERATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
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
   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top functions");
   const auto top_fun_id = *(top_function_ids.begin());

   const hlsRef top_hls = HLSMgr->get_HLS(top_fun_id);
   const std::string suffix = "_beh";
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Generating simulation scripts");
   std::list<std::string> full_list;
   for(const auto& aux_file : HLSMgr->aux_files)
   {
      full_list.push_back(aux_file);
   }
   for(const auto& hdl_file : HLSMgr->hdl_files)
   {
      full_list.push_back(hdl_file);
   }
#if HAVE_LATTICE
   if(BackendFlow::DetermineBackendFlowType(HLSMgr->get_HLS_target()->get_target_device(), parameters) == BackendFlow::LATTICE_FPGA)
   {
      full_list.push_back(std::string(LATTICE_PMI_TDPBE));
      full_list.push_back(std::string(LATTICE_PMI_MUL));
   }
#endif
   THROW_ASSERT(HLSMgr->RSim->filename_bench != "", "Testbench not yet set");
   full_list.push_back(HLSMgr->RSim->filename_bench);

   if(parameters->getOption<std::string>(OPT_simulator) == "MODELSIM")
   {
      HLSMgr->RSim->sim_tool = SimulationTool::CreateSimulationTool(SimulationTool::MODELSIM, parameters, suffix);
   }
   else if(parameters->getOption<std::string>(OPT_simulator) == "ISIM")
   {
      HLSMgr->RSim->sim_tool = SimulationTool::CreateSimulationTool(SimulationTool::ISIM, parameters, suffix);
   }
   else if(parameters->getOption<std::string>(OPT_simulator) == "XSIM")
   {
      HLSMgr->RSim->sim_tool = SimulationTool::CreateSimulationTool(SimulationTool::XSIM, parameters, suffix);
   }
   else if(parameters->getOption<std::string>(OPT_simulator) == "ICARUS")
   {
      HLSMgr->RSim->sim_tool = SimulationTool::CreateSimulationTool(SimulationTool::ICARUS, parameters, suffix);
   }
   else if(parameters->getOption<std::string>(OPT_simulator) == "VERILATOR")
   {
      HLSMgr->RSim->sim_tool = SimulationTool::CreateSimulationTool(SimulationTool::VERILATOR, parameters, suffix);
   }
   else
   {
      THROW_ERROR("Unknown simulator: " + parameters->getOption<std::string>(OPT_simulator));
   }
   HLSMgr->RSim->sim_tool->GenerateSimulationScript(top_hls->top->get_circ()->get_id(), full_list);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Generated simulation scripts");
   return DesignFlowStep_Status::SUCCESS;
}

bool GenerateSimulationScripts::HasToBeExecuted() const
{
   return true;
}
