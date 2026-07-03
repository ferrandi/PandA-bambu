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
 *              Copyright (C) 2023-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file generic_device.cpp
 * @brief Generic device description
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "generic_device.hpp"

#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "polixml.hpp"
#include "string_manipulation.hpp"
#include "technology_manager.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include <filesystem>

#include "config_PANDA_LIB_INSTALLDIR.hpp"

generic_device::generic_device(const ParameterConstRef& _Param, const technology_managerRef& _TM)
    : Param(_Param), TM(_TM), debug_level(_Param->get_class_debug_level(GET_CLASS(*this)))
{
}

generic_deviceRef generic_device::factory(const ParameterConstRef& param, const technology_managerRef& TM)
{
   return generic_deviceRef(new generic_device(param, TM));
}

void generic_device::xload(const xml_element* node)
{
   const xml_node::node_list& c_list = node->get_children();
   for(const auto& n : c_list)
   {
      if(n->get_name() == "device")
      {
         const auto* dev_xml = GetPointer<const xml_element>(n);
         xload_device_parameters(dev_xml);
      }
      else if(n->get_name() == "backend")
      {
         const auto back_xml = GetPointer<const xml_element>(n);
         for(const auto& t : back_xml->get_children())
         {
            const auto* t_elem = GetPointer<const xml_element>(t);
            if(!t_elem)
            {
               continue;
            }

            std::string id, env;
            LOAD_XVM(id, t_elem);
            LOAD_XVM(env, t_elem);

            if(!backends.emplace(id, env).second)
            {
               THROW_WARNING("Duplicate backend flow for " + Param->getOption<std::string>(OPT_device_string) +
                             " device.");
            }
         }
      }
   }

   for(const auto& n : c_list)
   {
      if(n->get_name() == "technology")
      {
         const auto* tech_xml = GetPointer<const xml_element>(n);
         TM->xload(tech_xml);
      }
   }
}

void generic_device::xwrite(xml_element* nodeRoot)
{
   xml_element* tmRoot = nodeRoot->add_child_element("device");

   THROW_ASSERT(has_parameter("vendor"), "vendor value is missing");
   xml_element* vendor_el = tmRoot->add_child_element("vendor");
   auto vendor = get_parameter<std::string>("vendor");
   WRITE_XNVM2("value", vendor, vendor_el);

   THROW_ASSERT(has_parameter("family"), "family value is missing");
   xml_element* family_el = tmRoot->add_child_element("family");
   auto family = get_parameter<std::string>("family");
   WRITE_XNVM2("value", family, family_el);

   THROW_ASSERT(has_parameter("model"), "model value is missing");
   xml_element* model_el = tmRoot->add_child_element("model");
   auto model = get_parameter<std::string>("model");
   WRITE_XNVM2("value", model, model_el);

   THROW_ASSERT(has_parameter("package"), "package value is missing");
   xml_element* package_el = tmRoot->add_child_element("package");
   auto package = get_parameter<std::string>("package");
   WRITE_XNVM2("value", package, package_el);

   THROW_ASSERT(has_parameter("speed_grade"), "speed_grade value is missing");
   xml_element* speed_grade_el = tmRoot->add_child_element("speed_grade");
   auto speed_grade = get_parameter<std::string>("speed_grade");
   WRITE_XNVM2("value", speed_grade, speed_grade_el);

   for(auto p = parameters.begin(); p != parameters.end(); ++p)
   {
      if(p->first == "vendor" || p->first == "family" || p->first == "model" || p->first == "package" ||
         p->first == "speed_grade" || p->first == "clock_period")
      {
         continue;
      }
      xml_element* elRoot = tmRoot->add_child_element(p->first);
      WRITE_XNVM2("value", p->second, elRoot);
   }

   xml_element* backendRoot = nodeRoot->add_child_element("backend");
   for(const auto& [id, env] : backends)
   {
      xml_element* flow = backendRoot->add_child_element("flow");
      WRITE_XNVM2("id", id, flow);
      WRITE_XNVM2("env", env, flow);
   }
}

void generic_device::load_devices()
{
   const auto load_data = [&](const std::string& device_data) {
      XMLDomParser parser(device_data);
      parser.Exec();
      if(parser)
      {
         xload(parser.get_document()->get_root_node());
      }
   };
   std::vector<std::string> data_files;
   auto out_lvl = Param->getOption<unsigned int>(OPT_output_level);

   try
   {
      if(Param->isOption(OPT_target_device_file))
      {
         const auto file_devices = Param->getOption<std::vector<std::string>>(OPT_target_device_file);
         for(const auto& file_device : file_devices)
         {
            PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, out_lvl, "Imported user data from file " + file_device);
            load_data(file_device);
         }
      }
      else
      {
         const auto device_string = Param->getOption<std::string>(OPT_device_string);
         const auto device_data =
             relocate_install_path(PANDA_LIB_INSTALLDIR "/libtech/targets/" + device_string + ".spec_data");
         if(!std::filesystem::exists(device_data))
         {
            THROW_ERROR("Target device not supported: " + device_string);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Loading " + device_string);
         load_data(device_data);
      }

      return;
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
   THROW_ERROR("Error during XML parsing of device files");
}

void generic_device::xload_device_parameters(const xml_element* dev_xml)
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

      bool is_bash_var = false;
      if(CE_XVM(is_bash_var, t_elem))
      {
         LOAD_XVM(is_bash_var, t_elem);
      }
      if(is_bash_var)
      {
         vars[t_elem->get_name()] = value;
      }
      else
      {
         parameters[t_elem->get_name()] = value;
         const_cast<Parameter*>(Param.get())->SetPandaParameterFromDevice(t_elem->get_name(), value);
      }
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
   const auto device_string = Param->getOption<std::string>("device_name") +
                              Param->getOption<std::string>("device_speed") +
                              Param->getOption<std::string>("device_package");
   const_cast<Parameter*>(Param.get())->setOption(OPT_device_string, device_string);
}

technology_managerRef generic_device::get_technology_manager() const
{
   return TM;
}
