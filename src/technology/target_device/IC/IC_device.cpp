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
 * @file IC_device.cpp
 * @brief
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */
#include "IC_device.hpp"

#include "config_PANDA_DATA_INSTALLDIR.hpp"

/// technology includes
#include "BackendFlow.hpp"
#include "CMOS_technology.hpp"
#include "technology_manager.hpp"
/// design includes
#include "structural_manager.hpp"
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

IC_device::IC_device(const ParameterConstRef _Param, const technology_managerRef _TM) : target_device(_Param, _TM, TargetDevice_Type::IC)
{
   if(has_parameter("core_height"))
      core_height = get_parameter<double>("core_height");
   if(has_parameter("core_width"))
      core_width = get_parameter<double>("core_width");
   initialize();
   debug_level = Param->get_class_debug_level(GET_CLASS(*this));
}

IC_device::~IC_device() = default;

void IC_device::xwrite(xml_element* nodeRoot)
{
   xml_element* tmRoot = nodeRoot->add_child_element("device");

   for(auto p = parameters.begin(); p != parameters.end(); ++p)
   {
      xml_element* elRoot = tmRoot->add_child_element(p->first);
      WRITE_XNVM2("value", p->second, elRoot);
   }
}

void IC_device::set_dimension(double area)
{
   THROW_ASSERT(GetPointer<CMOS_technology>(target), "The target device is not compatible with the target technology");
   const CMOS_technology* tech = GetPointer<CMOS_technology>(target);
   if(core_height == 0 or core_width == 0)
   {
      std::cerr << "module area: " << area << std::endl;

      area /= get_parameter<double>("utilization_factor");
      if(core_height == 0 and core_width == 0)
      {
         double float_height = sqrt(area * get_parameter<double>("aspect_ratio") / pow(tech->get_parameter<double>("cell_height"), 2.0));
         std::cerr << "float height = " << float_height << std::endl;

         unsigned int rows = 1;
         if((float_height - floor(float_height)) < 0.5 and floor(float_height) > 0)
            rows = static_cast<unsigned int>(floor(float_height));
         else
            rows = static_cast<unsigned int>(ceil(float_height));

         std::cerr << "number of rows = " << rows << std::endl;
         core_height = (static_cast<double>(rows) * tech->get_parameter<double>("cell_height"));
         core_width = area / core_height;
      }
      else if(core_width == 0)
      {
         core_width = area / core_height;
      }
      else if(core_height == 0)
      {
         core_height = area / core_width;
      }
   }

   std::cerr << "die area = " << core_height * core_width << " [" << core_width << "," << core_height << "]" << std::endl;
}

void IC_device::initialize()
{
   auto output_level = Param->getOption<unsigned int>(OPT_output_level);

   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "   Aspect ratio: " << parameters["aspect_ratio"]);
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "   Utilization factor: " << parameters["utilization_factor"]);
   if(has_parameter("clock_period"))
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "   Clock period: " << parameters["clock_period"] << " (" << 1.0 / get_parameter<double>("clock_period") << ")");
}

void IC_device::load_devices(const target_deviceRef device)
{
   /// Load default resources
   const char* builtin_technology = {"Nangate.data"};

   auto output_level = Param->getOption<int>(OPT_output_level);

   /// creating the data structure representing the target technology
   target = target_technology::create_technology(target_technology::CMOS, Param);

   try
   {
      XMLDomParser parser(relocate_compiler_path(PANDA_DATA_INSTALLDIR "/panda/technology/target_device/IC/") + builtin_technology[0]);
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
         xload(device, node);
      }

      /// update with specified device information
      if(Param->isOption(OPT_target_device_file))
      {
         const auto file_name = GetPath(Param->getOption<std::string>(OPT_target_device_file));
         if(!boost::filesystem::exists(file_name))
         {
            THROW_ERROR("Device information file " + file_name + " does not exist!");
         }

         PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "(target device) Loading information about the target device from file \"" + file_name + "\"");
         XMLDomParser parser1(file_name);
         parser1.Exec();
         if(parser1)
         {
            // Walk the tree:
            const xml_element* node = parser1.get_document()->get_root_node(); // deleted by DomParser.
            xload(device, node);
         }
      }
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
      THROW_ERROR("Error during parsing of technology file");
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
      THROW_ERROR("Error during parsing of technology file");
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
      THROW_ERROR("Error during parsing of technology file");
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
      THROW_ERROR("Error during parsing of technology file");
   }
}
