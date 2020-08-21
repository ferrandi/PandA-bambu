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
 *              Copyright (c) 2018-2020 Politecnico di Milano
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
 * @file testbench_values_c_generation.cpp
 * @brief Class to compute testbench values exploiting C
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Marco Lattuada<marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "testbench_values_c_generation.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// constants include
#include "testbench_generation_constants.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// design_flows/backend/ToC/progModels include
#include "c_backend.hpp"

/// design_flows/backend/ToC include
#include "c_backend_step_factory.hpp"
#include "hls_c_backend_information.hpp"

/// HLS include
#include "hls_manager.hpp"

/// tree include
#include "behavioral_helper.hpp"

/// utility includes
#include "fileIO.hpp"
#include "utility.hpp"

/// wrapper/treegcc include
#include "gcc_wrapper.hpp"

void TestbenchValuesCGeneration::Initialize()
{
}

TestbenchValuesCGeneration::TestbenchValuesCGeneration(const ParameterConstRef _parameters, const HLS_managerRef _hls_manager, const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _hls_manager, _design_flow_manager, HLSFlowStep_Type::TESTBENCH_VALUES_C_GENERATION), output_directory(parameters->getOption<std::string>(OPT_output_directory) + "/simulation/")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   if(!boost::filesystem::exists(output_directory))
      boost::filesystem::create_directories(output_directory);
}

TestbenchValuesCGeneration::~TestbenchValuesCGeneration()
{
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> TestbenchValuesCGeneration::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::TEST_VECTOR_PARSER, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

DesignFlowStep_Status TestbenchValuesCGeneration::Exec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Executing C testbench");
   const GccWrapperConstRef gcc_wrapper(new GccWrapper(parameters, parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler), GccWrapper_OptimizationSet::O0));
   std::string compiler_flags = "-fwrapv -ffloat-store -flax-vector-conversions -msse2 -mfpmath=sse -D'__builtin_bambu_time_start()=' -D'__builtin_bambu_time_stop()=' ";
   if(!parameters->isOption(OPT_input_format) || parameters->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_C || parameters->isOption(OPT_pretty_print))
      compiler_flags += " -fexcess-precision=standard ";
   if(parameters->isOption(OPT_testbench_extra_gcc_flags))
      compiler_flags += " " + parameters->getOption<std::string>(OPT_testbench_extra_gcc_flags) + " ";
   if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
   {
      if(false
#if HAVE_I386_GCC48_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC48
#endif
#if HAVE_I386_GCC49_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC49
#endif
#if HAVE_I386_GCC5_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC5
#endif
#if HAVE_I386_GCC6_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC6
#endif
#if HAVE_I386_GCC7_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC7
#endif
      )
      {
         compiler_flags += " -g -fsanitize=address -fno-omit-frame-pointer -fno-common ";
      }
      if(false
#if HAVE_I386_GCC5_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC5
#endif
#if HAVE_I386_GCC6_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC6
#endif
#if HAVE_I386_GCC7_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC7
#endif
      )
      {
         compiler_flags += " -fsanitize=undefined -fsanitize-recover=undefined ";
      }
   }
   if(parameters->isOption(OPT_gcc_optimizations))
   {
      const auto gcc_parameters = parameters->getOption<const CustomSet<std::string>>(OPT_gcc_optimizations);
      if(gcc_parameters.find("tree-vectorize") != gcc_parameters.end())
      {
         boost::replace_all(compiler_flags, "-msse2", "");
         compiler_flags += "-m32 ";
      }
   }
   // setup source files
   std::list<std::string> file_sources;
   file_sources.push_front(output_directory + STR(STR_CST_testbench_generation_basename) + ".c");
   // add source files to interface with python golden reference, if any
   std::string exec_name = output_directory + "test";
   if(parameters->isOption(OPT_no_parse_c_python))
   {
      const auto no_parse_files = parameters->getOption<const CustomSet<std::string>>(OPT_no_parse_c_python);
      for(const auto& no_parse_file : no_parse_files)
      {
         file_sources.push_back(no_parse_file);
      }
   }
   // compute top function name and use it to setup the artificial main for cosimulation
   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top functions");
   const auto top_function_id = *(top_function_ids.begin());
   const auto top_function_name = HLSMgr->CGetFunctionBehavior(top_function_id)->CGetBehavioralHelper()->get_function_name();
