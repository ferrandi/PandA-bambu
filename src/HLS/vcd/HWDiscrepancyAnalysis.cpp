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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

// include class header
#include "HWDiscrepancyAnalysis.hpp"

// include from ./
#include "Parameter.hpp"

// include from behavior/
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

// include from design_flows/
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

// includes from design_flows/backend/ToC/
#include "c_backend_step_factory.hpp"
#include "hls_c_backend_information.hpp"

// includes from design_flows/backend/ToHDL
#include "language_writer.hpp"

// include from frontend_analysis/
#include "frontend_flow_step_factory.hpp"
#include "application_frontend_flow_step.hpp"

// include from HLS/
#include "hls_manager.hpp"

// include from tree/
#include "behavioral_helper.hpp"

HWDiscrepancyAnalysis::HWDiscrepancyAnalysis(
      const ParameterConstRef _parameters,
      const HLS_managerRef _HLSMgr,
      const DesignFlowManagerConstRef _design_flow_manager)
   : HLS_step(
         _parameters,
         _HLSMgr,
         _design_flow_manager,
         HLSFlowStep_Type::VCD_UTILITY)
   , Discr(_HLSMgr->RDiscr)
   , present_state_name(
         static_cast<HDLWriter_Language>(_parameters->getOption<unsigned int>(OPT_writer_language)) == HDLWriter_Language::VERILOG ?
         "_present_state" :
         "present_state")
{}

HWDiscrepancyAnalysis::~HWDiscrepancyAnalysis()
{}

DesignFlowStep_Status HWDiscrepancyAnalysis::Exec()
{
   return DesignFlowStep_Status::SUCCESS;
}

bool HWDiscrepancyAnalysis::HasToBeExecuted() const
{
   return true;
}

void HWDiscrepancyAnalysis::ComputeRelationships(
      DesignFlowStepSet & relationship,
      const DesignFlowStep::RelationshipType relationship_type)
{
   HLS_step::ComputeRelationships(relationship, relationship_type);
   if (parameters->isOption(OPT_discrepancy) and
         parameters->getOption<bool>(OPT_discrepancy) == true and
         relationship_type == DEPENDENCE_RELATIONSHIP)
   {
      const FrontendFlowStepFactory * frontend_step_factory =
         GetPointer<const FrontendFlowStepFactory>
            (design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"));

      const vertex call_graph_computation_step =
         design_flow_manager.lock()->GetDesignFlowStep
         (ApplicationFrontendFlowStep::ComputeSignature(FUNCTION_ANALYSIS));

      const DesignFlowGraphConstRef design_flow_graph =
         design_flow_manager.lock()->CGetDesignFlowGraph();

      const DesignFlowStepRef cg_design_flow_step = call_graph_computation_step ?
         design_flow_graph->CGetDesignFlowStepInfo(call_graph_computation_step)->design_flow_step :
         frontend_step_factory->CreateApplicationFrontendFlowStep(FUNCTION_ANALYSIS);

      relationship.insert(cg_design_flow_step);

      // Root function cannot be computed at the beginning so if the
      // call graph is not ready yet we exit. The relationships will
      // be computed again after the call graph computation.
      const CallGraphManagerConstRef call_graph_manager = HLSMgr->CGetCallGraphManager();
      if(boost::num_vertices(*(call_graph_manager->CGetCallGraph())) == 0)
         return;

      const CBackendStepFactory * c_backend_step_factory =
         GetPointer<const CBackendStepFactory>
         (design_flow_manager.lock()->CGetDesignFlowStepFactory("CBackend"));

      vertex vcd_debug_step = design_flow_manager.lock()->
         GetDesignFlowStep(CBackend::ComputeSignature(CBackend::CB_DISCREPANCY_ANALYSIS));
      const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
      THROW_ASSERT(top_function_ids.size() == 1, "Multiple top functions");
      const auto top_function_id= *(top_function_ids.begin());
      std::string out_dir = parameters->getOption<std::string>(OPT_output_directory) + "/simulation/";
      std::string c_file_name = HLSMgr->GetFunctionBehavior(top_function_id)->CGetBehavioralHelper()->get_function_name() + "_discrepancy.c";
      std::string dump_file_name = HLSMgr->GetFunctionBehavior(top_function_id)->CGetBehavioralHelper()->get_function_name() + "_discrepancy.txt";
      const DesignFlowStepRef design_flow_step = vcd_debug_step ?
         design_flow_graph->CGetDesignFlowStepInfo(vcd_debug_step)->design_flow_step :
         c_backend_step_factory->CreateCBackendStep(
            CBackend::CB_DISCREPANCY_ANALYSIS, out_dir + c_file_name,
            CBackendInformationConstRef(new HLSCBackendInformation(out_dir + dump_file_name, HLSMgr))
         );
      relationship.insert(design_flow_step);
   }
}
