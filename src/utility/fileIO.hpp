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
 * @file fileIO.hpp
 * @brief utility function used to read files.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef FILEIO_HPP
#define FILEIO_HPP

#include "Parameter.hpp"                   // for ParameterConstRef
#include "dbgPrintHelper.hpp"              // for OUTPUT_LEVEL_PED...
#include "exceptions.hpp"                  // for THROW_UNREACHABLE
#include "file_IO_constants.hpp"           // for STR_CST_file_IO_...
#include "gzstream.hpp"                    // for igzstream, ogzst...
#include "refcount.hpp"                    // for refcount
#include <boost/filesystem/operations.hpp> // for copy_file, remove
#include <boost/filesystem/path.hpp>       // for path
#include <boost/lexical_cast.hpp>          // for lexical_cast
#include <boost/system/error_code.hpp>     // for error_code
#include <boost/version.hpp>               // for BOOST_VERSION
#include <cstdio>                          // for size_t, fclose
#include <cstdlib>                         // for system
#include <iostream>                        // for operator<<, basi...
#include <string>                          // for string, operator+

/// Return value of timeout signaling timeout has reached
#define TIMEOUT 124

/// Return value of an application which was signaled by ulimit
#define ULIMIT 153

/**
 * RefCount type definition for the input stream object.
 */
typedef refcount<std::istream> fileIO_istreamRef;
typedef refcount<const std::istream> fileIO_istreamConstRef;

/**
 * RefCount type definition for the input stream object.
 */
typedef refcount<std::ostream> fileIO_ostreamRef;

/**
 * this function returns an istream compressed or not.
 * It first check for a compressed file, then search for the compressed version of the file and finally in case no compressed file is found it look for the plain text file.
 * this function is mainly based on the gzstream wrapper and on zlib library.
 * @param name is the file name.
 * @return the refcount to the istream
 */
inline fileIO_istreamRef fileIO_istream_open(const std::string& name)
{
   fileIO_istreamRef res_file;
   res_file = fileIO_istreamRef(new igzstream((name).c_str()));
   if(res_file->fail())
   {
      res_file = fileIO_istreamRef(new igzstream((name + ".gz").c_str()));
      if(res_file->fail())
      {
         res_file = fileIO_istreamRef(new igzstream((name + ".Z").c_str()));
         if(res_file->fail())
         {
            res_file = fileIO_istreamRef(new std::ifstream(name.c_str()));
            if(res_file->fail())
            {
               THROW_ERROR("Error in opening " + name);
            }
         }
      }
   }
   return res_file;
}

/**
 * Create a fileIO_istreamRef starting from a string
 */
inline fileIO_istreamRef fileIO_istream_open_from_string(const std::string& input)
{
   fileIO_istreamRef output;
   output = fileIO_istreamRef(new std::istringstream(input));
   return output;
}

/**
 * this function returns an ostream compressed or not.
 * this function is mainly based on the gzstream wrapper and on zlib library.
 * @param name is the file name.
 * @return the refcount to the ostream
 */
inline fileIO_ostreamRef fileIO_ostream_open(const std::string& name)
{
   fileIO_ostreamRef res_file;
   res_file = fileIO_ostreamRef(new ogzstream((name).c_str()));
   return res_file;
}

#ifdef _WIN32
#define EXEEXT std::string(".exe")
#else
#define EXEEXT std::string("")
#endif

/**
 * Copy a file to the standard output
 */
inline void CopyStdout(const std::string& filename)
{
   FILE* filese;
   filese = fopen(filename.c_str(), "r");
   char buffer[255];
   size_t nBytes;
   while((nBytes = fread(buffer, 1, sizeof(buffer), filese)) > 0)
   {
      size_t wBytes = fwrite(buffer, 1, nBytes, stdout);
      if(wBytes < nBytes)
         break;
   }
   fclose(filese);
}

inline void rename_file(boost::filesystem::path from, boost::filesystem::path to)
{
#if BOOST_VERSION >= 104600
   /// FIXME: this part has not yet been tested
   boost::system::error_code return_error;
   boost::filesystem::rename(from, to, return_error);
   if(return_error.value())
   {
      boost::filesystem::remove(to);
      boost::filesystem::copy_file(from, to);
      boost::filesystem::remove(from);
   }
#else
   try
   {
      boost::filesystem::rename(from, to);
   }
   catch(const boost::filesystem::basic_filesystem_error<boost::filesystem::path>)
   {
      boost::filesystem::remove(to);
      boost::filesystem::copy_file(from, to);
      boost::filesystem::remove(from);
   }
#endif
}

/**
 * Return the filename (base + extension) without the path
 * @param file is the starting file
 * @return the file without the path
 */
inline std::string GetLeafFileName(boost::filesystem::path file)
{
#if BOOST_VERSION >= 104600
   return file.filename().string();
#else
   return file.leaf();
#endif
}

/**
 * Return the file without the extension
 * @param file is the starting file
 * @return the file without path and extension
 */
inline std::string GetBaseName(boost::filesystem::path file)
{
   std::string basename;
#if BOOST_VERSION >= 104600
   basename = file.stem().string();
#else
   basename = file.stem();
#endif
   return basename;
}

/**
 * Return the directory given a file
 * @param file is the file
 * @return the directory
 */
inline std::string GetDirectory(const boost::filesystem::path file)
{
#if BOOST_VERSION >= 104600
   return file.parent_path().string();
#else
   return file.parent_path();
#endif
}

/**
 * Return the extension of the file
 * @param file is the starting file
 * @return the extension of the file
 */
inline std::string GetExtension(const std::string& file)
{
   return file.find(".") == std::string::npos ? "" : file.substr(file.find_last_of(".") + 1);
}

