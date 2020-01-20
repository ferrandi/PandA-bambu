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
 * @file minimal_interface.hpp
 * @brief Class to generate minimal interfaces for high-level synthesis
 *
 * This class generates minimal intefaces for connecting modules to
 * microprocessors or busses
 *
 * @author Marco Minutoli <mminutoli@gmail.com>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _MINIMAL_INTERFACE_HPP_
#define _MINIMAL_INTERFACE_HPP_

#include "module_interface.hpp"

/**
 * Class generating minimal interfaces
 */
class minimal_interface : public module_interface
{
 public:
   /**
    * Constructor
    */
   minimal_interface(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION);

   void build_wrapper(structural_objectRef wrappedObj, structural_objectRef interfaceObj, structural_managerRef SM_minimal_interface);
   /**
    * Destructor
    */
   ~minimal_interface() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif
