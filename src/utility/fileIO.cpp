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
 *              Copyright (C) 2023-2026 Politecnico di Milano
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
 * @file fileIO.cpp
 * @brief utility function used to read files.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
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

#include "config_BUILD_APPIMAGE.hpp"

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

static std::filesystem::path get_exe_path()
{
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
}

std::filesystem::path relocate_install_path(const std::filesystem::path& path, bool relocatable)
{
   static const auto reloc_exe_path = []() {
      const auto abs_exe_path = get_exe_path().parent_path().parent_path();
      if(getenv("BAMBU_HLS"))
      {
         return std::filesystem::path("$BAMBU_HLS");
      }
      return abs_exe_path;
   }();
   static const auto abs_exe_path = get_exe_path().parent_path().parent_path();

   return (relocatable ? reloc_exe_path : abs_exe_path) / path;
}

int PandaSystem(const ParameterConstRef& Param, const std::string& system_command,
                bool
#if BUILD_APPIMAGE
                    host_exec
#endif
                ,
                const std::filesystem::path& output)
{
   static size_t counter = 0;
   const auto run_index = counter++;
   const auto output_temp_directory = Param->getOption<std::filesystem::path>(OPT_output_temporary_directory);
   const auto script_path = output_temp_directory / (STR_CST_file_IO_shell_script "_" + STR(run_index));
   const auto actual_output = proximate_if_subpath(
       output.empty() ? output_temp_directory / (STR_CST_file_IO_shell_output_file "_" + STR(run_index)) : output,
       std::filesystem::current_path());
   std::ofstream script_file(script_path);
   script_file << "#!/usr/bin/env bash\n"
               << "ulimit -s 131072\n";
#if BUILD_APPIMAGE
   if(host_exec)
   {
      script_file
          << "if [ ! -z \"$APPDIR\" ]; then\n"
          << "  export PATH=`sed -E \"s,${APPDIR}/[^\\:]+[\\:^](\\:|\\$),,g\" <<< $PATH`\n"
          << "  export LD_LIBRARY_PATH=`sed -E \"s,${APPDIR}/[^\\:]+[\\:^](\\:|\\$),,g\" <<< $LD_LIBRARY_PATH`\n"
          << "  export PERLLIB=`sed -E \"s,${APPDIR}/[^\\:]+[\\:^](\\:|\\$),,g\" <<< $PERLLIB`\n"
          << "fi\n";
   }
#endif
   script_file << "(" << system_command << ") ";
   if(Param->getOption<unsigned int>(OPT_output_level) >= OUTPUT_LEVEL_PEDANTIC)
   {
      script_file << " 2>&1 | tee " << actual_output;
   }
   else
   {
      script_file << " > " << actual_output << " 2>&1 ";
   }
   script_file << "\n";
   if(Param->getOption<unsigned int>(OPT_output_level) >= OUTPUT_LEVEL_PEDANTIC)
   {
      script_file << "exit ${PIPESTATUS[0]}\n";
   }
   script_file.close();
   const std::string command = "bash -f " + script_path.string();
   const auto wstatus = system(command.c_str());
   if(wstatus == -1)
   {
      THROW_ERROR("system call error: " + std::string(strerror(errno)));
   }
   return wstatus;
}

bool IsError(int wstatus)
{
   return wstatus == -1 || WIFSIGNALED(wstatus) || (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) != EXIT_SUCCESS);
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

std::filesystem::path unique_path(const std::filesystem::path& model)
{
   std::filesystem::path::string_type s(model.native());
   static constexpr auto hex_digits = "0123456789abcdef";
   thread_local std::mt19937_64 generator(std::random_device{}());
   std::uniform_int_distribution<unsigned> hex_digit(0, 15);

   for(auto& c : s)
   {
      if(c == std::filesystem::path::value_type{'%'})
      {
         c = static_cast<std::filesystem::path::value_type>(hex_digits[hex_digit(generator)]);
      }
   }

   return s;
}
