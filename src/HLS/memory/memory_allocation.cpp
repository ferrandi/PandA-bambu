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
 * @file memory_allocation.cpp
 * @brief Base class to allocate memories in high-level synthesis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "memory_allocation.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "generic_device.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

MemoryAllocationSpecialization::MemoryAllocationSpecialization(
    const MemoryAllocation_Policy _memory_allocation_policy,
    const MemoryAllocation_ChannelsType _memory_allocation_channels_type)
    : memory_allocation_policy(_memory_allocation_policy),
      memory_allocation_channels_type(_memory_allocation_channels_type)
{
}

std::string MemoryAllocationSpecialization::GetName() const
{
   std::string ret;
   switch(memory_allocation_policy)
   {
      case MemoryAllocation_Policy::LSS:
         ret += "LSS";
         break;
      case MemoryAllocation_Policy::GSS:
         ret += "GSS";
         break;
      case MemoryAllocation_Policy::GLSS:
         ret += "GLSS";
         break;
      case MemoryAllocation_Policy::ALL_BRAM:
         ret += "ALL_BRAM";
         break;
      case MemoryAllocation_Policy::NO_BRAM:
         ret += "NO_BRAM";
         break;
      case MemoryAllocation_Policy::EXT_PIPELINED_BRAM:
         ret += "EXT_PIPELINED_BRAM";
         break;
      case MemoryAllocation_Policy::NONE:
      default:
         THROW_UNREACHABLE("");
   }
   ret += "-";
   switch(memory_allocation_channels_type)
   {
      case MemoryAllocation_ChannelsType::MEM_ACC_11:
         ret += "11";
         break;
      case MemoryAllocation_ChannelsType::MEM_ACC_N1:
         ret += "N1";
         break;
      case MemoryAllocation_ChannelsType::MEM_ACC_NN:
         ret += "NN";
         break;
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

HLSFlowStepSpecialization::context_t MemoryAllocationSpecialization::GetSignatureContext() const
{
   THROW_ASSERT(static_cast<unsigned long long>(memory_allocation_policy) < (1 << 4) &&
                    static_cast<unsigned long long>(memory_allocation_channels_type) < (1 << 4),
                "Signature clash may occurr.");
   return ComputeSignatureContext(
       MEMORY_ALLOCATION, static_cast<unsigned char>(static_cast<unsigned char>(memory_allocation_policy) << 4U) |
                              static_cast<unsigned char>(memory_allocation_channels_type));
}

memory_allocation::memory_allocation(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                     const DesignFlowManager& _design_flow_manager,
                                     const HLSFlowStep_Type _hls_flow_step_type,
                                     const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, _hls_flow_step_type,
               _hls_flow_step_specialization and
                       GetPointer<const MemoryAllocationSpecialization>(_hls_flow_step_specialization) ?
                   _hls_flow_step_specialization :
                   HLSFlowStepSpecializationConstRef(new MemoryAllocationSpecialization(
                       _parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy),
                       _parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type)))),
      last_ver_sum(0),
      memory_version(0)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

HLS_step::HLSRelationships
memory_allocation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::INITIALIZE_HLS, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::ALL_FUNCTIONS));
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_function_allocation_algorithm),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
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

void memory_allocation::setup_memory_allocation()
{
   const auto& CGM = HLSMgr->CGetCallGraphManager();
   func_list = CGM.GetReachedBodyFunctions();
   /// the analysis has to be performed only on the reachable functions
   for(const auto It : func_list)
   {
      const auto function_behavior = HLSMgr->CGetFunctionBehavior(It);
      /// add parm_decls that have to be copied
      const auto& parm_decl_copied = function_behavior->get_parm_decl_copied();
      for(const auto p : parm_decl_copied)
      {
         HLSMgr->Rmem->add_parm_decl_copied(p);
      }
      /// add parm_decls that have to be stored
      const auto& parm_decl_stored = function_behavior->get_parm_decl_stored();
      for(const auto p : parm_decl_stored)
      {
         HLSMgr->Rmem->add_parm_decl_stored(p);
      }
      /// add actual parameters that have to be loaded
      const auto& parm_decl_loaded = function_behavior->get_parm_decl_loaded();
      for(const auto p : parm_decl_loaded)
      {
         HLSMgr->Rmem->add_actual_parm_loaded(p);
      }
   }
}

