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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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

#include "structural_manager.hpp"
#include "behavioral_helper.hpp"

#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_constraints.hpp"

#include "BackendFlow.hpp"

#include "Parameter.hpp"

///behavior include
#include "call_graph_manager.hpp"

///design_flow_manager/backend/ToHDL includes
#include "HDL_manager.hpp"

///HLS includes
#include "hls_flow_step_factory.hpp"
#include "hls_target.hpp"

generate_hdl::generate_hdl(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager):
   HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::GENERATE_HDL)
{

}

const std::unordered_set<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship> > generate_hdl::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   std::unordered_set<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship> > ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::ALL_FUNCTIONS));
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

generate_hdl::~generate_hdl()
{

}

DesignFlowStep_Status generate_hdl::Exec()
{
   HDL_managerRef HM = HDL_managerRef(new HDL_manager(HLSMgr, HLSMgr->get_HLS_target()->get_target_device(), parameters));
   std::string file_name = parameters->getOption<std::string>(OPT_top_file);
   std::unordered_set<structural_objectRef> top_circuits;
   for(const auto top_function : HLSMgr->CGetCallGraphManager()->GetRootFunctions())
   {
      top_circuits.insert(HLSMgr->get_HLS(top_function)->top->get_circ());
   }
   HM->hdl_gen(file_name, top_circuits, false, HLSMgr->hdl_files, HLSMgr->aux_files);
   return DesignFlowStep_Status::SUCCESS;
}

bool generate_hdl::HasToBeExecuted() const
{
   return true;
}
