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
 * @file XilinxBackendFlow.hpp
 * @brief Wrapper to implement a synthesis tools by Xilinx
 *
 * A object used to invoke synthesis tools by Xilinx
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _XILINX_BACKEND_FLOW_HPP_
#define _XILINX_BACKEND_FLOW_HPP_

/// superclass include
#include "BackendFlow.hpp"

#include "refcount.hpp"
REF_FORWARD_DECL(target_device);

#define PARAM_ucf_file "ucf_file"
#define PARAM_xcf_file "xcf_file"
#define PARAM_power_optimization "power_optimization"
#define PARAM_vivado_sources_macro_list "vivado_sources_macro_list"
#define PARAM_yosys_vivado_sources_macro_list "yosys_vivado_sources_macro_list"
#define PARAM_vivado_report "vivado_report"
#define PARAM_vivado_timing_report "vivado_timing_report"

class XilinxBackendFlow : public BackendFlow
{
 protected:
   /// results from the synthesis
   std::map<std::string, double> design_values;

   /**
    * Creates the UCF file
    */
   virtual void create_cf(const DesignParametersRef dp, bool xst);

   /**
    * Writes the proper flow configuration in the output script
    */
   void WriteFlowConfiguration(std::ostream& script) override;

   /**
    * Checks the synthesis results and fills the corresponding datastructures
    */
   void CheckSynthesisResults() override;

   /**
    * Parses the utilization file in XML format
    */
   void xparse_xst_utilization(const std::string& fn);

   /**
    * retrieve the number of DSPs from the xst log
    */
   void parse_DSPs(const std::string& log_file);

   /**
    * Parses the utilization file in XML format
    */
   void xparse_map_utilization(const std::string& fn);

   /**
    * parse vivado results
    * @param fn si the file where the synthesis results are written
    */
   void vivado_xparse_utilization(const std::string& fn);

   /**
    * Fixed the parsing of timing results from trce
    */
   void xparse_timing(const std::string& fn, bool post);

   /**
    * Fixed the parsing of timing results from xst
    */
   void parse_timing(const std::string& fn);

 public:
   /**
    * Constructor
    */
   XilinxBackendFlow(const ParameterConstRef Param, const std::string& flow_name, const target_managerRef manager);

   /**
    * Destructor
    */
   ~XilinxBackendFlow() override;

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
typedef refcount<XilinxBackendFlow> XilinxBackendFlowRef;

#endif
