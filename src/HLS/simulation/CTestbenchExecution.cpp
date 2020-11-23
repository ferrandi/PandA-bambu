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
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

// include autoheaders
#include "config_HAVE_I386_CLANG10_COMPILER.hpp"
#include "config_HAVE_I386_CLANG11_COMPILER.hpp"
#include "config_HAVE_I386_CLANG4_COMPILER.hpp"
#include "config_HAVE_I386_CLANG5_COMPILER.hpp"
#include "config_HAVE_I386_CLANG6_COMPILER.hpp"
#include "config_HAVE_I386_CLANG7_COMPILER.hpp"
#include "config_HAVE_I386_CLANG8_COMPILER.hpp"
#include "config_HAVE_I386_CLANG9_COMPILER.hpp"
#include "config_HAVE_I386_GCC47_COMPILER.hpp"
#include "config_HAVE_I386_GCC48_COMPILER.hpp"
#include "config_HAVE_I386_GCC49_COMPILER.hpp"
#include "config_HAVE_I386_GCC5_COMPILER.hpp"
#include "config_HAVE_I386_GCC6_COMPILER.hpp"
#include "config_HAVE_I386_GCC7_COMPILER.hpp"
#include "config_HAVE_I386_GCC8_COMPILER.hpp"

// include class header
#include "CTestbenchExecution.hpp"

// include from ./
#include "Parameter.hpp"

// include from behavior/
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

// include from design_flows/
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

// include from design_flows/backend/ToC/progModels/
#include "c_backend.hpp"

// include from design_flows/backend/ToC/
#include "c_backend_step_factory.hpp"
#include "hls_c_backend_information.hpp"

// include from frontend_analysis/
#include "application_frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"

// include from HLS/
#include "hls_manager.hpp"

/// STD include
#include <string>

/// STL includes
#include "custom_set.hpp"
#include <list>
#include <tuple>

// include from tree/
#include "behavioral_helper.hpp"

// include from utility/
#include "fileIO.hpp"

// include from wrapper/treegcc/
#include "gcc_wrapper.hpp"

#include "string_manipulation.hpp" // for GET_CLASS

CTestbenchExecution::CTestbenchExecution(const ParameterConstRef Param, const HLS_managerRef AppM, const DesignFlowManagerConstRef _design_flow_manager, const std::string& _testbench_basename)
    : HLS_step(Param, AppM, _design_flow_manager, HLSFlowStep_Type::C_TESTBENCH_EXECUTION), output_directory(Param->getOption<std::string>(OPT_output_directory) + "/simulation/"), testbench_basename(_testbench_basename)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> CTestbenchExecution::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::TEST_VECTOR_PARSER, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::TESTBENCH_MEMORY_ALLOCATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::VCD_SIGNAL_SELECTION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         }
         break;
      }
      case INVALIDATION_RELATIONSHIP:
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
   return ret;
}

DesignFlowStep_Status CTestbenchExecution::Exec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Executing C testbench");
   // compute top function name and use it to setup the artificial main for cosimulation
   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top functions");
   const auto top_function_id = *(top_function_ids.begin());
   const auto top_function_name = HLSMgr->CGetFunctionBehavior(top_function_id)->CGetBehavioralHelper()->get_function_name();
   const GccWrapperConstRef gcc_wrapper(new GccWrapper(parameters, parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler), GccWrapper_OptimizationSet::O0));

   std::string compiler_flags = "-fwrapv -ffloat-store -flax-vector-conversions -msse2 -mfpmath=sse -D'__builtin_bambu_time_start()=' -D'__builtin_bambu_time_stop()=' ";

#if HAVE_I386_CLANG4_COMPILER
   if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG4)
      compiler_flags = "-fwrapv -flax-vector-conversions -msse2 -mfpmath=sse -D'__builtin_bambu_time_start()=' -D'__builtin_bambu_time_stop()=' ";
#endif
#if HAVE_I386_CLANG5_COMPILER
   if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG5)
      compiler_flags = "-fwrapv -flax-vector-conversions -msse2 -mfpmath=sse -D'__builtin_bambu_time_start()=' -D'__builtin_bambu_time_stop()=' ";
#endif
#if HAVE_I386_CLANG6_COMPILER
   if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG6)
      compiler_flags = "-fwrapv -flax-vector-conversions -msse2 -mfpmath=sse -D'__builtin_bambu_time_start()=' -D'__builtin_bambu_time_stop()=' ";
#endif
#if HAVE_I386_CLANG7_COMPILER
   if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG7)
      compiler_flags = "-fwrapv -flax-vector-conversions -msse2 -mfpmath=sse -D'__builtin_bambu_time_start()=' -D'__builtin_bambu_time_stop()=' ";
