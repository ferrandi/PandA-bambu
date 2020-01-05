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
 * @file bambu.cpp
 * @brief High level Synthesis tool.
 *
 * Main file used to perform high-level synthesis starting from a C-based specification.
 * See \ref src_bambu for further information
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader includes
#include "config_HAVE_ACTOR_GRAPHS_BUILT.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_PRAGMA_BUILT.hpp"
#include "config_NPROFILE.hpp"

#include <boost/filesystem/operations.hpp>

///. includes
#include "BambuParameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "call_graph_manager.hpp"

/// design_flows includes
#include "design_flow.hpp"
#include "design_flow_factory.hpp"
#include "design_flow_manager.hpp"

/// design_flows/c_backend/ToC includes
#include "c_backend_step_factory.hpp"
#include "hls_c_backend_information.hpp"

#if HAVE_ACTOR_GRAPHS_BUILT
/// design_flows/codesign include
#include "actor_graph_flow_step_factory.hpp"
#endif

/// frontend_flow includes
#include "frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"

/// HLS includes
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "hls_target.hpp"

#if HAVE_FROM_AADL_ASN_BUILT
/// parser include
#include "parser_flow_step_factory.hpp"
#endif

#if HAVE_PRAGMA_BUILT
/// pragma includes
#include "pragma_manager.hpp"
#endif

/// STD includes
#include <cstdlib>
#include <iosfwd>

/// technology include
#include "technology_flow_step_factory.hpp"

/// tree include
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

/// utility include
#include "cpu_time.hpp"

/// wrapper/treegcc includes
#include "gcc_wrapper.hpp"

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
            std::string cat_args;
            for(int i = 0; i < argc; i++)
            {
               cat_args += std::string(argv[i]) + " ";
            }

            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, parameters->getOption<int>(OPT_output_level), " ==  Bambu executed with: " + cat_args + "\n");
            THROW_ERROR("Bad Parameters format");
            break;
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
      if(output_level >= OUTPUT_LEVEL_MINIMUM)
         parameters->PrintFullHeader(std::cerr);

      // Include sysdir
      if(parameters->getOption<bool>(OPT_gcc_include_sysdir))
      {
         const GccWrapperRef gcc_wrapper(new GccWrapper(parameters, GccWrapper_CompilerTarget::CT_NO_GCC, GccWrapper_OptimizationSet::O0));
         std::vector<std::string> system_includes;
         gcc_wrapper->GetSystemIncludes(system_includes);
         std::vector<std::string>::const_iterator system_include, system_include_end = system_includes.end();
         for(system_include = system_includes.begin(); system_include != system_include_end; ++system_include)
         {
            INDENT_OUT_MEX(0, 0, *system_include);
         }
         if(not(parameters->getOption<bool>(OPT_no_clean)))
         {
            boost::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
         }
         return EXIT_SUCCESS;
      }

      if(parameters->getOption<bool>(OPT_gcc_config))
      {
         const GccWrapperRef gcc_wrapper(new GccWrapper(parameters, GccWrapper_CompilerTarget::CT_NO_GCC, GccWrapper_OptimizationSet::O0));
         gcc_wrapper->GetGccConfig();
         if(not(parameters->getOption<bool>(OPT_no_clean)))
         {
            boost::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
         }
         return EXIT_SUCCESS;
      }
      if(!parameters->isOption(OPT_input_file))
      {
         PRINT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "no input files\n");
         if(not(parameters->getOption<bool>(OPT_no_clean)))
         {
            boost::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
         }
         return EXIT_SUCCESS;
      }
      STOP_TIME(cpu_time);
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Parameters parsed in " + print_cpu_time(cpu_time) + " seconds\n");

      // up to now all parameters have been parsed and data structures created, so synthesis can start

      /// ==== Creating target for the synthesis ==== ///
      HLS_targetRef HLS_T = HLS_target::create_target(parameters);

      /// ==== Creating intermediate representation ==== ///
      START_TIME(cpu_time);
      /// ==== Creating behavioral specification ==== ///
      HLS_managerRef HLSMgr = HLS_managerRef(new HLS_manager(parameters, HLS_T));
      START_TIME(HLSMgr->HLS_execution_time);
      // create the datastructures (inside application_manager) where the problem specification is contained
      const DesignFlowManagerRef design_flow_manager(new DesignFlowManager(parameters));
      const DesignFlowStepFactoryConstRef frontend_flow_step_factory(new FrontendFlowStepFactory(HLSMgr, design_flow_manager, parameters));
      design_flow_manager->RegisterFactory(frontend_flow_step_factory);
      const DesignFlowStepFactoryConstRef hls_flow_step_factory(new HLSFlowStepFactory(design_flow_manager, HLSMgr, parameters));
      design_flow_manager->RegisterFactory(hls_flow_step_factory);
      const DesignFlowStepFactoryConstRef c_backend_step_factory(new CBackendStepFactory(design_flow_manager, HLSMgr, parameters));
      design_flow_manager->RegisterFactory(c_backend_step_factory);
      const DesignFlowStepFactoryConstRef technology_flow_step_factory(new TechnologyFlowStepFactory(HLS_T->get_technology_manager(), HLS_T->get_target_device(), design_flow_manager, parameters));
      design_flow_manager->RegisterFactory(technology_flow_step_factory);
