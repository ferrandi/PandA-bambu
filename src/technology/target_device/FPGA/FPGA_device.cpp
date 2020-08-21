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
 * @file FPGA_device.cpp
 * @brief
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */
#include "FPGA_device.hpp"

#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_PANDA_DATA_INSTALLDIR.hpp"

#include "target_technology.hpp"

/// XML includes
#include "fileIO.hpp"
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

/// parameters' includes
#include "Parameter.hpp"
#include "constant_strings.hpp"

/// Boost includes
#include "boost/filesystem.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

FPGA_device::FPGA_device(const ParameterConstRef& _Param, const technology_managerRef& _TM) : target_device(_Param, _TM, TargetDevice_Type::FPGA)
{
   /// creating the datastructure representing the target technology
   target = target_technology::create_technology(target_technology::FPGA, Param);
   debug_level = Param->get_class_debug_level(GET_CLASS(*this));
}

FPGA_device::~FPGA_device() = default;

void FPGA_device::load_devices(const target_deviceRef device)
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

#if(0 && HAVE_EXPERIMENTAL)
   default_device_data["xc3s1500l-4fg676"] = "Spartan-3-xc3s1500l-4fg676.data";
#endif

   default_device_data["EP2C70F896C6"] = "EP2C70F896C6.data";
   default_device_data["EP2C70F896C6-R"] = "EP2C70F896C6-R.data";
   default_device_data["5CSEMA5F31C6"] = "5CSEMA5F31C6.data";
   default_device_data["EP4SGX530KH40C2"] = "EP4SGX530KH40C2.data";
   default_device_data["5SGXEA7N2F45C1"] = "5SGXEA7N2F45C1.data";
   default_device_data["LFE335EA8FN484C"] = "LFE335EA8FN484C.data";
   default_device_data["nx1h35S"] = "nx1h35S.data";
   default_device_data["nx1h140tsp"] = "nx1h140tsp.data";

   default_device_data["nangate45"] = "nangate45.data";

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
            const auto file_devices = Param->getOption<const std::list<std::string>>(OPT_target_device_file);
            for(const auto& file_device : file_devices)
            {
               PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Imported user data from file " + file_device);
               ret.insert(XMLDomParserRef(new XMLDomParser(GetPath(file_device))));
            }
         }
         else
         {
            auto device_string = Param->getOption<std::string>(OPT_device_string);

            if(default_device_data.find(device_string) != default_device_data.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Loading " + device_string);
               ret.insert(XMLDomParserRef(new XMLDomParser(relocate_compiler_path(PANDA_DATA_INSTALLDIR "/panda/technology/target_device/FPGA/") + default_device_data[device_string])));
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
            xload(device, node);
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

void FPGA_device::xwrite(xml_element* nodeRoot)
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
      if(p->first == "vendor" || p->first == "family" || p->first == "model" || p->first == "package" || p->first == "speed_grade" || p->first == "clock_period")
      {
         continue;
      }
      xml_element* elRoot = tmRoot->add_child_element(p->first);
      WRITE_XNVM2("value", p->second, elRoot);
   }
}

void FPGA_device::initialize()
{
}