#endif
#if HAVE_I386_CLANG8_COMPILER
   if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG8)
      compiler_flags = "-fwrapv -flax-vector-conversions -msse2 -mfpmath=sse -D'__builtin_bambu_time_start()=' -D'__builtin_bambu_time_stop()=' ";
#endif
#if HAVE_I386_CLANG9_COMPILER
   if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG9)
      compiler_flags = "-fwrapv -flax-vector-conversions -msse2 -mfpmath=sse -D'__builtin_bambu_time_start()=' -D'__builtin_bambu_time_stop()=' ";
#endif
#if HAVE_I386_CLANG10_COMPILER
   if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG10)
      compiler_flags = "-fwrapv -flax-vector-conversions -msse2 -mfpmath=sse -D'__builtin_bambu_time_start()=' -D'__builtin_bambu_time_stop()=' ";
#endif
#if HAVE_I386_CLANG11_COMPILER
   if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG11)
      compiler_flags = "-fwrapv -flax-vector-conversions -msse2 -mfpmath=sse -D'__builtin_bambu_time_start()=' -D'__builtin_bambu_time_stop()=' ";
#endif

   if(!parameters->isOption(OPT_input_format) || parameters->getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_C || parameters->isOption(OPT_pretty_print))
#if HAVE_I386_CLANG4_COMPILER
      if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) != GccWrapper_CompilerTarget::CT_I386_CLANG4)
#endif
#if HAVE_I386_CLANG5_COMPILER
         if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) != GccWrapper_CompilerTarget::CT_I386_CLANG5)
#endif
#if HAVE_I386_CLANG6_COMPILER
            if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) != GccWrapper_CompilerTarget::CT_I386_CLANG6)
#endif
#if HAVE_I386_CLANG7_COMPILER
               if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) != GccWrapper_CompilerTarget::CT_I386_CLANG7)
#endif
#if HAVE_I386_CLANG8_COMPILER
                  if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) != GccWrapper_CompilerTarget::CT_I386_CLANG8)
#endif
#if HAVE_I386_CLANG9_COMPILER
                     if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) != GccWrapper_CompilerTarget::CT_I386_CLANG9)
#endif
#if HAVE_I386_CLANG10_COMPILER
                        if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) != GccWrapper_CompilerTarget::CT_I386_CLANG10)
#endif
#if HAVE_I386_CLANG11_COMPILER
                           if(parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) != GccWrapper_CompilerTarget::CT_I386_CLANG11)
#endif
                              compiler_flags += " -fexcess-precision=standard ";
   if(parameters->isOption(OPT_testbench_extra_gcc_flags))
   {
      compiler_flags += " " + parameters->getOption<std::string>(OPT_testbench_extra_gcc_flags) + " ";
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

   if(((parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy)) or (parameters->isOption(OPT_discrepancy_hw) and parameters->getOption<bool>(OPT_discrepancy_hw))) and
      (!parameters->isOption(OPT_discrepancy_permissive_ptrs) || !parameters->getOption<bool>(OPT_discrepancy_permissive_ptrs)))
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
#if HAVE_I386_GCC8_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC8
#endif
#if HAVE_I386_CLANG4_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG4
#endif
#if HAVE_I386_CLANG5_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG5
#endif
#if HAVE_I386_CLANG6_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG6
#endif
#if HAVE_I386_CLANG7_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG7
#endif
#if HAVE_I386_CLANG8_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG8
#endif
#if HAVE_I386_CLANG9_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG9
#endif
#if HAVE_I386_CLANG10_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG10
#endif
#if HAVE_I386_CLANG11_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG11
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
#if HAVE_I386_GCC8_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC8
#endif
#if HAVE_I386_CLANG4_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG4
#endif
#if HAVE_I386_CLANG5_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG5
#endif
#if HAVE_I386_CLANG6_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG6
#endif
#if HAVE_I386_CLANG7_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG7
#endif
#if HAVE_I386_CLANG8_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG8
#endif
#if HAVE_I386_CLANG9_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG9
#endif
#if HAVE_I386_CLANG10_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG10
#endif
#if HAVE_I386_CLANG11_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG11
#endif
      )
      {
         compiler_flags += " -fsanitize=undefined -fsanitize-recover=undefined ";
      }
   }
   // setup source files
   std::list<std::string> file_sources;
   file_sources.push_front(output_directory + testbench_basename + ".c");
   // add source files to interface with python golden reference, if any
   if(parameters->isOption(OPT_no_parse_c_python))
   {
      const auto no_parse_files = parameters->getOption<const CustomSet<std::string>>(OPT_no_parse_c_python);
      for(const auto& no_parse_file : no_parse_files)
      {
         file_sources.push_back(no_parse_file);
      }
   }
   if((parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy)) or (parameters->isOption(OPT_discrepancy_hw) and parameters->getOption<bool>(OPT_discrepancy_hw)))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---with discrepancy");
      /// Nothing to do
   }
   else if(top_function_name != "main")
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---without discrepancy and not main");
      if(parameters->isOption(OPT_pretty_print))
      {
         file_sources.push_back(parameters->getOption<std::string>(OPT_pretty_print));
      }
      else
      {
         compiler_flags += " -Dmain=_undefined_main ";
         for(const auto& input_file : parameters->getOption<const CustomSet<std::string>>(OPT_input_file))
         {
            file_sources.push_back(input_file);
         }
      }
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---without discrepancy and main");
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
      INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---create executable: " + main_file_name);
      gcc_wrapper->CreateExecutable(main_sources, main_file_name, compiler_flags);
   }

   std::string exec_name = output_directory + "test";
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---create executable: " + exec_name);
   // compile the source file to get an executable
   gcc_wrapper->CreateExecutable(file_sources, exec_name, compiler_flags);
   // set some parameters for redirection of discrepancy statistics
   std::string c_stdout_file = "";
   if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
      c_stdout_file = output_directory + "dynamic_discrepancy_stats";
   // executing the test to generate inputs and expected outputs values
   if((parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy)) or (parameters->isOption(OPT_discrepancy_hw) and parameters->getOption<bool>(OPT_discrepancy_hw)))
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
#if HAVE_I386_GCC8_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC8
#endif
#if HAVE_I386_CLANG4_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG4
#endif
#if HAVE_I386_CLANG5_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG5
#endif
#if HAVE_I386_CLANG6_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG6
#endif
#if HAVE_I386_CLANG7_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG7
#endif
#if HAVE_I386_CLANG8_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG8
#endif
#if HAVE_I386_CLANG9_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG9
#endif
#if HAVE_I386_CLANG10_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG10
#endif
#if HAVE_I386_CLANG11_COMPILER
         or parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG11
