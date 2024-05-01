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
 * @file datapath.cpp
 * @brief Base class for all datapath creation algorithms.
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "datapath_creator.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "utility.hpp"

datapath_creator::datapath_creator(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId,
                                   const DesignFlowManagerConstRef _design_flow_manager,
                                   const HLSFlowStep_Type _hls_flow_step_type)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
}

datapath_creator::~datapath_creator() = default;

HLS_step::HLSRelationships
datapath_creator::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         HLSFlowStep_Type synthesis_flow = HLSFlowStep_Type::VIRTUAL_DESIGN_FLOW;
         ret.insert(std::make_tuple(synthesis_flow, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));
         if(parameters->isOption(OPT_discrepancy_hw) and parameters->getOption<bool>(OPT_discrepancy_hw))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::CONTROL_FLOW_CHECKER, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return ret;
}
