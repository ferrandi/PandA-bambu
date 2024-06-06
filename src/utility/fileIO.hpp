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
 * @file fileIO.hpp
 * @brief utility function used to read files.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef FILEIO_HPP
#define FILEIO_HPP

#include "gzstream.hpp"
#include "refcount.hpp"

#include <filesystem>
#include <iostream>
#include <string>

CONSTREF_FORWARD_DECL(Parameter);

/// Return value of timeout signaling timeout has reached
#define TIMEOUT 124

/// Return value of an application which was signaled by ulimit
#define ULIMIT 153

/**
 * RefCount type definition for the input stream object.
 */
using fileIO_istreamRef = refcount<std::istream>;
using fileIO_istreamConstRef = refcount<const std::istream>;

/**
 * RefCount type definition for the input stream object.
 */
using fileIO_ostreamRef = refcount<std::ostream>;

/**
 * this function returns an istream compressed or not.
 * It first check for a compressed file, then search for the compressed version of the file and finally in case no
 * compressed file is found it look for the plain text file. this function is mainly based on the gzstream wrapper and
 * on zlib library.
 * @param name is the file name.
 * @return the refcount to the istream
 */
fileIO_istreamRef fileIO_istream_open(const std::string& name);

/**
 * Create a fileIO_istreamRef starting from a string
 */
inline fileIO_istreamRef fileIO_istream_open_from_string(const std::string& input)
{
   return fileIO_istreamRef(new std::istringstream(input));
}

/**
 * this function returns an ostream compressed or not.
 * this function is mainly based on the gzstream wrapper and on zlib library.
 * @param name is the file name.
 * @return the refcount to the ostream
 */
inline fileIO_ostreamRef fileIO_ostream_open(const std::string& name)
{
   return fileIO_ostreamRef(new ogzstream((name).c_str()));
}

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
      {
         break;
      }
   }
   fclose(filese);
}

/**
 * @brief Convert relative path to install prefix made relative to base
 *
 * @param path Relative path to install prefix
 * @param base Base directory to
 * @return std::filesystem::path Input path made relative to current working directory
 */
std::filesystem::path relocate_install_path(const std::filesystem::path& path,
                                            const std::filesystem::path& base = std::filesystem::current_path());

/**
 * Copy file; if target already exist, overwrite
 * @param file_source is the file to be copied
 * @param file_target is the destination
 */
inline void CopyFile(std::filesystem::path file_source, std::filesystem::path file_target)
{
   if(file_source.string() == "-")
   {
      std::string line;
      std::ofstream new_file(file_target);
      while(std::cin)
      {
         std::getline(std::cin, line);
         new_file << line << std::endl;
      }
   }
   else
   {
      std::filesystem::copy_file(file_source, file_target, std::filesystem::copy_options::overwrite_existing);
   }
}

/**
 * System call forcing execution with bash
 * @param Param is the set of input parameters
 * @param system_command is the  to be executed
 * @param host_exec specifies if the executable is expected to be in the host system or distributed within the AppImage
 * @param output is the file where output has to be saved
 * @param type specifies which streams have to be saved; possible values are 0 (none), 1 (stdout), 2 (stderr), 3(stdout
 * and stderr)
 * @param background specifies if the command has to be executed in background
 * @param timeout is the timeout for the command (in minutes)
 * @return the value returned by the shell
 */
int PandaSystem(const ParameterConstRef Param, const std::string& system_command, bool host_exec = true,
                const std::filesystem::path& output = "", const unsigned int type = 3, const bool background = false,
                const size_t timeout = 0);

bool NaturalVersionOrder(const std::filesystem::path& _x, const std::filesystem::path& _y);

std::filesystem::path unique_path(const std::filesystem::path& model);

#endif