#endif
      )
      {
         exec_name.insert(0, "ASAN_OPTIONS='symbolize=1:redzone=2048' ");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---exec executable: " + exec_name);
   int ret = PandaSystem(parameters, exec_name, c_stdout_file);
   if(IsError(ret))
   {
      THROW_ERROR("Error in generating the expected test results");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--Executed C testbench");
   return DesignFlowStep_Status::SUCCESS;
}

bool CTestbenchExecution::HasToBeExecuted() const
{
   return true;
}

void CTestbenchExecution::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   HLS_step::ComputeRelationships(relationship, relationship_type);
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         const FrontendFlowStepFactory* frontend_step_factory = GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"));

         const vertex call_graph_computation_step = design_flow_manager.lock()->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(FUNCTION_ANALYSIS));

         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();

         const DesignFlowStepRef cg_design_flow_step = call_graph_computation_step ? design_flow_graph->CGetDesignFlowStepInfo(call_graph_computation_step)->design_flow_step : frontend_step_factory->CreateApplicationFrontendFlowStep(FUNCTION_ANALYSIS);

         relationship.insert(cg_design_flow_step);

         // Root function cannot be computed at the beginning so if the
         // call graph is not ready yet we exit. The relationships will
         // be computed again after the call graph computation.
         const CallGraphManagerConstRef call_graph_manager = HLSMgr->CGetCallGraphManager();
         if(boost::num_vertices(*(call_graph_manager->CGetCallGraph())) == 0)
            return;

         const CBackendStepFactory* c_backend_factory = GetPointer<const CBackendStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("CBackend"));

         const bool is_discrepancy = (parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy)) or (parameters->isOption(OPT_discrepancy_hw) and parameters->getOption<bool>(OPT_discrepancy_hw));
         CBackend::Type hls_c_backend_type = is_discrepancy ? CBackend::CB_DISCREPANCY_ANALYSIS : CBackend::CB_HLS;
         vertex hls_c_backend_step = design_flow_manager.lock()->GetDesignFlowStep(CBackend::ComputeSignature(hls_c_backend_type));

         const DesignFlowStepRef design_flow_step =
             hls_c_backend_step ? design_flow_graph->CGetDesignFlowStepInfo(hls_c_backend_step)->design_flow_step :
                                  c_backend_factory->CreateCBackendStep(hls_c_backend_type, output_directory + testbench_basename + ".c", CBackendInformationConstRef(new HLSCBackendInformation(output_directory + testbench_basename + ".txt", HLSMgr)));
         relationship.insert(design_flow_step);
      }
      case PRECEDENCE_RELATIONSHIP:
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
}
