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
 * @brief Generate testbench for the top-level kernel testing
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "testbench_generation.hpp"

#include "Parameter.hpp"
#include "c_backend_information.hpp"
#include "c_backend_step_factory.hpp"
#include "design_flow_manager.hpp"
#include "fu_binding.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "memory.hpp"

TestbenchGeneration::TestbenchGeneration(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                         const DesignFlowManager& _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::TESTBENCH_GENERATION)
{
   composed = true;
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

HLS_step::HLSRelationships
TestbenchGeneration::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::GENERATE_HDL, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         if(parameters->getOption<std::string>(OPT_simulator) == "CSIM")
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::C_TESTBENCH_GENERATION, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::TOP_FUNCTION));
         }
         else
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::HDL_TESTBENCH_GENERATION, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::TOP_FUNCTION));
         }
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
         THROW_UNREACHABLE("");
   }
   return ret;
}

void TestbenchGeneration::ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                                               const DesignFlowStep::RelationshipType relationship_type)
{
   HLS_step::ComputeRelationships(design_flow_step_set, relationship_type);

   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         const auto c_backend_factory = GetPointer<const CBackendStepFactory>(
             design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::C_BACKEND));

         const auto sim_dir = parameters->getOption<std::filesystem::path>(OPT_output_hls_directory) / "simulation";
         design_flow_step_set.insert(c_backend_factory->CreateCBackendStep(CBackendInformationConstRef(
             new CBackendInformation(CBackendInformation::CB_HLS, sim_dir / "generated_tb.c"))));
         design_flow_step_set.insert(c_backend_factory->CreateCBackendStep(CBackendInformationConstRef(
             new CBackendInformation(CBackendInformation::CB_MDPI_WRAPPER, sim_dir / "mdpi_wrapper.cpp"))));
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

bool TestbenchGeneration::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status TestbenchGeneration::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}

std::vector<std::string> TestbenchGeneration::print_var_init(const ir_managerConstRef TM, unsigned int var,
                                                             const std::unique_ptr<memory>& mem)
{
   std::vector<std::string> init_els;
   const auto tn = TM->GetIRNode(var);
   const auto init_node = [&]() -> ir_nodeRef {
      const auto vd = GetPointer<const variable_val_node>(tn);
      if(vd && vd->init)
      {
         return vd->init;
      }
      return nullptr;
   }();

   if(init_node && (!GetPointer<const constructor_node>(init_node) ||
                    GetPointerS<const constructor_node>(init_node)->list_of_idx_valu.size()))
   {
      fu_binding::write_init(TM, tn, init_node, init_els, mem, 0);
   }
   else if(tn->get_kind() == constant_int_val_node_K || tn->get_kind() == constant_fp_val_node_K)
   {
      fu_binding::write_init(TM, tn, tn, init_els, mem, 0);
   }
   else if(!GetPointer<call_stmt>(tn))
   {
      if(ir_helper::IsArrayType(tn))
      {
         const auto type = ir_helper::CGetType(tn);
         const auto data_bitsize = ir_helper::GetArrayElementSize(type);
         const auto num_elements = ir_helper::GetArrayTotalSize(type);
         init_els.insert(init_els.end(), num_elements, std::string(data_bitsize, '0'));
      }
      else
      {
         const auto data_bitsize = ir_helper::SizeAlloc(tn);
         init_els.push_back(std::string(data_bitsize, '0'));
      }
   }
   return init_els;
}

unsigned long long TestbenchGeneration::generate_init_file(const std::string& dat_filename, const ir_managerConstRef TM,
                                                           unsigned int var, const std::unique_ptr<memory>& mem)
{
   std::stringstream init_bits;
   std::ofstream useless;
   unsigned long long vec_size = 0, elts_size = 0;
   const auto var_type = ir_helper::CGetType(TM->GetIRNode(var));
   const auto bitsize_align = GetPointer<const type_node>(var_type)->algn;
   THROW_ASSERT((bitsize_align % 8) == 0, "Alignment is not byte aligned.");
   fu_binding::fill_array_memory(init_bits, useless, var, vec_size, elts_size, mem, TM, false, bitsize_align);

   std::ofstream init_dat(dat_filename, std::ios::binary);
   while(!init_bits.eof())
   {
      std::string bitstring;
      init_bits >> bitstring;
      THROW_ASSERT(bitstring.size() % 8 == 0, "Memory word initializer is not aligned");
      size_t i;
      // Memory is little-endian, thus last byte goes in first
      for(i = bitstring.size(); i >= 8; i -= 8)
      {
         char byteval = 0;
         for(size_t k = 0; k < 8; ++k)
         {
            byteval = byteval | static_cast<char>((bitstring.at(i - k - 1U) != '0') << k);
         }
         init_dat.put(byteval);
      }
   }
   unsigned long long bytes = static_cast<unsigned long long>(init_dat.tellp());
   THROW_ASSERT((bytes % (bitsize_align / 8)) == 0, "Memory initialization bytes not aligned");
   return bytes;
}
