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
 * @file bambu-cc.cpp
 * @brief panda working as cross compiler targeting IR nodes...
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "config_RELEASE.hpp"

#include "CompilerWrapper.hpp"
#include "Parameter.hpp"
#include "bambu-cc-Parameter.hpp"
#include "cost_latency_table.hpp"
#include "cpu_time.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "ir_manager.hpp"
#include "parse_ir.hpp"
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
   if(Param && !Param->getOption<bool>(OPT_no_clean) && Param->isOption(OPT_output_temporary_directory))
   {
      std::filesystem::remove_all(Param->getOption<std::string>(OPT_output_temporary_directory));
   }
}
/**
 * Main file used to perform Hardware/Software Codesign starting from C/C++/SystemC specification.
 * @param argc an integer value
 * @param argv_orig an array of char pointer
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
      Param = ParameterRef(new bambu_cc_parameter(argv[0], argc, argv));

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

      /// wrapper to create the raw
      // it means that the Param are source code input files: Compiler wrapper has to be invoked
      const ir_managerRef TM(new ir_manager(Param));
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
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "*    Starting CC wrapping     *");
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "************************************");
            }
            else
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "\n ==== Starting CC wrapping ====");
            }
         }

         auto input_files = Param->getOption<std::vector<std::string>>(OPT_input_file);

         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                       "Created list of files: " + std::to_string(input_files.size()) +
                           " input source code files to be concatenated");

         /// creating the IR manager from the data structure
         const auto compiler_target = Param->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
         CompilerWrapper(Param, compiler_target).FillIRManager(TM, input_files, STR_cost_latency_table_default);

         // Dump the configuration file if it has been requested by the user. Note that if the configuration
         // has been dumped in this point (after compiler has been invoked), the compilation has been completed
         // with success and, so, the configuration is correct.

         STOP_TIME(wrapping_time);
#ifndef NDEBUG
         if(debug_level >= DEBUG_LEVEL_MINIMUM)
         {
            dump_exec_time("CC wrapping time", wrapping_time);
         }
#endif
      }
      if(!Param->isOption(OPT_cc_E) && !Param->isOption(OPT_cc_S))
      {
         long int ir_time;
         START_TIME(ir_time);
         if(Param->isOption(OPT_obj_files))
         {
            const auto object_files = Param->getOption<CustomSet<std::string>>(OPT_obj_files);
            for(const auto& object_file : object_files)
            {
               if(!std::filesystem::exists(object_file))
               {
                  THROW_ERROR("File " + object_file + " does not exist");
               }
               const ir_managerRef TM_new = ParseIRFile(Param, object_file);
               TM->merge_ir_managers(TM_new);
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

               command += " ar x " + std::filesystem::absolute(archive_file).lexically_proximate(temp_path).string() +
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
               const auto TM_new = ParseIRFile(Param, archive.path().string());
               TM->merge_ir_managers(TM_new);
            }
            if(!Param->getOption<bool>(OPT_no_clean))
            {
               std::filesystem::remove_all(temp_path);
            }
         }
         STOP_TIME(ir_time);

         if(debug_level >= DEBUG_LEVEL_MINIMUM)
         {
            dump_exec_time("IR analysis time", ir_time);
         }
         std::string raw_file_name;
         if(Param->isOption(OPT_compress_archive))
         {
            const auto archive_file = Param->getOption<std::filesystem::path>(OPT_compress_archive);
            auto temp_obj = Param->getOption<std::filesystem::path>(OPT_output_temporary_directory) /
                            archive_file.filename().replace_extension(".o");
            {
               fileIO_ostreamRef raw_file = fileIO_ostream_open(temp_obj.string());
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Dumping IR manager");
               TM->print(*raw_file);
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Dumped IR manager");
            }
            const auto command = "ar cru " + archive_file.string() + " " + temp_obj.string();
            if(IsError(PandaSystem(Param, command)))
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
               raw_file_name = "a.ir";
            }
            fileIO_ostreamRef raw_file = fileIO_ostream_open(raw_file_name);
            PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Dumping IR manager");
            TM->print(*raw_file);
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Dumped IR manager");
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
