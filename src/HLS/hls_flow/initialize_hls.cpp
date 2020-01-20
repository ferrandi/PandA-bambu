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
 * @file initialize_hls.cpp
 * @brief Step which initializes HLS data structure
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "initialize_hls.hpp"

///. include
#include "Parameter.hpp"

/// circuit include
#include "structural_manager.hpp"
#include "structural_objects.hpp"

/// HLS include
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

#include "memory_allocation.hpp"

/// technology include
#include "technology_manager.hpp"

/// technology/physical_library include
#include "technology_node.hpp"

/// tree include
#include "tree_helper.hpp"
#include "tree_manager.hpp"

InitializeHLS::InitializeHLS(const ParameterConstRef _parameters, const HLS_managerRef _HLS_mgr, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLS_mgr, _function_id, _design_flow_manager, HLSFlowStep_Type::INITIALIZE_HLS)
{
}

InitializeHLS::~InitializeHLS() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> InitializeHLS::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_function_allocation_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         ret.insert(
             std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_memory_allocation_algorithm),
                             HLSFlowStepSpecializationConstRef(new MemoryAllocationSpecialization(parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy), parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type))),
                             HLSFlowStep_Relationship::WHOLE_APPLICATION));
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

void InitializeHLS::Initialize()
{
   /// NOTE: this overrides HLSFunctionStep::Initialize which cannot be invoked since HLS has not yet been set
}

DesignFlowStep_Status InitializeHLS::InternalExec()
{
#if 0
   const tree_managerConstRef TreeM = HLSMgr->get_tree_manager();
   const technology_managerRef TM = HLSMgr->get_HLS_target()->get_technology_manager();
   std::string function_name = tree_helper::normalized_ID(tree_helper::name_function(TreeM, funId));
   structural_managerRef SM;
   TM->add_resource(WORK_LIBRARY, function_name, SM);
   TM->add_operation(WORK_LIBRARY, function_name, function_name);
   functional_unit* fu = GetPointer<functional_unit>(TM->get_fu(function_name, WORK_LIBRARY));
#endif

   HLS = HLS_manager::create_HLS(HLSMgr, funId);
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT
   if(GetPointer<const function_decl>(HLSMgr->get_tree_manager()->CGetTreeNode(funId))->omp_for_wrapper)
   {
      HLS->controller_type = HLSFlowStep_Type::PARALLEL_CONTROLLER_CREATOR;
      HLS->module_binding_algorithm = static_cast<HLSFlowStep_Type>(parameters->getOption<int>(OPT_fu_binding_algorithm));
      HLS->liveness_algorithm = HLSFlowStep_Type::CHAINING_BASED_LIVENESS;
      HLS->chaining_algorithm = HLSFlowStep_Type::EPDG_SCHED_CHAINING;
   }
   else
#endif
   {
      HLS->controller_type = static_cast<HLSFlowStep_Type>(parameters->getOption<int>(OPT_controller_architecture));
      HLS->module_binding_algorithm = static_cast<HLSFlowStep_Type>(parameters->getOption<int>(OPT_fu_binding_algorithm));
      HLS->liveness_algorithm = static_cast<HLSFlowStep_Type>(parameters->getOption<int>(OPT_liveness_algorithm));
      HLS->chaining_algorithm = static_cast<HLSFlowStep_Type>(parameters->getOption<int>(OPT_chaining_algorithm));
   }
#if 0
   fu->set_clock_period(HLS->HLS_C->get_clock_period());
   fu->set_clock_period_resource_fraction(HLS->HLS_C->get_clock_period_resource_fraction());
#endif
   return DesignFlowStep_Status::SUCCESS;
}
