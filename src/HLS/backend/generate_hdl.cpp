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
 * @file generate_hdl.cpp
 * @brief Implementation of the class to generate HDL code
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "generate_hdl.hpp"

#include "BackendFlow.hpp"
#include "HDL_manager.hpp"
#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "custom_set.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "structural_manager.hpp"
#include "tree_manager.hpp"

#include <list>
#include <string>
#include <tuple>

generate_hdl::generate_hdl(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                           const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::GENERATE_HDL)
{
}

HLS_step::HLSRelationships
generate_hdl::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::ALL_FUNCTIONS));
         if(parameters->isOption(OPT_discrepancy_hw) && parameters->getOption<bool>(OPT_discrepancy_hw))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::HW_DISCREPANCY_ANALYSIS, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::TOP_FUNCTION));
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

generate_hdl::~generate_hdl() = default;

bool generate_hdl::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status generate_hdl::Exec()
{
   HDL_manager HM(HLSMgr, HLSMgr->get_HLS_device(), parameters);
   const auto file_name = parameters->getOption<std::string>(OPT_top_file);
   std::list<structural_objectRef> top_circuits;
   const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
   for(const auto& symbol : top_symbols)
   {
      const auto top_fnode = HLSMgr->get_tree_manager()->GetFunction(symbol);
      top_circuits.push_back(HLSMgr->get_HLS(top_fnode->index)->top->get_circ());
   }

   HM.hdl_gen(file_name, top_circuits, HLSMgr->hdl_files, HLSMgr->aux_files, false);
   return DesignFlowStep_Status::SUCCESS;
}
