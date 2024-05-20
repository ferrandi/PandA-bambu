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
 * @file tree-panda-gcc.cpp
 * @brief panda working as cross compiler targeting tree nodes...
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "config_RELEASE.hpp"

#include "Parameter.hpp"
#include "compiler_wrapper.hpp"
#include "cost_latency_table.hpp"
#include "cpu_time.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "parse_tree.hpp"
#include "tree-panda-gcc-Parameter.hpp"
#include "tree_manager.hpp"
#include "utility.hpp"

#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iosfwd>
#include <string>

static char* alloc_long_option(char* argv[], int& i, int& dec)
{
   size_t len1 = strlen(argv[i]);
   size_t len2 = strlen(argv[i + 1]);
   auto* tmp = new char[1 + len1 + 1 + len2 + 1];
   *tmp = '-';
   strcpy(tmp + 1, argv[i]);
   *(tmp + 1 + len1) = '=';
   strcpy(tmp + 1 + len1 + 1, argv[i + 1]);
   ++i;
   --dec;
   return tmp;
}

static char** alloc_argv(int& argc, char* argv[])
{
   auto** argv_copied = new char*[static_cast<unsigned>(argc) + 1u];
   int dec = 0;
   for(int i = 0; i < argc; ++i)
   {
      char* tmp;
      // std::cerr << argv[i] << std::endl;
      if(strcmp(argv[i], "-include") == 0 || strcmp(argv[i], "-isystem") == 0 || strcmp(argv[i], "-iquote") == 0 ||
         strcmp(argv[i], "-isysroot") == 0 || strcmp(argv[i], "-imultilib") == 0 || strcmp(argv[i], "-MF") == 0 ||
         strcmp(argv[i], "-MT") == 0 || strcmp(argv[i], "-MQ") == 0)
      {
         tmp = alloc_long_option(argv, i, dec);
      }
      else if(strcmp(argv[i], "-print-libgcc-file-name") == 0)
      {
         const char newvalue[] = "--print-file-name=libgcc.a";
         tmp = new char[strlen(newvalue) + 1];
         strcpy(tmp, newvalue);
      }
      else
      {
         tmp = new char[strlen(argv[i]) + 2];
         strcpy(tmp, argv[i]);
      }
      argv_copied[i + dec] = tmp;
   }
   argc = argc + dec;
   argv_copied[argc] = nullptr;
   return argv_copied;
}

static void dealloc_argv(int argc, char* argv_copied[])
{
   for(int i = 0; i < argc; ++i)
   {
      delete[] argv_copied[i];
   }
   delete[] argv_copied;
}

static void close_everything(int argc, char* argv[], const ParameterRef& Param)
{
   dealloc_argv(argc, argv);
   if(Param && not(Param->getOption<bool>(OPT_no_clean)))
   {
      std::filesystem::remove_all(Param->getOption<std::string>(OPT_output_temporary_directory));
   }
}
/**
 * Main file used to perform Hardware/Software Codesign starting from C/C++/SystemC specification.
 * @param argc an integer value
 * @param argv an array of char pointer
 * @return The main ending status
 */
int main(int argc, char* argv_orig[])
{
   char** argv = alloc_argv(argc, argv_orig);
   ParameterRef Param;
   try
   {
      long int total_time;
      START_TIME(total_time);
      // ---------- Parameter parsing ------------ //
      Param = ParameterRef(new tree_panda_gcc_parameter(argv[0], argc, argv));

      switch(Param->Exec())
      {
         case PARAMETER_NOTPARSED:
         {
            exit_code = PARAMETER_NOTPARSED;
            throw "Bad Parameters format";
         }
         case EXIT_SUCCESS:
         {
            close_everything(argc, argv, Param);
            return EXIT_SUCCESS;
         }
         case PARAMETER_PARSED:
         {
            exit_code = EXIT_FAILURE;
            break;
         }
         default:
         {
            THROW_ERROR("Bad Parameters parsing");
         }
      }
      auto output_level = Param->getOption<int>(OPT_output_level);
      if(output_level >= OUTPUT_LEVEL_MINIMUM)
      {
         Param->PrintFullHeader(std::cerr);
      }

      auto debug_level = Param->getOption<int>(OPT_debug_level);

      /// GCC wrapper to create the raw
      // it means that the Param are source code input files: GCC wrapper has to be invoked
      const tree_managerRef TM(new tree_manager(Param));
      if(Param->isOption(OPT_input_file))
      {
         long int wrapping_time;

         START_TIME(wrapping_time);
         if(debug_level >= DEBUG_LEVEL_MINIMUM)
         {
            if(debug_level >= DEBUG_LEVEL_VERBOSE)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "");
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "************************************");
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "*    Starting GNU/GCC wrapping     *");
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "************************************");
            }
            else
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "\n ==== Starting GNU/GCC wrapping ====");
            }
         }

         const CompilerWrapper_OptimizationSet optimization_set =
             Param->getOption<CompilerWrapper_OptimizationSet>(OPT_gcc_optimization_set);
         const CompilerWrapper_CompilerTarget compiler_target =
             Param->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
         CompilerWrapperRef Wrap = CompilerWrapperRef(new CompilerWrapper(Param, compiler_target, optimization_set));

         auto input_files = Param->getOption<std::vector<std::string>>(OPT_input_file);

         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                       "Created list of files: " + std::to_string(input_files.size()) +
                           " input source code files to be concatenated");

         /// creating the tree manager from the data structure
         Wrap->FillTreeManager(TM, input_files, STR_cost_latency_table_default);

         // Dump the configuration file if it has been requested by the user. Note that if the configuration
         // has been dumped in this point (after GCC compiler has been invoked), the compilation has been completed
         // with success and, so, the configuration is correct.

         STOP_TIME(wrapping_time);
