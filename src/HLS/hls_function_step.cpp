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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
#include "hls_function_step.hpp"

#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "hls.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "memory.hpp"
#include "technology_manager.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "utility.hpp"

HLSFunctionStep::HLSFunctionStep(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId,
                                 const DesignFlowManagerConstRef _design_flow_manager,
                                 const HLSFlowStep_Type _hls_flow_step_type,
                                 const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLS_step(_Param, _HLSMgr, _design_flow_manager, _hls_flow_step_type, _hls_flow_step_specialization),
      funId(_funId),
      bb_version(0),
      bitvalue_version(0),
      memory_version(0)
{
}

HLSFunctionStep::~HLSFunctionStep() = default;

bool HLSFunctionStep::HasToBeExecuted() const
{
   CallGraphManagerConstRef CGMan = HLSMgr->CGetCallGraphManager();
   const auto funcs = CGMan->GetReachedBodyFunctions();
   THROW_ASSERT(funId, "unexpected case");
   if(funcs.find(funId) == funcs.end())
   {
      return false;
   }
   auto FB = HLSMgr->GetFunctionBehavior(funId);
   if(bb_version == 0 or bb_version != FB->GetBBVersion())
   {
      return true;
   }
   if(bitvalue_version == 0 or bitvalue_version != FB->GetBitValueVersion())
   {
      return true;
   }
   if(memory_version == 0 or memory_version != HLSMgr->GetMemVersion())
   {
      return true;
   }
   std::map<unsigned int, unsigned int> cur_bb_ver;
   std::map<unsigned int, unsigned int> cur_bitvalue_ver;
   const CallGraphManagerConstRef call_graph_manager = HLSMgr->CGetCallGraphManager();
   THROW_ASSERT(funId, "unexpected case");
   const auto called_functions = call_graph_manager->GetReachedBodyFunctionsFrom(funId);
   for(auto const called_function : called_functions)
   {
      if(called_function == funId)
      {
         continue;
      }
      const FunctionBehaviorConstRef FBCalled = HLSMgr->CGetFunctionBehavior(called_function);
      cur_bb_ver[called_function] = FBCalled->GetBBVersion();
      cur_bitvalue_ver[called_function] = FBCalled->GetBitValueVersion();
   }
   return cur_bb_ver != last_bb_ver || cur_bitvalue_ver != last_bitvalue_ver;
}

void HLSFunctionStep::Initialize()
{
   HLS_step::Initialize();
   THROW_ASSERT(funId, "unexpected case");
   HLS = HLSMgr->get_HLS(funId);
}

const std::string HLSFunctionStep::GetSignature() const
{
   return ComputeSignature(hls_flow_step_type, hls_flow_step_specialization, funId);
}

const std::string
HLSFunctionStep::ComputeSignature(const HLSFlowStep_Type hls_flow_step_type,
                                  const HLSFlowStepSpecializationConstRef hls_flow_step_specialization,
                                  const unsigned int function_id)
{
   return "HLS::" + std::to_string(static_cast<unsigned int>(hls_flow_step_type)) +
          (hls_flow_step_specialization ? "::" + hls_flow_step_specialization->GetSignature() : "") +
          "::" + std::to_string(function_id);
}

const std::string HLSFunctionStep::GetName() const
{
#ifndef NDEBUG
   const std::string version = std::string(bb_version != 0 ? ("(" + STR(bb_version) + ")") : "") +
                               std::string(bitvalue_version != 0 ? ("(" + STR(bitvalue_version) + ")") : "") +
                               std::string(memory_version != 0 ? ("(" + STR(memory_version) + ")") : "");
#else
   const std::string version = "";
#endif
   const std::string function =
       funId ? "::" + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name() : "";
   return "HLS::" + GetKindText() + function + version;
}

void HLSFunctionStep::ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                                           const DesignFlowStep::RelationshipType relationship_type)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing relationships of " + GetName());
   const auto* hls_flow_step_factory = GetPointer<const HLSFlowStepFactory>(CGetDesignFlowStepFactory());
   const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
   const CallGraphManagerConstRef call_graph_manager = HLSMgr->CGetCallGraphManager();
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   const HLS_targetRef HLS_T = HLSMgr->get_HLS_target();
   const technology_managerRef TM = HLS_T->get_technology_manager();

   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
       steps_to_be_created = ComputeHLSRelationships(relationship_type);
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
               {
                  continue;
               }
               std::string function_name = tree_helper::NormalizeTypename(tree_helper::name_function(TreeM, function));
               /// FIXME: temporary deactivated
               if(false) // function already implemented
               {
                  continue;
               }
               vertex hls_step = design_flow_manager.lock()->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(
                   std::get<0>(step_to_be_created), std::get<1>(step_to_be_created), function));
               const DesignFlowStepRef design_flow_step =
                   hls_step ? design_flow_graph->CGetDesignFlowStepInfo(hls_step)->design_flow_step :
                              hls_flow_step_factory->CreateHLSFlowStep(std::get<0>(step_to_be_created), function,
                                                                       std::get<1>(step_to_be_created));
               design_flow_step_set.insert(design_flow_step);
            }

            break;
         }
         case HLSFlowStep_Relationship::SAME_FUNCTION:
         {
            vertex hls_step = design_flow_manager.lock()->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(
                std::get<0>(step_to_be_created), std::get<1>(step_to_be_created), funId));
            const DesignFlowStepRef design_flow_step =
                hls_step ? design_flow_graph->CGetDesignFlowStepInfo(hls_step)->design_flow_step :
                           hls_flow_step_factory->CreateHLSFlowStep(std::get<0>(step_to_be_created), funId,
                                                                    std::get<1>(step_to_be_created));
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
   DesignFlowStep_Status status;
   status = InternalExec();
   THROW_ASSERT(funId, "unexpected case");
   auto FB = HLSMgr->GetFunctionBehavior(funId);
   bb_version = FB->GetBBVersion();
   bitvalue_version = FB->GetBitValueVersion();
   memory_version = HLSMgr->GetMemVersion();
   const CallGraphManagerConstRef call_graph_manager = HLSMgr->CGetCallGraphManager();
   const auto called_functions = call_graph_manager->GetReachedBodyFunctionsFrom(funId);
   for(auto const called_function : called_functions)
   {
      if(called_function == funId)
      {
         continue;
      }
      const FunctionBehaviorConstRef FBCalled = HLSMgr->CGetFunctionBehavior(called_function);
      last_bb_ver[called_function] = FBCalled->GetBBVersion();
      last_bitvalue_ver[called_function] = FBCalled->GetBitValueVersion();
   }
   return status;
}
