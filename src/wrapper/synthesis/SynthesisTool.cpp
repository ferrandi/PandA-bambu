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
 * @file SynthesisTool.cpp
 * @brief Implementation of some methods for the interface to synthesis tools
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#include "SynthesisTool.hpp"

#include <utility>

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"

/// supported synthesis tools
#include "DesignCompilerWrapper.hpp"
#include "lattice_flow_wrapper.hpp"
#include "map_wrapper.hpp"
#include "ngdbuild_wrapper.hpp"
#include "nxpython_flow_wrapper.hpp"
#include "par_wrapper.hpp"
#include "quartus_13_report_wrapper.hpp"
#include "quartus_13_wrapper.hpp"
#include "quartus_power_wrapper.hpp"
#include "quartus_report_wrapper.hpp"
#include "quartus_wrapper.hpp"
#include "trce_wrapper.hpp"
#include "vivado_flow_wrapper.hpp"
#include "xst_wrapper.hpp"

#if HAVE_EXPERIMENTAL
#if 0
#include "DesignOptimizerWrapper.hpp"
#include "FormalityWrapper.hpp"
#include "LibraryCompilerWrapper.hpp"
#include "LibraryCreatorWrapper.hpp"
#include "PrimeTimeWrapper.hpp"
#include "SocEncounterWrapper.hpp"
#include "xpwr_wrapper.hpp"
#endif
#endif

#include "Parameter.hpp"

#include "boost/filesystem.hpp"

#include "exceptions.hpp"

#include "fileIO.hpp"
#include "utility.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include "DesignParameters.hpp"
#include "xml_script_command.hpp"

SynthesisTool::SynthesisTool(const ParameterConstRef& _Param, std::string _tool_exec, const target_deviceRef& _device, const std::string& _flow_name, std::string _output_dir)
    : device(_device), Param(_Param), debug_level(Param->getOption<int>(OPT_debug_level)), output_level(Param->getOption<unsigned int>(OPT_output_level)), tool_exec(std::move(_tool_exec)), output_dir(std::move(_output_dir))
{
   /// creating the output directory
   create_output_directory(_flow_name);

   init_reserved_vars();
}

SynthesisTool::~SynthesisTool() = default;

bool SynthesisTool::has_scripts() const
{
   return !script_map.empty() || !xml_script_nodes.empty();
}

SynthesisToolRef SynthesisTool::create_synthesis_tool(type_t type, const ParameterConstRef& _Param, const std::string& _output_dir, const target_deviceRef& _device)
{
   switch(type)
   {
      case UNKNOWN:
         THROW_ERROR("Synthesis tool not specified");
         break;
      case DESIGN_COMPILER:
         return SynthesisToolRef(new DesignCompilerWrapper(_Param, _device, _output_dir));
         break;
      case XST:
         return SynthesisToolRef(new xst_wrapper(_Param, _output_dir, _device));
         break;
      case NGDBUILD:
         return SynthesisToolRef(new ngdbuild_wrapper(_Param, _output_dir, _device));
         break;
      case MAP:
         return SynthesisToolRef(new map_wrapper(_Param, _output_dir, _device));
         break;
      case TRCE:
         return SynthesisToolRef(new trce_wrapper(_Param, _output_dir, _device));
         break;
      case PAR:
         return SynthesisToolRef(new par_wrapper(_Param, _output_dir, _device));
         break;
      case VIVADO_FLOW:
         return SynthesisToolRef(new vivado_flow_wrapper(_Param, _output_dir, _device));
         break;
      case QUARTUS_13_SETUP:
         return SynthesisToolRef(new Quartus13Wrapper(_Param, _output_dir, _device));
         break;
      case QUARTUS_13_STA:
         return SynthesisToolRef(new Quartus13ReportWrapper(_Param, _output_dir, _device));
         break;
      case QUARTUS_13_FLOW:
         return SynthesisToolRef(new Quartus13Wrapper(_Param, _output_dir, _device));
         break;
      case QUARTUS_SETUP:
         return SynthesisToolRef(new QuartusWrapper(_Param, _output_dir, _device));
         break;
      case QUARTUS_FLOW:
         return SynthesisToolRef(new QuartusWrapper(_Param, _output_dir, _device));
         break;
      case QUARTUS_POW:
         return SynthesisToolRef(new QuartusPowerWrapper(_Param, _output_dir, _device));
         break;
      case QUARTUS_STA:
         return SynthesisToolRef(new QuartusReportWrapper(_Param, _output_dir, _device));
         break;
      case LATTICE_FLOW:
         return SynthesisToolRef(new lattice_flow_wrapper(_Param, _output_dir, _device));
         break;
      case NXPYTHON_FLOW:
         return SynthesisToolRef(new nxpython_flow_wrapper(_Param, _output_dir, _device));
         break;
#if(0 && HAVE_EXPERIMENTAL)
      case PRIME_TIME:
         return SynthesisToolRef(new PrimeTimeWrapper(_Param, _device, _output_dir));
         break;
      case FORMALITY:
         return SynthesisToolRef(new FormalityWrapper(_Param, _device, _output_dir));
         break;
      case LIBRARY_COMPILER:
         return SynthesisToolRef(new LibraryCompilerWrapper(_Param, _device, _output_dir));
         break;
      case SOC_ENCOUNTER:
         return SynthesisToolRef(new SoCEncounterWrapper(_Param, _device, _output_dir));
         break;
      case LIBRARY_CREATOR:
         return SynthesisToolRef(new LibraryCreatorWrapper(_Param, _device, _output_dir));
         break;
      case DESIGN_OPTIMIZER:
         return SynthesisToolRef(new DesignOptimizerWrapper(_Param, _device, _output_dir));
         break;
      case XPWR:
         return SynthesisToolRef(new xpwr_wrapper(_Param, _output_dir));
         break;
#else
#if HAVE_EXPERIMENTAL
      case PRIME_TIME:
      case FORMALITY:
      case LIBRARY_COMPILER:
      case SOC_ENCOUNTER:
      case LIBRARY_CREATOR:
      case DESIGN_OPTIMIZER:
      case XPWR:
#endif
#endif
      default:
         THROW_ERROR("Synthesis tool currently not supported");
   }
   /// this point should never be reached
   return SynthesisToolRef();
}

