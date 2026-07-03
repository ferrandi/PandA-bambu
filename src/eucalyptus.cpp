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
 * @file eucalyptus.cpp
 * @brief Tool for estimation of RTL descriptions.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "EucalyptusParameter.hpp"
#include "RTL_characterization.hpp"
#include "cpu_time.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "design_flow_step.hpp"
#include "generic_device.hpp"
#include "load_builtin_technology.hpp"
#include "load_default_technology.hpp"
#include "parse_technology.hpp"
#include "technology_flow_step_factory.hpp"
#include "technology_manager.hpp"
#include "utility.hpp"
#include <cstdlib>
#include <filesystem>
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
                        get_settings_script_path() + ") before running eucalyptus.");
   }
} // namespace

int main(int argc, char* argv[])
{
   // Program name

   ParameterRef parameters;

   try
   {
      // ---------- General options ------------ //
      // Synthesis cpu time
      long total_time;
      START_TIME(total_time);
      // General options register

      // ---------- Initialization ------------ //

      // ---------- Parameter parsing ------------ //
      long cpu_time;
      START_TIME(cpu_time);
      parameters = ParameterRef(new EucalyptusParameter(argv[0], argc, argv));

      switch(parameters->Exec())
      {
         case PARAMETER_NOTPARSED:
         {
            exit_code = PARAMETER_NOTPARSED;
            throw "Bad Parameters format";
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
      STOP_TIME(cpu_time);
      if(output_level >= OUTPUT_LEVEL_MINIMUM)
      {
         parameters->PrintFullHeader(std::cerr);
      }

      /// eucalyptus does not perform a clock constrained synthesis
      if(!parameters->isOption(OPT_clock_period))
      {
         parameters->setOption(OPT_clock_period, 0.01);
      }

      // Technology library manager
      technology_managerRef TM = technology_managerRef(new technology_manager(parameters));

      /// creating the data-structure representing the target device
      generic_deviceRef device = generic_device::factory(parameters, TM);
      device->set_parameter("clock_period", parameters->getOption<double>(OPT_clock_period));
      DesignFlowManager design_flow_manager(parameters);
      const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.CGetDesignFlowGraph();

      const DesignFlowStepFactoryConstRef technology_flow_step_factory(
          new TechnologyFlowStepFactory(TM, device, design_flow_manager, parameters));
      design_flow_manager.RegisterFactory(technology_flow_step_factory);

      const auto technology_flow_signature =
          TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
      const auto technology_flow_step = design_flow_manager.GetDesignFlowStep(technology_flow_signature);
      const DesignFlowStepRef technology_design_flow_step =
          technology_flow_step != DesignFlowGraph::null_vertex() ?
              design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
              GetPointer<const TechnologyFlowStepFactory>(technology_flow_step_factory)
                  ->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
      design_flow_manager.AddStep(technology_design_flow_step);

      if(parameters->isOption(OPT_component_name))
      {
         const DesignFlowStepRef design_flow_step(new RTLCharacterization(
             device, parameters->getOption<std::string>(OPT_component_name), design_flow_manager, parameters));
         design_flow_manager.AddStep(design_flow_step);
      }
      design_flow_manager.Exec();

      STOP_TIME(total_time);
      PRINT_MSG(" ==== Total Execution Time: " + print_cpu_time(total_time) + " seconds; ====\n");

      if(!parameters->getOption<bool>(OPT_no_clean))
      {
         std::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
      }
      return EXIT_SUCCESS; // Eucalyptus tool has completed execution without errors
   }

   // exception catching
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

   switch(exit_code)
   {
      case PARAMETER_NOTPARSED:
      {
         parameters->PrintUsage(std::cout);
         break;
      }
      case EXIT_FAILURE:
      {
         parameters->PrintBugReport(std::cout);
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
