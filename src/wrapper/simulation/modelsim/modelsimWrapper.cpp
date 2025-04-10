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
 * @file modelsimWrapper.cpp
 * @brief Implementation of the wrapper to modelsim compiler and simulator.
 *
 * Implementation of the methods for the wrapper.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#include "modelsimWrapper.hpp"

#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "language_writer.hpp"
#include "utility.hpp"

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include <utility>

#define MODELSIM_BIN                                                                                          \
   (Param->isOption(OPT_mentor_modelsim_bin) ? Param->getOption<std::string>(OPT_mentor_modelsim_bin) + "/" : \
                                               std::string(""))

#define MODELSIM_VDEL (MODELSIM_BIN + "vdel")
#define MODELSIM_VLIB (MODELSIM_BIN + "vlib")
#define MODELSIM_VMAP (MODELSIM_BIN + "vmap")
#define MODELSIM_VCOM (MODELSIM_BIN + "vcom")
#define MODELSIM_VLOG (MODELSIM_BIN + "vlog")
#define MODELSIM_VSIM (MODELSIM_BIN + "vsim")

// constructor
modelsimWrapper::modelsimWrapper(const ParameterConstRef& _Param, const std::string& _top_fname,
                                 const std::string& _inc_dirs)
    : SimulationTool(_Param, _top_fname, _inc_dirs)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Creating the MODELSIM wrapper...");
   const auto lic_path = std::getenv("LM_LICENSE_FILE");
   if(!lic_path || std::string(lic_path) == "")
   {
      THROW_WARNING("Mentor license file has not been specified. User must set LM_LICENSE_FILE variable to point to "
                    "the license file location.");
   }
}

std::string modelsimWrapper::GenerateScript(std::ostream& script, const std::string& top_filename,
                                            const std::list<std::string>& file_list)
{
   THROW_ASSERT(!file_list.empty(), "File list is empty");
   const auto modelsim_bin = MODELSIM_BIN;
   std::string beh_cflags = "-DMODEL_TECH " + (modelsim_bin.size() ? ("-isystem " + modelsim_bin + "../include") : "");
   const auto cflags = GenerateLibraryBuildScript(script, beh_cflags);
   const auto vflags = [&]() {
      std::string flags;
      if(cflags.find("-m32") != std::string::npos)
      {
         flags += " +define+__M32";
      }
      else if(cflags.find("-mx32") != std::string::npos)
      {
         flags += " +define+__MX32";
      }
      else if(cflags.find("-m64") != std::string::npos)
      {
         flags += " +define+__M64";
      }
      if(Param->isOption(OPT_generate_vcd) && Param->getOption<bool>(OPT_generate_vcd))
      {
         flags += " +define+GENERATE_VCD";
      }
      if(Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy))
      {
         flags += " +define+GENERATE_VCD_DISCREPANCY";
      }
      flags += " +define+__BAMBU_SIM__";
      const auto inc_dir_list = string_to_container<std::vector<std::string>>(inc_dirs, ";");
      for(const auto& inc : inc_dir_list)
      {
         flags += " +incdir+" + inc;
      }
      return flags;
   }();

   std::string MODELSIM_OPTIMIZER_FLAGS_DEF = "";
   if(Param->getOption<bool>(OPT_mentor_optimizer))
   {
      MODELSIM_OPTIMIZER_FLAGS_DEF = "-O5";
   }
   script << "work_dir=\"${BEH_DIR}/modelsim_work\"\n";
   script << "if [ ! -d ${BEH_DIR} ]; then\n";
   script << "   mkdir -p ${BEH_DIR}\n";
   script << "fi\n\n";

   log_file = "${BEH_DIR}/" + top_filename + "_modelsim.log";

   script << "if [ -d ${work_dir} ]; then\n";
   script << "  " << MODELSIM_VDEL << " -all -lib ${work_dir}\n";
   script << "fi\n\n";

   script << MODELSIM_VLIB << " ${work_dir}";
   if(output_level < OUTPUT_LEVEL_VERY_PEDANTIC)
   {
      script << " > /dev/null 2>&1 ";
   }
   script << "\n\n";

   script << MODELSIM_VMAP << " work ${work_dir}\n\n";

   script << "sed -i 's/; AssertionFailAction = 1/AssertionFailAction = 2/g' modelsim.ini\n\n";

   /// prepare input files
   for(const auto& file : file_list)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Examining " + file);
      std::filesystem::path file_path(file);
      const auto extension = file_path.extension().string();
      if(extension == ".vhd" || extension == ".vhdl" || extension == ".VHD" || extension == ".VHDL")
      {
         script << MODELSIM_VCOM << " " << vflags << " " << MODELSIM_OPTIMIZER_FLAGS_DEF;
         if(Param->isOption(OPT_assert_debug) && Param->getOption<bool>(OPT_assert_debug))
         {
            script << " -lint -check_synthesis -fsmsingle -fsmverbose w";
         }
         script << " -work work -2008 " << file << "\n";
      }
      else if(extension == ".v" || extension == ".V" || extension == ".sv" || extension == ".SV")
      {
         script << MODELSIM_VLOG << " " << vflags << " " << MODELSIM_OPTIMIZER_FLAGS_DEF << " -sv";
         if(Param->isOption(OPT_assert_debug) && Param->getOption<bool>(OPT_assert_debug))
         {
            script << " -lint -fsmsingle -hazards -pedanticerrors -fsmverbose w";
         }
         script << " -work work " << file << "\n";
      }
      else if(extension == ".c" || extension == ".cpp")
      {
         script << MODELSIM_VLOG << " " << MODELSIM_OPTIMIZER_FLAGS_DEF << " -sv -ccflags \"" << beh_cflags
                << "\" -work work " << file << "\n";
      }
      else
      {
         THROW_UNREACHABLE("Extension not recognized! " + file_path.string());
      }
      script << "if [ $? -ne 0 ]; then exit 1; fi\n\n";
   }

   std::string sim_cmd = MODELSIM_VSIM + " " + vflags + " -noautoldlibpath";
   if(Param->isOption(OPT_assert_debug) && Param->getOption<bool>(OPT_assert_debug))
   {
      sim_cmd += " -pedanticerrors -assertdebug";
      MODELSIM_OPTIMIZER_FLAGS_DEF = "+acc -hazards " + MODELSIM_OPTIMIZER_FLAGS_DEF;
   }
   sim_cmd += " -c";
   if(!MODELSIM_OPTIMIZER_FLAGS_DEF.empty())
   {
      sim_cmd += " -voptargs=\"" + MODELSIM_OPTIMIZER_FLAGS_DEF + "\"";
   }
   sim_cmd += " -do \"set StdArithNoWarnings 1; set StdNumNoWarnings 1; set NumericStdNoWarnings 1; onerror {quit -f "
              "-code 1;}; run -all; exit -f;\"  work.clocked_bambu_testbench 2>&1 | tee " +
              log_file;
   return sim_cmd;
}