void SynthesisTool::CheckExecution()
{
}

void SynthesisTool::init_reserved_vars()
{
}

std::string SynthesisTool::get_output_directory() const
{
   return output_dir;
}

void SynthesisTool::create_output_directory(const std::string& sub_dir)
{
   auto general_output_dir = Param->getOption<std::string>(OPT_output_directory);
   if(!sub_dir.empty())
   {
      general_output_dir += "/" + sub_dir;
   }
   output_dir = general_output_dir + std::string("/") + output_dir;

   std::string candidate_dir;
   if(boost::filesystem::exists(output_dir))
   {
      unsigned int progressive = 0;
      do
      {
         candidate_dir = output_dir + "_" + boost::lexical_cast<std::string>(progressive++);
      } while(boost::filesystem::exists(candidate_dir));
      output_dir = candidate_dir;
   }
   boost::filesystem::create_directories(output_dir);
   boost::filesystem::create_directories(output_dir + "/input");
   boost::filesystem::create_directories(output_dir + "/output");
}

void SynthesisTool::xload_scripts(const xml_element* child)
{
   xml_script_nodes.clear();
   xml_tool_options.clear();

   bool nodes_found = false;
   const xml_node::node_list& lines = child->get_children();
   for(const auto& line : lines)
   {
      const xml_element* script_element = GetPointer<xml_element>(line);
      if(!script_element)
      {
         continue;
      }
      if(xml_script_node_t::find_type(script_element) == NODE_PARAMETER)
      {
         if(nodes_found)
         {
            THROW_ERROR("Cannot instantiate tool options after script nodes");
         }
         xml_parameter_tRef option = xml_parameter_tRef(new xml_parameter_t(script_element));
         xml_tool_options.push_back(option);
      }
      else
      {
         xml_script_node_tRef script_node = xml_script_node_tRef(xml_script_node_t::create(script_element));
         xml_script_nodes.push_back(script_node);
         nodes_found = true;
      }
   }
}

void SynthesisTool::xload(const xml_element* node, const std::string& tool_config)
{
   bool node_found = false;
   const xml_node::node_list& list = node->get_children();
   for(const auto& l : list)
   {
      const xml_element* child = GetPointer<xml_element>(l);
      if(!child)
      {
         continue;
      }
      if(child->get_name() != get_tool_exec())
      {
         continue;
      }
      std::string config;
      if(!CE_XVM(config, child))
      {
         THROW_ERROR("missing configuration for the tool: " + get_tool_exec());
      }
      LOAD_XVM(config, child);
      if(config != tool_config)
      {
         continue;
      }
      if(node_found)
      {
         THROW_ERROR("double configuration for <" + get_tool_exec() + "," + tool_config);
      }
      node_found = true;

      xload_scripts(child);
   }
}

xml_nodeRef SynthesisTool::xwrite() const
{
   xml_elementRef root = xml_elementRef(new xml_element(get_tool_exec()));
   for(const auto& node : xml_tool_options)
   {
      root->add_child_element(node->create_xml_node());
   }
   for(const auto& node : xml_script_nodes)
   {
      root->add_child_element(node->create_xml_node());
   }
   return root;
}

std::string SynthesisTool::generate_bare_script(const std::vector<xml_script_node_tRef>& nodes, const DesignParametersRef& dp)
{
   std::string script;
   for(const auto& node : nodes)
   {
      if(node->checkCondition(dp) || node->nodeType == NODE_ITE_BLOCK)
      {
         script += toString(node, dp);
         script += "\n";
      }
   }
   return script;
}

xml_set_variable_tRef SynthesisTool::get_reserved_parameter(const std::string& name)
{
   for(auto i = xml_reserved_vars.begin(); i != xml_reserved_vars.end(); ++i)
   {
      const xml_set_variable_tRef& node = *i;
      if(name == node->name)
      {
         return node;
      }
   }
   THROW_ERROR("\"" + name + "\" is not a reserved tool parameter");
   return xml_set_variable_tRef(new xml_set_variable_t(nullptr));
}

void SynthesisTool::replace_parameters(const DesignParametersRef& dp, std::string& script) const
{
   const DesignParameters::map_t map = dp->parameter_values;
   for(const auto& it : map)
   {
      const std::string& name = it.first;
      const std::string& value = it.second;
      boost::algorithm::replace_all(script, "${__" + name + "__}", value);
      boost::algorithm::replace_all(script, "$__" + name + "__", value);
   }
}

void SynthesisTool::EvaluateVariables(const DesignParametersRef /*dp*/)
{
}

std::string SynthesisTool::get_tool_exec() const
{
   return tool_exec;
}
