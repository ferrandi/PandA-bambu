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
 * @file conn_binding_creator.cpp
 * @brief Base class for all interconnection binding algorithms.
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"

#include "conn_binding_creator.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"

#include "polixml.hpp"
#include "xml_helper.hpp"

#include "Parameter.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_manager.hpp"

/// HLS/binding/interconnection
#include "conn_binding.hpp"

/// HLS/binding/module includes
#include "cdfc_module_binding.hpp"

/// HLS/binding/register/algorithms includes
#include "weighted_clique_register.hpp"
/// behavior include
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_

conn_binding_creator::conn_binding_creator(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
}

conn_binding_creator::~conn_binding_creator() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> conn_binding_creator::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         if(parameters->IsParameter("PortSwapping") and parameters->GetParameter<bool>("PortSwapping"))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::PORT_SWAPPING, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->module_binding_algorithm, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_register_allocation_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
#if HAVE_EXPERIMENTAL
         if(parameters->IsParameter("MemoryConflictGraph") and parameters->GetParameter<bool>("MemoryConflictGraph"))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::MEMORY_CONFLICT_GRAPH, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
#endif
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

void conn_binding_creator::add_parameter_ports()
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();
   input_ports.clear();
   output_ports.clear();
   /// list containing the parameters of the original function (representing input and output values)
   const std::list<unsigned int>& function_parameters = BH->get_parameters();
   PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "Parameters values: ");
   for(auto function_parameter : function_parameters)
   {
      PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, BH->PrintVariable(function_parameter) + "(");
      PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "IN)-@" + STR(function_parameter) + " ");
      input_ports[function_parameter] = HLS->Rconn->bind_port(function_parameter, conn_binding::IN);
   }
   const unsigned int return_type_index = BH->GetFunctionReturnType(BH->get_function_index());
   if(return_type_index)
   {
      PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "Return format " + BH->print_type(return_type_index) + " (OUT) ");
      output_ports[return_type_index] = HLS->Rconn->bind_port(return_type_index, conn_binding::OUT);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "");
}