#if HAVE_FROM_AADL_ASN_BUILT
      const DesignFlowStepFactoryConstRef parser_flow_step_factory(new ParserFlowStepFactory(design_flow_manager, HLSMgr, parameters));
      design_flow_manager->RegisterFactory(parser_flow_step_factory);
#endif

      if(parameters->isOption(OPT_dry_run_evaluation) and parameters->getOption<bool>(OPT_dry_run_evaluation))
      {
         design_flow_manager->AddStep(GetPointer<const HLSFlowStepFactory>(hls_flow_step_factory)->CreateHLSFlowStep(HLSFlowStep_Type::EVALUATION, 0));
         design_flow_manager->Exec();
         return EXIT_SUCCESS;
      }

      if(parameters->getOption<bool>(OPT_find_max_cfg_transformations))
      {
         const DesignFlowStepRef find_max_cfg_transformations = GetPointer<const FrontendFlowStepFactory>(frontend_flow_step_factory)->CreateApplicationFrontendFlowStep(FrontendFlowStepType::FIND_MAX_CFG_TRANSFORMATIONS);
         design_flow_manager->AddStep(find_max_cfg_transformations);
         design_flow_manager->Exec();
         return EXIT_FAILURE;
      }
      if(parameters->isOption(OPT_test_multiple_non_deterministic_flows))
      {
         const DesignFlowStepFactoryRef design_flow_factory(new DesignFlowFactory(design_flow_manager, parameters));
         const DesignFlowStepRef non_deterministic_flows = GetPointer<const DesignFlowFactory>(design_flow_factory)->CreateDesignFlow(DesignFlow_Type::NON_DETERMINISTIC_FLOWS);
         design_flow_manager->AddStep(non_deterministic_flows);
         design_flow_manager->Exec();
         return EXIT_SUCCESS;
      }

      /// pretty printing
      if(parameters->isOption(OPT_pretty_print))
      {
         std::string outFileName = parameters->getOption<std::string>(OPT_pretty_print);
         const DesignFlowStepRef c_backend = GetPointer<const CBackendStepFactory>(c_backend_step_factory)->CreateCBackendStep(CBackend::CB_SEQUENTIAL, outFileName, CBackendInformationConstRef());
         design_flow_manager->AddStep(c_backend);
      }

#if HAVE_PRAGMA_BUILT && HAVE_EXPERIMENTAL
      if(parameters->isOption(OPT_parse_pragma) && parameters->getOption<bool>(OPT_parse_pragma))
      {
         const DesignFlowStepFactoryConstRef ag_frontend_flow_step_factory(new ActorGraphFlowStepFactory(HLSMgr, design_flow_manager, parameters));
         design_flow_manager->RegisterFactory(ag_frontend_flow_step_factory);
         DesignFlowStepSet design_flow_steps;
         CustomOrderedSet<unsigned int> input_functions = HLSMgr->get_functions_with_body();
         for(const auto input_fun_id : input_functions)
         {
            const DesignFlowStepRef design_flow_step = GetPointer<const ActorGraphFlowStepFactory>(ag_frontend_flow_step_factory)->CreateActorGraphStep(ACTOR_GRAPHS_CREATOR, input_fun_id);
            design_flow_steps.insert(design_flow_step);
         }
         design_flow_manager->AddSteps(design_flow_steps);
      }
#endif
      std::pair<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef> hls_flow_step(parameters->getOption<HLSFlowStep_Type>(OPT_synthesis_flow), HLSFlowStepSpecializationConstRef());
      design_flow_manager->AddSteps(GetPointer<const HLSFlowStepFactory>(hls_flow_step_factory)->CreateHLSFlowSteps(hls_flow_step));
      design_flow_manager->Exec();
      if(not(parameters->getOption<bool>(OPT_no_clean)))
      {
         boost::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
      }
      if(parameters->isOption(OPT_serialize_output) && parameters->isOption(OPT_output_file))
      {
         std::ofstream ofile(parameters->getOption<std::string>(OPT_output_file), std::ios::out);
         for(auto files : {HLSMgr->aux_files, HLSMgr->hdl_files})
            for(auto file : files)
            {
               std::cerr << "File name: " << file << "\n";
               std::ifstream ifile(file, std::ios::in);
               ofile << ifile.rdbuf();
            }
      }
      return EXIT_SUCCESS; // Bambu tool has completed execution without errors
   }

   // exception catching
   catch(const char* str)
   {
      if(EXIT_SUCCESS == exit_code)
         exit_code = EXIT_FAILURE;
      std::cerr << str << std::endl;
   }
   catch(const std::string& str)
   {
      if(EXIT_SUCCESS == exit_code)
         exit_code = EXIT_FAILURE;
      std::cerr << str << std::endl;
   }
   catch(std::exception& e)
   {
      std::cerr << e.what() << std::endl;
      if(EXIT_SUCCESS == exit_code)
         exit_code = EXIT_FAILURE;
   }
   catch(...)
   {
      if(EXIT_SUCCESS == exit_code)
         exit_code = EXIT_FAILURE;
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
            parameters->PrintBugReport(std::cout);
         break;
      }
      default:
      {
      }
   }
   if(parameters && not(parameters->getOption<bool>(OPT_no_clean)))
   {
      boost::filesystem::remove_all(parameters->getOption<std::string>(OPT_output_temporary_directory));
   }
   return exit_code;
}
