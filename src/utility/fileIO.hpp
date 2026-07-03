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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file fileIO.hpp
 * @brief utility function used to read files.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef FILEIO_HPP
#define FILEIO_HPP

#include "gzstream/gzstream.hpp"
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
 * @brief Convert relative path to install prefix to absolute path
 *
 * @param path Relative path to install prefix
 * @param relocatable Leave unresolved environment variables in computed path
 * @return std::filesystem::path Absolute input path
 */
std::filesystem::path relocate_install_path(const std::filesystem::path& path, bool relocatable = false);

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
 * @return int return value of system function call (error code or 'wait status' of the executed process)
 */
int PandaSystem(const ParameterConstRef& Param, const std::string& system_command, bool host_exec = true,
                const std::filesystem::path& output = "");

/**
 * Return true if wstatus corresponds to an error
 * @param wstatus is the return value of PandaSystem
 * @return true if wstatus corresponds to an error
 */
bool IsError(int wstatus);

bool NaturalVersionOrder(const std::filesystem::path& _x, const std::filesystem::path& _y);

std::filesystem::path unique_path(const std::filesystem::path& model);

inline bool is_subpath(const std::filesystem::path& path, const std::filesystem::path& base)
{
   const auto rel = std::filesystem::relative(path, base);
   return !rel.empty() && rel.native()[0] != '.';
}

inline std::filesystem::path proximate_if_subpath(const std::filesystem::path& p, const std::filesystem::path& base)
{
   if(is_subpath(p, base))
   {
      return p.lexically_proximate(base);
   }
   return std::filesystem::absolute(p);
}

inline bool is_elf(const std::filesystem::path& path)
{
   if(!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
   {
      return false;
   }

   std::ifstream file(path, std::ios::binary);
   if(!file.is_open())
   {
      return false;
   }

   uint32_t magicNumber = 0;
   file.read(reinterpret_cast<char*>(&magicNumber), sizeof(magicNumber));

   // ELF magic number in little-endian: 0x464C457F ('F' 'L' 'E' 0x7F)
   return magicNumber == 0x464C457F;
}

#endif
