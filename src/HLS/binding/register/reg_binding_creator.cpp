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
 * @file reg_binding_creator.cpp
 * @brief Base class for all register allocation algorithms
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "reg_binding_creator.hpp"

#include "Parameter.hpp"
#include "cdfc_module_binding.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "liveness.hpp"
#include "refcount.hpp"
#include "storage_value_insertion.hpp"
#include "utility.hpp"

#include <boost/version.hpp>
#include <iosfwd>

reg_binding_creator::reg_binding_creator(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr,
                                         unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager,
                                         const HLSFlowStep_Type _hls_flow_step_type,
                                         const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type,
                      _hls_flow_step_specialization),
      register_lower_bound(0)
{
}

HLS_step::HLSRelationships
reg_binding_creator::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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
         if(HLSMgr->GetFunctionBehavior(funId)->is_simple_pipeline())
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::UNIQUE_MODULE_BINDING, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         else
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::EASY_MODULE_BINDING, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_stg_algorithm),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->chaining_algorithm, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->liveness_algorithm, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
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
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_storage_value_insertion_algorithm),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
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

DesignFlowStep_Status reg_binding_creator::InternalExec()
{
   return RegisterBinding();
}