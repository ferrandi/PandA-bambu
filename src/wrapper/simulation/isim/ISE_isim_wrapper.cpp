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
 * @file ISE_isim_wrapper.cpp
 * @brief Implementation of the wrapper to ISIM
 *
 * Implementation of the methods to wrap ISIM by Xilinx
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_XILINX_SETTINGS.hpp"

/// Includes the class definition
#include "ISE_isim_wrapper.hpp"

#include "ToolManager.hpp"

#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "utility.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <fileIO.hpp>
#include <polixml.hpp>
#include <utility>
#include <xml_dom_parser.hpp>
#include <xml_helper.hpp>

#include <fstream>

// constructor
ISE_isim_wrapper::ISE_isim_wrapper(const ParameterConstRef& _Param, std::string _suffix) : SimulationTool(_Param), suffix(std::move(_suffix))
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Creating the ISIM wrapper...");
   boost::filesystem::create_directory(ISIM_SUBDIR + suffix + "/");
}

// destructor
ISE_isim_wrapper::~ISE_isim_wrapper() = default;

void ISE_isim_wrapper::CheckExecution()
{
}

std::string ISE_isim_wrapper::create_project_script(const std::string& top_filename, const std::list<std::string>& file_list)
{
   std::string project_filename = ISIM_SUBDIR + suffix + "/" + top_filename + ".prj";
   std::ofstream prj_file(project_filename.c_str());
   for(const auto& file : file_list)
   {
      boost::filesystem::path file_path(file);
      std::string extension = GetExtension(file_path);
      std::string filename;
      std::string language;
      if(extension == "vhd" || extension == "vhdl" || extension == "VHD" || extension == "VHDL")
      {
         language = "VHDL";
      }
      else if(extension == "v" || extension == "V" || extension == "sv")
      {
         language = "VERILOG";
      }
      else
      {
         THROW_ERROR("Extension not recognized! " + extension);
      }
      filename = file_path.string();
      if(filename[0] == '/')
      {
         prj_file << language << " "
                  << "work"
                  << " " << filename << std::endl;
      }
      else
      {
         prj_file << language << " "
                  << "work"
                  << " " << boost::filesystem::path(GetCurrentPath()).string() << "/" << filename << std::endl;
      }
   }
   prj_file.close();
   return project_filename;
}

void ISE_isim_wrapper::GenerateScript(std::ostringstream& script, const std::string& top_filename, const std::list<std::string>& file_list)
{
   std::string project_file = create_project_script(top_filename, file_list);
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "Project file: " + project_file);

   log_file = ISIM_SUBDIR + suffix + "/" + top_filename + "_isim.log";

   script << "#configuration" << std::endl;
   auto setupscr = STR(XILINX_SETTINGS);
   if(!setupscr.empty() && setupscr != "0")
   {
      if(boost::algorithm::starts_with(setupscr, "export"))
      {
         script << setupscr + " >& /dev/null; ";
      }
      else
      {
         script << ". " << setupscr << " >& /dev/null;";
      }
      script << std::endl << std::endl;
   }

   std::string exe_filename = ISIM_SUBDIR + suffix + "/" + top_filename + "_isim.exe";

   script << "#setting up the simulation" << std::endl;
   script << "fuse";
   std::string ise_style;
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      ise_style = std::string(INTSTYLE_ISE);
   }
   else
   {
      ise_style = std::string(INTSTYLE_SILENT);
   }
   script << " -intstyle " << ise_style;
   script << " -lib simprims_ver";
   script << " -lib unisims_ver";
   script << " -lib unimacro_ver";
   script << " -lib xilinxcorelib_ver";
   script << " -lib secureip";
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      script << " -mt off";
      script << " -v 1";
   }
   if(!Param->isOption(OPT_assert_debug) || !Param->getOption<bool>(OPT_assert_debug))
   {
      script << " -nodebug";
   }
   script << " -o " + exe_filename;
   script << " -prj " + project_file;
   script << " work." + top_filename + "_tb_top";
   script << " >& " + ISIM_SUBDIR + suffix + "/" + "fuse.log";
   script << std::endl << std::endl;

   /// prepare tclbatch script
   std::string tclbatch_filename(ISIM_SUBDIR + suffix + "/" + "isim.command");
   std::ofstream tclbatch_file(tclbatch_filename.c_str());
   tclbatch_file << "run all" << std::endl;
   tclbatch_file.close();

   script << "#performing the simulation" << std::endl;
   script << exe_filename;
   script << " -tclbatch " + tclbatch_filename;
   script << " 2>&1 | tee " << log_file << std::endl << std::endl;
}