#ifndef NDEBUG
         if(debug_level >= DEBUG_LEVEL_MINIMUM)
         {
            dump_exec_time("Gcc wrapping time", wrapping_time);
         }
#endif
      }
      if(!Param->isOption(OPT_gcc_E) && !Param->isOption(OPT_gcc_S))
      {
         long int tree_time;
         START_TIME(tree_time);
         if(Param->isOption(OPT_obj_files))
         {
            const auto object_files = Param->getOption<CustomSet<std::string>>(OPT_obj_files);
            for(const auto& object_file : object_files)
            {
               if(!std::filesystem::exists(object_file))
               {
                  THROW_ERROR("File " + object_file + " does not exist");
               }
               const tree_managerRef TM_new = ParseTreeFile(Param, object_file);
               TM->merge_tree_managers(TM_new);
            }
         }
         if(Param->isOption(OPT_archive_files))
         {
            const auto archive_files = Param->getOption<CustomSet<std::string>>(OPT_archive_files);
            const auto output_temporary_directory =
                Param->getOption<std::filesystem::path>(OPT_output_temporary_directory);
            const auto temp_path = output_temporary_directory / "archives";
            std::filesystem::create_directories(temp_path);
            std::string command = "cd " + temp_path.string() + "\n";
            for(const auto& archive_file : archive_files)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reading " + archive_file);
               if(!std::filesystem::exists(archive_file))
               {
                  THROW_ERROR("File " + archive_file + " does not exist");
               }

               command += " ar x " + std::filesystem::path(archive_file).lexically_proximate(temp_path).string() +
                          " || touch error &\n";
            }
            command += " wait\n if [ -e \"error\" ]; then exit -1; fi";
            if(IsError(PandaSystem(Param, command)))
            {
               THROW_ERROR("ar returns an error during archive extraction.");
            }
            for(const auto& archive : std::filesystem::directory_iterator{temp_path})
            {
               const auto fileExtension = archive.path().extension().string();
               if(fileExtension != ".o" && fileExtension != ".O")
               {
                  continue;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Loading " + archive.path().string());
               const auto TM_new = ParseTreeFile(Param, archive.path().string());
               TM->merge_tree_managers(TM_new);
            }
            if(!Param->getOption<bool>(OPT_no_clean))
            {
               std::filesystem::remove_all(temp_path);
            }
         }
         STOP_TIME(tree_time);

         if(debug_level >= DEBUG_LEVEL_MINIMUM)
         {
            dump_exec_time("Tree analysis time", tree_time);
         }
         std::string raw_file_name;
         if(Param->isOption(OPT_compress_archive))
         {
            auto archive_file = Param->getOption<std::string>(OPT_compress_archive);
            std::string fname = archive_file.substr(0, archive_file.find('.'));
            fname = fname + ".o";
            {
               fileIO_ostreamRef raw_file = fileIO_ostream_open(fname);
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Dumping Tree-Manager");
               (*raw_file) << TM;
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Dumped Tree-Manager");
            }
            std::string command = "ar cru " + archive_file + " " + fname;
            // std::cout << command << std::endl;
            int ret = PandaSystem(Param, command);
            if(IsError(ret))
            {
               THROW_ERROR("ar returns an error during archive creation ");
            }
         }
         else
         {
            if(Param->isOption(OPT_output_file))
            {
               raw_file_name = Param->getOption<std::string>(OPT_output_file);
            }
            else
            {
               raw_file_name = "a.tree";
            }
            fileIO_ostreamRef raw_file = fileIO_ostream_open(raw_file_name);
            PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Dumping Tree-Manager");
            (*raw_file) << TM;
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Dumped Tree-Manager");
         }
      }
      STOP_TIME(total_time);

      if(debug_level > DEBUG_LEVEL_NONE)
      {
         dump_exec_time("\nTotal execution time", total_time);
      }

      close_everything(argc, argv, Param);
      return EXIT_SUCCESS;
   }
   catch(const char* str)
   {
      std::cerr << str << std::endl;
   }
   catch(const std::string& str)
   {
      std::cerr << str << std::endl;
   }
   catch(std::exception& e)
   {
      std::cerr << e.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "Unknown error type" << std::endl;
   }

   close_everything(argc, argv, Param);
   return exit_code;
}
