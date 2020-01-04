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
 * @file fu_binding_creator.cpp
 * @brief Implementation of module binding class.
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "fu_binding_creator.hpp"

#include "fu_binding.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "parallel_memory_fu_binding.hpp"

/// implemented algorithms
#include "cdfc_module_binding.hpp"
#include "unique_binding.hpp"

#include "Parameter.hpp"
#include "exceptions.hpp"
#include "polixml.hpp"
#include "xml_helper.hpp"

/// HLS/module_allocation include
#include "allocation.hpp"

fu_binding_creator::fu_binding_creator(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type,
                                       const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type, _hls_flow_step_specialization)
{
}

fu_binding_creator::~fu_binding_creator() = default;

void fu_binding_creator::Initialize()
{
   HLSFunctionStep::Initialize();
   if(not HLS->Rfu)
   {
      if(parameters->getOption<int>(OPT_memory_banks_number) > 1 && !parameters->isOption(OPT_context_switch))
      {
         HLS->Rfu = fu_bindingRef(new ParallelMemoryFuBinding(HLSMgr, funId, parameters));
      }
      else
      {
         HLS->Rfu = fu_bindingRef(fu_binding::create_fu_binding(HLSMgr, funId, parameters));
      }
   }
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> fu_binding_creator::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_storage_value_insertion_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::EASY_MODULE_BINDING, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->liveness_algorithm, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->chaining_algorithm, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
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
