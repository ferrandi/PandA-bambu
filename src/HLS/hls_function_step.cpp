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
 * @file hls_function_step.cpp
 * @brief Base class for all HLS algorithms
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "hls_function_step.hpp"

/// behavior include
#include "call_graph_manager.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// HLS/memory include
#include "memory.hpp"

/// technology include
#include "technology_manager.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "tree_helper.hpp"
#include "tree_manager.hpp"

HLSFunctionStep::HLSFunctionStep(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type,
                                 const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLS_step(_Param, _HLSMgr, _design_flow_manager, _hls_flow_step_type, _hls_flow_step_specialization), funId(_funId), bb_version(0), memory_version(0)
{
}

HLSFunctionStep::~HLSFunctionStep() = default;

bool HLSFunctionStep::HasToBeExecuted() const
{
   CallGraphManagerConstRef CGMan = HLSMgr->CGetCallGraphManager();
   CustomOrderedSet<unsigned int> funcs = CGMan->GetReachedBodyFunctions();
   if(funId and funcs.find(funId) == funcs.end())
      return false;
   if(bb_version == 0 or bb_version != HLSMgr->GetFunctionBehavior(funId)->GetBBVersion())
      return true;
   if(memory_version == 0 or memory_version != HLSMgr->GetMemVersion())
      return true;
   const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
   vertex current_step = design_flow_manager.lock()->GetDesignFlowStep(GetSignature());
   InEdgeIterator ie, ie_end;
   for(boost::tie(ie, ie_end) = boost::in_edges(current_step, *design_flow_graph); ie != ie_end; ie++)
   {
      vertex pre_dependence_vertex = boost::source(*ie, *design_flow_graph);
      const DesignFlowStepInfoConstRef pre_info = design_flow_graph->CGetDesignFlowStepInfo(pre_dependence_vertex);
      if(pre_info->status != DesignFlowStep_Status::SUCCESS and pre_info->status != DesignFlowStep_Status::EMPTY)
         continue;
      if(GetPointer<HLSFunctionStep>(pre_info->design_flow_step) && GetPointer<HLSFunctionStep>(pre_info->design_flow_step)->funId != funId)
         continue;
      return true;
   }
   return false;
}

void HLSFunctionStep::Initialize()
{
   HLS_step::Initialize();
   if(funId)
      HLS = HLSMgr->get_HLS(funId);
}

const std::string HLSFunctionStep::GetSignature() const
{
   return ComputeSignature(hls_flow_step_type, hls_flow_step_specialization, funId);
}

const std::string HLSFunctionStep::ComputeSignature(const HLSFlowStep_Type hls_flow_step_type, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization, const unsigned int function_id)
{
   return "HLS::" + std::to_string(static_cast<unsigned int>(hls_flow_step_type)) + (hls_flow_step_specialization ? "::" + hls_flow_step_specialization->GetSignature() : "") + "::" + std::to_string(function_id);
}

const std::string HLSFunctionStep::GetName() const
{
#ifndef NDEBUG
   const std::string version = std::string(bb_version != 0 ? ("(" + STR(bb_version) + ")") : "") + std::string(memory_version != 0 ? ("(" + STR(memory_version) + ")") : "");
#else
   const std::string version = "";
#endif
   const std::string function = funId ? "::" + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name() : "";
   return "HLS::" + GetKindText() + function + version;
}

void HLSFunctionStep::ComputeRelationships(DesignFlowStepSet& design_flow_step_set, const DesignFlowStep::RelationshipType relationship_type)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing relationships of " + GetName());
   const auto* hls_flow_step_factory = GetPointer<const HLSFlowStepFactory>(CGetDesignFlowStepFactory());
   const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
   const CallGraphManagerConstRef call_graph_manager = HLSMgr->CGetCallGraphManager();
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   const HLS_targetRef HLS_T = HLSMgr->get_HLS_target();
   const technology_managerRef TM = HLS_T->get_technology_manager();

   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> steps_to_be_created = ComputeHLSRelationships(relationship_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Computed steps to be created");
   for(auto const& step_to_be_created : steps_to_be_created)
   {
      switch(std::get<2>(step_to_be_created))
      {
         case HLSFlowStep_Relationship::CALLED_FUNCTIONS:
         {
            const auto called_functions = call_graph_manager->GetReachedBodyFunctionsFrom(funId);
            for(auto const function : called_functions)
            {
               if(function == funId)
                  continue;
               std::string function_name = tree_helper::normalized_ID(tree_helper::name_function(TreeM, function));
               /// FIXME: temporary deactivated
               if(false) // function already implemented
               {
                  continue;
               }
               vertex hls_step = design_flow_manager.lock()->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(std::get<0>(step_to_be_created), std::get<1>(step_to_be_created), function));
               const DesignFlowStepRef design_flow_step =
                   hls_step ? design_flow_graph->CGetDesignFlowStepInfo(hls_step)->design_flow_step : hls_flow_step_factory->CreateHLSFlowStep(std::get<0>(step_to_be_created), function, std::get<1>(step_to_be_created));
               design_flow_step_set.insert(design_flow_step);
            }

            break;
         }
         case HLSFlowStep_Relationship::SAME_FUNCTION:
         {
            vertex hls_step = design_flow_manager.lock()->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(std::get<0>(step_to_be_created), std::get<1>(step_to_be_created), funId));
            const DesignFlowStepRef design_flow_step = hls_step ? design_flow_graph->CGetDesignFlowStepInfo(hls_step)->design_flow_step : hls_flow_step_factory->CreateHLSFlowStep(std::get<0>(step_to_be_created), funId, std::get<1>(step_to_be_created));
            design_flow_step_set.insert(design_flow_step);
            break;
         }
         case HLSFlowStep_Relationship::ALL_FUNCTIONS:
         case HLSFlowStep_Relationship::TOP_FUNCTION:
         case HLSFlowStep_Relationship::WHOLE_APPLICATION:
         {
            /// Managed in HLS_step::ComputeRelationships
            break;
         }
         default:
            THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Computed function only HLS step dependencies");
   HLS_step::ComputeRelationships(design_flow_step_set, relationship_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computing relationships of " + GetName());
}

DesignFlowStep_Status HLSFunctionStep::Exec()
{
   const auto status = InternalExec();
   if(funId)
      bb_version = HLSMgr->GetFunctionBehavior(funId)->GetBBVersion();
   memory_version = HLSMgr->GetMemVersion();
   return status;
}
