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
 * @file VIVADO_xsim_wrapper.cpp
 * @brief Implementation of the wrapper to XSIM
 *
 * Implementation of the methods to wrap XSIM by Xilinx
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_XILINX_VIVADO_SETTINGS.hpp"

/// Includes the class definition
#include "VIVADO_xsim_wrapper.hpp"

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
VIVADO_xsim_wrapper::VIVADO_xsim_wrapper(const ParameterConstRef& _Param, std::string _suffix) : SimulationTool(_Param), suffix(std::move(_suffix))
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Creating the XSIM wrapper...");
   boost::filesystem::create_directory(XSIM_SUBDIR + suffix + "/");
}

// destructor
VIVADO_xsim_wrapper::~VIVADO_xsim_wrapper() = default;

void VIVADO_xsim_wrapper::CheckExecution()
{
}

std::string VIVADO_xsim_wrapper::create_project_script(const std::string& top_filename, const std::list<std::string>& file_list)
{
   std::string project_filename = XSIM_SUBDIR + suffix + "/" + top_filename + ".prj";
   std::ofstream prj_file(project_filename.c_str());
   for(auto const& file : file_list)
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

void VIVADO_xsim_wrapper::GenerateScript(std::ostringstream& script, const std::string& top_filename, const std::list<std::string>& file_list)
{
   std::string project_file = create_project_script(top_filename, file_list);
   PRINT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "Project file: " + project_file);

   log_file = XSIM_SUBDIR + suffix + "/" + top_filename + "_xsim.log";
   script << "#configuration" << std::endl;
   auto setupscr = STR(XILINX_VIVADO_SETTINGS);
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

   if(Param->isOption(OPT_assert_debug) && Param->getOption<bool>(OPT_assert_debug))
      script << "xelab --debug all --rangecheck -L work -L unifast_ver -L unisims_ver -L unimacro_ver -L secureip --snapshot " + top_filename + "tb_behav --prj " + project_file + " work." + top_filename + "_tb_top -O2 -nolog -stat -R";
   else
      script << "xelab --debug off -L work -L unifast_ver -L unisims_ver -L unimacro_ver -L secureip --snapshot " + top_filename + "tb_behav --prj " + project_file + " work." + top_filename + "_tb_top -O3 -nolog -stat -R";
   script << " 2>&1 | tee " << log_file << std::endl << std::endl;
}

void VIVADO_xsim_wrapper::Clean() const
{
}