#if HAVE_HLS_BUILT && HAVE_EXPERIMENTAL
   if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
   {
      /// Nothing to do
   }
   else
#endif
       if(top_function_name != "main")
   {
      if(parameters->isOption(OPT_pretty_print))
      {
         file_sources.push_back(parameters->getOption<std::string>(OPT_pretty_print));
      }
      else
      {
#if !defined(__APPLE__)
         compiler_flags += " -Wl,--allow-multiple-definition ";
#endif
         for(const auto& input_file : parameters->getOption<const CustomSet<std::string>>(OPT_input_file))
         {
            file_sources.push_back(input_file);
         }
      }
   }
   else
   {
      const std::string main_file_name = output_directory + "main_exec";
      CustomSet<std::string> main_sources;
      if(parameters->isOption(OPT_pretty_print))
      {
         main_sources.insert(parameters->getOption<std::string>(OPT_pretty_print));
      }
      else
      {
         for(const auto& input_file : parameters->getOption<const CustomSet<std::string>>(OPT_input_file))
         {
            main_sources.insert(input_file);
         }
      }
      gcc_wrapper->CreateExecutable(main_sources, main_file_name, compiler_flags);
   }
   // compile the source file to get an executable
   gcc_wrapper->CreateExecutable(file_sources, exec_name, compiler_flags);
   // set some parameters for redirection of discrepancy statistics
   std::string c_stdout_file = "";
   if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
      c_stdout_file = output_directory + "dynamic_discrepancy_stats";
   // executing the test to generate inputs and executed outputs values
   if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
   {
      if(false
#if HAVE_I386_GCC49_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC49
#endif
#if HAVE_I386_GCC5_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC5
#endif
#if HAVE_I386_GCC6_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC6
#endif
#if HAVE_I386_GCC7_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC7
#endif
      )
      {
         exec_name.insert(0, "ASAN_OPTIONS='symbolize=1:redzone=2048' ");
      }
   }
   int ret = PandaSystem(parameters, exec_name, c_stdout_file);
   if(IsError(ret))
   {
      THROW_ERROR("Error in generating the expected test results");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--Executed C testbench");
   return DesignFlowStep_Status::SUCCESS;
}

bool TestbenchValuesCGeneration::HasToBeExecuted() const
{
   return true;
}

void TestbenchValuesCGeneration::ComputeRelationships(DesignFlowStepSet& design_flow_step_set, const DesignFlowStep::RelationshipType relationship_type)
{
   HLS_step::ComputeRelationships(design_flow_step_set, relationship_type);

   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         const CBackendStepFactory* c_backend_factory = GetPointer<const CBackendStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("CBackend"));

         CBackend::Type hls_c_backend_type;
#if HAVE_HLS_BUILT
         if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
         {
            hls_c_backend_type = CBackend::CB_DISCREPANCY_ANALYSIS;
         }
         else
#endif
         {
            hls_c_backend_type = CBackend::CB_HLS;
            if(parameters->isOption(OPT_pretty_print))
            {
               const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
               const CBackendStepFactory* c_backend_step_factory = GetPointer<const CBackendStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("CBackend"));
               const std::string output_file_name = parameters->getOption<std::string>(OPT_pretty_print);
               const vertex c_backend_vertex = design_flow_manager.lock()->GetDesignFlowStep(CBackend::ComputeSignature(CBackend::CB_SEQUENTIAL));
               const DesignFlowStepRef c_backend_step =
                   c_backend_vertex ? design_flow_graph->CGetDesignFlowStepInfo(c_backend_vertex)->design_flow_step : c_backend_step_factory->CreateCBackendStep(CBackend::CB_SEQUENTIAL, output_file_name, CBackendInformationConstRef());
               design_flow_step_set.insert(c_backend_step);
            }
         }

         const DesignFlowStepRef hls_c_backend_step = c_backend_factory->CreateCBackendStep(hls_c_backend_type, output_directory + STR(STR_CST_testbench_generation_basename) + ".c",
                                                                                            CBackendInformationConstRef(new HLSCBackendInformation(output_directory + STR(STR_CST_testbench_generation_basename) + ".txt", HLSMgr)));
         design_flow_step_set.insert(hls_c_backend_step);
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
         break;
      }
   }
}
