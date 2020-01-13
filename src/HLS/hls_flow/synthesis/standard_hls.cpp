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
 * @file standard_hls.cpp
 * @brief Implementation of the methods to create the structural description of the component
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "standard_hls.hpp"

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"

#include "Parameter.hpp"
#include "TopEntityMemoryMapped.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "cpu_time.hpp"
#include "datapath_creator.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "module_interface.hpp"
#include "omp_functions.hpp"
#include "technology_manager.hpp"

#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// HLS include
#include "hls_flow_step_factory.hpp"

/// HLS/module_allocation includes
#include "add_library.hpp"

/// STL include
#include "custom_set.hpp"
#include <tuple>

standard_hls::standard_hls(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::STANDARD_HLS_FLOW)
{
   composed = true;
}

standard_hls::~standard_hls() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> standard_hls::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         HLSFlowStep_Type synthesis_flow = HLSFlowStep_Type::VIRTUAL_DESIGN_FLOW;
#if HAVE_EXPERIMENTAL
         if(parameters->isOption("explore-mux") && parameters->getOption<bool>("explore-mux"))
            synthesis_flow = HLSFlowStep_Type::EXPLORE_MUX_DESIGN_FLOW;
         if(parameters->isOption("explore-fu-reg") && parameters->getOption<bool>("explore-fu-reg"))
            synthesis_flow = HLSFlowStep_Type::FU_REG_BINDING_DESIGN_FLOW;
#endif
         ret.insert(std::make_tuple(synthesis_flow, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_datapath_architecture), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         if(HLSMgr->get_HLS(funId))
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->controller_type, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         HLSFlowStep_Type top_entity_type;

         bool found = false;
         if(parameters->isOption(OPT_context_switch))
         {
            auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
            THROW_ASSERT(omp_functions, "OMP_functions must not be null");
            if(omp_functions->kernel_functions.find(funId) != omp_functions->kernel_functions.end())
               found = true;
            if(omp_functions->parallelized_functions.find(funId) != omp_functions->parallelized_functions.end())
               found = true;
            if(omp_functions->atomic_functions.find(funId) != omp_functions->atomic_functions.end())
               found = true;
            if(found) // use new top_entity
            {
               top_entity_type = HLSFlowStep_Type::TOP_ENTITY_CS_CREATION;
               ret.insert(std::make_tuple(top_entity_type, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
            }
         }
         if(!found) // use standard
         {
            top_entity_type = HLSMgr->hasToBeInterfaced(funId) and (HLSMgr->CGetCallGraphManager()->ExistsAddressedFunction() or parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::WB4_INTERFACE_GENERATION) ?
                                  HLSFlowStep_Type::TOP_ENTITY_MEMORY_MAPPED_CREATION :
                                  HLSFlowStep_Type::TOP_ENTITY_CREATION;
         }
         ret.insert(std::make_tuple(top_entity_type, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::ADD_LIBRARY, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(false)), HLSFlowStep_Relationship::SAME_FUNCTION));
         if(HLSMgr->hasToBeInterfaced(funId))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::ADD_LIBRARY, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(true)), HLSFlowStep_Relationship::SAME_FUNCTION));
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
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

DesignFlowStep_Status standard_hls::InternalExec()
{
   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   if(top_function_ids.find(funId) != top_function_ids.end())
      STOP_TIME(HLSMgr->HLS_execution_time);
   return DesignFlowStep_Status::EMPTY;
}
