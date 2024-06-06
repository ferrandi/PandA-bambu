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
 *              Copyright (C) 2023-2024 Politecnico di Milano
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
#include <regex>

#if !defined(PATH_MAX)
// For GNU Hurd
#if defined(__GNU__)
#define PATH_MAX 4096
#endif
#endif

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

std::filesystem::path relocate_install_path(const std::filesystem::path& path, const std::filesystem::path& base)
{
   static const std::filesystem::path main_exe_path =
       []() {
#if defined(__linux__) || defined(__CYGWIN__) || defined(__gnu_hurd__)
          char exe_path[PATH_MAX];
          const char* aPath = "/proc/self/exe";
          if(std::filesystem::exists(aPath))
          {
             // /proc is not always mounted under Linux (chroot for example).
             ssize_t len = readlink(aPath, exe_path, sizeof(exe_path));
             if(len >= 0)
             {
                // Null terminate the string for realpath. readlink never null
                // terminates its output.
                len = std::min(len, ssize_t(sizeof(exe_path) - 1));
                exe_path[len] = '\0';

            // On Linux, /proc/self/exe always looks through symlinks. However, on
            // GNU/Hurd, /proc/self/exe is a symlink to the path that was used to start
            // the program, and not the eventual binary file. Therefore, call realpath
            // so this behaves the same on all platforms.
#if _POSIX_VERSION >= 200112 || defined(__GLIBC__)
                std::unique_ptr<char, void (*)(void*)> real_path(realpath(exe_path, nullptr), std::free);
                if(real_path)
                {
                   return std::filesystem::path(real_path.get());
                }
#else
                char real_path[PATH_MAX];
                if(realpath(exe_path, real_path))
                   return std::filesystem::path(real_path);
#endif
             }
          }
#elif defined(__sun__) && defined(__svr4__)
          char exe_path[PATH_MAX];
          const char* aPath = "/proc/self/execname";
          if(std::filesystem::path::exists(aPath))
          {
             int fd = open(aPath, O_RDONLY);
             if(fd != -1 && read(fd, exe_path, sizeof(exe_path)) >= 0)
                return std::filesystem::path(exe_path);
          }
#else
#error Main executable path retrieve is not implemented on this host yet.
#endif
          THROW_ERROR("Bambu executable path too long.");
          return std::filesystem::path();
       }()
           .parent_path()
           .parent_path();
   return (main_exe_path / path).lexically_proximate(base);
}

int PandaSystem(const ParameterConstRef Param, const std::string& system_command, bool host_exec,
                const std::filesystem::path& output, const unsigned int type, const bool background,
                const size_t timeout)
{
   static size_t counter = 0;
   const auto run_index = counter++;
   const auto script_path = Param->getOption<std::filesystem::path>(OPT_output_temporary_directory) /
                            (STR_CST_file_IO_shell_script "_" + STR(run_index));
   const auto actual_output = output.empty() ? Param->getOption<std::filesystem::path>(OPT_output_temporary_directory) /
                                                   (STR_CST_file_IO_shell_output_file "_" + STR(run_index)) :
                                               output;
   std::ofstream script_file(script_path);
   script_file << "#!/bin/bash\n"
               << "ulimit -s 131072\n";
   if(host_exec)
   {
      script_file << "if [ ! -z \"$APPDIR\" ]; then\n"
                  << "  export PATH=$(sed -E 's/\\/tmp\\/.mount[^\\:]+\\://g' <<< $PATH)\n"
                  << "  export LD_LIBRARY_PATH=$(sed -E 's/\\/tmp\\/.mount[^\\:]+\\://g' <<< $LD_LIBRARY_PATH)\n"
                  << "  export PERLLIB=$(sed -E 's/\\/tmp\\/.mount[^\\:]+\\://g' <<< $PERLLIB)\n"
                  << "fi\n";
   }
   THROW_ASSERT(!background || timeout == 0, "Background and timeout cannot be specified at the same time");
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
      script_file << "exit ${PIPESTATUS[0]}\n";
   }
   script_file.close();
   if(timeout != 0)
   {
      const auto timeout_path = Param->getOption<std::filesystem::path>(OPT_output_temporary_directory) /
                                (STR_CST_file_IO_shell_script "_" + STR(counter++));
      std::ofstream timeout_file(timeout_path);
      timeout_file << "#!/bin/bash\ntimeout --foreground " << timeout << "m bash -f " << script_path.filename() << "\n";
      timeout_file.close();
      const std::string command = "bash -f " + timeout_path.string();
      return system(command.c_str());
   }
   else
   {
      const std::string command = "bash -f " + script_path.string();
      return system(command.c_str());
   }
}

bool NaturalVersionOrder(const std::filesystem::path& _x, const std::filesystem::path& _y)
{
   const std::regex version_number("\\d+(\\.\\d+)*");
   const auto x = _x.string(), y = _y.string();
   std::cmatch mx, my;
   if(std::regex_search(x.c_str(), mx, version_number))
   {
      if(std::regex_search(y.c_str(), my, version_number))
      {
         const char *px = mx[0].first, *lx;
         const char *py = my[0].first, *ly;
         do
         {
            lx = std::find(px, mx[0].second, '.');
            ly = std::find(py, my[0].second, '.');
            if(py == ly)
            {
               return false;
            }
            auto dx = std::distance(px, lx), dy = std::distance(py, ly);
            if(dx != dy)
            {
               return dx < dy;
            }
            do
            {
               if(*px != *py)
               {
                  return *px < *py;
               }
               ++py;
            } while(++px != lx);
            ++py;
         } while(++px != mx[0].second);
         return true;
      }
      return false;
   }
   else if(std::regex_search(_y.string().c_str(), my, version_number))
   {
      return true;
   }
   return _x < _y;
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
