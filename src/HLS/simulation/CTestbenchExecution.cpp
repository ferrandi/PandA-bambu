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
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */
#include "CTestbenchExecution.hpp"

#include "Parameter.hpp"
#include "application_frontend_flow_step.hpp"
#include "behavioral_helper.hpp"
#include "c_backend.hpp"
#include "c_backend_information.hpp"
#include "c_backend_step_factory.hpp"
#include "call_graph_manager.hpp"
#include "compiler_wrapper.hpp"
#include "custom_set.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "fileIO.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "string_manipulation.hpp"

#include <filesystem>
#include <list>
#include <string>
#include <tuple>

CTestbenchExecution::CTestbenchExecution(const ParameterConstRef Param, const HLS_managerRef AppM,
                                         const DesignFlowManagerConstRef _design_flow_manager,
                                         const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLS_step(Param, AppM, _design_flow_manager, HLSFlowStep_Type::C_TESTBENCH_EXECUTION),
      c_backend_info(RefcountCast<const CBackendInformation>(_hls_flow_step_specialization))
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

void CTestbenchExecution::ComputeRelationships(DesignFlowStepSet& relationship,
                                               const DesignFlowStep::RelationshipType relationship_type)
{
   HLS_step::ComputeRelationships(relationship, relationship_type);
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         const auto c_backend_factory = GetPointer<const CBackendStepFactory>(
             design_flow_manager.lock()->CGetDesignFlowStepFactory(DesignFlowStep::C_BACKEND));
         relationship.insert(c_backend_factory->CreateCBackendStep(c_backend_info));
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
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

bool CTestbenchExecution::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status CTestbenchExecution::Exec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "-->Executing C testbench");
   // compute top function name and use it to setup the artificial main for cosimulation
   const auto is_discrepancy = c_backend_info->type == CBackendInformation::CB_DISCREPANCY_ANALYSIS;
   const auto default_compiler = parameters->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
   // NOTE: starting from version 13 on it seems clang is not respecting the -fno-strict-aliasing flag generating
   // incorrect code when type punning is present
   const auto opt_set = default_compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG13 ||
                                default_compiler == CompilerWrapper_CompilerTarget::CT_I386_CLANG16 ?
                            CompilerWrapper_OptimizationSet::O0 :
                            CompilerWrapper_OptimizationSet::O2;
   const CompilerWrapperConstRef compiler_wrapper(new CompilerWrapper(parameters, default_compiler, opt_set));
   const auto is_clang = CompilerWrapper::isClangCheck(default_compiler);
   std::string compiler_flags = "-fwrapv -flax-vector-conversions -msse2 -fno-strict-aliasing "
                                "-D'__builtin_bambu_time_start()=' -D'__builtin_bambu_time_stop()=' ";

   if(parameters->isOption(OPT_tb_extra_gcc_options))
   {
      compiler_flags += parameters->getOption<std::string>(OPT_tb_extra_gcc_options) + " ";
   }

   if(parameters->isOption(OPT_gcc_optimizations))
   {
      const auto gcc_parameters = parameters->getOption<CustomSet<std::string>>(OPT_gcc_optimizations);
      if(gcc_parameters.find("tree-vectorize") != gcc_parameters.end())
      {
         boost::replace_all(compiler_flags, "-msse2", "");
         compiler_flags += "-m32 ";
      }
   }

   if(is_discrepancy && (!parameters->isOption(OPT_discrepancy_permissive_ptrs) ||
                         !parameters->getOption<bool>(OPT_discrepancy_permissive_ptrs)))
   {
      if(is_clang || CompilerWrapper::isCurrentOrNewer(default_compiler, CompilerWrapper_CompilerTarget::CT_I386_GCC49))
      {
         compiler_flags += "-g -fsanitize=address -fno-omit-frame-pointer -fno-common ";
      }
      if(is_clang || CompilerWrapper::isCurrentOrNewer(default_compiler, CompilerWrapper_CompilerTarget::CT_I386_GCC5))
      {
         compiler_flags += "-fsanitize=undefined -fsanitize-recover=undefined ";
      }
   }
   // setup source files
   std::list<std::string> file_sources = {c_backend_info->src_filename};

   auto exec_name = std::filesystem::path(c_backend_info->src_filename).replace_extension().string();
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---create executable: " + exec_name);
   // compile the source file to get an executable
   compiler_wrapper->CreateExecutable(file_sources, exec_name, compiler_flags);
   // executing the test to generate inputs and expected outputs values
   if(is_discrepancy)
   {
      if(is_clang || CompilerWrapper::isCurrentOrNewer(default_compiler, CompilerWrapper_CompilerTarget::CT_I386_GCC49))
      {
         exec_name = "ASAN_OPTIONS='symbolize=1:redzone=2048' " + exec_name;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "---exec executable: " + exec_name);
   const auto ret = PandaSystem(parameters, exec_name, false, c_backend_info->out_filename);
   if(IsError(ret))
   {
      THROW_ERROR("Error in generating the expected test results");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "<--Executed C testbench");
   return DesignFlowStep_Status::SUCCESS;
}
