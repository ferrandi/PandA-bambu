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
 *              Copyright (C) 2014-2022 Politecnico di Milano
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
 * @file sdc_scheduling_base.hpp
 * @brief SDC scheduling base class
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef SDC_SCHEDULING_BASE_HPP
#define SDC_SCHEDULING_BASE_HPP

#include "scheduling.hpp"

#include <list>
#include <vector>

class SDCScheduling_base : public Scheduling
{
 public:
   /// Result of SPECULATIVE_LOOP: the list of movement to be performed (first element is the operation, second element
   /// is the old basic block, third element is the new basic block) Movements have to be performed in order
   std::list<std::vector<unsigned int>> movements_list;

   SDCScheduling_base(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId,
                      const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type,
                      const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
       : Scheduling(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type,
                    _hls_flow_step_specialization)
   {
   }
};
#endif
