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
 * @file memory_allocation.cpp
 * @brief Base class to allocate memories in high-level synthesis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
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
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "generic_device.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

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
      case MemoryAllocation_ChannelsType::MEM_ACC_P1N:
         ret += "P1N";
         break;
      case MemoryAllocation_ChannelsType::MEM_ACC_CS:
         ret += "CS";
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
                                     const DesignFlowManagerConstRef _design_flow_manager,
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

memory_allocation::~memory_allocation() = default;

HLS_step::HLSRelationships
memory_allocation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::INITIALIZE_HLS, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::ALL_FUNCTIONS));
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_function_allocation_algorithm),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
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

void memory_allocation::setup_memory_allocation()
{
   const auto cg_man = HLSMgr->CGetCallGraphManager();
   func_list = cg_man->GetReachedBodyFunctions();
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
   auto m64P = parameters->getOption<std::string>(OPT_gcc_m_env).find("-m64") != std::string::npos;
   bool assume_aligned_access_p =
       parameters->isOption(OPT_aligned_access) && parameters->getOption<bool>(OPT_aligned_access);
   const auto TreeM = HLSMgr->get_tree_manager();
   for(const auto It : func_list)
   {
      const auto FB = HLSMgr->CGetFunctionBehavior(It);
      const auto BH = FB->CGetBehavioralHelper();
      const auto& function_parameters = BH->get_parameters();

      if(FB->get_dereference_unknown_addr())
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                        "---This function uses unknown addresses deref: " + BH->get_function_name());
      }

      use_unknown_address |= FB->get_dereference_unknown_addr();

      if(FB->get_unaligned_accesses())
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                        "---This function performs unaligned accesses: " + BH->get_function_name());
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
      const auto memcpy_function = TreeM->GetFunction(MEMCPY);
      func_list.insert(memcpy_function->index);
   }

   unsigned long long maximum_bus_size = 0;
   bool use_databus_width = false;
   bool has_intern_shared_data = false;
   bool has_misaligned_indirect_ref = HLSMgr->Rmem->has_packed_vars();
   bool needMemoryMappedRegisters = false;
   const auto call_graph_manager = HLSMgr->CGetCallGraphManager();
   const auto root_functions = call_graph_manager->GetRootFunctions();
   /// looking for the maximum data bus size needed
   for(auto fun_id : func_list)
   {
      const auto function_behavior = HLSMgr->CGetFunctionBehavior(fun_id);
      const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
      const auto is_interfaced = HLSMgr->hasToBeInterfaced(behavioral_helper->get_function_index());
      const auto fname = behavioral_helper->GetMangledFunctionName();
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
            const auto iface_mode = iface_attrs.at(FunctionArchitecture::iface_mode);
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
         if(!use_unknown_address && is_interfaced && tree_helper::is_a_pointer(TreeM, function_parameter))
         {
            use_unknown_address = true;
            if(output_level > OUTPUT_LEVEL_NONE)
            {
               THROW_WARNING("This function uses unknown addresses: " + behavioral_helper->get_function_name());
            }
         }
      }
      if(function_behavior->has_packed_vars())
      {
         has_misaligned_indirect_ref = true;
      }
      const auto& parm_decl_stored = function_behavior->get_parm_decl_stored();
      for(unsigned int p : parm_decl_stored)
      {
         maximum_bus_size =
             std::max(maximum_bus_size, tree_helper::SizeAlloc(tree_helper::CGetType(TreeM->GetTreeNode(p))));
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "param with maximum_bus_size=" + STR(maximum_bus_size));
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                    "Analyzing function for bus size: " + behavioral_helper->get_function_name());
      const auto g = function_behavior->CGetOpGraph(FunctionBehavior::CFG);
      graph::vertex_iterator v, v_end;
      const auto TM = HLSMgr->get_tree_manager();
      const auto fnode = TM->GetTreeNode(fun_id);
      CustomUnorderedSet<vertex> RW_stmts;
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
                     const auto op_it = g->CGetOpGraphInfo()->tree_node_to_operation.find(stmt);
                     if(op_it != g->CGetOpGraphInfo()->tree_node_to_operation.end())
                     {
                        RW_stmts.insert(op_it->second);
                     }
                  }
               }
            }
         }
      }

      for(boost::tie(v, v_end) = boost::vertices(*g); v != v_end; ++v)
      {
         if(RW_stmts.find(*v) != RW_stmts.end())
         {
            continue;
         }
         const auto current_op = g->CGetOpNodeInfo(*v)->GetOperation();
         const auto var_read = HLSMgr->get_required_values(fun_id, *v);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing operation " + GET_NAME(g, *v));
         if(GET_TYPE(g, *v) & (TYPE_LOAD | TYPE_STORE))
         {
            const auto curr_tn = TreeM->GetTreeNode(g->CGetOpNodeInfo(*v)->GetNodeId());
            const auto me = GetPointer<const gimple_assign>(curr_tn);
            THROW_ASSERT(me, "only gimple_assign's are allowed as memory operations");
            tree_nodeRef expr;
            if(GET_TYPE(g, *v) | TYPE_STORE)
            {
               expr = me->op0;
            }
            else
            {
               expr = me->op1;
            }
            const auto var = tree_helper::GetBaseVariable(expr);
            if(tree_helper::is_a_misaligned_vector(TreeM, expr->index))
            {
               has_misaligned_indirect_ref = true;
            }
            /// check for packed struct/union accesses
            if(!has_misaligned_indirect_ref)
            {
               has_misaligned_indirect_ref = tree_helper::is_packed_access(TreeM, expr->index);
            }

            /// check if a global variable may be accessed from an external component
            if(!has_intern_shared_data && var && function_behavior->is_variable_mem(var->index) &&
               !HLSMgr->Rmem->is_private_memory(var->index) && parameters->isOption(OPT_expose_globals) &&
               parameters->getOption<bool>(OPT_expose_globals))
            {
               const auto vd = GetPointer<const var_decl>(var);
               if(vd && (((!vd->scpe || vd->scpe->get_kind() == translation_unit_decl_K) && !vd->static_flag) ||
                         tree_helper::IsVolatile(var) || call_graph_manager->ExistsAddressedFunction()))
               {
                  has_intern_shared_data =
                      true; /// an external component can access the var possibly (global and volatile vars)
               }
            }
            unsigned long long value_bitsize;
            if(GET_TYPE(g, *v) & TYPE_STORE)
            {
               const auto size_var = std::get<0>(var_read[0]);
               const auto size_type = tree_helper::CGetType(TreeM->GetTreeNode(size_var));
               value_bitsize = tree_helper::SizeAlloc(size_type);
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "store with value_bitsize=" + STR(value_bitsize));
            }
            else
            {
               const auto size_var = HLSMgr->get_produced_value(fun_id, *v);
               const auto size_type = tree_helper::CGetType(TreeM->GetTreeNode(size_var));
               value_bitsize = tree_helper::SizeAlloc(size_type);
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

            auto vr_it_end = var_read.end();
            for(auto vr_it = var_read.begin(); vr_it != vr_it_end; ++vr_it)
            {
               const auto var = std::get<0>(*vr_it);
               if(var && tree_helper::is_a_pointer(TreeM, var))
               {
                  const auto var_node = TreeM->GetTreeNode(var);
                  const auto type_node = tree_helper::CGetType(var_node);
                  tree_nodeRef type_node_ptd;
                  if(type_node->get_kind() == pointer_type_K)
                  {
                     type_node_ptd = GetPointerS<const pointer_type>(type_node)->ptd;
                  }
                  else if(type_node->get_kind() == reference_type_K)
                  {
                     type_node_ptd = GetPointerS<const reference_type>(type_node)->refd;
                  }
                  else
                  {
                     THROW_ERROR("A pointer type is expected");
                  }
                  const auto bitsize = tree_helper::AccessedMaximumBitsize(type_node_ptd, 1);
                  maximum_bus_size = std::max(maximum_bus_size, bitsize);
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                                " with maximum_bus_size=" + STR(maximum_bus_size) + " " +
                                    g->CGetOpNodeInfo(*v)->node->ToString());
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + GET_NAME(g, *v));
      }
      const auto top_functions = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
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
            const auto type = tree_helper::CGetType(par);
            const auto is_a_struct_union =
                tree_helper::IsStructType(type) || tree_helper::IsUnionType(type) || tree_helper::IsComplexType(type);
            if(is_a_struct_union)
            {
               maximum_bus_size = std::max(addr_bus_bitsize, maximum_bus_size);
            }
            else
            {
               maximum_bus_size = std::max(tree_helper::SizeAlloc(par), maximum_bus_size);
            }
         }
         const auto function_return = tree_helper::GetFunctionReturnType(fnode);
         if(function_return)
         {
            maximum_bus_size = std::max(tree_helper::SizeAlloc(function_return), maximum_bus_size);
         }
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                    "Analyzed function for bus size: " + behavioral_helper->get_function_name());
   }

   const auto HLS_D = HLSMgr->get_HLS_device();
   unsigned long long bram_bitsize = 0;
   unsigned addr_bus_bitsize = 0;
   const auto bram_bitsize_min = HLS_D->get_parameter<unsigned int>("BRAM_bitsize_min");
   const auto bram_bitsize_max = HLS_D->get_parameter<unsigned int>("BRAM_bitsize_max");
   HLSMgr->Rmem->set_maxbram_bitsize(bram_bitsize_max);

   maximum_bus_size = ceil_pow2(maximum_bus_size);

   if(has_misaligned_indirect_ref || has_unaligned_accesses)
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
         addr_range =
             std::max(addr_range, ((2 * HLS_D->get_parameter<unsigned long long int>("BRAM_bitsize_max")) / 8));
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

