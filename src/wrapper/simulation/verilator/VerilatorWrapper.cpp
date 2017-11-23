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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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

/// Includes the class definition
#include "VerilatorWrapper.hpp"

///Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_VERILATOR.hpp"

///Constants include
#include "file_IO_constants.hpp"

/// Standard PandA include
#include "dbgPrintHelper.hpp"
#include "utility.hpp"
#include "exceptions.hpp"

/// includes all needed Boost.Filesystem declarations
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <fstream>
#include <unistd.h>
#include <cerrno>

#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "utility.hpp"
#include "fileIO.hpp"

#define SIM_SUBDIR       (Param->getOption<std::string>(OPT_output_directory) + std::string("/verilator"))

//constructor
VerilatorWrapper::VerilatorWrapper(const ParameterConstRef _Param, const std::string& _suffix) :
   SimulationTool(_Param),
   suffix(_suffix)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Creating the VERILATOR wrapper...");
   std::string verilator_beh_dir = SIM_SUBDIR + suffix;
   if (boost::filesystem::exists(verilator_beh_dir)) boost::filesystem::remove_all(verilator_beh_dir);
   boost::filesystem::create_directory(verilator_beh_dir + "/" );
}

//destructor
VerilatorWrapper::~VerilatorWrapper()
{

}

void VerilatorWrapper::CheckExecution()
{
#if !HAVE_VERILATOR
   THROW_ERROR("VERILATOR not correctly configured!");
#endif
}

void VerilatorWrapper::GenerateScript(std::ostringstream& script, const std::string& top_filename, const std::list<std::string> & file_list)
{
   for(const auto& file : file_list)
      if(file.find(".vhd") != std::string::npos)
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Mixed simulation not supported by Verilator");
   bool generate_vcd_output = (Param->isOption(OPT_generate_vcd) && Param->getOption<bool>(OPT_generate_vcd)) ||
      (Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy));

   const std::string output_directory = Param->getOption<std::string>(OPT_output_directory);
   log_file = SIM_SUBDIR + suffix + "/" + top_filename + "_verilator.log";
#if HAVE_EXPERIMENTAL
   script << "verilator --cc --exe --Mdir " + SIM_SUBDIR + suffix + "/verilator_obj -Wall -Wno-DECLFILENAME -Wno-WIDTH -Wno-UNUSED -Wno-CASEINCOMPLETE -Wno-UNOPTFLAT -Wno-PINMISSING -Wno-UNDRIVEN -Wno-SYNCASYNCNET -Ox -sv";
#else
   script << "verilator --cc --exe --Mdir " + SIM_SUBDIR + suffix + "/verilator_obj -Wno-fatal -Wno-lint -Ox -sv";
#endif
   if(generate_vcd_output)
      script << " --trace --trace-underscore"; // --trace-params
   for(const auto& file : file_list)
      script << " " << file;
   script << " " << output_directory + "/simulation/testbench_" + top_filename << "_tb.v";
   script << " --top-module " << top_filename << "_tb";
   script << std::endl;
   script << "if [ $? -ne 0 ]; then" << std::endl;
   script << "   exit 1;" << std::endl;
   script << "fi" << std::endl;
   script << std::endl << std::endl;
   script << "ln -s ../../../"+ output_directory +" " + SIM_SUBDIR + suffix + "/verilator_obj\n";

   script << "make -C " + SIM_SUBDIR + suffix + "/verilator_obj -j4 OPT_FAST=\"-O1 -fstrict-aliasing\" -f V"+ top_filename +"_tb.mk V"+ top_filename << "_tb";
   script << std::endl << std::endl;

   script << SIM_SUBDIR + suffix + "/verilator_obj/V"+top_filename+"_tb";
   script << " 2>&1 | tee " << log_file << std::endl << std::endl;
}

void VerilatorWrapper::Clean() const
{
}
