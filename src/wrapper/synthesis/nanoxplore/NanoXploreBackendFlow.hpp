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
 * @file NanoXploreBackendFlow.hpp
 * @brief Wrapper to NanoXplore synthesis tools
 * *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef _NANOXPLORE_BACKEND_FLOW_HPP_
#define _NANOXPLORE_BACKEND_FLOW_HPP_

/// superclass include
#include "BackendFlow.hpp"

#include "refcount.hpp"
REF_FORWARD_DECL(target_device);

#define PARAM_ucf_file "ucf_file"
#define PARAM_power_optimization "power_optimization"
#define PARAM_nxpython_sources_macro_list "nxpython_sources_macro_list"
#define PARAM_nxpython_report "nxpython_report"
#define PARAM_nxpython_timing_report "nxpython_timing_report"

class NanoXploreBackendFlow : public BackendFlow
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
    * Checks the synthesis results and fills the corresponding datastructures
    */
   void CheckSynthesisResults() override;

 public:
   /**
    * Constructor
    */
   NanoXploreBackendFlow(const ParameterConstRef Param, const std::string& flow_name, const target_managerRef manager);

   /**
    * Destructor
    */
   ~NanoXploreBackendFlow() override;

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
typedef refcount<NanoXploreBackendFlow> NanoXploreBackendFlowRef;

#endif
