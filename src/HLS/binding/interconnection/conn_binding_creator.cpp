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
#include "behavioral_helper.hpp"

#include "Parameter.hpp"
#include "call_graph_manager.hpp"
#include "cdfc_module_binding.hpp"
#include "conn_binding.hpp"
#include "conn_binding_creator.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "utility.hpp"

conn_binding_creator::conn_binding_creator(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr,
                                           unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager,
                                           const HLSFlowStep_Type _hls_flow_step_type)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
}

conn_binding_creator::~conn_binding_creator() = default;

HLS_step::HLSRelationships
conn_binding_creator::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         if(parameters->IsParameter("PortSwapping") and parameters->GetParameter<bool>("PortSwapping"))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::PORT_SWAPPING, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         if(HLSMgr->get_HLS(funId))
         {
            if(HLSMgr->GetFunctionBehavior(funId)->is_simple_pipeline())
            {
               ret.insert(std::make_tuple(HLSFlowStep_Type::UNIQUE_MODULE_BINDING, HLSFlowStepSpecializationConstRef(),
                                          HLSFlowStep_Relationship::SAME_FUNCTION));
            }
            else
            {
               ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->module_binding_algorithm,
                                          HLSFlowStepSpecializationConstRef(),
                                          HLSFlowStep_Relationship::SAME_FUNCTION));
            }
         }
         if(HLSMgr->GetFunctionBehavior(funId)->is_simple_pipeline())
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::UNIQUE_REGISTER_BINDING, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         else
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_register_allocation_algorithm),
                                       HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
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
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();
   input_ports.clear();
   output_ports.clear();
   /// list containing the parameters of the original function (representing input and output values)
   const auto function_parameters = BH->get_parameters();
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
      PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level,
                       "Return format " + BH->print_type(return_type_index) + " (OUT) ");
      output_ports[return_type_index] = HLS->Rconn->bind_port(return_type_index, conn_binding::OUT);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "");
}