void memory_allocation::finalize_memory_allocation()
{
   THROW_ASSERT(func_list.size(), "Empty list of functions to be analyzed");
   bool use_unknown_address = false;
   bool has_unaligned_accesses = false;
   auto m64P = parameters->getOption<std::string>(OPT_cc_m_env).find("-m64") != std::string::npos;
   bool assume_aligned_access_p =
       parameters->isOption(OPT_aligned_access) && parameters->getOption<bool>(OPT_aligned_access);
   const auto IRM = HLSMgr->get_ir_manager();
   for(const auto It : func_list)
   {
      const auto FB = HLSMgr->CGetFunctionBehavior(It);
      const auto BH = FB->CGetBehavioralHelper();
      const auto& function_parameters = BH->get_parameters();

      if(FB->get_dereference_unknown_addr())
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                        "---This function uses unknown addresses deref: " + BH->GetFunctionName());
      }

      use_unknown_address |= FB->get_dereference_unknown_addr();

      if(FB->get_unaligned_accesses())
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                        "---This function performs unaligned accesses: " + BH->GetFunctionName());
         if(assume_aligned_access_p)
         {
            THROW_ERROR("Option --aligned-access have been specified on a function with unaligned accesses");
         }
      }

      has_unaligned_accesses |= FB->get_unaligned_accesses();

      for(auto const parameter : function_parameters)
      {
         if(HLSMgr->Rmem->is_parm_decl_copied(parameter) && !HLSMgr->Rmem->is_parm_decl_stored(parameter))
         {
            HLSMgr->Rmem->set_implicit_memcpy(true);
         }
      }
   }

   if(HLSMgr->Rmem->has_implicit_memcpy())
   {
      const auto memcpy_function = IRM->GetFunction(MEMCPY);
      func_list.insert(memcpy_function->index);
   }

   unsigned long long maximum_bus_size = 0;
   bool use_databus_width = false;
   bool has_intern_shared_data = false;
   bool has_unaligned_mem_access = HLSMgr->Rmem->has_packed_vars();
   bool needMemoryMappedRegisters = false;
   const auto& CGM = HLSMgr->CGetCallGraphManager();
   const auto root_functions = CGM.GetRootFunctions();
   /// looking for the maximum data bus size needed
   for(auto fun_id : func_list)
   {
      const auto function_behavior = HLSMgr->CGetFunctionBehavior(fun_id);
      const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
      const auto is_interfaced = HLSMgr->hasToBeInterfaced(behavioral_helper->get_function_index());
      const auto fname = behavioral_helper->GetFunctionName();
      const auto func_arch = HLSMgr->module_arch->GetArchitecture(fname);
      if(function_behavior->get_has_globals() && parameters->isOption(OPT_expose_globals) &&
         parameters->getOption<bool>(OPT_expose_globals))
      {
         has_intern_shared_data = true;
      }
      const auto& function_parameters = behavioral_helper->get_parameters();
      for(const auto function_parameter : function_parameters)
      {
         const auto pname = behavioral_helper->PrintVariable(function_parameter);
         if(func_arch && root_functions.find(fun_id) != root_functions.end())
         {
            THROW_ASSERT(func_arch->parms.find(pname) != func_arch->parms.end(),
                         "Parameter " + pname + " not found in function " + fname);
            const auto& parm_attrs = func_arch->parms.at(pname);
            const auto& iface_attrs = func_arch->ifaces.at(parm_attrs.at(FunctionArchitecture::parm_bundle));
            const auto& iface_mode = iface_attrs.at(FunctionArchitecture::iface_mode);
            if(iface_mode != "default")
            {
               continue;
            }
         }
         if(HLSMgr->Rmem->is_parm_decl_copied(function_parameter) &&
            !HLSMgr->Rmem->is_parm_decl_stored(function_parameter))
         {
            use_databus_width = true;
            maximum_bus_size = std::max(maximum_bus_size, 8ull);
         }
         if(!use_unknown_address && is_interfaced && ir_helper::IsPointerType(IRM->GetIRNode(function_parameter)))
         {
            use_unknown_address = true;
            if(output_level > OUTPUT_LEVEL_NONE)
            {
               THROW_WARNING("This function uses unknown addresses: " + behavioral_helper->GetFunctionName());
            }
         }
      }
      if(function_behavior->has_packed_vars())
      {
         has_unaligned_mem_access = true;
      }
      const auto& parm_decl_stored = function_behavior->get_parm_decl_stored();
      for(unsigned int p : parm_decl_stored)
      {
         maximum_bus_size = std::max(maximum_bus_size, ir_helper::SizeAlloc(ir_helper::CGetType(IRM->GetIRNode(p))));
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "param with maximum_bus_size=" + STR(maximum_bus_size));
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                    "Analyzing function for bus size: " + behavioral_helper->GetFunctionName());
      const auto g = function_behavior->GetOpGraph(FunctionBehavior::CFG);
      const auto TM = HLSMgr->get_ir_manager();
      const auto fnode = TM->GetIRNode(fun_id);
      CustomUnorderedSet<OpGraph::vertex_descriptor> RW_stmts;
      if(HLSMgr->design_interface_io.find(fname) != HLSMgr->design_interface_io.end())
      {
         for(const auto& bb2arg2stmtsR : HLSMgr->design_interface_io.find(fname)->second)
         {
            for(const auto& arg2stms : bb2arg2stmtsR.second)
            {
               if(arg2stms.second.size() > 0)
               {
                  for(const auto& stmt : arg2stms.second)
                  {
                     const auto op_it = g.CGetGraphInfo().ir_node_to_operation.find(stmt);
                     if(op_it != g.CGetGraphInfo().ir_node_to_operation.end())
                     {
                        RW_stmts.insert(op_it->second);
                     }
                  }
               }
            }
         }
      }

      for(const auto v : g.vertices())
      {
         if(RW_stmts.find(v) != RW_stmts.end())
         {
            continue;
         }
         const auto& op_info = g.CGetNodeInfo(v);
         const auto current_op = op_info.GetOperation();
         const auto var_read = HLSMgr->get_required_values(fun_id, v);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing operation " + op_info.vertex_name);
         if(op_info.node_type & (TYPE_LOAD | TYPE_STORE))
         {
            const auto curr_tn = IRM->GetIRNode(op_info.GetNodeId());
            const auto me = GetPointer<const assign_stmt>(curr_tn);
            THROW_ASSERT(me, "only assign_stmt's are allowed as memory operations");
            ir_nodeRef expr;
            if(op_info.node_type | TYPE_STORE)
            {
               expr = me->op0;
            }
            else
            {
               expr = me->op1;
            }
            const auto var = ir_helper::GetBaseVariable(expr);
            if(ir_helper::is_a_misaligned_vector(IRM, expr->index))
            {
               has_unaligned_mem_access = true;
            }
            /// check for packed struct/union accesses
            if(!has_unaligned_mem_access)
            {
               has_unaligned_mem_access = ir_helper::IsPackedAccess(expr);
            }

            /// check if a global variable may be accessed from an external component
            if(!has_intern_shared_data && var && function_behavior->is_variable_mem(var->index) &&
               !HLSMgr->Rmem->is_private_memory(var->index) && parameters->isOption(OPT_expose_globals) &&
               parameters->getOption<bool>(OPT_expose_globals))
            {
               const auto vd = GetPointer<const variable_val_node>(var);
               if(vd && (((!vd->parent || vd->parent->get_kind() == module_unit_node_K) && !vd->static_flag) ||
                         CGM.ExistsAddressedFunction()))
               {
                  has_intern_shared_data =
                      true; /// an external component can access the var possibly (global and volatile vars)
               }
            }
            unsigned long long value_bitsize;
            if(op_info.node_type & TYPE_STORE)
            {
               const auto size_var = std::get<0>(var_read[0]);
               const auto size_type = ir_helper::CGetType(IRM->GetIRNode(size_var));
               value_bitsize = ir_helper::SizeAlloc(size_type);
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "store with value_bitsize=" + STR(value_bitsize));
            }
            else
            {
               const auto size_var = HLSMgr->get_produced_value(fun_id, v);
               const auto size_type = ir_helper::CGetType(IRM->GetIRNode(size_var));
               value_bitsize = ir_helper::SizeAlloc(size_type);
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "load with value_bitsize=" + STR(value_bitsize));
            }
            if(!(function_behavior->is_variable_mem(var->index) && HLSMgr->Rmem->is_private_memory(var->index)))
            {
               maximum_bus_size = std::max(maximum_bus_size, value_bitsize);
            }
            PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                          " with maximum_bus_size=" + STR(maximum_bus_size) + " " + curr_tn->ToString());
         }
         else
         {
            if(current_op == MEMCPY || current_op == MEMCMP || current_op == MEMSET)
            {
               use_databus_width = true;
               maximum_bus_size = std::max(maximum_bus_size, 8ull);
               if(assume_aligned_access_p)
               {
                  THROW_ERROR("Option --aligned-access cannot be used in presence of memcpy, memcmp or memset");
               }
            }

            for(const auto& [var, _] : var_read)
            {
               if(var && ir_helper::IsPointerType(IRM->GetIRNode(var)))
               {
                  const auto var_node = IRM->GetIRNode(var);
                  const auto type_node = ir_helper::CGetType(var_node);
                  ir_nodeRef type_node_ptd;
                  if(type_node->get_kind() == pointer_ty_node_K)
                  {
                     type_node_ptd = GetPointerS<const pointer_ty_node>(type_node)->ptd;
                  }
                  else
                  {
                     THROW_ERROR("A pointer type is expected");
                  }
                  const auto bitsize = ir_helper::AccessedMaximumBitsize(type_node_ptd, 1);
                  maximum_bus_size = std::max(maximum_bus_size, bitsize);
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                                " with maximum_bus_size=" + STR(maximum_bus_size) + " " + op_info.node->ToString());
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + op_info.vertex_name);
      }
      const auto top_functions = HLSMgr->CGetCallGraphManager().GetRootFunctions();
      const auto local_needMemoryMappedRegisters = top_functions.count(fun_id) ?
                                                       parameters->getOption<bool>(OPT_memory_mapped_top) :
                                                       HLSMgr->hasToBeInterfaced(fun_id);
      needMemoryMappedRegisters = needMemoryMappedRegisters || local_needMemoryMappedRegisters;
      if(local_needMemoryMappedRegisters)
      {
         unsigned long long addr_bus_bitsize;
         if(parameters->isOption(OPT_addr_bus_bitsize))
         {
            addr_bus_bitsize = parameters->getOption<unsigned int>(OPT_addr_bus_bitsize);
         }
         else
         {
            addr_bus_bitsize = m64P ? 64 : 32;
         }
         for(const auto& par : behavioral_helper->GetParameters())
         {
            const auto type = ir_helper::CGetType(par);
            const auto is_a_struct_union = ir_helper::IsStructType(type);
            if(is_a_struct_union)
            {
               maximum_bus_size = std::max(addr_bus_bitsize, maximum_bus_size);
            }
            else
            {
               maximum_bus_size = std::max(ir_helper::SizeAlloc(par), maximum_bus_size);
            }
         }
         const auto function_return = ir_helper::GetFunctionReturnType(fnode);
         if(function_return)
         {
            maximum_bus_size = std::max(ir_helper::SizeAlloc(function_return), maximum_bus_size);
         }
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                    "Analyzed function for bus size: " + behavioral_helper->GetFunctionName());
   }

   const auto HLS_D = HLSMgr->get_HLS_device();
   unsigned long long bram_bitsize = 0;
   unsigned addr_bus_bitsize = 0;
   const auto bram_bitsize_min =
       HLSMgr->GetParameterFromParameterOrDeviceOrDefault<unsigned int>("BRAM_bitsize_min", HLS_D, 0U);
   const auto bram_bitsize_max =
       HLSMgr->GetParameterFromParameterOrDeviceOrDefault<unsigned int>("BRAM_bitsize_max", HLS_D, 0U);
   HLSMgr->Rmem->set_maxbram_bitsize(bram_bitsize_max);

   maximum_bus_size = ceil_pow2(maximum_bus_size);

   if(has_unaligned_mem_access || has_unaligned_accesses)
   {
      if(maximum_bus_size > bram_bitsize_max)
      {
         THROW_ERROR("Unsupported data bus size. In case, try a device supporting this BRAM BITSIZE: " +
                     STR(maximum_bus_size) + " available maximum BRAM BITSIZE: " + STR(bram_bitsize_max));
      }
      else
      {
         bram_bitsize = maximum_bus_size;
      }
   }
   else if(maximum_bus_size / 2 > bram_bitsize_max)
   {
      THROW_ERROR("Unsupported data bus size. In case, try a device supporting this BRAM BITSIZE: " +
                  STR(maximum_bus_size / 2) + " available maximum BRAM BITSIZE: " + STR(bram_bitsize_max));
   }
   else
   {
      bram_bitsize = maximum_bus_size / 2;
   }

   if(bram_bitsize < bram_bitsize_min)
   {
      bram_bitsize = bram_bitsize_min;
   }

   if(bram_bitsize < 8)
   {
      bram_bitsize = 8;
   }

   if(parameters->isOption(OPT_addr_bus_bitsize))
   {
      addr_bus_bitsize = parameters->getOption<unsigned int>(OPT_addr_bus_bitsize);
   }
   else if(use_unknown_address)
   {
      addr_bus_bitsize = m64P ? 64 : 32;
   }
   else if(has_intern_shared_data && parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) !=
                                         MemoryAllocation_Policy::ALL_BRAM)
   {
      addr_bus_bitsize = m64P ? 64 : 32;
   }
   else if(HLSMgr->Rmem->get_memory_address() - HLSMgr->base_address > 0)
   {
      addr_bus_bitsize = m64P ? 64 : 32;
   }
   else
   {
      unsigned long long int addr_range = HLSMgr->Rmem->get_max_address();
      if(addr_range)
      {
         addr_range = std::max(addr_range, ((2 * static_cast<unsigned long long int>(bram_bitsize_max)) / 8));
         --addr_range;
      }
      unsigned int index;
      for(index = 1; addr_range >= (1ull << index); ++index)
      {
         ;
      }
      addr_bus_bitsize = index;
      if(HLSMgr->Rmem->count_non_private_internal_symbols() == 1)
      {
         ++addr_bus_bitsize;
      }
   }

   if(needMemoryMappedRegisters)
   {
      HLS_manager::check_bitwidth(maximum_bus_size);
      maximum_bus_size = std::max(maximum_bus_size, static_cast<unsigned long long>(addr_bus_bitsize));
   }
   HLSMgr->set_address_bitsize(addr_bus_bitsize);
   HLSMgr->Rmem->set_bus_data_bitsize(maximum_bus_size);
   HLSMgr->Rmem->set_bus_size_bitsize(std::max(4ULL, ceil_log2(maximum_bus_size + 1ULL)));

   HLSMgr->Rmem->set_bram_bitsize(bram_bitsize);
   HLSMgr->Rmem->set_intern_shared_data(has_intern_shared_data);
   HLSMgr->Rmem->set_use_unknown_addresses(use_unknown_address);
   HLSMgr->Rmem->set_unaligned_accesses(has_unaligned_accesses);

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---BRAM bitsize: " + STR(HLSMgr->Rmem->get_bram_bitsize()));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "---" + (use_databus_width ? std::string("Spec may exploit DATA bus width") :
                                               std::string("Spec may not exploit DATA bus width")));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "---" + (!use_unknown_address ?
                               std::string("All the data have a known address") :
                               std::string("Spec accesses data having an address unknown at compile time")));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "---" + (!has_intern_shared_data ? std::string("Internal data is not externally accessible") :
                                                     std::string("Internal data may be accessed")));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "---DATA bus bitsize: " + STR(HLSMgr->Rmem->get_bus_data_bitsize()));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---ADDRESS bus bitsize: " + STR(HLSMgr->get_address_bitsize()));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "---SIZE bus bitsize: " + STR(HLSMgr->Rmem->get_bus_size_bitsize()));
   if(HLSMgr->Rmem->has_all_pointers_resolved())
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---ALL pointers have been resolved");
   }
   if(has_unaligned_accesses)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Code has LOADs or STOREs with unaligned accesses");
   }
   if(HLSMgr->Rmem->get_allocated_parameters_memory())
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---Total amount of memory allocated for memory mapped parameters: " +
                         STR(HLSMgr->Rmem->get_allocated_parameters_memory()));
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "---Internally allocated memory (no private memories): " +
                      STR(HLSMgr->Rmem->get_allocated_internal_memory()));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "---Internally allocated memory: " + STR(HLSMgr->Rmem->get_allocated_space()));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
}

