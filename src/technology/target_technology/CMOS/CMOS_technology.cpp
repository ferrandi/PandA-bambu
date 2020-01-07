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

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"

#include "CMOS_technology.hpp"

#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "polixml.hpp"
#include "xml_dom_parser.hpp"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <cmath>

CMOS_technology::CMOS_technology(const ParameterConstRef& param) : target_technology(param)
{
   type = CMOS;

#if HAVE_EXPERIMENTAL
   /// Load default resources
   const char* builtin_technology = {
#include "CMOS_technology.data"
   };

   const int output_level = Param->getOption<int>(OPT_output_level);

   try
   {
      XMLDomParserRef parser;
      /// update with specified information
      if(Param->isOption(OPT_target_technology_file))
      {
         std::string file_name = Param->getOption<std::string>(OPT_target_technology_file);
         PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "(target technology) Loading information about the target technology from file \"" + file_name + "\"");
         if(!boost::filesystem::exists(file_name))
         {
            THROW_ERROR("Technology information file " + file_name + " does not exist!");
         }
         parser = XMLDomParserRef(new XMLDomParser(file_name));
      }
      else
      {
         parser = XMLDomParserRef(new XMLDomParser("builtin_technology", builtin_technology));
      }

      parser->Exec();
      if(parser and *parser)
      {
         // Walk the tree:
         const xml_element* node = parser->get_document()->get_root_node(); // deleted by DomParser.
         xload(node);
      }

      initialize();
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
#endif
}

CMOS_technology::~CMOS_technology() = default;

void CMOS_technology::initialize()
{
   auto output_level = Param->getOption<int>(OPT_output_level);

#if HAVE_EXPERIMENTAL
   /// size of the manufacturing grid in um
   if(Param->isOption("grid-size"))
      set_parameter("manufacturing_grid_size", Param->getOption<double>("grid-size"));
   /// it represents the number of blocks for the cell height
   if(Param->isOption("height-blocks"))
      set_parameter("grid_cell_height", Param->getOption<double>("height-blocks"));
   /// it represents the size of the pitch grid on x axis
   if(Param->isOption("grid-x"))
      set_parameter("pitch_grid_x", Param->getOption<double>("grid-x"));
   /// it represents the size of the pitch grid on y axis
   if(Param->isOption("grid-y"))
      set_parameter("pitch_grid_y", Param->getOption<double>("grid-y"));
#endif

   /// it represents the height of the cell based on the manufacturing information
   set_parameter("cell_height", get_parameter<float>("pitch_grid_y") * get_parameter<float>("grid_cell_height") * get_parameter<float>("manufacturing_grid_size"));
   /// it represents the width of each single cell column based on the manufacturing information
   set_parameter("column_width", get_parameter<float>("pitch_grid_x") * get_parameter<float>("manufacturing_grid_size"));
   std::cerr.setf(std::ios::fixed);
   std::cerr.precision(3);
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "   Manufacturing grid size: " << get_parameter<double>("manufacturing_grid_size"));
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "   Grid cell height: " << get_parameter<unsigned int>("grid_cell_height"));
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "   Cell height: " << get_parameter<double>("cell_height"));
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "   Column width: " << get_parameter<double>("column_width"));
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "   Pitch Grid X: " << get_parameter<unsigned int>("pitch_grid_x"));
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "   Pitch Grid Y: " << get_parameter<unsigned int>("pitch_grid_y"));
#if !RELEASE
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "   Power net: " << get_parameter<std::string>("pwr_name"));
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "   Ground net: " << get_parameter<std::string>("gnd_name"));
#endif
}

std::string CMOS_technology::get_string_type() const
{
   return "CMOS";
}
