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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file ASICBackendFlow.hpp
 * @brief Wrapper to implement a synthesis tools
 *
 * A object used to invoke synthesis tools
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _ASIC_BACKEND_FLOW_HPP_
#define _ASIC_BACKEND_FLOW_HPP_

/// superclass include
#include "BackendFlow.hpp"

class ASICBackendFlow : public BackendFlow
{
   /**
    * Writes the proper flow configuration in the output script
    */
   void WriteFlowConfiguration(std::ostream& script) override;

   /**
    * Checks the synthesis results and fills the corresponding data structures
    */
   void CheckSynthesisResults() override;

   /**
    * Evaluates design variables
    */
   void InitDesignParameters() override;

   /**
    * Creates the constraint file
    */
   void create_sdc(const DesignParametersRef dp);

 public:
   /**
    * Constructor
    */
   ASICBackendFlow(const ParameterConstRef Param, const std::string& flow_name, const target_managerRef target);

   /**
    * Destructor
    */
   ~ASICBackendFlow() override;
};
/// Refcount definition for the class
using ASICBackendFlowRef = refcount<ASICBackendFlow>;

#endif
