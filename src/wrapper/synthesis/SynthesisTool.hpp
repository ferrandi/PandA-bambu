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
 * @file SynthesisTool.hpp
 * @brief Abstract class for a generic synthesis tool
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _SYNTHESIS_TOOL_HPP_
#define _SYNTHESIS_TOOL_HPP_

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"

#include <string>
#include <vector>

#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(SynthesisTool);
REF_FORWARD_DECL(ToolManager);
REF_FORWARD_DECL(DesignParameters);
REF_FORWARD_DECL(xml_node);
REF_FORWARD_DECL(xml_script_node_t);
REF_FORWARD_DECL(xml_set_variable_t);
REF_FORWARD_DECL(xml_parameter_t);
REF_FORWARD_DECL(target_device);
class xml_element;

#include "DesignParameters.hpp"
#define ADD_RES_VAR(name) xml_reserved_vars.push_back(xml_set_variable_tRef(new xml_set_variable_t((name), nullptr, nullptr)))

#define PARAM_top_id "top_id"
#define PARAM_target_device "target_device"
#define PARAM_target_family "target_family"
#define PARAM_HDL_files "HDL_files"
#define PARAM_clk_name "clk_name"
#define PARAM_clk_period "clk_period"
#define PARAM_clk_period_default (1.0 / 50)
#define PARAM_clk_freq "clk_freq"
#define PARAM_time_constrained "time_constrained"
#define PARAM_is_combinational "is_combinational"
#define PARAM_sdc_file "sdc_file"
#define PARAM_backend_script_extensions "backend_script_extensions"
#define PARAM_has_script_extensions "has_script_extensions"
#define PARAM_has_VHDL_library "has_VHDL_library"
#define PARAM_VHDL_library "VHDL_library"
#define PARAM_connect_iob "connect_iob"

/// used by Intel/Altera Characterization
#define PARAM_fu "fu"

class SynthesisTool
{
 public:
   /// supported synthesis tools
   typedef enum
   {
      UNKNOWN = 0,
      DESIGN_COMPILER,
#if HAVE_EXPERIMENTAL
      PRIME_TIME,
      FORMALITY,
      LIBRARY_COMPILER,
      LIBRARY_CREATOR,
      DESIGN_OPTIMIZER,
      SOC_ENCOUNTER,
      XPWR,
#endif
      XST,
      NGDBUILD,
      MAP,
      PAR,
      TRCE,
      VIVADO_FLOW,
      QUARTUS_13_FLOW,
      QUARTUS_13_SETUP,
      QUARTUS_13_STA,
      QUARTUS_SETUP,
      QUARTUS_FLOW,
      QUARTUS_POW,
      QUARTUS_STA,
      LATTICE_FLOW,
      NXPYTHON_FLOW
   } type_t;

 protected:
   /// class containing information about the target device
   const target_deviceRef device;

   /// class containing all the parameters
   const ParameterConstRef Param;

   /// debug level of the class
   int debug_level;

   /// verbosity level of the class
   unsigned int output_level;

   /// utility class to manage the executable
   ToolManagerRef tool;

   /// name of the tool executable
   const std::string tool_exec;

   /// the output directory
   std::string output_dir;

   /// name of the script
   std::string script_name;

   /// map between the identifier of the script and the corresponding stream
   std::map<unsigned int, std::string> script_map;

   /**
    * Creates the directory to store the output files
    */
   void create_output_directory(const std::string& sub_dir);

   // Script from XML
   std::vector<xml_parameter_tRef> xml_tool_options;
   std::vector<xml_script_node_tRef> xml_script_nodes;
   std::vector<xml_set_variable_tRef> xml_reserved_vars;

   /**
    * Initializes the reserved variables
    */
   virtual void init_reserved_vars();

 public:
   /**
    * Constructor
    */
   SynthesisTool(const ParameterConstRef& Param, std::string tool_exec, const target_deviceRef& device, const std::string& output_dir, std::string default_output_dir);

   /**
    * Destructor
    */
   virtual ~SynthesisTool();

   /**
    * Check if the tool can be really executed; i.e., it has been properly configured
    */
   virtual void CheckExecution();

   /**
    * Evaluates the design variables
    */
   virtual void EvaluateVariables(const DesignParametersRef dp);

   /**
    * Creates the proper configuration script
    */
   virtual void generate_synthesis_script(const DesignParametersRef& dp, const std::string& file_name) = 0;

   /**
    * @return The tool command line
    */
   virtual std::string get_command_line(const DesignParametersRef& dp) const = 0;

   /**
    * Returns the string-based representation of the XML element
    */
   virtual std::string toString(const xml_script_node_tRef node, const DesignParametersRef dp) const = 0;

   /**
    * Returns the string-based representation of the XML element
    */
   virtual std::string getStringValue(const xml_script_node_tRef node, const DesignParametersRef& dp) const = 0;

   /**
    * Returns the path of the output directory
    */
   std::string get_output_directory() const;

   /**
    * @return The bare script without any special string replacement
    */
   std::string generate_bare_script(const std::vector<xml_script_node_tRef>& nodes, const DesignParametersRef& dp);

   /**
    * Gets a reserved (tool) parameter by name.
    *
    * @param name Parameter name.
    * @return Reserved parameter reference.
    */
   xml_set_variable_tRef get_reserved_parameter(const std::string& name);

   /**
    * Replaces occurrences of parameters inside a script.
    * @param dp Design parameters.
    * @param script Script whose parameters have to be replaced.
    */
   void replace_parameters(const DesignParametersRef& dp, std::string& script) const;

   /**
    * Factory method
    */
   static SynthesisToolRef create_synthesis_tool(type_t type, const ParameterConstRef& Param, const std::string& output_dir, const target_deviceRef& device);

   /**
    * Actual parsing of parameters and script nodes
    */
   void xload_scripts(const xml_element* child);

   /**
    * Method parsing the configuration file directly from an XML node
    */
   void xload(const xml_element* node, const std::string& tool_config);

   /**
    * Method writing the configuration file
    */
   xml_nodeRef xwrite() const;

   /**
    * Checks if there is a configuration script loaded
    */
   bool has_scripts() const;

   /**
    * Returns the name of the tool executable
    */
   virtual std::string get_tool_exec() const;
};
/// refcount definition of the class
typedef refcount<SynthesisTool> SynthesisToolRef;

#endif
