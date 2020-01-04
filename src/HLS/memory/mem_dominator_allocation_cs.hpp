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
 *              Copyright (c) 2016-2020 Politecnico di Milano
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
 * @file mem_dominator_allocation_CS.hpp
 * @brief Class to allocate memories in HLS based on dominators, add tag for context switch
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 */

#ifndef MEMORY_DOMINATOR_ALLOCATION_CS_HPP
#define MEMORY_DOMINATOR_ALLOCATION_CS_HPP

#include "mem_dominator_allocation.hpp"

class mem_dominator_allocation_cs : public mem_dominator_allocation
{
 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    */
   mem_dominator_allocation_cs(const ParameterConstRef Param, const HLS_managerRef HLSMgr, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization,
                               const HLSFlowStep_Type hls_flow_step_type);

   /**
    * Destructor
    */
   virtual ~mem_dominator_allocation_cs();

   /**
    * Execute the step
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status Exec();
};

#endif // MEM_DOMINATOR_ALLOCATION_CS_H