void memory_allocation::allocate_parameters(unsigned int functionId, memoryRef Rmem)
{
   const auto out_lvl = Rmem == HLSMgr->Rmem ? output_level : OUTPUT_LEVEL_NONE;
   if(!Rmem)
   {
      Rmem = HLSMgr->Rmem;
   }
   const auto function_behavior = HLSMgr->CGetFunctionBehavior(functionId);
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
   const auto function_return = behavioral_helper->GetFunctionReturnType(functionId);

   // Allocate memory for the start register.
   const auto functionName = tree_helper::name_function(HLSMgr->get_tree_manager(), functionId);
   Rmem->add_parameter(functionId, functionId, functionName,
                       behavioral_helper->get_parameters().empty() && !function_return);
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Function: " + functionName);
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Id: " + STR(functionId));
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                  "---Base Address: " + STR(Rmem->get_parameter_base_address(functionId, functionId)));
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                  "---Size: " + STR(compute_n_bytes(tree_helper::SizeAlloc(
                                    tree_helper::CGetType(HLSMgr->get_tree_manager()->GetTreeNode(functionId))))));
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
                     "---Size: " + STR(tree_helper::SizeAlloc(HLSMgr->get_tree_manager()->GetTreeNode(*itr)) / 8u));
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
                         STR(tree_helper::SizeAlloc(HLSMgr->get_tree_manager()->GetTreeNode(function_return)) / 8u));
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
   const auto CGMan = HLSMgr->CGetCallGraphManager();
   for(const auto i : CGMan->GetReachedBodyFunctions())
   {
      const auto FB = HLSMgr->CGetFunctionBehavior(i);
      curr_ver_sum += FB->GetBBVersion() + FB->GetBitValueVersion();
   }
   return curr_ver_sum > last_ver_sum;
}

DesignFlowStep_Status memory_allocation::Exec()
{
   const auto status = InternalExec();
   const auto CGMan = HLSMgr->CGetCallGraphManager();
   last_ver_sum = 0;
   for(const auto i : CGMan->GetReachedBodyFunctions())
   {
      const auto FB = HLSMgr->CGetFunctionBehavior(i);
      last_ver_sum += FB->GetBBVersion() + FB->GetBitValueVersion();
   }
   memory_version = HLSMgr->GetMemVersion();
   return status;
}
