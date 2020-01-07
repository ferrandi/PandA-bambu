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
 * @file target_technology.cpp
 * @brief
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */

/// Autoheader include
#include "config_HAVE_CMOS_BUILT.hpp"

#include "target_technology.hpp"

#if HAVE_CMOS_BUILT
#include "CMOS_technology.hpp"
#endif
#include "FPGA_technology.hpp"

#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include <boost/lexical_cast.hpp>

target_technology::target_technology(const ParameterConstRef& param) : Param(param), type(FPGA)
{
}

target_technology::~target_technology() = default;

target_technologyRef target_technology::create_technology(const target_t type, const ParameterConstRef& param)
{
   switch(type)
   {
#if HAVE_CMOS_BUILT
      case CMOS:
         return target_technologyRef(new CMOS_technology(param));
#endif
      case FPGA:
         return target_technologyRef(new FPGA_technology(param));
      default:
         THROW_ERROR("Not supported target technology");
   }
   /// this point should never be reached
   return target_technologyRef();
}

target_technology::target_t target_technology::get_type() const
{
   return type;
}

void target_technology::xload(const xml_element* node)
{
   const xml_node::node_list& c_list = node->get_children();
   for(const auto& n : c_list)
   {
      if(n->get_name() == "technology")
      {
         const auto* tech_xml = GetPointer<const xml_element>(n);
         xload_technology_parameters(tech_xml);
      }
   }
   return;
}

void target_technology::xload_technology_parameters(const xml_element* tech_xml)
{
   const xml_node::node_list& t_list = tech_xml->get_children();
   for(const auto& t : t_list)
   {
      const auto* t_elem = GetPointer<const xml_element>(t);
      if(!t_elem)
      {
         continue;
      }

      if(t_elem->get_name() == "parameter")
      {
         std::string name;
         LOAD_XVM(name, t_elem);
         std::string value;
         LOAD_XVM(value, t_elem);
         parameters[name] = value;
      }
   }
}
