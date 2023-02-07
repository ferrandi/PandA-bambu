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
 * @file bash_flow_wrapper.hpp
 * @brief Wrapper to invoke a generic bash script
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef GENERIC_BASH_FLOW_HPP
#define GENERIC_BASH_FLOW_HPP

#include "SynthesisTool.hpp"

#define BASH_FLOW_TOOL_ID "bash_flow"
#define BASH_FLOW_TOOL_EXEC "bash"

/**
 * @class bash_flow_wrapper
 * Main class for wrapping bash_flow tool by NANOXPLORE
 */
class bash_flow_wrapper : public SynthesisTool
{
 protected:
   /**
    * Evaluates the design variables
    */
   void EvaluateVariables(const DesignParametersRef dp) override;

   /**
    * Creates the proper configuration script
    */
   void generate_synthesis_script(const DesignParametersRef& dp, const std::string& file_name) override;

   /**
    * Returns the string-based representation of the XML element
    */
   std::string toString(const xml_script_node_tRef node, const DesignParametersRef dp) const override;

   /**
    * Returns the string-based representation of the XML element
    */
   std::string getStringValue(const xml_script_node_tRef node, const DesignParametersRef& dp) const override;

   /**
    * Returns the proper command line
    */
   std::string get_command_line(const DesignParametersRef& dp) const override;

 public:
   /**
    * Constructor
    * @param Param is the set of parameters
    */
   bash_flow_wrapper(const ParameterConstRef& Param, const std::string& _output_dir, const target_deviceRef& _device);

   /**
    * Destructor
    */
   ~bash_flow_wrapper() override;
};
/// Refcount definition for the class
using bash_flow_wrapperRef = refcount<bash_flow_wrapper>;

#endif
