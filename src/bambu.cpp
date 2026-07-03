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
 * @file bambu.cpp
 * @brief High level Synthesis tool.
 *
 * Main file used to perform high-level synthesis starting from a C-based specification.
 * See \ref bambu_overview for further information
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "BackendWrapper.hpp"
#include "BambuParameter.hpp"
#include "CompilerWrapper.hpp"
#include "SimulationInformation.hpp"
#include "application_manager.hpp"
#include "c_backend_information.hpp"
#include "c_backend_step_factory.hpp"
#include "call_graph_manager.hpp"
#include "cpu_time.hpp"
#include "design_flow.hpp"
#include "design_flow_factory.hpp"
#include "design_flow_manager.hpp"
#include "evaluation.hpp"
#include "evaluation_mode.hpp"
#include "frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"
#include "functions.hpp"
#include "hls_device.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "memory.hpp"
#include "string_manipulation.hpp"
#include "technology_flow_step_factory.hpp"

#include "config_NPROFILE.hpp"

#include <cstdlib>
#include <filesystem>
#include <iosfwd>
#include <string>

namespace
{
   std::string get_settings_script_path()
   {
      try
      {
         const auto executable_path = std::filesystem::canonical("/proc/self/exe");
         return (executable_path.parent_path().parent_path() / "settings.sh").string();
      }
      catch(...)
      {
         return "<install_dir>/settings.sh";
      }
   }

   void check_runtime_environment()
   {
      const auto bambu_hls = std::getenv("BAMBU_HLS");
      const auto backend_path = std::getenv("BAMBU_HLS_BACKEND_PATH");
      if(bambu_hls != nullptr && *bambu_hls != '\0' && backend_path != nullptr && *backend_path != '\0')
      {
         return;
      }

      THROW_ERROR_USAGE("Environment not initialized: BAMBU_HLS and BAMBU_HLS_BACKEND_PATH must be set. "
                        "Please source <install_dir>/settings.sh (for example " +
                        get_settings_script_path() + ") before running bambu.");
   }
} // namespace

/**
 * Main file used to perform high-level synthesis starting from a C specification.
 * @anchor MainBambu
 * @param argc is the number of arguments
 * @param argv is the array of arguments passed to the program.
 */
