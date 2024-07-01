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
 * @file scheduling.cpp
 * @brief Base class for all scheduling algorithms.
 *
 *
 * @author Matteo Barbati <mbarbati@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "scheduling.hpp"

#include "Parameter.hpp"
#include "allocation.hpp"
#include "allocation_information.hpp"
#include "dbgPrintHelper.hpp"
#include "fu_binding.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "op_graph.hpp"
#include "parallel_memory_fu_binding.hpp"
#include "parametric_list_based.hpp"
#include "refcount.hpp"
#include "utility.hpp"

Scheduling::Scheduling(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId,
                       const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type,
                       const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type,
                      _hls_flow_step_specialization),
      speculation(_Param->getOption<bool>(OPT_speculative))
{
}

Scheduling::~Scheduling() = default;

void Scheduling::Initialize()
{
   HLSFunctionStep::Initialize();
   if(HLS->Rsch)
   {
      HLS->Rsch->Initialize();
   }
   else
   {
      const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
      HLS->Rsch = ScheduleRef(new Schedule(HLSMgr, funId, FB->CGetOpGraph(FunctionBehavior::FLSAODG), parameters));
   }
   if(parameters->getOption<int>(OPT_memory_banks_number) > 1 && !parameters->isOption(OPT_context_switch))
   {
      HLS->Rfu = fu_bindingRef(new ParallelMemoryFuBinding(HLSMgr, funId, parameters));
   }
   else
   {
      HLS->Rfu = fu_bindingRef(fu_binding::create_fu_binding(HLSMgr, funId, parameters));
   }
}

HLS_step::HLSRelationships
Scheduling::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
#if HAVE_FROM_PRAGMA_BUILT
         if(parameters->getOption<bool>(OPT_parse_pragma))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::OMP_ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         else
#endif
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         ret.insert(std::make_tuple(HLSFlowStep_Type::DOMINATOR_ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::WHOLE_APPLICATION));
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
