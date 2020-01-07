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
 * @file BackendFlow.hpp
 * @brief This file contains the definition of the configurable flow for generating and executing synthesis scripts
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _BACKEND_FLOW_HPP_
#define _BACKEND_FLOW_HPP_

/// Autoheader include
#include "config_HAVE_IPXACT_BUILT.hpp"
#include "config_HAVE_SIMULATION_WRAPPER_BUILT.hpp"
#include "config_HAVE_TASTE.hpp"

#include <list>

#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(SynthesisTool);
REF_FORWARD_DECL(target_manager);
REF_FORWARD_DECL(target_device);
REF_FORWARD_DECL(area_model);
REF_FORWARD_DECL(time_model);
REF_FORWARD_DECL(Design_manager);
REF_FORWARD_DECL(BackendFlow);
REF_FORWARD_DECL(DesignParameters);
REF_FORWARD_DECL(HDL_manager);
REF_FORWARD_DECL(language_writer);
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(technology_node);
REF_FORWARD_DECL(XMLDomParser);
class xml_element;
typedef refcount<std::istream> fileIO_istreamRef;

#include "custom_map.hpp"
#include "custom_set.hpp"
#include <string>
#include <vector>

#include "DesignParameters.hpp"

struct BackendStep
{
   /// name of the step
   std::string name;

   /// configuration identifier
   std::string config_name;

   /// name of the step
   std::string script_name;

   /// wrapper to the tool
   SynthesisToolRef tool;

   /// output directory
   std::string out_dir;
};
typedef refcount<BackendStep> BackendStepRef;

class BackendFlow
{
 public:
   /// implemented flow
   typedef enum
   {
      UNKNOWN,
      ASIC,
      XILINX_FPGA,
#if HAVE_TASTE
      XILINX_TASTE_FPGA,
#endif
      ALTERA_FPGA,
      LATTICE_FPGA,
      NANOXPLORE_FPGA,
   } type_t;

 protected:
   /// class containing all the parameters
   const ParameterConstRef Param;

   /// debugging level of the class
   int debug_level;

   /// verbosity level of the class
   unsigned int output_level;

   /// string-based identifier of the flow
   std::string flow_name;

   /// name of the output directory
   std::string out_dir;

   /// map between the identifiers of the synthesis flows and the corresponding implementations
   std::map<std::string, std::string> default_data;

   /// information about the target device
   const target_managerRef target;

   /// root node of the configuration device
   xml_element* root;

   /// pointer to the datastructure containing information about the resources
   area_modelRef area_m;

   /// pointer to the datastructure containing timing information
   time_modelRef time_m;

   /// ordered list of synthesis steps
   std::vector<BackendStepRef> steps;

   /// set of design parameters to be set
   DesignParametersRef default_flow_parameters;

   /// list of undefined parameters
   CustomOrderedSet<std::string> undefined_parameters;

   /// set of design parameters with the actual values
   DesignParametersRef actual_parameters;

   /// name of the synthesis script
   std::string generated_synthesis_script;

   /**
    * Parses the description of the backend flow given its identifier
    * @param flow_name is the string that represents the identifier of the flow
    */
   void ParseBackendFlow(const std::string& flow_name);

   /**
    * Loads the backend flow from an XML node
    */
   void xload(const xml_element* node);

   /**
    * Writes the proper flow configuration in the output script
    */
   virtual void WriteFlowConfiguration(std::ostream& script) = 0;

   /**
    * Checks the synthesis results and fills the corresponding datastructures
    */
   virtual void CheckSynthesisResults() = 0;

 public:
   /**
    * Constructor
    * @param Param is the reference to the class containing all the parameters
    * @param flow_name is a string representing the name of the flow
    * @param target is the datastructure containing all the information about the target of the synthesis
    */
   BackendFlow(const ParameterConstRef Param, std::string flow_name, const target_managerRef target);

   /**
    * Destructor
    */
   virtual ~BackendFlow();

   /**
    * Creates the flow specification based on the given parameters
    */
   static BackendFlowRef CreateFlow(const ParameterConstRef Param, const std::string& flow_name, const target_managerRef target);

   /**
    * Determines the type of the backend flow based on the target device
    */
   static type_t DetermineBackendFlowType(const target_deviceRef device, const ParameterConstRef parameters);

   /**
    * Generates the synthesis scripts for the specified design
    */
   virtual std::string GenerateSynthesisScripts(const std::string& fu_name, const structural_managerRef SM, const std::list<std::string>& hdl_files, const std::list<std::string>& aux_files);

   /**
    * Executes the synthesis with the implemented flow
    */
   virtual void ExecuteSynthesis();

   /**
    * Executes the synthesis with the implemented flow
    */
   void Execute(const std::string& top_id, const std::string& top_normalized, const std::string& filestring, const std::string& filename_bench, const std::string& clock, double clk_period, bool syntax_check);

   /**
    * Creates the scripts for the specified tools in the right order, along with the overall configuration.
    */
   std::string CreateScripts(const DesignParametersRef dp);

   /**
    * Initializes the parameters
    */
   virtual void InitDesignParameters();

   /**
    * Creates the synthesis flow based on the user's requirements
    * @param xml_file is the XML-based configuration file of the entire flow
    */
   void parse_flow(const XMLDomParserRef parser);

   /**
    * Creates the XML script containing all the steps
    */
   void create_xml_scripts(const std::string& xml_file);

   /**
    * Adds a backend step to the list of the ones to be performed
    * @param step is the reference to the datastructure containing information about the step
    */
   void add_backend_step(const BackendStepRef& step);

   /**
    * Returns the name of the flow
    */
   std::string get_flow_name() const;

#if HAVE_IPXACT_BUILT
   /**
    * Parses an XML-based configuration based on IP-XACT standard
    */
   static BackendFlowRef xload_generator_chain(const ParameterConstRef Param, const std::string& xml_file);
#endif

   /**
    * Sets parameters
    */
   void set_initial_parameters(const DesignParametersRef& flow_parameters, const CustomOrderedSet<std::string>& undefined_parameters);

   /**
    * Sets actual parameters
    */
   void set_parameters(const DesignParametersRef& _flow_parameters);

   /**
    * Returns the list of used resources
    */
   area_modelRef get_used_resources() const;

   /**
    * Returns the timing information
    */
   time_modelRef get_timing_results() const;
};
/// refcount definition of the class
typedef refcount<BackendFlow> BackendFlowRef;

#endif
