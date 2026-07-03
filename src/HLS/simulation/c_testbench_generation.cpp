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
 *              Copyright (C) 2025-2026 Politecnico di Milano
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
 * @file testbench_generation.cpp
 * @brief Generate C testbench for the top-level kernel testing
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "c_testbench_generation.hpp"

#include "BackendWrapper.hpp"
#include "Parameter.hpp"
#include "SimulationInformation.hpp"
#include "c_backend_information.hpp"
#include "c_backend_step_factory.hpp"
#include "call_graph_manager.hpp"
#include "csim_testbench_c_writer.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_manager.hpp"
#include "fileIO.hpp"
#include "hls_manager.hpp"
#include "host_profiling.hpp"
#include "host_profiling_constants.hpp"
#include "indented_output_stream.hpp"
#include "instruction_writer.hpp"

#include "config_PACKAGE_NAME.hpp"

CTestbenchGeneration::CTestbenchGeneration(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                           const DesignFlowManager& _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::C_TESTBENCH_GENERATION),
      output_sim_directory(parameters->getOption<std::filesystem::path>(OPT_output_hls_directory) / "simulation"),
      bbp_filename(output_sim_directory / "bbp_csim.c")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

HLS_step::HLSRelationships
CTestbenchGeneration::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::TEST_VECTOR_PARSER, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

void CTestbenchGeneration::ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                                                const DesignFlowStep::RelationshipType relationship_type)
{
   HLS_step::ComputeRelationships(design_flow_step_set, relationship_type);

   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         const auto c_backend_factory = GetPointer<const CBackendStepFactory>(
             design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::C_BACKEND));

         design_flow_step_set.insert(c_backend_factory->CreateCBackendStep(
             CBackendInformationConstRef(new CBackendInformation(CBackendInformation::CB_BBP, bbp_filename))));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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

bool CTestbenchGeneration::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status CTestbenchGeneration::Exec()
{
   const auto testbench_filename = (output_sim_directory / "bambu_csim.c").string();
   HLSMgr->aux_files.push_back(bbp_filename);

   const auto csim_output = parameters->getOption<std::filesystem::path>(OPT_simulation_output);
   const auto bbp_output =
       parameters->getOption<HostProfiling_Method>(OPT_profiling_method) != HostProfiling_Method::PM_NONE ?
           parameters->getOption<std::filesystem::path>(OPT_output_temporary_directory) / STR_CST_host_profiling_data :
           std::filesystem::path();

   const IndentedOutputStreamRef indented_output_stream(new IndentedOutputStream());
   const InstructionWriterRef instruction_writer(new InstructionWriter(HLSMgr, indented_output_stream, parameters));
   CSimTestbenchCWriter tb_writer(HLSMgr, instruction_writer, indented_output_stream, csim_output, bbp_output);

   tb_writer.WriteFile(testbench_filename);

   auto res = BackendWrapper::LoadResults(parameters);
   auto outputs = res.child("application").child("outputs");
   if(!outputs)
   {
      outputs = res.child("application").append_child("outputs");
   }
   outputs.append_child("testbench").text() =
       std::filesystem::proximate(testbench_filename,
                                  parameters->getOption<std::filesystem::path>(OPT_output_directory))
           .c_str();
   BackendWrapper::StoreResults(res, parameters);

   return DesignFlowStep_Status::SUCCESS;
}
