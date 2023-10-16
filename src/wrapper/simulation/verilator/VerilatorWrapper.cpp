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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file VerilatorWrapper.cpp
 * @brief Wrapper to Verilator simulator.
 *
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#include "VerilatorWrapper.hpp"

#include "config_PANDA_INCLUDE_INSTALLDIR.hpp"

#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "constants.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "file_IO_constants.hpp"
#include "utility.hpp"

#include <boost/algorithm/string.hpp>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <utility>

#define SIM_SUBDIR (Param->getOption<std::string>(OPT_output_directory) + std::string("/verilator"))

// constructor
VerilatorWrapper::VerilatorWrapper(const ParameterConstRef& _Param, const std::string& _suffix,
                                   const std::string& _top_fname)
    : SimulationTool(_Param, _top_fname), suffix(_suffix)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Creating the VERILATOR wrapper...");
   std::string verilator_beh_dir = SIM_SUBDIR + suffix;
   if(std::filesystem::exists(verilator_beh_dir))
   {
      std::filesystem::remove_all(verilator_beh_dir);
   }
   std::filesystem::create_directory(verilator_beh_dir + "/");
}

// destructor
VerilatorWrapper::~VerilatorWrapper() = default;

void VerilatorWrapper::CheckExecution()
{
}

void VerilatorWrapper::GenerateScript(std::ostringstream& script, const std::string& top_filename,
                                      const std::list<std::string>& file_list)
{
   for(const auto& file : file_list)
   {
      if(file.find(".vhd") != std::string::npos)
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Mixed simulation not supported by Verilator");
      }
   }
   const auto generate_vcd_output = (Param->isOption(OPT_generate_vcd) && Param->getOption<bool>(OPT_generate_vcd)) ||
                                    (Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy)) ||
                                    (Param->isOption(OPT_discrepancy_hw) && Param->getOption<bool>(OPT_discrepancy_hw));
   const auto output_directory = Param->getOption<std::string>(OPT_output_directory);
   log_file = "${BEH_DIR}/" + top_filename + "_verilator.log";
   script << "export VM_PARALLEL_BUILDS=1" << std::endl
          << "BEH_DIR=\"" << SIM_SUBDIR << suffix << "\"" << std::endl
          << "BEH_CC=\"${CC}\"" << std::endl
          << "obj_dir=\"${BEH_DIR}/verilator_obj\"" << std::endl
          << std::endl;
   std::string beh_cflags = "-DVERILATOR -I$(dirname $(which verilator))/../share/verilator/include/vltstd";
   const auto cflags = GenerateLibraryBuildScript(script, "${BEH_DIR}", beh_cflags);
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
      return flags;
   }();

#ifdef _WIN32
   /// this removes the dependency from perl on MinGW32
   script << "verilator_bin"
#else
   script << "verilator"
#endif
          << " --cc --exe --Mdir ${obj_dir} -Wno-fatal -Wno-lint -sv " << vflags
          << " ${BEH_DIR}/libmdpi.so -O3  --output-split-cfuncs 3000  --output-split-ctrace 3000";
   if(!generate_vcd_output)
   {
      script << " --x-assign fast --x-initial fast --noassert";
   }

   const auto nThreads = Param->getOption<bool>(OPT_verilator_parallel) ? std::thread::hardware_concurrency() : 1;
   if(nThreads > 1)
   {
      script << " --threads " << nThreads;
   }
   if(generate_vcd_output)
   {
      script << " --trace --trace-underscore"; // --trace-params
      if(Param->getOption<bool>(OPT_verilator_l2_name))
      {
         script << " --l2-name bambu_testbench";
      }
   }
   for(const auto& file : file_list)
   {
      if(ends_with(file, "mdpi.c"))
      {
         script << " ${BEH_DIR}/libmdpi.so";
      }
      else
      {
         script << " " << file;
      }
   }
   script << " --top-module bambu_testbench" << std::endl
          << "if [ $? -ne 0 ]; then exit 1; fi" << std::endl
          << std::endl
          << std::endl
          << "ln -sf " + output_directory + " ${obj_dir}\n";

   script << "make -C ${obj_dir}"
          << " -j " << std::thread::hardware_concurrency() << " OPT=\"-fstrict-aliasing\""
          << " -f Vbambu_testbench.mk Vbambu_testbench";
#ifdef _WIN32
   /// VM_PARALLEL_BUILDS=1 removes the dependency from perl
   script << " VM_PARALLEL_BUILDS=1 CFG_CXXFLAGS_NO_UNUSED=\"\"";
#endif
   script << std::endl << std::endl;

   script << "${obj_dir}/Vbambu_testbench 2>&1 | tee " << log_file << std::endl << std::endl;
}

void VerilatorWrapper::Clean() const
{
}
