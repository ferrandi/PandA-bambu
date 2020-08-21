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
 * @file IcarusWrapper.cpp
 * @brief Implementation of the wrapper to Icarus for Verilog sources.
 *
 * Implementation of the methods for the object for invoke the Icarus compiler from Verilog sources.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

/// Includes the class definition
#include "IcarusWrapper.hpp"

/// Constants include
#include "file_IO_constants.hpp"

/// Standard PandA include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "utility.hpp"

/// includes all needed Boost.Filesystem declarations
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <cerrno>
#include <fstream>
#include <unistd.h>

/// STL include
#include "custom_set.hpp"
#include <list>
#include <utility>
#include <vector>

#define IVERILOG "iverilog"
#define IVL "ivl"
#define VVP "vvp"

#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "fileIO.hpp"

/// utility include
#include "string_manipulation.hpp"

#define SIM_SUBDIR (Param->getOption<std::string>(OPT_output_directory) + std::string("/icarus"))

// constructor
IcarusWrapper::IcarusWrapper(const ParameterConstRef& _Param, std::string _suffix) : SimulationTool(_Param), suffix(std::move(_suffix))
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Creating the Icarus wrapper...");
   boost::filesystem::create_directory(SIM_SUBDIR + suffix + "/");
}

// destructor
IcarusWrapper::~IcarusWrapper() = default;

void IcarusWrapper::CheckExecution()
{
}

void IcarusWrapper::GenerateScript(std::ostringstream& script, const std::string& top_filename, const std::list<std::string>& file_list)
{
   log_file = SIM_SUBDIR + suffix + "/" + top_filename + "_icarus.log";
   script << "#IVERILOG" << std::endl; //-gstrict-expr-width
   script << IVERILOG;
   for(const auto& file : file_list)
   {
      script << " " << file;
   }
   script << " -g2012";
   script << " -gstrict-ca-eval";
   script << " -o " << GetPath("a.out");
   script << " -v >& " << log_file;
   script << std::endl << std::endl;

   script << "#VVP" << std::endl;
   script << VVP;
   script << " " << GetPath("a.out") << "  2>&1 | tee -a " << log_file << std::endl << std::endl;
}

#if HAVE_EXPERIMENTAL
unsigned int IcarusWrapper::convert_to_xml(const std::string& SourceFileName, const std::string& LibraryName, const std::string& TargetFileName)
{
   std::string output_file = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_file_IO_shell_output_file;
   unsigned int icarus_debug_level = Param->getOption<unsigned int>(OPT_icarus_debug_level);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Converting file \"" + SourceFileName + "\" into \"" + TargetFileName + "\"");
   std::string command = std::string(IVL);
   if(LibraryName.size())
      command += " -X \"" + LibraryName + "\"";
   std::vector<std::string> FileList = convert_string_to_vector<std::string>(SourceFileName, ";");
   std::string TmpFileName;
   bool temp_file = false;
   if(FileList.size() > 1)
   {
      TmpFileName = "__temp__.v";
      std::ostringstream TmpFileStream;
      for(unsigned int f = 0; f < FileList.size(); f++)
      {
         std::ifstream FileStream(FileList[f].c_str());
         TmpFileStream << FileStream.rdbuf() << std::endl;
         FileStream.close();
      }
      std::ofstream filestream(TmpFileName.c_str());
      filestream << TmpFileStream.str() << std::endl;
      filestream.close();
      temp_file = true;
   }
   else
   {
      TmpFileName = FileList[0];
   }
   command += " -x " + TargetFileName + " -d " + boost::lexical_cast<std::string>(icarus_debug_level) + std::string(" ") + TmpFileName;
   int err = PandaSystem(Param, command, output_file);
   if(temp_file)
      boost::filesystem::remove_all(TmpFileName);
   if(!err)
   {
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "File \"" + SourceFileName + "\" converted without errors into \"" + TargetFileName + "\"");
      if(output_level >= OUTPUT_LEVEL_VERBOSE)
         CopyStdout(output_file);
   }
   else
      THROW_ERROR("Error during conversion of file \"" + SourceFileName + "\"!!");
   return 0;
}

unsigned int IcarusWrapper::compile_verilog(const std::string& FileName)
{
   std::string output_file = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_file_IO_shell_output_file;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Compiling file \"" + FileName + "\"");
   std::string IncludeList, Include;
   if(Param->isOption("include"))
      IncludeList = Param->getOption<std::string>("include");
   if(IncludeList.size())
   {
      std::vector<std::string> splitted = SplitString(IncludeList, ";");
      for(unsigned int i = 0; i < splitted.size(); i++)
      {
         Include += " -y " + splitted[i];
      }
   }
   std::string command = IVERILOG;
   command += " " + FileName + Include + " -g2001-noconfig -gstrict-ca-eval -gstrict-expr-width -v -o /dev/null";
   int err = PandaSystem(Param, command, output_file);
   if(!err)
   {
      if(output_level >= OUTPUT_LEVEL_VERBOSE)
         CopyStdout(output_file);
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Command: \"" + command + "\" executed without errors");
   }
   else
   {
      CopyStdout(output_file);
      THROW_ERROR("Error during compilation of file \"" + FileName + "\"!!");
   }
   return 0;
}
#endif
