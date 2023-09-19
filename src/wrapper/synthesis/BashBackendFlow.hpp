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
 *              Copyright (C) 2020-2023 Politecnico di Milano
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
 * @file BashBackendFlow.hpp
 *
 * @brief Backend based on a simple bash script
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@elet.polimi.it>
 *
 */
#ifndef _BASH_BACKEND_FLOW_HPP_
#define _BASH_BACKEND_FLOW_HPP_

/// superclass include
#include "BackendFlow.hpp"

#define PARAM_bash_sources_macro_list "generic_sources_macro_list"
#define PARAM_bash_backend_report "bash_backend_report"
#define PARAM_bash_backend_timing_report "bash_backend_timing_report"

class BashBackendFlow : public BackendFlow
{
 protected:
   /// results from the synthesis
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
   BashBackendFlow(const ParameterConstRef Param, const std::string& flow_name, const generic_deviceRef _device);

   /**
    * Destructor
    */
   ~BashBackendFlow() override;
};
/// Refcount definition for the class
using BashBackendFlowRef = refcount<BashBackendFlow>;

#endif
