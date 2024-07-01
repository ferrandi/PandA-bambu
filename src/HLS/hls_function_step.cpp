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
 * @file hls_function_step.cpp
 * @brief Base class for all HLS algorithms
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "hls_function_step.hpp"

#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "memory.hpp"
#include "technology_manager.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "utility.hpp"

CustomMap<unsigned int, unsigned int> HLSFunctionStep::curr_ver;

HLSFunctionStep::HLSFunctionStep(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId,
                                 const DesignFlowManagerConstRef _design_flow_manager,
                                 const HLSFlowStep_Type _hls_flow_step_type,
                                 const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLS_step(ComputeSignature(_hls_flow_step_type, _hls_flow_step_specialization, _funId), _Param, _HLSMgr,
               _design_flow_manager, _hls_flow_step_type, _hls_flow_step_specialization),
      last_ver_sum(0),
      funId(_funId),
      bb_version(0),
      bitvalue_version(0),
      memory_version(0)
{
   THROW_ASSERT(funId, "unexpected case");
}

HLSFunctionStep::~HLSFunctionStep() = default;

static inline unsigned int compute_sum(const CustomMap<unsigned int, unsigned int>& ver_map,
                                       const CustomSet<unsigned int>& functions)
{
   unsigned int sum = 0;
   for(const auto f_id : functions)
   {
      const auto v_it = ver_map.find(f_id);
      if(v_it != ver_map.end())
      {
         sum += v_it->second;
      }
   }
   return sum;
}

bool HLSFunctionStep::HasToBeExecuted() const
{
   const auto CGM = HLSMgr->CGetCallGraphManager();
   if(!CGM->GetReachedBodyFunctions().count(funId))
   {
      return false;
   }
   const auto FB = HLSMgr->GetFunctionBehavior(funId);
   if(!bb_version || !bitvalue_version || !memory_version || bb_version != FB->GetBBVersion() ||
      bitvalue_version != FB->GetBitValueVersion() || memory_version != HLSMgr->GetMemVersion())
   {
      return true;
   }

   const auto called_functions = CGM->GetReachedFunctionsFrom(funId);
   const auto curr_ver_sum = compute_sum(curr_ver, called_functions);
   return curr_ver_sum > last_ver_sum;
}

void HLSFunctionStep::Initialize()
{
   HLS_step::Initialize();
   HLS = HLSMgr->get_HLS(funId);
}

DesignFlowStep::signature_t
HLSFunctionStep::ComputeSignature(const HLSFlowStep_Type hls_flow_step_type,
                                  const HLSFlowStepSpecializationConstRef hls_flow_step_specialization,
                                  const unsigned int function_id)
{
   THROW_ASSERT(function_id < (1 << 24), "Signature clash may occurr.");
   return DesignFlowStep::ComputeSignature(
       HLS_FUNCTION, static_cast<unsigned short>(hls_flow_step_type),
       static_cast<unsigned long long>(function_id & 0xFFFFFF) << 16 |
           (hls_flow_step_specialization ? hls_flow_step_specialization->GetSignatureContext() : 0));
}

std::string HLSFunctionStep::GetName() const
{
   const auto function =
       funId ? ("::" + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name()) : "";
   return HLS_step::GetName() + function
#ifndef NDEBUG
          + (bb_version != 0 ? ("(" + STR(bb_version) + ")") : "") +
          (bitvalue_version != 0 ? ("(" + STR(bitvalue_version) + ")") : "") +
          (memory_version != 0 ? ("(" + STR(memory_version) + ")") : "")
#endif
       ;
}

void HLSFunctionStep::ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                                           const DesignFlowStep::RelationshipType relationship_type)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing relationships of " + GetName());
   const auto hls_flow_step_factory = GetPointerS<const HLSFlowStepFactory>(CGetDesignFlowStepFactory());
   const auto DFM = design_flow_manager.lock();
   const auto DFG = DFM->CGetDesignFlowGraph();
   const auto CGM = HLSMgr->CGetCallGraphManager();
   const auto TreeM = HLSMgr->get_tree_manager();
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TM = HLS_D->get_technology_manager();

   const auto steps_to_be_created = ComputeHLSRelationships(relationship_type);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Computed steps to be created");
   for(auto const& [step_type, step_spec, rel_type] : steps_to_be_created)
   {
      switch(rel_type)
      {
         case HLSFlowStep_Relationship::CALLED_FUNCTIONS:
         {
            const auto called_functions = CGM->GetReachedFunctionsFrom(funId);
            for(auto const function : called_functions)
            {
               if(function != funId)
               {
                  const auto hls_step =
                      DFM->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(step_type, step_spec, function));
                  const auto design_flow_step =
                      hls_step != DesignFlowGraph::null_vertex() ?
                          DFG->CGetNodeInfo(hls_step)->design_flow_step :
                          hls_flow_step_factory->CreateHLSFlowStep(step_type, function, step_spec);
                  design_flow_step_set.insert(design_flow_step);
               }
            }
            break;
         }
         case HLSFlowStep_Relationship::SAME_FUNCTION:
         {
            const auto hls_step =
                DFM->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(step_type, step_spec, funId));
            const auto design_flow_step = hls_step != DesignFlowGraph::null_vertex() ?
                                              DFG->CGetNodeInfo(hls_step)->design_flow_step :
                                              hls_flow_step_factory->CreateHLSFlowStep(step_type, funId, step_spec);
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
   const auto FB = HLSMgr->GetFunctionBehavior(funId);
   bb_version = FB->GetBBVersion();
   bitvalue_version = FB->GetBitValueVersion();
   curr_ver[funId] = bb_version + bitvalue_version;
   memory_version = HLSMgr->GetMemVersion();
   const auto called_functions = HLSMgr->CGetCallGraphManager()->GetReachedFunctionsFrom(funId);
   last_ver_sum = compute_sum(curr_ver, called_functions);
   return status;
}
