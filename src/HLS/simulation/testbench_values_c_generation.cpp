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
 *              Copyright (c) 2018-2023 Politecnico di Milano
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
#include "testbench_values_c_generation.hpp"

#include "config_HAVE_EXPERIMENTAL.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "c_backend.hpp"
#include "c_backend_step_factory.hpp"
#include "call_graph_manager.hpp"
#include "compiler_wrapper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "fileIO.hpp"
#include "function_behavior.hpp"
#include "hls_c_backend_information.hpp"
#include "hls_manager.hpp"
#include "testbench_generation_constants.hpp"
#include "tree_manager.hpp"
#include "utility.hpp"

TestbenchValuesCGeneration::TestbenchValuesCGeneration(const ParameterConstRef _parameters,
                                                       const HLS_managerRef _hls_manager,
                                                       const DesignFlowManagerConstRef _design_flow_manager)
    : HLS_step(_parameters, _hls_manager, _design_flow_manager, HLSFlowStep_Type::TESTBENCH_VALUES_C_GENERATION),
      output_directory(parameters->getOption<std::string>(OPT_output_directory) + "/simulation/"),
      impl_filename(output_directory + STR(STR_CST_testbench_generation_basename) + ".c"),
      values_filename(output_directory + STR(STR_CST_testbench_generation_basename) + ".txt")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   if(!boost::filesystem::exists(output_directory))
   {
      boost::filesystem::create_directories(output_directory);
   }
}

TestbenchValuesCGeneration::~TestbenchValuesCGeneration() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
TestbenchValuesCGeneration::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::TEST_VECTOR_PARSER, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
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

void TestbenchValuesCGeneration::ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                                                      const DesignFlowStep::RelationshipType relationship_type)
{
   HLS_step::ComputeRelationships(design_flow_step_set, relationship_type);

   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         const auto* c_backend_factory =
             GetPointer<const CBackendStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("CBackend"));

         CBackend::Type hls_c_backend_type;
#if HAVE_HLS_BUILT
         if(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy))
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
               const auto* c_backend_step_factory = GetPointer<const CBackendStepFactory>(
                   design_flow_manager.lock()->CGetDesignFlowStepFactory("CBackend"));
               const auto output_file_name = parameters->getOption<std::string>(OPT_pretty_print);
               const auto c_backend_vertex =
                   design_flow_manager.lock()->GetDesignFlowStep(CBackend::ComputeSignature(CBackend::CB_SEQUENTIAL));
               const auto c_backend_step =
                   c_backend_vertex ? design_flow_graph->CGetDesignFlowStepInfo(c_backend_vertex)->design_flow_step :
                                      c_backend_step_factory->CreateCBackendStep(
                                          CBackend::CB_SEQUENTIAL, output_file_name, CBackendInformationConstRef());
               design_flow_step_set.insert(c_backend_step);
            }
         }

         const auto hls_c_backend_step = c_backend_factory->CreateCBackendStep(
             hls_c_backend_type, impl_filename,
             CBackendInformationConstRef(new HLSCBackendInformation(values_filename, HLSMgr)));
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

bool TestbenchValuesCGeneration::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status TestbenchValuesCGeneration::Exec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Executing C testbench");
   const auto default_compiler = parameters->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
   // NOTE: starting from version 13 on it seems clang is not respecting the -fno-strict-aliasing flag generating
   // incorrect code when type punning is present
   const auto opt_lvl = default_compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG13 ||
                                default_compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG16 ?
                            CompilerWrapper_OptimizationSet::O0 :
                            CompilerWrapper_OptimizationSet::O2;
   const CompilerWrapperConstRef compiler_wrapper(new CompilerWrapper(parameters, default_compiler, opt_lvl));
   std::string compiler_flags =
       "-fwrapv -ffloat-store -flax-vector-conversions -msse2 -mfpmath=sse -fno-strict-aliasing "
       "-D'__builtin_bambu_time_start()=' -D'__builtin_bambu_time_stop()=' -D__BAMBU_SIM__ ";
   if(!parameters->isOption(OPT_input_format) ||
      parameters->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_C ||
      parameters->isOption(OPT_pretty_print))
   {
      compiler_flags += "-fexcess-precision=standard ";
   }

   const auto is_clang = CompilerWrapper::isClangCheck(default_compiler);
   if(parameters->isOption(OPT_testbench_extra_gcc_flags))
   {
      compiler_flags += parameters->getOption<std::string>(OPT_testbench_extra_gcc_flags) + " ";
   }
   if((parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy)) ||
      (parameters->isOption(OPT_discrepancy_hw) && parameters->getOption<bool>(OPT_discrepancy_hw)))
   {
      if(is_clang || default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC48 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC49 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC5 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC6 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC7)
      {
         compiler_flags += "-g -fsanitize=address -fno-omit-frame-pointer -fno-common ";
      }
      if(default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC48 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC49 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC5 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC6 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC7 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC8)
      {
         compiler_flags += "-static-libasan ";
      }
      if(is_clang || default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC5 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC6 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC7)
      {
         compiler_flags += "-fsanitize=undefined -fsanitize-recover=undefined ";
      }
      if(default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC5 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC6 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC7 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC8)
      {
         compiler_flags += "-static-libubsan ";
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
   file_sources.push_front(impl_filename);
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
   const auto top_function_name =
       HLSMgr->CGetFunctionBehavior(top_function_id)->CGetBehavioralHelper()->get_function_name();
#if HAVE_HLS_BUILT && HAVE_EXPERIMENTAL
   if(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy))
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
         compiler_flags += "-Wl,--allow-multiple-definition ";
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

         if(is_clang)
         {
            compiler_flags += "-fbracket-depth=1024 ";
         }
      }
      else
      {
         for(const auto& input_file : parameters->getOption<const CustomSet<std::string>>(OPT_input_file))
         {
            main_sources.insert(input_file);
         }
      }
      compiler_wrapper->CreateExecutable(main_sources, main_file_name, compiler_flags);
   }
   // compile the source file to get an executable
   compiler_wrapper->CreateExecutable(file_sources, exec_name, compiler_flags);
   // set some parameters for redirection of discrepancy statistics
   std::string c_stdout_file = "";
   if(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy))
   {
      c_stdout_file = output_directory + "dynamic_discrepancy_stats";
   }
   // executing the test to generate inputs and executed outputs values
   if(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy))
   {
      if(default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC49 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC5 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC6 ||
         default_compiler == CompilerWrapper_CompilerTarget::CT_I386_GCC7)
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
