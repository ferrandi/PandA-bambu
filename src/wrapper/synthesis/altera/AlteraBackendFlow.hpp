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
 * @file AlteraBackendFlow.hpp
 * @brief Wrapper to implement a synthesis tools by Altera
 *
 * A object used to invoke synthesis tools by Altera
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef ALTERA_BACKEND_FLOW_HPP
#define ALTERA_BACKEND_FLOW_HPP

/// superclass include
#include "BackendFlow.hpp"

class AlteraBackendFlow : public BackendFlow
{
   /// design values reported by the tools
   std::map<std::string, double> design_values;

   /**
    * Writes the proper flow configuration in the output script
    */
   void WriteFlowConfiguration(std::ostream& script) override;

   /**
    * Parses device utilization
    */
   void xparse_utilization(const std::string& fn);

   /**
    * Checks the synthesis results and fills the corresponding data-structures
    */
   void CheckSynthesisResults() override;

   /**
    * Creates the constraint file
    */
   void create_sdc(const DesignParametersRef dp);

 public:
   /**
    * Constructor
    */
   AlteraBackendFlow(const ParameterConstRef Param, const std::string& flow_name, const generic_deviceRef _device);

   /**
    * Destructor
    */
   ~AlteraBackendFlow() override;

   /**
    * Initializes the parameters
    */
   void InitDesignParameters() override;

   /**
    * Checks if the execution can be performed and, in case, performs the synthesis
    */
   void ExecuteSynthesis() override;
};
/// Refcount definition for the class
using AlteraBackendFlowRef = refcount<AlteraBackendFlow>;

#endif
