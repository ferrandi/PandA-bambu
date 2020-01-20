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
 * @file virtual_hls.cpp
 * @brief A brief description of the C++ Source File
 *
 * Here goes a detailed description of the file
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"

/// header include
#include "virtual_hls.hpp"

///. include
#include "Parameter.hpp"

/// design_flows
#include "design_flow_manager.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"

/// HLS/binding/module
#include "cdfc_module_binding.hpp"

/// HLS/binding/register/algorithms
#include "string_manipulation.hpp" // for GET_CLASS
#include "weighted_clique_register.hpp"

virtual_hls::virtual_hls(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::VIRTUAL_DESIGN_FLOW)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   composed = true;
}

virtual_hls::~virtual_hls() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> virtual_hls::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
#if HAVE_FROM_PRAGMA_BUILT
         if(parameters->getOption<bool>(OPT_parse_pragma))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::OMP_ALLOCATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         else
#endif
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::ALLOCATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         ret.insert(std::make_tuple(HLSFlowStep_Type::EASY_MODULE_BINDING, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
#if HAVE_ILP_BUILT
         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::LIST_BASED_SCHEDULING, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         else
#endif
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_stg_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::INITIALIZE_HLS, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
#if HAVE_EXPERIMENTAL
         if(parameters->IsParameter("MemoryConflictGraph") and parameters->GetParameter<bool>("MemoryConflictGraph"))
         {
            /// MEMORY_CONFLICT_GRAPH
            ret.insert(std::make_tuple(HLSFlowStep_Type::MEMORY_CONFLICT_GRAPH, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
#endif
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->chaining_algorithm, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->liveness_algorithm, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_storage_value_insertion_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         if(HLSMgr->get_HLS(funId))
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->module_binding_algorithm, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_register_allocation_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_datapath_interconnection_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
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

DesignFlowStep_Status virtual_hls::InternalExec()
{
   return DesignFlowStep_Status::EMPTY;
}
