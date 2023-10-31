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
 *              Copyright (C) 2023 Politecnico di Milano
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
 * @file fileIO.cpp
 * @brief utility function used to read files.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "fileIO.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "file_IO_constants.hpp"
#include "string_manipulation.hpp"

#include <cstdlib>
#include <random>

fileIO_istreamRef fileIO_istream_open(const std::string& name)
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

int PandaSystem(const ParameterConstRef Param, const std::string& system_command, bool host_exec,
                const std::string& output, const unsigned int type, const bool background, const size_t timeout)
{
   static size_t counter = 0;
   const std::string actual_output = output == "" ? Param->getOption<std::string>(OPT_output_temporary_directory) +
                                                        STR_CST_file_IO_shell_output_file + "_" + STR(counter) :
                                                    GetPath(output);
   const std::string script_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) +
                                        STR_CST_file_IO_shell_script + "_" + STR(counter++);
   counter++;
   std::ofstream script_file(script_file_name.c_str());
   script_file << "#!/bin/bash" << std::endl;
   if(host_exec)
   {
      script_file << "if [ ! -z \"$APPDIR\" ]; then\n"
                  << "  export PATH=$(sed -E 's/\\/tmp\\/.mount[^\\:]+\\://g' <<< $PATH)\n"
                  << "  export LD_LIBRARY_PATH=$(sed -E 's/\\/tmp\\/.mount[^\\:]+\\://g' <<< $LD_LIBRARY_PATH)\n"
                  << "  export PERLLIB=$(sed -E 's/\\/tmp\\/.mount[^\\:]+\\://g' <<< $PERLLIB)\n"
                  << "fi\n";
   }
   script_file << "ulimit -s 131072" << std::endl;
   script_file << "cd " << GetCurrentPath() << std::endl;
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
            THROW_UNREACHABLE("Unexpected type of stream selected " + STR(type));
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
            THROW_UNREACHABLE("Unexpected type of stream selected " + STR(type));
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
      const std::string timeout_file_name = Param->getOption<std::string>(OPT_output_temporary_directory) +
                                            STR_CST_file_IO_shell_script + "_" + STR(counter++);
      counter++;
      std::ofstream timeout_file(timeout_file_name.c_str());
      timeout_file << "#!/bin/bash" << std::endl;
      timeout_file << "timeout --foreground " << STR(timeout) << "m bash -f " << script_file_name << std::endl;
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

bool NaturalVersionOrder(const std::filesystem::path& _x, const std::filesystem::path& _y)
{
   const auto splitx = SplitString(_x.string(), ".");
   const auto splity = SplitString(_y.string(), ".");
   for(size_t i = 0U; i < splitx.size(); ++i)
   {
      if(splity.size() <= i)
      {
         return false;
      }
      if(splitx.at(i).size() != splity.at(i).size())
      {
         return splitx.at(i).size() < splity.at(i).size();
      }
      return splitx.at(i) < splity.at(i);
   }
   return false;
}

template <typename T>
void array_rand(T* arr, size_t size)
{
   static std::random_device rd;
   static std::mt19937_64 gen(rd());
   uint64_t rnd = 0;
   size_t i;

   for(i = 0; i < size; ++i, rnd >>= sizeof(T) * 8)
   {
      if(i % sizeof(uint64_t) == 0)
         rnd = gen();
      arr[i] = static_cast<T>(rnd);
   }
}

/*
 * unique_path was implemented based on the Boost implementation
 * whose copyright notice is reported below
 *
 * (C) Copyright Beman Dawes 2010.
 * Use, modification and distribution are subject to the
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
std::filesystem::path unique_path(const std::filesystem::path& model)
{
   // This function used wstring for fear of misidentifying
   // a part of a multibyte character as a percent sign.
   // However, double byte encodings only have 80-FF as lead
   // bytes and 40-7F as trailing bytes, whereas % is 25.
   // So, use string on POSIX and avoid conversions.

   std::filesystem::path::string_type s(model.native());

   const char hex[] = "0123456789abcdef";
   const char percent = '%';

   char ran[] = "123456789abcdef";          // init to avoid clang static analyzer message
                                            // see ticket #8954
   const int max_nibbles = 2 * sizeof(ran); // 4-bits per nibble

   int nibbles_used = max_nibbles;
   for(std::filesystem::path::string_type::size_type i = 0; i < s.size(); ++i)
   {
      if(s[i] == percent) // digit request
      {
         if(nibbles_used == max_nibbles)
         {
            array_rand(ran, sizeof(ran));
            nibbles_used = 0;
         }
         int c = ran[nibbles_used / 2];
         c >>= 4 * (nibbles_used++ & 1); // if odd, shift right 1 nibble
         s[i] = hex[c & 0xf];            // convert to hex digit and replace
      }
   }

   return s;
}