int main(int argc, char* argv[])
{
   srand(static_cast<unsigned int>(time(nullptr)));

   // General options register
   ParameterRef parameters;

   try
   {
      // ---------- Initialization ------------ //

      // Synthesis cpu time

      // ---------- Parameter parsing ------------ //
      long cpu_time;
      START_TIME(cpu_time);
      parameters = ParameterRef(new BambuParameter(argv[0], argc, argv));

      switch(parameters->Exec())
      {
         case PARAMETER_NOTPARSED:
         {
            exit_code = PARAMETER_NOTPARSED;
            const auto cat_args = shell_escape_argv(argc, argv);

            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, parameters->getOption<int>(OPT_output_level),
                           " ==  Bambu executed with: " + cat_args + "\n");
            THROW_ERROR_USAGE("Bad command-line parameter format. "
                              "Please check the provided options and in case run --help.");
            break;
         }
         case EXIT_SUCCESS:
         {
            if(!parameters->getOption<bool>(OPT_no_clean) && parameters->isOption(OPT_output_temporary_directory))
            {
               std::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
            }
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

      auto output_level = parameters->getOption<int>(OPT_output_level);
      check_runtime_environment();
      if(output_level >= OUTPUT_LEVEL_MINIMUM)
      {
         parameters->PrintFullHeader(std::cerr);
      }

      if(!parameters->isOption(OPT_input_file))
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "no input files\n");
         if(!parameters->getOption<bool>(OPT_no_clean))
         {
            std::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
         }
         return EXIT_SUCCESS;
      }
      STOP_TIME(cpu_time);
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                    "Parameters parsed in " + print_cpu_time(cpu_time) + " seconds\n");

      // up to now all parameters have been parsed and data structures created, so synthesis can start

      /// ==== Creating target for the synthesis ==== ///
      HLS_deviceRef HLS_D = HLS_device::factory(parameters);

      /// ==== Creating intermediate representation ==== ///
      START_TIME(cpu_time);
      /// ==== Creating behavioral specification ==== ///
      HLS_managerRef HLSMgr = HLS_managerRef(new HLS_manager(parameters, HLS_D));
      START_TIME(HLSMgr->HLS_execution_time);
      // create the data-structures (inside application_manager) where the problem specification is contained
      DesignFlowManager design_flow_manager(parameters);
      const DesignFlowStepFactoryConstRef frontend_flow_step_factory(
          new FrontendFlowStepFactory(HLSMgr, design_flow_manager, parameters));
      design_flow_manager.RegisterFactory(frontend_flow_step_factory);
      const DesignFlowStepFactoryConstRef hls_flow_step_factory(
          new HLSFlowStepFactory(design_flow_manager, HLSMgr, parameters));
      design_flow_manager.RegisterFactory(hls_flow_step_factory);
      const DesignFlowStepFactoryConstRef c_backend_step_factory(
          new CBackendStepFactory(design_flow_manager, HLSMgr, parameters));
      design_flow_manager.RegisterFactory(c_backend_step_factory);
      const DesignFlowStepFactoryConstRef technology_flow_step_factory(
          new TechnologyFlowStepFactory(HLS_D->get_technology_manager(), HLS_D, design_flow_manager, parameters));
      design_flow_manager.RegisterFactory(technology_flow_step_factory);

      if(parameters->getOption<EvaluationMode::evaluation_mode>(OPT_evaluation_mode) == EvaluationMode::DRY_RUN)
      {
         design_flow_manager.AddStep(GetPointer<const HLSFlowStepFactory>(hls_flow_step_factory)
                                         ->CreateHLSFlowStep(HLSFlowStep_Type::EVALUATION, 0));
         design_flow_manager.Exec();
         return EXIT_SUCCESS;
      }

      if(parameters->getOption<bool>(OPT_find_max_transformations))
      {
         const DesignFlowStepRef find_max_transformations =
             GetPointer<const FrontendFlowStepFactory>(frontend_flow_step_factory)
                 ->CreateApplicationFrontendFlowStep(FrontendFlowStepType::FIND_MAX_TRANSFORMATIONS);
         design_flow_manager.AddStep(find_max_transformations);
         design_flow_manager.Exec();
         return EXIT_FAILURE;
      }
      if(parameters->isOption(OPT_test_multiple_non_deterministic_flows))
      {
         DesignFlowFactory design_flow_factory(design_flow_manager, parameters);
         const auto non_deterministic_flows =
             design_flow_factory.CreateDesignFlow(DesignFlow_Type::NON_DETERMINISTIC_FLOWS);
         design_flow_manager.AddStep(non_deterministic_flows);
         design_flow_manager.Exec();
         return EXIT_SUCCESS;
      }

      /// pretty printing
      if(parameters->isOption(OPT_pretty_print))
      {
         const auto c_backend =
             GetPointer<const CBackendStepFactory>(c_backend_step_factory)
                 ->CreateCBackendStep(CBackendInformationConstRef(new CBackendInformation(
                     CBackendInformation::CB_SEQUENTIAL, parameters->getOption<std::string>(OPT_pretty_print))));
         design_flow_manager.AddStep(c_backend);
      }

      design_flow_manager.AddStep(GetPointer<const HLSFlowStepFactory>(hls_flow_step_factory)
                                      ->CreateHLSFlowStep(parameters->getOption<HLSFlowStep_Type>(OPT_synthesis_flow)));
      design_flow_manager.Exec();
      if(!parameters->getOption<bool>(OPT_no_clean))
      {
         std::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
      }
      if(parameters->isOption(OPT_serialize_output) && parameters->isOption(OPT_output_file))
      {
         std::ofstream ofile(parameters->getOption<std::string>(OPT_output_file), std::ios::out);
         for(const auto& files : {HLSMgr->aux_files, HLSMgr->hdl_files})
         {
            for(const auto& file : files)
            {
               std::cerr << "File name: " << file << "\n";
               std::ifstream ifile(file, std::ios::in);
               ofile << ifile.rdbuf();
            }
         }
      }
      return EXIT_SUCCESS; // Bambu tool has completed execution without errors
   }

   // exception catching
   catch(const char* str)
   {
      if(EXIT_SUCCESS == exit_code)
      {
         exit_code = EXIT_FAILURE;
      }
      std::cerr << str << std::endl;
   }
   catch(const std::string& str)
   {
      if(EXIT_SUCCESS == exit_code)
      {
         exit_code = EXIT_FAILURE;
      }
      std::cerr << str << std::endl;
   }
   catch(std::exception& e)
   {
      std::cerr << e.what() << std::endl;
      if(EXIT_SUCCESS == exit_code)
      {
         exit_code = EXIT_FAILURE;
      }
   }
   catch(...)
   {
      if(EXIT_SUCCESS == exit_code)
      {
         exit_code = EXIT_FAILURE;
      }
      std::cerr << "Unknown error type" << std::endl;
   }

   switch(exit_code)
   {
      case PARAMETER_NOTPARSED:
      {
         parameters->PrintUsage(std::cout);
         break;
      }
      case EXIT_FAILURE:
      {
         if(parameters)
         {
            parameters->PrintBugReport(std::cout);
         }
         break;
      }
      case USAGE_EC:
      {
         // Usage/specification errors are expected tool diagnostics, not internal bugs.
         break;
      }
      default:
      {
      }
   }
   if(parameters && !parameters->getOption<bool>(OPT_no_clean) && parameters->isOption(OPT_output_temporary_directory))
   {
      std::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
   }
   return exit_code;
}
