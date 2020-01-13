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
 * @file DesignParameters.cpp
 * @brief This file contains the implementation of the methods to manage the parameters of the synthesis flow
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
/// Autoheader include

#include "DesignParameters.hpp"

#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#if HAVE_IPXACT_BUILT
#include "ip_xact_xml.hpp"
#endif

/// Parameter include
#include "Parameter.hpp"

/// Utility include
#include "fileIO.hpp"

#if HAVE_IPXACT_BUILT
void DesignParameters::xload_design_configuration(const ParameterConstRef DEBUG_PARAMETER(Param), const std::string& xml_file)
{
   if(!boost::filesystem::exists(xml_file))
      THROW_ERROR("File \"" + xml_file + "\" does not exist!");
#ifndef NDEBUG
   unsigned int debug_level = Param->getOption<unsigned int>(OPT_debug_level);
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Parsing of design_configuration " + xml_file);
   XMLDomParser parser(xml_file);
   parser.Exec();
   if(parser)
   {
      const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
      const xml_node::node_list list = node->get_children();
      for(xml_node::node_list::const_iterator l = list.begin(); l != list.end(); ++l)
      {
         const xml_element* child = GetPointer<xml_element>(*l);
         if(!child)
            continue;

         if(child->get_name() == STR_XML_ip_xact_design_ref)
         {
            xml_attribute* name = child->get_attribute(STR_XML_ip_xact_name);
            if(name)
            {
               component_name = name->get_value();
               std::string token("design_");
               if(component_name.find(token) != std::string::npos)
                  component_name = component_name.substr(token.size(), component_name.size());
            }
         }
         if(child->get_name() == STR_XML_ip_xact_generator_chain_configuration)
         {
            const xml_node::node_list list_gen = child->get_children();
            for(xml_node::node_list::const_iterator g = list_gen.begin(); g != list_gen.end(); ++g)
            {
               const xml_element* child_gen = GetPointer<xml_element>(*g);
               if(!child_gen)
                  continue;
               if(child_gen->get_name() == STR_XML_ip_xact_generator_chain_ref)
               {
                  xml_attribute* name = child_gen->get_attribute(STR_XML_ip_xact_name);
                  if(name)
                  {
                     chain_name = name->get_value();
                  }
               }
               if(child_gen->get_name() == STR_XML_ip_xact_configurable_element_values)
               {
                  const xml_node::node_list list_values = child_gen->get_children();
                  for(xml_node::node_list::const_iterator v = list_values.begin(); v != list_values.end(); ++v)
                  {
                     const xml_element* child_value = GetPointer<xml_element>(*v);
                     if(!child_value)
                        continue;
                     if(child_value->get_name() == STR_XML_ip_xact_configurable_element_value)
                     {
                        std::string referenceId = child_value->get_attribute(STR_XML_ip_xact_reference_id)->get_value();
                        std::string value;
                        if(child_value->get_child_text())
                        {
                           value = child_value->get_child_text()->get_content();
                        }
                        INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---adding parameter \"" + referenceId + "\" with value \"" + value + "\"");
                        parameter_values[referenceId] = value;
                     }
                  }
               }
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--Parsed configuration file of design \"" + component_name + "\"");
}
#endif
