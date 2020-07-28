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
 * @file XilinxWrapper.cpp
 * @brief Implementation of the wrapper to Xilinx tools
 *
 * Implementation of the methods to wrap synthesis tools by Xilinx
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "XilinxWrapper.hpp"

#include "xml_dom_parser.hpp"
#include "xml_script_command.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "fileIO.hpp"
#include "utility.hpp"

#include <fstream>

XilinxWrapper::XilinxWrapper(const ParameterConstRef& _Param, const std::string& _tool_exec, const target_deviceRef& _device, const std::string& _output_dir, const std::string& _default_output_dir)
    : SynthesisTool(_Param, _tool_exec, _device, _output_dir, _default_output_dir)
{
}

XilinxWrapper::~XilinxWrapper() = default;

void XilinxWrapper::generate_synthesis_script(const DesignParametersRef& dp, const std::string& file_name)
{
   if(xml_script_nodes.empty())
   {
      return;
   }

   // Export reserved (constant) values to design parameters
   for(auto it = xml_reserved_vars.begin(); it != xml_reserved_vars.end(); ++it)
   {
      const xml_set_variable_tRef& var = (*it);
      // std::cerr << "setting = " << var->name << " #" << getStringValue(var, dp) << "#" << std::endl;
      dp->assign(var->name, getStringValue(var, dp), false);
   }

   // Bare script generation
   std::ostringstream script;
   script << generate_bare_script(xml_script_nodes, dp);

   // Replace all reserved variables with their value
   std::string script_string = script.str();
   replace_parameters(dp, script_string);
   /// replace some of the escape sequences
   remove_escaped(script_string);

   // Save the generated script
   if(boost::filesystem::exists(file_name))
   {
      boost::filesystem::remove_all(file_name);
   }
   dp->parameter_values[SCRIPT_FILENAME] = file_name;
   script_name = file_name;
   std::ofstream file_stream(file_name.c_str());
   file_stream << script_string << std::endl;
   file_stream.close();
}

std::string XilinxWrapper::getStringValue(const xml_script_node_tRef node, const DesignParametersRef& dp) const
{
   switch(node->nodeType)
   {
      case NODE_VARIABLE:
      {
         std::string result;
         const xml_set_variable_t* var = GetPointer<xml_set_variable_t>(node);
         if(var->singleValue)
         {
            result += *(var->singleValue);
         }
         else if(!var->multiValues.empty())
         {
            result += "{";
            for(auto it = var->multiValues.begin(); it != var->multiValues.end(); ++it)
            {
               const xml_set_entry_tRef e = *it;
               if(it != var->multiValues.begin())
               {
                  result += " ";
               }
               result += toString(e, dp);
            }
            result += "}";
         }
         else
         {
            result += "\"\"";
         }
         return result;
      }
      case NODE_COMMAND:
      case NODE_ENTRY:
      case NODE_FOREACH:
      case NODE_ITE_BLOCK:
      case NODE_PARAMETER:
      case NODE_SHELL:
      case NODE_UNKNOWN:
      default:
         THROW_ERROR("Not supported node type: " + STR(node->nodeType));
   }
   return "";
}

std::string XilinxWrapper::toString(const xml_script_node_tRef node, const DesignParametersRef dp) const
{
   switch(node->nodeType)
   {
      case NODE_ENTRY:
      {
         const xml_set_entry_t* ent = GetPointer<xml_set_entry_t>(node);
         return ent->value;
      }
      case NODE_VARIABLE:
      {
         const xml_set_variable_t* var = GetPointer<xml_set_variable_t>(node);
         return "set -" + var->name + " " + getStringValue(node, dp);
      }
      case NODE_PARAMETER:
      {
         const xml_parameter_t* par = GetPointer<xml_parameter_t>(node);
         std::string result;
         if(par->condition && !xml_script_node_t::evaluate_condition(par->condition, dp))
         {
            return result;
         }
         if(par->name)
         {
            result += "-" + *(par->name);
         }
         if(par->name && (par->singleValue || !par->multiValues.empty()))
         {
            result += par->separator;
         }
         if(par->singleValue)
         {
            result += *(par->singleValue);
         }
         else if(!par->multiValues.empty())
         {
            NOT_YET_IMPLEMENTED();
         }
         return result;
      }
      case NODE_COMMAND:
      {
         const xml_command_t* comm = GetPointer<xml_command_t>(node);
         std::string result;
         if(comm->name)
         {
            result += *(comm->name);
         }
         if(!comm->parameters.empty())
         {
            for(const auto& p : comm->parameters)
            {
               if(p->condition && !xml_script_node_t::evaluate_condition(p->condition, dp))
               {
                  continue;
               }
               result += "\n" + toString(p, dp);
            }
         }
         if(comm->output)
         {
            THROW_ERROR("Not supported");
         }
         return result;
      }
      case NODE_UNKNOWN:
      case NODE_SHELL:
      case NODE_ITE_BLOCK:
      case NODE_FOREACH:
      default:
         THROW_ERROR("Not supported node type: " + STR(node->nodeType));
   }
   return "";
}
