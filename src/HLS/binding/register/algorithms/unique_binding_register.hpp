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
 * @file unique_binding_register.hpp
 * @brief Class specification of unique binding register allocation algorithm
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @version $Revision$
 * @date $Date$
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef UNIQUE_BINDING_REGISTER_HPP
#define UNIQUE_BINDING_REGISTER_HPP

#include "reg_binding_creator.hpp"

class unique_binding_register : public reg_binding_creator
{
 public:
   /**
    * Constructor of the class.
    * @param design_flow_manager is the design flow manager
    */
   unique_binding_register(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor of the class.
    */
   ~unique_binding_register() override;

   /**
    * Unique binding register allocation algorithm execution.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif
