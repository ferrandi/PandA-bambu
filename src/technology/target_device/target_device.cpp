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
 * @file target_device.cpp
 * @brief
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */
#include "target_device.hpp"

/// Autoheader include
#include "config_HAVE_CMOS_BUILT.hpp"
#if HAVE_CMOS_BUILT
#include "IC_device.hpp"
#endif
#include "FPGA_device.hpp"

#include "target_technology.hpp"
#include "technology_manager.hpp"

#include "exceptions.hpp"

/// XML includes
#include "fileIO.hpp"
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"
/// parameters' includes
#include "Parameter.hpp"
#include "constant_strings.hpp"
/// boost includes for file manipulations
#include "string_manipulation.hpp" // for GET_CLASS
#include <boost/filesystem.hpp>

target_device::target_device(const ParameterConstRef& _Param, const technology_managerRef& _TM, const TargetDevice_Type type)
    : device_type(type), Param(_Param), TM(_TM), core_height(0), core_width(0), debug_level(_Param->get_class_debug_level(GET_CLASS(*this)))
{
}

target_device::~target_device() = default;

target_deviceRef target_device::create_device(const TargetDevice_Type type, const ParameterConstRef& param, const technology_managerRef& TM)
{
   switch(type)
   {
#if HAVE_CMOS_BUILT
      case TargetDevice_Type::IC:
         return target_deviceRef(new IC_device(param, TM));
#endif
      case TargetDevice_Type::FPGA:
         return target_deviceRef(new FPGA_device(param, TM));
      default:
         THROW_UNREACHABLE("");
   }
   /// this point should never be reached
   return target_deviceRef();
}

void target_device::xload(const target_deviceRef& device, const xml_element* node)
{
   const xml_node::node_list& c_list = node->get_children();
   for(const auto& n : c_list)
   {
      if(n->get_name() == "device")
      {
         const auto* dev_xml = GetPointer<const xml_element>(n);
         xload_device_parameters(dev_xml);
      }
   }

   for(const auto& n : c_list)
   {
      // The second part of the condition is false when we are generating the list of functional units in spider
      if(n->get_name() == "technology" and (not Param->isOption(OPT_input_format) or Param->getOption<Parameters_FileFormat>(OPT_input_format) != Parameters_FileFormat::FF_XML_TEC))
      {
         const auto* tech_xml = GetPointer<const xml_element>(n);
         TM->xload(tech_xml, device);
      }
   }
}

void target_device::xload_device_parameters(const xml_element* dev_xml)
{
   const xml_node::node_list& t_list = dev_xml->get_children();
   for(const auto& t : t_list)
   {
      const auto* t_elem = GetPointer<const xml_element>(t);
      if(!t_elem)
      {
         continue;
      }

      std::string value;
      LOAD_XVM(value, t_elem);

      parameters[t_elem->get_name()] = value;
      if(t_elem->get_name() == "model")
      {
         const_cast<Parameter*>(Param.get())->setOption("device_name", value);
      }
      if(t_elem->get_name() == "speed_grade")
      {
         const_cast<Parameter*>(Param.get())->setOption("device_speed", value);
      }
      if(t_elem->get_name() == "package")
      {
         const_cast<Parameter*>(Param.get())->setOption("device_package", value);
      }
   }
   if(device_type == TargetDevice_Type::FPGA)
   {
      std::string device_string = Param->getOption<std::string>("device_name") + Param->getOption<std::string>("device_speed") + Param->getOption<std::string>("device_package");
      const_cast<Parameter*>(Param.get())->setOption(OPT_device_string, device_string);
   }
}

double target_device::get_core_height() const
{
   return core_height;
}

double target_device::get_core_width() const
{
   return core_width;
}

target_technologyRef target_device::get_target_technology() const
{
   return target;
}

technology_managerRef target_device::get_technology_manager() const
{
   return TM;
}
