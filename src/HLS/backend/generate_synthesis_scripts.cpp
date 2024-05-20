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
 * @file generate_synthesis_scripts.cpp
 * @brief Wrapper used to generate synthesis scripts
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "generate_synthesis_scripts.hpp"

#include "BackendFlow.hpp"
#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "custom_set.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "tree_manager.hpp"
#include "utility.hpp"

#include <tuple>

GenerateSynthesisScripts::GenerateSynthesisScripts(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                                   const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::GENERATE_SYNTHESIS_SCRIPT)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

GenerateSynthesisScripts::~GenerateSynthesisScripts() = default;

HLS_step::HLSRelationships
GenerateSynthesisScripts::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_HDL, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
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

DesignFlowStep_Status GenerateSynthesisScripts::Exec()
{
   const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = HLSMgr->get_tree_manager()->GetFunction(top_symbols.front());
   const auto top_hls = HLSMgr->get_HLS(top_fnode->index);
   const auto FB = HLSMgr->CGetFunctionBehavior(top_fnode->index);
   HLSMgr->get_backend_flow()->GenerateSynthesisScripts(FB->CGetBehavioralHelper()->get_function_name(), top_hls->top,
                                                        HLSMgr->hdl_files, HLSMgr->aux_files);
   return DesignFlowStep_Status::SUCCESS;
}

bool GenerateSynthesisScripts::HasToBeExecuted() const
{
   return true;
}
