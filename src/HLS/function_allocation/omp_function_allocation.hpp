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
 *              Copyright (c) 2015-2020 Politecnico di Milano
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
 * @file omp_function_allocation.hpp
 * @brief Class to allocate function in HLS based on dominators and openmp information
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef OMP_FUNCTION_ALLOCATION_HPP
#define OMP_FUNCTION_ALLOCATION_HPP

/// Superclass include
#include "fun_dominator_allocation.hpp"

class OmpFunctionAllocation : public fun_dominator_allocation
{
 public:
   /**
    * Constructor
    */
   OmpFunctionAllocation(const ParameterConstRef Param, const HLS_managerRef HLSMgr, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~OmpFunctionAllocation();

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec();

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize();
};
#endif
