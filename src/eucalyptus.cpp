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
 * @file eucalyptus.cpp
 * @brief Tool for estimation of RTL descriptions.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Autoheader include
#include "config_HAVE_ALTERA.hpp"
#include "config_HAVE_DESIGN_COMPILER.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_LATTICE.hpp"
#include "config_HAVE_XILINX.hpp"

#include <boost/filesystem/operations.hpp>

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "design_flow_step.hpp"

/// design_flows/technology include
#include "technology_flow_step_factory.hpp"

#include "EucalyptusParameter.hpp"

#include "parse_technology.hpp"
#include "target_device.hpp"
#include "target_manager.hpp"
#include "technology_manager.hpp"

#if HAVE_XILINX || HAVE_ALTERA || HAVE_LATTICE || HAVE_DESIGN_COMPILER
#include "RTL_characterization.hpp"
#endif
#if HAVE_EXPERIMENTAL
#include "core_generation.hpp"
#endif

#include "cpu_time.hpp"
#include "utility.hpp"

/// technology include
#include "load_builtin_technology.hpp"
#include "load_default_technology.hpp"

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
            if(not(parameters->getOption<bool>(OPT_no_clean)))
            {
               boost::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
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
      STOP_TIME(cpu_time);
      if(output_level >= OUTPUT_LEVEL_MINIMUM)
         parameters->PrintFullHeader(std::cerr);

      /// eucalyptus does not perform a clock constrained synthesis
      if(!parameters->isOption(OPT_clock_period))
         parameters->setOption(OPT_clock_period, 0.0);

      // Technology library manager
      technology_managerRef TM = technology_managerRef(new technology_manager(parameters));

      /// creating the datastructure representing the target device
      const auto target_device = static_cast<TargetDevice_Type>(parameters->getOption<unsigned int>(OPT_target_device_type));
      target_deviceRef device = target_device::create_device(target_device, parameters, TM);
      device->set_parameter("clock_period", parameters->getOption<double>(OPT_clock_period));
      target_managerRef target = target_managerRef(new target_manager(parameters, TM, device));

      const DesignFlowManagerRef design_flow_manager(new DesignFlowManager(parameters));
      const DesignFlowGraphConstRef design_flow_graph = design_flow_manager->CGetDesignFlowGraph();

      const DesignFlowStepFactoryConstRef technology_flow_step_factory(new TechnologyFlowStepFactory(TM, device, design_flow_manager, parameters));
      design_flow_manager->RegisterFactory(technology_flow_step_factory);

      const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
      const vertex technology_flow_step = design_flow_manager->GetDesignFlowStep(technology_flow_signature);
      const DesignFlowStepRef technology_design_flow_step = technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step :
                                                                                   GetPointer<const TechnologyFlowStepFactory>(technology_flow_step_factory)->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
      design_flow_manager->AddStep(technology_design_flow_step);

#if HAVE_XILINX || HAVE_ALTERA || HAVE_LATTICE || HAVE_DESIGN_COMPILER
      if(parameters->isOption(OPT_component_name))
      {
         const DesignFlowStepRef design_flow_step(new RTLCharacterization(target, parameters->getOption<std::string>(OPT_component_name), design_flow_manager, parameters));
         design_flow_manager->AddStep(design_flow_step);
      }
#endif
      design_flow_manager->Exec();

#if HAVE_EXPERIMENTAL
      if(parameters->isOption(OPT_import_ip_core))
      {
         START_TIME(cpu_time);
         std::string core_hdl = parameters->getOption<std::string>(OPT_import_ip_core);
         core_generationRef core_gen = core_generationRef(new core_generation(parameters));
         core_gen->convert_to_XML(core_hdl, device->get_type());
         STOP_TIME(cpu_time);
         PRINT_OUT_MEX(DEBUG_LEVEL_MINIMUM, output_level, " ==== Core generation performed in " + print_cpu_time(cpu_time) + " seconds; ====\n");
      }

      if(parameters->isOption(OPT_export_ip_core))
      {
         START_TIME(cpu_time);
         std::string core_name = parameters->getOption<std::string>(OPT_export_ip_core);
         core_generationRef core_gen = core_generationRef(new core_generation(parameters));
         core_gen->export_core(TM, core_name);
         STOP_TIME(cpu_time);
         PRINT_OUT_MEX(DEBUG_LEVEL_MINIMUM, output_level, " ==== Core exported in " + print_cpu_time(cpu_time) + " seconds; ====\n");
      }
#endif
      STOP_TIME(total_time);
      PRINT_MSG(" ==== Total Execution Time: " + print_cpu_time(total_time) + " seconds; ====\n");

      if(not(parameters->getOption<bool>(OPT_no_clean)))
      {
         boost::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
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
      default:
      {
      }
   }
   if(parameters and not(parameters->getOption<bool>(OPT_no_clean)))
   {
      boost::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
   }
   return exit_code;
}