inline std::string GetExtension(boost::filesystem::path file)
{
   return GetExtension(file.string());
}
inline std::string GetExtension(const char* file)
{
   return GetExtension(std::string(file));
}

inline std::string GetCurrentPath()
{
   std::string current_dir;
   if(getenv("OWD"))
      current_dir = getenv("OWD");
   else
      current_dir = boost::filesystem::current_path().string();
#ifdef _WIN32
   boost::replace_all(current_dir, "\\", "/");
#endif
   return current_dir;
}

inline std::string GetPath(const std::string& path)
{
   boost::filesystem::path local_path_file = path;
   if(local_path_file.is_relative())
      local_path_file = boost::filesystem::path(GetCurrentPath()) / local_path_file;
   return local_path_file.string();
}

inline std::string relocate_compiler_path(const std::string& path)
{
   if(getenv("MINGW_INST_DIR"))
   {
      std::string app_prefix = getenv("MINGW_INST_DIR");
      return app_prefix + path;
   }
   else if(getenv("APPDIR"))
   {
      std::string app_prefix = getenv("APPDIR");
      return app_prefix + path;
   }
#ifdef _WIN32
   else
      return "c:/msys64/" + path;
#else
   else
      return path;
#endif
}
inline bool ExistFile(const std::string& file)
{
   return boost::filesystem::exists(file);
}

/**
 * Copy file; if target already exist, overwrite
 * @param file_source is the file to be copied
 * @param file_target is the destination
 */
inline void CopyFile(boost::filesystem::path file_source, boost::filesystem::path file_target)
{
   if(file_source.string() == "-")
   {
      std::string line;
      std::ofstream new_file(file_target.string().c_str());
      while(std::cin)
      {
         std::getline(std::cin, line);
         new_file << line << std::endl;
      }
   }
   else
      boost::filesystem::copy_file(file_source, file_target, boost::filesystem::copy_option::overwrite_if_exists);
}

/**
 * Build a path by combining two relative paths
 * @param first_part is the first part to be combined
 * @param second_part is the second part to be combined
 */
inline std::string BuildPath(const std::string& first_part, const std::string second_part)
{
   return (boost::filesystem::path(first_part) / boost::filesystem::path(second_part)).string();
}

/**
 * System call forcing execution with bash
 * @param Param is the set of input parameters
 * @param system_command is the  to be executed
 * @param output is the file where output has to be saved
 * @param type specifies which streams have to be saved; possible values are 0 (none), 1 (stdout), 2 (stderr), 3(stdout and stderr)
 * @param background specifies if the command has to be executed in background
 * @param timeout is the timeout for the command (in minutes)
 * @return the value returned by the shell
 */
inline int PandaSystem(const ParameterConstRef Param, const std::string& system_command, const std::string& output = "", const unsigned int type = 3, const bool background = false, const size_t timeout = 0)
{
   static size_t counter = 0;
   const std::string actual_output = output == "" ? Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_file_IO_shell_output_file + "_" + boost::lexical_cast<std::string>(counter) : GetPath(output);
   const std::string script_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_file_IO_shell_script + "_" + boost::lexical_cast<std::string>(counter++);
   counter++;
   std::ofstream script_file(script_file_name.c_str());
   script_file << "#!/bin/bash" << std::endl;
   THROW_ASSERT(not background or timeout == 0, "Background and timeout cannot be specified at the same time");
   if(background)
   {
      script_file << "(";
   }
   script_file << "(" << system_command << ") ";
   if(Param->getOption<unsigned int>(OPT_output_level) >= OUTPUT_LEVEL_PEDANTIC)
   {
      switch(type)
      {
         case(0):
         {
            script_file << " > /dev/null 2>&1 ";
            break;
         }
         case(1):
         {
            script_file << " 2>/dev/null | tee " << actual_output;
            break;
         }
         case(2):
         {
            script_file << " 2>&1 1>/dev/null | tee " << actual_output;
            break;
         }
         case(3):
         {
            script_file << " 2>&1 | tee " << actual_output;
            break;
         }
         default:
         {
            THROW_UNREACHABLE("Unexpected type of stream selected " + boost::lexical_cast<std::string>(type));
         }
      }
   }
   else
   {
      switch(type)
      {
         case(0):
         {
            script_file << " > /dev/null 2>&1 ";
            break;
         }
         case(1):
         {
            script_file << " 2> /dev/null > " << actual_output;
            break;
         }
         case(2):
         {
            script_file << " > /dev/null 2> " << actual_output;
            break;
         }
         case(3):
         {
            script_file << " > " << actual_output << " 2>&1 ";
            break;
         }
         default:
         {
            THROW_UNREACHABLE("Unexpected type of stream selected " + boost::lexical_cast<std::string>(type));
         }
      }
   }
   if(background)
   {
      script_file << ") &";
   }
   script_file << std::endl;
   if(Param->getOption<unsigned int>(OPT_output_level) >= OUTPUT_LEVEL_PEDANTIC)
   {
      script_file << "exit ${PIPESTATUS[0]}" << std::endl;
   }
   script_file.close();
   if(timeout != 0)
   {
      const std::string timeout_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_file_IO_shell_script + "_" + boost::lexical_cast<std::string>(counter++);
      counter++;
      std::ofstream timeout_file(timeout_file_name.c_str());
      timeout_file << "#!/bin/bash" << std::endl;
      timeout_file << "timeout --foreground " << boost::lexical_cast<std::string>(timeout) << "m bash -f " << script_file_name << std::endl;
      timeout_file.close();
      const std::string command = "bash -f " + timeout_file_name + "";
      return system(command.c_str());
   }
   else
   {
      const std::string command = "bash -f " + script_file_name + "";
      return system(command.c_str());
   }
}
#endif
