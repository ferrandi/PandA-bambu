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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file generate_hdl.cpp
 * @brief Implementation of the class to generate HDL code
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "generate_hdl.hpp"

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
#include "ir_manager.hpp"
#include "structural_manager.hpp"

#include <list>
#include <string>
#include <tuple>

generate_hdl::generate_hdl(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                           const DesignFlowManager& _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::GENERATE_HDL)
{
}

HLS_step::HLSRelationships
generate_hdl::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::ALL_FUNCTIONS));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

bool generate_hdl::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status generate_hdl::Exec()
{
   HDL_manager HM(HLSMgr, HLSMgr->get_HLS_device(), parameters);
   std::list<structural_objectRef> top_circuits;
   const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
   for(const auto& symbol : top_symbols)
   {
      const auto top_fnode = HLSMgr->get_ir_manager()->GetFunction(symbol);
      top_circuits.push_back(HLSMgr->get_HLS(top_fnode->index)->top->get_circ());
   }
   const auto file_name = parameters->getOption<std::string>(OPT_output_directory) +
                          (top_symbols.size() == 1 ? ("/" + top_symbols.front()) : "/top");

   const auto hdl_out_mode = parameters->getOption<HDL_output_mode>(OPT_generate_components);
   HM.hdl_gen(file_name, top_circuits, HLSMgr->hdl_files, HLSMgr->aux_files, hdl_out_mode);
   return DesignFlowStep_Status::SUCCESS;
}