void memory_allocation::allocate_parameters(unsigned int functionId, const std::unique_ptr<memory>& Rmem)
{
   const auto out_lvl = Rmem == HLSMgr->Rmem ? output_level : OUTPUT_LEVEL_NONE;
   THROW_ASSERT(Rmem, "unexpected condition");
   const auto function_behavior = HLSMgr->CGetFunctionBehavior(functionId);
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
   const auto function_return = [&]() -> unsigned int {
      const auto return_type = ir_helper::GetFunctionReturnType(HLSMgr->get_ir_manager()->GetIRNode(functionId));
      return return_type ? return_type->index : 0;
   }();
   const auto is_omp = HLSMgr->isOmpLambdaFunction(functionId);

   // Allocate memory for the start register.
   const auto functionName = behavioral_helper->GetFunctionName();
   Rmem->add_parameter(functionId, functionId, functionName,
                       (behavioral_helper->get_parameters().empty() && !function_return) || is_omp);
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Function: " + functionName);
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Id: " + STR(functionId));
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                  "---Base Address: " + STR(Rmem->get_parameter_base_address(functionId, functionId)));
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                  "---Size: " + STR(compute_n_bytes(ir_helper::SizeAlloc(
                                    ir_helper::CGetType(HLSMgr->get_ir_manager()->GetIRNode(functionId))))));
   if(is_omp)
   {
      return;
   }
   // Allocate every parameter on chip.
   const auto& topParams = behavioral_helper->get_parameters();
   for(auto itr = topParams.begin(), end = topParams.end(); itr != end; ++itr)
   {
      auto itr_next = itr;
      ++itr_next;
      auto par_name = behavioral_helper->PrintVariable(*itr);
      Rmem->add_parameter(behavioral_helper->get_function_index(), *itr, par_name, !function_return && itr_next == end);
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "-->Parameter " + par_name + " of Function " + functionName);
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Id: " + STR(*itr));
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                     "---Base Address: " + STR(Rmem->get_parameter_base_address(functionId, *itr)));
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                     "---Size: " + STR(ir_helper::SizeAlloc(HLSMgr->get_ir_manager()->GetIRNode(*itr)) / 8u));
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "<--");
   }

   // Allocate the return value on chip.
   if(function_return)
   {
      Rmem->add_parameter(behavioral_helper->get_function_index(), function_return, "@return_" + functionName, true);
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "-->Return parameter for Function: " + functionName);
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Id: " + STR(function_return));
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                     "---Base Address: " + STR(Rmem->get_parameter_base_address(functionId, function_return)));
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                     "---Size: " +
                         STR(ir_helper::SizeAlloc(HLSMgr->get_ir_manager()->GetIRNode(function_return)) / 8u));
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "<--");
   }
}

bool memory_allocation::HasToBeExecuted() const
{
   if(!memory_version || memory_version != HLSMgr->GetMemVersion())
   {
      return true;
   }
   unsigned int curr_ver_sum = 0;
   const auto& CGM = HLSMgr->CGetCallGraphManager();
   for(const auto i : CGM.GetReachedBodyFunctions())
   {
      const auto FB = HLSMgr->CGetFunctionBehavior(i);
      curr_ver_sum += FB->GetBBVersion() + FB->GetBitValueVersion();
   }
   return curr_ver_sum > last_ver_sum;
}

DesignFlowStep_Status memory_allocation::Exec()
{
   const auto status = InternalExec();
   const auto& CGM = HLSMgr->CGetCallGraphManager();
   last_ver_sum = 0;
   for(const auto i : CGM.GetReachedBodyFunctions())
   {
      const auto FB = HLSMgr->CGetFunctionBehavior(i);
      last_ver_sum += FB->GetBBVersion() + FB->GetBitValueVersion();
   }
   memory_version = HLSMgr->GetMemVersion();
   return status;
}
