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
 *              Copyright (C) 2023-2024 Politecnico di Milano
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
 * @file generic_device.cpp
 * @brief Generic device description
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "generic_device.hpp"

#include "Parameter.hpp"
#include "config_PANDA_DATA_INSTALLDIR.hpp"
#include "constant_strings.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "polixml.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "technology_manager.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"
#include <filesystem>

generic_device::generic_device(const ParameterConstRef& _Param, const technology_managerRef& _TM)
    : Param(_Param), TM(_TM), debug_level(_Param->get_class_debug_level(GET_CLASS(*this)))
{
}

generic_device::~generic_device() = default;

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
   }

   for(const auto& n : c_list)
   {
      // The second part of the condition is false when we are generating the list of functional units in spider
      if(n->get_name() == "technology" and
         (not Param->isOption(OPT_input_format) or
          Param->getOption<Parameters_FileFormat>(OPT_input_format) != Parameters_FileFormat::FF_XML_TEC))
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
}

void generic_device::load_devices()
{
   /// map between the default device string and the corresponding configuration stream
   std::map<std::string, std::string> default_device_data;
   /// Load default device options
   default_device_data["xc4vlx100-10ff1513"] = "xc4vlx100-10ff1513.data";
   default_device_data["xc5vlx50-3ff1153"] = "xc5vlx50-3ff1153.data";
   default_device_data["xc5vlx110t-1ff1136"] = "xc5vlx110t-1ff1136.data";
   default_device_data["xc5vlx330t-2ff1738"] = "xc5vlx330t-2ff1738.data";
   default_device_data["xc6vlx240t-1ff1156"] = "xc6vlx240t-1ff1156.data";
   default_device_data["xc7z020-1clg484"] = "xc7z020-1clg484.data";
   default_device_data["xc7z020-1clg484-VVD"] = "xc7z020-1clg484-VVD.data";
   default_device_data["xc7z045-2ffg900-VVD"] = "xc7z045-2ffg900-VVD.data";
   default_device_data["xc7z020-1clg484-YOSYS-VVD"] = "xc7z020-1clg484-YOSYS-VVD.data";
   default_device_data["xc7vx485t-2ffg1761-VVD"] = "xc7vx485t-2ffg1761-VVD.data";
   default_device_data["xc7vx690t-3ffg1930-VVD"] = "xc7vx690t-3ffg1930-VVD.data";
   default_device_data["xc7vx330t-1ffg1157"] = "xc7vx330t-1ffg1157.data";
   default_device_data["xc7a100t-1csg324-VVD"] = "xc7a100t-1csg324-VVD.data";
   default_device_data["xcku060-3ffva1156-VVD"] = "xcku060-3ffva1156-VVD.data";
   default_device_data["xcu280-2Lfsvh2892-VVD"] = "xcu280-2Lfsvh2892-VVD.data";
   default_device_data["xcu55c-2Lfsvh2892-VVD"] = "xcu55c-2Lfsvh2892-VVD.data";

   default_device_data["EP2C70F896C6"] = "EP2C70F896C6.data";
   default_device_data["EP2C70F896C6-R"] = "EP2C70F896C6-R.data";
   default_device_data["5CSEMA5F31C6"] = "5CSEMA5F31C6.data";
   default_device_data["EP4SGX530KH40C2"] = "EP4SGX530KH40C2.data";
   default_device_data["5SGXEA7N2F45C1"] = "5SGXEA7N2F45C1.data";
   default_device_data["LFE335EA8FN484C"] = "LFE335EA8FN484C.data";
   default_device_data["LFE5UM85F8BG756C"] = "LFE5UM85F8BG756C.data";
   default_device_data["LFE5U85F8BG756C"] = "LFE5U85F8BG756C.data";
   default_device_data["nx1h35S"] = "nx1h35S.data";
   default_device_data["nx1h140tsp"] = "nx1h140tsp.data";
   default_device_data["nx2h540tsc"] = "nx2h540tsc.data";

   default_device_data["nangate45"] = "nangate45.data";
   default_device_data["asap7-BC"] = "asap7-BC.data";
   default_device_data["asap7-TC"] = "asap7-TC.data";
   default_device_data["asap7-WC"] = "asap7-WC.data";

   auto output_level = Param->getOption<unsigned int>(OPT_output_level);

   try
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Available devices:");
      for(auto d = default_device_data.begin(); d != default_device_data.end(); ++d)
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, " - " + d->first);
      }

      const CustomSet<XMLDomParserRef> parsers = [&]() -> CustomSet<XMLDomParserRef> {
         CustomSet<XMLDomParserRef> ret;
         if(Param->isOption(OPT_target_device_file))
         {
            const auto file_devices = Param->getOption<std::vector<std::string>>(OPT_target_device_file);
            for(const auto& file_device : file_devices)
            {
               PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Imported user data from file " + file_device);
               ret.insert(XMLDomParserRef(new XMLDomParser(file_device)));
            }
         }
         else
         {
            auto device_string = Param->getOption<std::string>(OPT_device_string);

            if(default_device_data.find(device_string) != default_device_data.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Loading " + device_string);
               ret.insert(XMLDomParserRef(
                   new XMLDomParser(relocate_compiler_path(PANDA_DATA_INSTALLDIR "/panda/technology/", true) +
                                    default_device_data[device_string])));
            }
            else
            {
               THROW_ERROR("Target device not supported: " + device_string);
            }
         }
         return ret;
      }();

      for(const auto& parser : parsers)
      {
         parser->Exec();
         if(parser and *parser)
         {
            const xml_element* node = parser->get_document()->get_root_node(); // deleted by DomParser.
            xload(node);
         }
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
   std::string device_string = Param->getOption<std::string>("device_name") +
                               Param->getOption<std::string>("device_speed") +
                               Param->getOption<std::string>("device_package");
   const_cast<Parameter*>(Param.get())->setOption(OPT_device_string, device_string);
}

technology_managerRef generic_device::get_technology_manager() const
{
   return TM;
}
