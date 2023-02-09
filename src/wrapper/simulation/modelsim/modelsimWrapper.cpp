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

/// Includes the class definition
#include "modelsimWrapper.hpp"

/// Standard PandA include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "utility.hpp"

/// includes all needed Boost.Filesystem declarations
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

/// include to understand which backend is used
#include "language_writer.hpp"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <unistd.h>

/// STL include
#include "custom_set.hpp"
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

#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "fileIO.hpp"

#define SIM_SUBDIR (Param->getOption<std::string>(OPT_output_directory) + std::string("/modelsim"))

// constructor
modelsimWrapper::modelsimWrapper(const ParameterConstRef& _Param, std::string _suffix)
    : SimulationTool(_Param), suffix(std::move(_suffix))
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Creating the modelsim wrapper...");
   const auto lic_path = std::getenv("LM_LICENSE_FILE");
   if(!lic_path || std::string(lic_path) == "")
   {
      THROW_WARNING("Mentor license file has not been specified. User must set LM_LICENSE_FILE variable to point to "
                    "the license file location.");
   }
   boost::filesystem::create_directory(SIM_SUBDIR + suffix + "/");
}

// destructor
modelsimWrapper::~modelsimWrapper() = default;

void modelsimWrapper::CheckExecution()
{
}

void modelsimWrapper::GenerateScript(std::ostringstream& script, const std::string& top_filename,
                                     const std::list<std::string>& file_list)
{
   THROW_ASSERT(!file_list.empty(), "File list is empty");
   std::string MODELSIM_OPTIMIZER_FLAGS_DEF;

   if(Param->getOption<bool>(OPT_mentor_optimizer))
   {
      MODELSIM_OPTIMIZER_FLAGS_DEF = "-O5";
   }
   else
   {
      MODELSIM_OPTIMIZER_FLAGS_DEF = "";
   }
   script << "if [ ! -d " << SIM_SUBDIR + suffix << " ]; then" << std::endl;
   script << "   mkdir " << SIM_SUBDIR + suffix << std::endl;
   script << "fi" << std::endl;

   std::string modelsim_work = SIM_SUBDIR + suffix + "/modelsim_work";
   log_file = SIM_SUBDIR + suffix + "/" + top_filename + "_modelsim.log";

   script << "if [ -d " << modelsim_work << " ]; then" << std::endl;
   script << "  " << MODELSIM_VDEL;
   script << " -all -lib " << modelsim_work;
   script << std::endl;
   script << "fi" << std::endl << std::endl;

   script << MODELSIM_VLIB;
   script << " " + modelsim_work;
   if(output_level < OUTPUT_LEVEL_VERY_PEDANTIC)
   {
      script << " > /dev/null 2>&1 ";
   }
   script << std::endl << std::endl;

   script << MODELSIM_VMAP;
   script << " work " + modelsim_work;
   script << std::endl << std::endl;

   script << "sed -i 's/; AssertionFailAction = 1/AssertionFailAction = 2/g' modelsim.ini" << std::endl << std::endl;

   /// prepare input files
   for(const auto& file : file_list)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Examining " + file);
      boost::filesystem::path file_path(file);
      std::string extension = GetExtension(file_path);
      if(extension == "vhd" || extension == "vhdl" || extension == "HD" || extension == "VHDL")
      {
         script << MODELSIM_VCOM;
         if(Param->isOption(OPT_assert_debug) && Param->getOption<bool>(OPT_assert_debug))
         {
            script << std::string(" ") + MODELSIM_OPTIMIZER_FLAGS_DEF +
                          " -lint -check_synthesis -fsmsingle -fsmverbose w -work work -2008 "
                   << file;
         }
         else
         {
            script << std::string(" ") + " " + MODELSIM_OPTIMIZER_FLAGS_DEF + " -work work -2008 " << file;
         }
         script << std::endl << std::endl;
         script << "if [ $? -ne 0 ]; then" << std::endl;
         script << "   exit 1;" << std::endl;
         script << "fi" << std::endl;
      }
      else if(extension == "v" || extension == "V" || extension == "sv")
      {
         script << MODELSIM_VLOG;
         if(Param->isOption(OPT_assert_debug) && Param->getOption<bool>(OPT_assert_debug))
         {
            script << std::string(" ") + MODELSIM_OPTIMIZER_FLAGS_DEF +
                          " -sv -lint -fsmsingle -hazards -pedanticerrors -fsmverbose w -work work " + file;
         }
         else
         {
            script << std::string(" ") + MODELSIM_OPTIMIZER_FLAGS_DEF + " -sv -work work " + file;
         }
         script << std::endl << std::endl;
         script << "if [ $? -ne 0 ]; then" << std::endl;
         script << "   exit 1;" << std::endl;
         script << "fi" << std::endl;
      }
      else
      {
         THROW_UNREACHABLE("Extension not recognized! " + file_path.string());
      }
   }

   script << MODELSIM_VSIM;
   if(MODELSIM_OPTIMIZER_FLAGS_DEF.empty())
   {
      if(Param->isOption(OPT_assert_debug) && Param->getOption<bool>(OPT_assert_debug))
      {
         script << " -c -pedanticerrors -assertdebug -do \"onerror {quit -f -code 1;}; run -all; exit -f;\" work." +
                       top_filename + "_tb_top";
      }
      else
      {
         script << " -c -do \"onerror {quit -f -code 1;}; run -all; exit -f;\" work." + top_filename + "_tb_top";
      }
   }
   else
   {
      if(Param->isOption(OPT_mentor_visualizer))
      {
         if(Param->isOption(OPT_visualizer) && Param->getOption<bool>(OPT_visualizer))
         {
            script << " -qwavedb=+memory+signal+class+glitch+vhdlvariable";
         }
         if(Param->isOption(OPT_visualizer) && Param->getOption<bool>(OPT_visualizer))
         {
            MODELSIM_OPTIMIZER_FLAGS_DEF += " -debug -designfile design.bin";
         }
      }
      if(Param->isOption(OPT_assert_debug) && Param->getOption<bool>(OPT_assert_debug))
      {
         script
             << " -c -voptargs=\"+acc -hazards " + MODELSIM_OPTIMIZER_FLAGS_DEF +
                    R"( " -pedanticerrors -assertdebug -do "set StdArithNoWarnings 1; set StdNumNoWarnings 1; set NumericStdNoWarnings 1; onerror {quit -f -code 1;}; run -all; exit -f;" work.)" +
                    top_filename + "_tb_top";
      }
      else
      {
         script
             << " -c -voptargs=\"" + MODELSIM_OPTIMIZER_FLAGS_DEF +
                    R"(" -do "set StdArithNoWarnings 1; set StdNumNoWarnings 1; set NumericStdNoWarnings 1; onerror {quit -f -code 1;}; run -all; exit -f;" work.)" +
                    top_filename + "_tb_top";
      }
   }
   script << " 2>&1 | tee " << log_file << std::endl << std::endl;
   if(Param->isOption(OPT_mentor_visualizer) && Param->isOption(OPT_visualizer) &&
      Param->getOption<bool>(OPT_visualizer))
   {
      script << Param->getOption<std::string>(OPT_mentor_visualizer) << " +designfile +wavefile -showglitches"
             << std::endl
             << std::endl;
   }
}

void modelsimWrapper::Clean() const
{
   if(boost::filesystem::exists(SIM_SUBDIR + suffix))
   {
      boost::filesystem::remove_all(SIM_SUBDIR + suffix);
   }
}
