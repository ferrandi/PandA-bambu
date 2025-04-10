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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
#include "VIVADO_xsim_wrapper.hpp"

#include "Parameter.hpp"
#include "ToolManager.hpp"
#include "constant_strings.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "utility.hpp"

#include <fstream>
#include <utility>

#define XSIM_VLOG "xvlog"
#define XSIM_VHDL "xvhdl"
#define XSIM_XELAB "xelab"
#define XSIM_XSC "xsc"

// constructor
VIVADO_xsim_wrapper::VIVADO_xsim_wrapper(const ParameterConstRef& _Param, const std::string& _top_fname,
                                         const std::string& _inc_dirs)
    : SimulationTool(_Param, _top_fname, _inc_dirs)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Creating the XSIM wrapper...");
}

static void create_project_file(const std::filesystem::path& project_filename, const std::list<std::string>& file_list)
{
   std::ofstream prj_file(project_filename);
   for(auto const& file : file_list)
   {
      if(ends_with(file, "mdpi.c"))
      {
         continue;
      }
      std::filesystem::path file_path(file);
      const auto extension = file_path.extension();
      if(extension == ".vhd" || extension == ".vhdl" || extension == ".VHD" || extension == ".VHDL")
      {
         prj_file << "VHDL";
      }
      else if(extension == ".v" || extension == ".V" || extension == ".sv" || extension == ".SV")
      {
         prj_file << "SV";
      }
      else
      {
         THROW_ERROR("Extension not recognized! " + extension.string());
      }
      prj_file << " work " << file_path.lexically_proximate(project_filename.parent_path()).string() << "\n";
   }
   prj_file.close();
}

std::string VIVADO_xsim_wrapper::GenerateScript(std::ostream& script, const std::string& top_filename,
                                                const std::list<std::string>& file_list)
{
   script << "#configuration\n";
   const auto setupscr =
       Param->isOption(OPT_xilinx_vivado_settings) ? Param->getOption<std::string>(OPT_xilinx_vivado_settings) : "";
   if(!setupscr.empty())
   {
      script << ". " << setupscr << " >& /dev/null;\n\n";
   }
   script << "BEH_CC=\"${CC}\"\n\n";
   log_file = "${BEH_DIR}/" + top_filename + "_xsim.log";
   const auto xilinx_root = Param->isOption(OPT_xilinx_root) ? Param->getOption<std::string>(OPT_xilinx_root) : "";
   std::string beh_cflags =
       "-DXILINX_SIMULATOR " + (xilinx_root.size() ? ("-isystem " + xilinx_root + "/data/xsim/include") : "");
   const auto cflags = GenerateLibraryBuildScript(script, beh_cflags);
   const auto vflags = [&]() {
      std::string flags;
      if(cflags.find("-m32") != std::string::npos)
      {
         flags += " -define __M32";
      }
      else if(cflags.find("-mx32") != std::string::npos)
      {
         flags += " -define __MX32";
      }
      else if(cflags.find("-m64") != std::string::npos)
      {
         flags += " -define __M64";
      }
      if(Param->isOption(OPT_generate_vcd) && Param->getOption<bool>(OPT_generate_vcd))
      {
         flags += " -define GENERATE_VCD";
      }
      if(Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy))
      {
         flags += " -define GENERATE_VCD_DISCREPANCY";
      }
      flags += "  -define __BAMBU_SIM__";
      const auto inc_dir_list = string_to_container<std::vector<std::string>>(inc_dirs, ";");
      for(const auto& inc : inc_dir_list)
      {
         flags += " -i " + inc;
      }
      return flags;
   }();
   const auto prj_file = beh_dir / (top_filename + ".prj");
   create_project_file(prj_file, file_list);

   std::string sim_cmd = "rm -rf xsim.* xelab.*; " XSIM_XELAB " -sv_root ${BEH_DIR} -sv_lib libmdpi " + vflags +
                         " -prj " + prj_file.string();
   if(Param->isOption(OPT_assert_debug) && Param->getOption<bool>(OPT_assert_debug))
   {
      sim_cmd += " --debug all --rangecheck -O2";
   }
   else
   {
      sim_cmd += " --debug off -O3";
   }
   sim_cmd += " -L work -L unifast_ver -L unisims_ver -L unimacro_ver -L secureip --snapshot " + top_filename +
              "tb_behav work.clocked_bambu_testbench -nolog -stat -R";
   sim_cmd += " 2>&1 | tee " + log_file;
   return sim_cmd;
}
