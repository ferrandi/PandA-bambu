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
 * @file mux_connection_binding.cpp
 * @brief Implementation of mux_connection_binding class
 *
 * Implementation of mux_connection_binding class. In this class all data-structures have been filled and
 * then datapath circuit is created.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "mux_connection_binding.hpp"

#include "Parameter.hpp"
#include "adder_conn_obj.hpp"
#include "allocation.hpp"
#include "allocation_information.hpp"
#include "behavioral_helper.hpp"
#include "commandport_obj.hpp"
#include "conn_binding.hpp"
#include "connection_obj.hpp"
#include "conv_conn_obj.hpp"
#include "cpu_time.hpp"
#include "dbgPrintHelper.hpp"
#include "direct_conn.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "funit_obj.hpp"
#include "graph.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "liveness.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "memory_symbol.hpp"
#include "multi_unbounded_obj.hpp"
#include "mux_conn.hpp"
#include "mux_obj.hpp"
#include "op_graph.hpp"
#include "parallel_memory_conn_binding.hpp"
#include "reg_binding.hpp"
#include "register_obj.hpp"
#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"
#include "storage_value_information.hpp"
#include "string_manipulation.hpp"
#include "technology_node.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "utility.hpp"

#define USE_ALIGNMENT_INFO 1
static unsigned int align_to_trimmed_bits(unsigned int algn)
{
   if(algn == 8)
   {
      return 0;
   }
   else if(algn == 16)
   {
      return 1;
   }
   else if(algn == 32)
   {
      return 2;
   }
   else if(algn == 64)
   {
      return 3;
   }
   else if(algn == 128)
   {
      return 4;
   }
   else if(algn == 256)
   {
      return 5;
   }
   else
   {
      return 0;
   }
}

mux_connection_binding::mux_connection_binding(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                               unsigned int _funId,
                                               const DesignFlowManagerConstRef _design_flow_manager)
    : conn_binding_creator(_parameters, _HLSMgr, _funId, _design_flow_manager,
                           HLSFlowStep_Type::MUX_INTERCONNECTION_BINDING),
      id(0),
      cur_phi_tree_var(0),
      is_PC(false)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

mux_connection_binding::~mux_connection_binding() = default;

void mux_connection_binding::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->Rconn = conn_bindingRef();
   regs_in.clear();
   chained_in.clear();
   module_in.clear();
   swap_computed_table.clear();
   noswap_computed_table.clear();
   id = 0;
   cur_phi_tree_var = 0;
}

/// This function is the public one to execute connection binding. First it computes virtual connection to be
/// implemented and then it allocates them based on chosen architecture
DesignFlowStep_Status mux_connection_binding::InternalExec()
{
   connCache.clear();
   long step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      START_TIME(step_time);
   }

   create_connections();
   auto mux = mux_interconnection();

   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      STOP_TIME(step_time);
   }
   if(mux)
   {
      if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "-->Connection Binding Information for function " +
                         HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name() + ":");
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---Number of allocated multiplexers (2-to-1 equivalent): " + std::to_string(mux));
      INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level,
                     "---Total number of bit-level multiplexers: " + STR(HLS->Rconn->determine_bit_level_mux()));

      if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "Time to perform interconnection binding: " + print_cpu_time(step_time) + " seconds");
      }
      else
      {
         HLS->Rconn->print();
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
      if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
      }
   }
   connCache.clear();
   HLS->Rconn->cleanInternals();
   return DesignFlowStep_Status::SUCCESS;
}

void mux_connection_binding::create_single_conn(const OpGraphConstRef data, const vertex& op, generic_objRef fu_obj_src,
                                                generic_objRef fu_obj, unsigned int port_num, unsigned int port_index,
                                                unsigned int tree_var, unsigned int precision, bool is_not_a_phi)
{
   const CustomOrderedSet<vertex>& running_states = HLS->Rliv->get_state_where_run(op);
   for(const auto state : running_states)
   {
      if(GetPointer<register_obj>(fu_obj) && !is_not_a_phi)
      {
         const StateInfoConstRef state_info = is_PC ? StateInfoConstRef() : HLS->STG->GetStg()->CGetStateInfo(state);
         if(state_info && state_info->is_duplicated && !state_info->all_paths)
         {
            bool found_branch = false;
            const tree_managerRef TreeM = HLSMgr->get_tree_manager();
            const auto gp = GetPointer<const gimple_phi>(TreeM->GetTreeNode(data->CGetOpNodeInfo(op)->GetNodeId()));
            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               auto bbID = def_edge.second;
               if(!state_info->isOriginalState && bbID != state_info->sourceBb)
               {
                  continue;
               }
               else if(state_info->isOriginalState && bbID == state_info->sourceBb)
               {
                  continue;
               }
               else if(state_info->moved_op_def_set.find(cur_phi_tree_var) != state_info->moved_op_def_set.end())
               {
                  continue;
               }
               else if(def_edge.first->index != cur_phi_tree_var)
               {
                  continue;
               }
               found_branch = true;
               break;
            }
            if(!found_branch)
            {
               continue;
            }
         }
         /// instead of tree_var we use cur_phi_tree_var when we look into the state in data structure since tree_var
         /// can be modified in the mean time
         THROW_ASSERT(HLS->Rliv->has_state_in(state, op, cur_phi_tree_var), " no state in for @" + STR(tree_var));
         const CustomOrderedSet<vertex>& states_in = HLS->Rliv->get_state_in(state, op, cur_phi_tree_var);
         for(const auto stateIn : states_in)
         {
            HLS->Rconn->add_data_transfer(fu_obj_src, fu_obj, port_num, port_index,
                                          data_transfer(tree_var, precision, stateIn, state, op));
            PRINT_DBG_MEX(
                DEBUG_LEVEL_PEDANTIC, debug_level,
                "       - add data transfer from "
                    << fu_obj_src->get_string() << " to " << fu_obj->get_string() << " port "
                    << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                    << HLS->Rliv->get_name(stateIn) + " to state " + HLS->Rliv->get_name(state) +
                           (tree_var ?
                                (" for " +
                                 HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var)) :
                                ""));
            generic_objRef enable_obj = GetPointer<register_obj>(fu_obj)->get_wr_enable();
            GetPointer<commandport_obj>(enable_obj)
                ->add_activation(commandport_obj::transition(
                    stateIn, state, commandport_obj::data_operation_pair(cur_phi_tree_var, op)));
            GetPointer<commandport_obj>(enable_obj)->set_phi_write_enable();
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                          "       - write enable for " + fu_obj->get_string() + " from state "
                              << HLS->Rliv->get_name(stateIn) + " to state " + HLS->Rliv->get_name(state));
         }
      }
      else
      {
         HLS->Rconn->add_data_transfer(fu_obj_src, fu_obj, port_num, port_index,
                                       data_transfer(tree_var, precision, state, NULL_VERTEX, op));
         PRINT_DBG_MEX(
             DEBUG_LEVEL_PEDANTIC, debug_level,
             "       - add data transfer from "
                 << fu_obj_src->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num)
                 << ":" << std::to_string(port_index) << " in state "
                 << HLS->Rliv->get_name(state) +
                        (tree_var ?
                             (" for " +
                              HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var)) :
                             ""));
      }
   }
}

unsigned int mux_connection_binding::address_precision(unsigned int precision, const vertex& op,
                                                       const OpGraphConstRef data, const tree_managerRef TreeM)
{
   auto fu_type = HLS->Rfu->get_assign(op);
   auto node_id = data->CGetOpNodeInfo(op)->GetNodeId();
   const auto node = TreeM->GetTreeNode(node_id);
   const auto gm = GetPointer<const gimple_assign>(node);
   bool right_addr_expr = false;
   if(gm && GetPointer<const addr_expr>(gm->op1))
   {
      right_addr_expr = true;
   }
   bool is_load_store = GET_TYPE(data, op) & (TYPE_LOAD | TYPE_STORE);
   if(!right_addr_expr && is_load_store && HLS->allocation_information->is_direct_access_memory_unit(fu_type))
   {
      unsigned var = HLS->allocation_information->is_memory_unit(fu_type) ?
                         HLS->allocation_information->get_memory_var(fu_type) :
                         HLS->allocation_information->get_proxy_memory_var(fu_type);
      if(var && HLSMgr->Rmem->is_private_memory(var))
      {
         unsigned long long int max_addr =
             HLSMgr->Rmem->get_base_address(var, HLS->functionId) + tree_helper::SizeAlloc(TreeM->GetTreeNode(var)) / 8;
         unsigned int address_bitsize;
         for(address_bitsize = 1; max_addr > (1ull << address_bitsize); ++address_bitsize)
         {
            ;
         }
         return address_bitsize;
      }
   }
   return precision;
}

bool mux_connection_binding::isConstantObj(unsigned int tree_index, const tree_managerRef TreeM)
{
   if(tree_index == 0)
   {
      return true;
   }
   tree_nodeRef tn = TreeM->GetTreeNode(tree_index);
   if(GetPointer<integer_cst>(tn))
   {
      return true;
   }
   else
   {
      return false;
   }
}

void mux_connection_binding::determine_connection(const vertex& op, const HLS_manager::io_binding_type& _var,
                                                  generic_objRef fu_obj, unsigned int port_num, unsigned int port_index,
                                                  const OpGraphConstRef data, unsigned int precision,
                                                  unsigned int alignment)
{
   bool is_not_a_phi = (GET_TYPE(data, op) & TYPE_PHI) == 0;
   auto tree_var = std::get<0>(_var);
   unsigned long long int constant_value = std::get<1>(_var);
   auto bus_addr_bitsize = HLSMgr->get_address_bitsize();
   bus_addr_bitsize = std::min(precision, bus_addr_bitsize);
   memory_symbolRef m_sym;
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);

   if(tree_var)
   {
      const tree_managerRef TreeM = HLSMgr->get_tree_manager();
      tree_nodeRef tn = TreeM->GetTreeNode(tree_var);
      switch(tn->get_kind())
      {
         case addr_expr_K:
         {
            auto* ae = GetPointer<addr_expr>(tn);
            auto node_id = data->CGetOpNodeInfo(op)->GetNodeId();
            const auto node = TreeM->GetTreeNode(node_id);
            auto* gm = GetPointer<const gimple_assign>(node);
            const auto type = tree_helper::CGetType(ae->op);
            if(type && GetPointer<const type_node>(type))
            {
#if USE_ALIGNMENT_INFO
               if(alignment)
               {
                  alignment = std::min(alignment, GetPointer<const type_node>(type)->algn);
               }
               else
               {
                  alignment = GetPointer<const type_node>(type)->algn;
               }
#endif
            }
            if(gm && gm->temporary_address)
            {
               const auto ref_var = tree_helper::GetBaseVariable(gm->op0);
               auto local_precision = bus_addr_bitsize;
               if(FB->is_variable_mem(ref_var->index))
               {
                  unsigned long long int max_addr = HLSMgr->Rmem->get_base_address(ref_var->index, HLS->functionId) +
                                                    tree_helper::SizeAlloc(ref_var) / 8;
                  for(local_precision = 1; max_addr > (1ull << local_precision); ++local_precision)
                  {
                     ;
                  }
               }
               determine_connection(op, HLS_manager::io_binding_type(ae->op->index, 0), fu_obj, port_num, port_index,
                                    data, local_precision, alignment);
            }
            else
            {
               determine_connection(op, HLS_manager::io_binding_type(ae->op->index, 0), fu_obj, port_num, port_index,
                                    data, precision, alignment);
            }
            return;
         }
         case mem_ref_K:
         {
            auto* mr = GetPointer<mem_ref>(tn);
            auto base_index = mr->op0->index;
            THROW_ASSERT(std::numeric_limits<long long>::min() <= tree_helper::GetConstValue(mr->op1) &&
                             tree_helper::GetConstValue(mr->op1) <= std::numeric_limits<long long>::max(),
                         "");
            auto offset = static_cast<long long>(tree_helper::GetConstValue(mr->op1));
            auto offset_index = offset ? mr->op1->index : 0;
            generic_objRef current_operand;
            auto local_precision = address_precision(precision, op, data, TreeM);
            if(offset_index)
            {
#if USE_ALIGNMENT_INFO
               const auto cost_val = offset;
               alignment = std::min(static_cast<unsigned int>(8 * (cost_val & -cost_val)), alignment);
#endif
               current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
               GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
               if(alignment)
               {
                  GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
               }
               HLS->Rconn->add_sparse_logic(current_operand);
               determine_connection(op, HLS_manager::io_binding_type(base_index, 0), current_operand, 0, 0, data,
                                    local_precision, alignment);
               determine_connection(op, HLS_manager::io_binding_type(offset_index, 0), current_operand, 1, 0, data,
                                    local_precision, alignment);

               create_single_conn(data, op, current_operand, fu_obj, port_num, port_index, 0, local_precision,
                                  is_not_a_phi);
            }
            else
            {
               determine_connection(op, HLS_manager::io_binding_type(base_index, 0), fu_obj, port_num, port_index, data,
                                    local_precision, alignment);
            }
            return;
         }
         case string_cst_K:
         case integer_cst_K:
         case real_cst_K:
         case vector_cst_K:
         case void_cst_K:
         case complex_cst_K:
         case ssa_name_K:
         case var_decl_K:
         {
            if(HLSMgr->Rmem->has_base_address(tree_var))
            {
               m_sym = HLSMgr->Rmem->get_symbol(tree_var, HLS->functionId);
               constant_value = HLSMgr->Rmem->get_base_address(tree_var, HLS->functionId);
               tree_var = 0;
               precision = bus_addr_bitsize;
            }
            // else a direct connection is considered
            break;
         }
         case function_decl_K:
         {
            m_sym = HLSMgr->Rmem->get_symbol(tree_var, tree_var);
            constant_value = HLSMgr->Rmem->get_base_address(tree_var, tree_var);
            tree_var = 0;
            precision = bus_addr_bitsize;
            break;
         }
         case indirect_ref_K:
         case realpart_expr_K:
         case imagpart_expr_K:
         case parm_decl_K:
         case view_convert_expr_K:
         case bit_field_ref_K:
         case constructor_K:
         case misaligned_indirect_ref_K:
         case abs_expr_K:
         case alignof_expr_K:
         case array_ref_K:
         case arrow_expr_K:
         case bit_not_expr_K:
         case buffer_ref_K:
         case card_expr_K:
         case cleanup_point_expr_K:
         case conj_expr_K:
         case component_ref_K:
         case convert_expr_K:
         case exit_expr_K:
         case fix_ceil_expr_K:
         case fix_floor_expr_K:
         case fix_round_expr_K:
         case fix_trunc_expr_K:
         case float_expr_K:
         case loop_expr_K:
         case lut_expr_K:
         case negate_expr_K:
         case non_lvalue_expr_K:
         case nop_expr_K:
         case reference_expr_K:
         case reinterpret_cast_expr_K:
         case sizeof_expr_K:
         case static_cast_expr_K:
         case throw_expr_K:
         case truth_not_expr_K:
         case unsave_expr_K:
         case va_arg_expr_K:
         case reduc_max_expr_K:
         case reduc_min_expr_K:
         case reduc_plus_expr_K:
         case vec_unpack_hi_expr_K:
         case vec_unpack_lo_expr_K:
         case vec_unpack_float_hi_expr_K:
         case vec_unpack_float_lo_expr_K:
         case assert_expr_K:
         case bit_and_expr_K:
         case bit_ior_expr_K:
         case bit_xor_expr_K:
         case catch_expr_K:
         case ceil_div_expr_K:
         case ceil_mod_expr_K:
         case complex_expr_K:
         case compound_expr_K:
         case eh_filter_expr_K:
         case eq_expr_K:
         case exact_div_expr_K:
         case fdesc_expr_K:
         case floor_div_expr_K:
         case floor_mod_expr_K:
         case ge_expr_K:
         case gt_expr_K:
         case goto_subroutine_K:
         case in_expr_K:
         case init_expr_K:
         case le_expr_K:
         case lrotate_expr_K:
         case lshift_expr_K:
         case lt_expr_K:
         case max_expr_K:
         case min_expr_K:
         case minus_expr_K:
         case modify_expr_K:
         case mult_expr_K:
         case mult_highpart_expr_K:
         case ne_expr_K:
         case ordered_expr_K:
         case paren_expr_K:
         case plus_expr_K:
         case pointer_plus_expr_K:
         case postdecrement_expr_K:
         case postincrement_expr_K:
         case predecrement_expr_K:
         case preincrement_expr_K:
         case range_expr_K:
         case rdiv_expr_K:
         case round_div_expr_K:
         case round_mod_expr_K:
         case rrotate_expr_K:
         case rshift_expr_K:
         case set_le_expr_K:
         case target_mem_ref_K:
         case target_mem_ref461_K:
         case trunc_div_expr_K:
         case trunc_mod_expr_K:
         case truth_and_expr_K:
         case truth_andif_expr_K:
         case truth_or_expr_K:
         case truth_orif_expr_K:
         case truth_xor_expr_K:
         case try_catch_expr_K:
         case try_finally_K:
         case uneq_expr_K:
         case ltgt_expr_K:
         case unge_expr_K:
         case ungt_expr_K:
         case unle_expr_K:
         case unlt_expr_K:
         case unordered_expr_K:
         case widen_sum_expr_K:
         case widen_mult_expr_K:
         case with_size_expr_K:
         case vec_lshift_expr_K:
         case vec_rshift_expr_K:
         case widen_mult_hi_expr_K:
         case widen_mult_lo_expr_K:
         case vec_cond_expr_K:
         case vec_pack_trunc_expr_K:
         case vec_pack_sat_expr_K:
         case vec_pack_fix_trunc_expr_K:
         case vec_perm_expr_K:
         case vec_extracteven_expr_K:
         case vec_extractodd_expr_K:
         case vec_interleavehigh_expr_K:
         case vec_interleavelow_expr_K:
         case CASE_CPP_NODES:
         case const_decl_K:
         case field_decl_K:
         case label_decl_K:
         case namespace_decl_K:
         case result_decl_K:
         case translation_unit_decl_K:
         case error_mark_K:
         case using_decl_K:
         case type_decl_K:
         case template_decl_K:
         case CASE_GIMPLE_NODES:
         case call_expr_K:
         case aggr_init_expr_K:
         case case_label_expr_K:
         case CASE_FAKE_NODES:
         case CASE_PRAGMA_NODES:
         case binfo_K:
         case block_K:
         case identifier_node_K:
         case statement_list_K:
         case tree_list_K:
         case tree_vec_K:
         case array_range_ref_K:
         case target_expr_K:
         case cond_expr_K:
         case dot_prod_expr_K:
         case ternary_plus_expr_K:
         case ternary_pm_expr_K:
         case ternary_mp_expr_K:
         case ternary_mm_expr_K:
         case fshl_expr_K:
         case fshr_expr_K:
         case bit_ior_concat_expr_K:
         case obj_type_ref_K:
         case save_expr_K:
         case vtable_ref_K:
         case with_cleanup_expr_K:
         case extract_bit_expr_K:
         case sat_plus_expr_K:
         case sat_minus_expr_K:
         case extractvalue_expr_K:
         case insertvalue_expr_K:
         case extractelement_expr_K:
         case insertelement_expr_K:
         case frem_expr_K:
         case CASE_TYPE_NODES:
         default:
            THROW_ERROR("determine_connection pattern not supported: " + tn->get_kind_text() + " @" + STR(tree_var));
      }
   }
   if(tree_var == 0)
   {
      /// create connection with the constant
      THROW_ASSERT(precision, "a precision greater than 0 is expected");
      auto string_value = convert_to_binary(constant_value, precision);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - Constant value: " + STR(constant_value));
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "         - " + string_value);
      std::string param_name;
      if(m_sym)
      {
         param_name = m_sym->get_symbol_name();
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "         - param: " + param_name);
         string_value = STR(m_sym->get_address());
      }
      const auto C_obj = HLS->Rconn->get_constant_obj(string_value, param_name, precision);
      create_single_conn(data, op, C_obj, fu_obj, port_num, port_index, 0, precision, is_not_a_phi);
      return;
   }
   const auto BH = FB->CGetBehavioralHelper();
   if(BH->is_a_constant(tree_var))
   {
      THROW_ASSERT(precision, "a precision greater than 0 is expected: " + STR(precision));
      const auto C_value = HLSMgr->get_constant_string(tree_var, precision);
      const auto C_obj = HLS->Rconn->get_constant_obj(C_value, "", precision);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - Tree constant value: " + BH->PrintVariable(tree_var));
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "         - " + C_value);
      create_single_conn(data, op, C_obj, fu_obj, port_num, port_index, tree_var, precision, is_not_a_phi);
      return;
   }
   connect_to_registers(op, data, fu_obj, port_num, port_index, tree_var, precision, is_not_a_phi);
}

unsigned int mux_connection_binding::extract_parm_decl(unsigned int tree_var, const tree_managerRef TreeM)
{
   unsigned int base_index;
   tree_nodeRef node = TreeM->GetTreeNode(tree_var);
   if(GetPointer<parm_decl>(node))
   {
      base_index = tree_var;
   }
   else
   {
      auto* sn = GetPointer<ssa_name>(node);
      base_index = sn->var->index;
   }
   return base_index;
}

void mux_connection_binding::connect_to_registers(vertex op, const OpGraphConstRef data, generic_objRef fu_obj,
                                                  unsigned int port_num, unsigned int port_index, unsigned int tree_var,
                                                  unsigned long long precision, const bool is_not_a_phi)
{
   THROW_ASSERT(tree_var, "a non-null tree var is expected");
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   const CustomOrderedSet<vertex>& running_states = HLS->Rliv->get_state_where_run(op);
   last_intermediate_state fetch_previous(HLS->STG->GetStg(),
                                          HLSMgr->CGetFunctionBehavior(funId)->is_simple_pipeline());
   next_unique_state get_next(HLS->STG->GetStg());
   for(const auto state : running_states)
   {
      unsigned int tree_var_state_in;
      if(!is_not_a_phi)
      {
         THROW_ASSERT(not HLSMgr->GetFunctionBehavior(HLS->functionId)->is_simple_pipeline(),
                      "A pipelined function should not contain any phi operations");
         const StateInfoConstRef state_info = is_PC ? StateInfoConstRef() : HLS->STG->GetStg()->CGetStateInfo(state);
         if(state_info && state_info->is_duplicated && !state_info->all_paths)
         {
            bool found_branch = false;
            const auto gp = GetPointer<const gimple_phi>(TreeM->GetTreeNode(data->CGetOpNodeInfo(op)->GetNodeId()));
            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               auto bbID = def_edge.second;
               if(!state_info->isOriginalState && bbID != state_info->sourceBb)
               {
                  continue;
               }
               else if(state_info->isOriginalState && bbID == state_info->sourceBb)
               {
                  continue;
               }
               else if(state_info->moved_op_def_set.find(cur_phi_tree_var) != state_info->moved_op_def_set.end())
               {
                  continue;
               }
               else if(state_info->moved_op_use_set.find(gp->res->index) != state_info->moved_op_use_set.end())
               {
                  continue;
               }
               else if(def_edge.first->index != cur_phi_tree_var)
               {
                  continue;
               }
               found_branch = true;
               break;
            }
            if(!found_branch)
            {
               continue;
            }
         }
         tree_var_state_in = cur_phi_tree_var;
      }
      else
      {
         tree_var_state_in = tree_var;
      }
      THROW_ASSERT(HLS->Rliv->has_state_in(state, op, tree_var_state_in),
                   " no state in for @" + STR(tree_var_state_in) + " - " + (is_not_a_phi ? " not a phi" : "phi") +
                       " - " + HLS->Rliv->get_name(state) + " for op " + GET_NAME(data, op));
      const CustomOrderedSet<vertex>& states_in = HLS->Rliv->get_state_in(state, op, tree_var_state_in);
      for(const auto stateIn : states_in)
      {
         generic_objRef reg_obj;

         if(!is_not_a_phi)
         {
            THROW_ASSERT(not HLSMgr->GetFunctionBehavior(HLS->functionId)->is_simple_pipeline(),
                         "A pipelined function should not contain any phi operations");
            vertex srcState = stateIn;
            vertex lstate = state;
            if(srcState == NULL_VERTEX)
            {
               std::swap(srcState, lstate);
            }
            if(tree_helper::is_parameter(TreeM, tree_var))
            {
               auto base_index = extract_parm_decl(tree_var, TreeM);
               const generic_objRef fu_src_obj = input_ports[base_index];
               THROW_ASSERT(fu_src_obj, "unexpected condition");
               HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index,
                                             data_transfer(tree_var, precision, srcState, lstate, op));
               PRINT_DBG_MEX(
                   DEBUG_LEVEL_PEDANTIC, debug_level,
                   "       - add data transfer from primary input "
                       << fu_src_obj->get_string() << " to " << fu_obj->get_string() << " port "
                       << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                       << HLS->Rliv->get_name(srcState) + " to state " + HLS->Rliv->get_name(lstate) + " for " +
                              HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
            }
            else
            {
               vertex def_op = HLS->Rliv->get_op_where_defined(tree_var);
               const CustomOrderedSet<vertex>& def_op_ending_states = HLS->Rliv->get_state_where_end(def_op);
               if((GET_TYPE(data, def_op) & TYPE_PHI) == 0)
               {
                  if(def_op_ending_states.find(srcState) != def_op_ending_states.end())
                  {
                     const generic_objRef fu_src_obj = HLS->Rfu->get(def_op);
                     HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index,
                                                   data_transfer(tree_var, precision, srcState, lstate, op));
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "       - add data transfer from "
                             << fu_src_obj->get_string() << " to " << fu_obj->get_string() << " port "
                             << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                             << HLS->Rliv->get_name(srcState) + " to state " + HLS->Rliv->get_name(lstate) + " for " +
                                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                        tree_var));
                  }
                  else if(HLS->storage_value_information->is_a_storage_value(fetch_previous(srcState, lstate),
                                                                             tree_var))
                  {
                     auto storage_value = HLS->storage_value_information->get_storage_value_index(
                         fetch_previous(srcState, lstate), tree_var);
                     auto r_index = HLS->Rreg->get_register(storage_value);
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "       - register: "
                             << r_index << " from "
                             << HLS->Rliv->get_name(srcState) + " to state " + HLS->Rliv->get_name(lstate) + " for " +
                                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                        tree_var));
                     reg_obj = HLS->Rreg->get(r_index);
                     if(reg_obj != fu_obj)
                     {
                        HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index,
                                                      data_transfer(tree_var, precision, srcState, lstate, op));
                        PRINT_DBG_MEX(
                            DEBUG_LEVEL_PEDANTIC, debug_level,
                            "       - add data transfer from "
                                << reg_obj->get_string() << " to " << fu_obj->get_string() << " port "
                                << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                << HLS->Rliv->get_name(srcState) + " to state " + HLS->Rliv->get_name(lstate) +
                                       " for " +
                                       HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                           tree_var));
                     }
                  }
                  else
                  {
                     THROW_ERROR("not expected from " + HLS->Rliv->get_name(srcState) + " to " +
                                 HLS->Rliv->get_name(lstate) + " " +
                                 HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                  }
               }
               else
               {
                  THROW_ASSERT(
                      HLS->storage_value_information->is_a_storage_value(fetch_previous(srcState, lstate), tree_var),
                      "it has to be a register");
                  auto storage_value = HLS->storage_value_information->get_storage_value_index(
                      fetch_previous(srcState, lstate), tree_var);
                  auto r_index = HLS->Rreg->get_register(storage_value);
                  PRINT_DBG_MEX(
                      DEBUG_LEVEL_PEDANTIC, debug_level,
                      "       - register: "
                          << r_index << " from "
                          << HLS->Rliv->get_name(srcState) + " to state " + HLS->Rliv->get_name(lstate) + " for " +
                                 HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                  reg_obj = HLS->Rreg->get(r_index);
                  if(reg_obj != fu_obj)
                  {
                     HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index,
                                                   data_transfer(tree_var, precision, srcState, lstate, op));
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "       - add data transfer from "
                             << reg_obj->get_string() << " to " << fu_obj->get_string() << " port "
                             << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                             << HLS->Rliv->get_name(srcState) + " to state " + HLS->Rliv->get_name(lstate) + " for " +
                                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                        tree_var));
                  }
               }
            }
            if(GetPointer<register_obj>(fu_obj) && reg_obj != fu_obj)
            {
               generic_objRef enable_obj = GetPointer<register_obj>(fu_obj)->get_wr_enable();
               GetPointer<commandport_obj>(enable_obj)
                   ->add_activation(
                       commandport_obj::transition(fetch_previous(srcState, lstate), lstate,
                                                   commandport_obj::data_operation_pair(tree_var_state_in, op)));
               GetPointer<commandport_obj>(enable_obj)->set_phi_write_enable();
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - write enable for " + fu_obj->get_string() + " from state "
                                 << HLS->Rliv->get_name(srcState) + " to state " + HLS->Rliv->get_name(lstate));
            }
         }
         else
         {
            vertex tgt_state = stateIn;
            if(tree_helper::is_parameter(TreeM, tree_var) &&
               not HLSMgr->GetFunctionBehavior(HLS->functionId)->is_simple_pipeline())
            {
               auto base_index = extract_parm_decl(tree_var, TreeM);
               const generic_objRef fu_src_obj = input_ports[base_index];
               THROW_ASSERT(fu_src_obj, "unexpected condition");
               HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index,
                                             data_transfer(tree_var, precision, state, tgt_state, op));
               PRINT_DBG_MEX(
                   DEBUG_LEVEL_PEDANTIC, debug_level,
                   "       - add data transfer from primary input "
                       << fu_src_obj->get_string() << " to " << fu_obj->get_string() << " port "
                       << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                       << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " +
                              HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
            }
            else if(tree_helper::is_parameter(TreeM, tree_var))
            {
               // Primary inputs need a dedicated register per each state if we
               // want to preserve their values along pipeline steps
               unsigned int storage_value;
               unsigned int r_index;
               auto tgt_port = port_num;
               auto tgt_index = port_index;
               vertex previous = fetch_previous(HLS->STG->get_entry_state(), tgt_state);
               generic_objRef tgt_obj = fu_obj;
               if(tgt_state != get_next(HLS->STG->get_entry_state()))
               {
                  while(previous != get_next(HLS->STG->get_entry_state()))
                  {
                     THROW_ASSERT(HLS->storage_value_information->is_a_storage_value(previous, tree_var),
                                  "The chain of registers propagating a primary input is broken");
                     storage_value = HLS->storage_value_information->get_storage_value_index(previous, tree_var);
                     r_index = HLS->Rreg->get_register(storage_value);
                     reg_obj = HLS->Rreg->get(r_index);
                     HLS->Rconn->add_data_transfer(reg_obj, tgt_obj, tgt_port, tgt_index,
                                                   data_transfer(tree_var, precision, previous, tgt_state, op));
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "       - add data transfer from "
                             << reg_obj->get_string() << " to " << tgt_obj->get_string() << " port "
                             << std::to_string(tgt_port) << ":" << std::to_string(tgt_index) << " from state "
                             << HLS->Rliv->get_name(previous) + " to state " + HLS->Rliv->get_name(tgt_state) +
                                    " for " +
                                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                        tree_var));
                     THROW_ASSERT(reg_obj != tgt_obj, "There is a loop in the propagation chain");
                     tgt_state = previous;
                     previous = fetch_previous(HLS->STG->get_entry_state(), tgt_state);
                     tgt_obj = reg_obj;
                     tgt_port = 0;
                     tgt_index = 0;
                  }
               }
               auto base_index = extract_parm_decl(tree_var, TreeM);
               const generic_objRef fu_src_obj = input_ports[base_index];
               THROW_ASSERT(fu_src_obj, "unexpected condition");
               HLS->Rconn->add_data_transfer(fu_src_obj, tgt_obj, tgt_port, port_index,
                                             data_transfer(tree_var, precision, previous, tgt_state, op));
               PRINT_DBG_MEX(
                   DEBUG_LEVEL_PEDANTIC, debug_level,
                   "       - add data transfer from primary input "
                       << fu_src_obj->get_string() << " to " << tgt_obj->get_string() << " port "
                       << std::to_string(tgt_port) << ":" << std::to_string(port_index) << " from state "
                       << HLS->Rliv->get_name(previous) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " +
                              HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
            }
            else
            {
               vertex def_op = HLS->Rliv->get_op_where_defined(tree_var);
               const CustomOrderedSet<vertex>& def_op_ending_states = HLS->Rliv->get_state_where_end(def_op);
               const StateInfoConstRef state_info =
                   is_PC ? StateInfoConstRef() : HLS->STG->GetStg()->CGetStateInfo(state);
               if((GET_TYPE(data, def_op) & TYPE_PHI) == 0)
               {
                  if(def_op_ending_states.find(state) != def_op_ending_states.end())
                  {
                     const generic_objRef fu_src_obj = HLS->Rfu->get(def_op);
                     HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index,
                                                   data_transfer(tree_var, precision, state, tgt_state, op));
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "       - add data transfer from "
                             << fu_src_obj->get_string() << " to " << fu_obj->get_string() << " port "
                             << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                             << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " +
                                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                        tree_var));
                  }
                  else if(HLS->storage_value_information->is_a_storage_value(fetch_previous(state, tgt_state),
                                                                             tree_var))
                  {
                     auto storage_value = HLS->storage_value_information->get_storage_value_index(
                         fetch_previous(state, tgt_state), tree_var);
                     auto r_index = HLS->Rreg->get_register(storage_value);
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "       - register: "
                             << r_index << " from "
                             << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " +
                                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                        tree_var));
                     reg_obj = HLS->Rreg->get(r_index);
                     THROW_ASSERT(
                         not(reg_obj == fu_obj && HLSMgr->GetFunctionBehavior(HLS->functionId)->is_simple_pipeline()),
                         "There can be no direct forwarding in pipelining");
                     if(reg_obj != fu_obj)
                     {
                        HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index,
                                                      data_transfer(tree_var, precision, state, tgt_state, op));
                        PRINT_DBG_MEX(
                            DEBUG_LEVEL_PEDANTIC, debug_level,
                            "       - add data transfer from "
                                << reg_obj->get_string() << " to " << fu_obj->get_string() << " port "
                                << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) +
                                       " for " +
                                       HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                           tree_var));
                     }
                  }
                  else
                  {
                     THROW_UNREACHABLE("not expected from " + HLS->Rliv->get_name(state) + " to " +
                                       HLS->Rliv->get_name(tgt_state) + " " +
                                       HLSMgr->get_tree_manager()->GetTreeNode(tree_var)->ToString());
                  }
               }
               else if(state_info && state_info->is_duplicated && state_info->clonedState != NULL_VERTEX &&
                       !state_info->all_paths && def_op_ending_states.find(state) != def_op_ending_states.end() &&
                       std::find(state_info->moved_exec_op.begin(), state_info->moved_exec_op.end(), op) ==
                           state_info->moved_exec_op.end())
               {
                  const auto gp =
                      GetPointer<const gimple_phi>(TreeM->GetTreeNode(data->CGetOpNodeInfo(def_op)->GetNodeId()));
                  bool phi_postponed = false;
                  unsigned int tree_temp = 0;
                  for(const auto& def_edge : gp->CGetDefEdgesList())
                  {
                     auto bbID = def_edge.second;
                     tree_temp = def_edge.first->index;
                     if(bbID != state_info->sourceBb)
                     {
                        continue;
                     }
                     else if(state_info->moved_op_def_set.find(tree_temp) != state_info->moved_op_def_set.end())
                     {
                        phi_postponed = true;
                        break;
                     }
                     else if(state_info->moved_op_use_set.find(tree_var) != state_info->moved_op_use_set.end())
                     {
                        phi_postponed = true;
                        break;
                     }
                     else
                     {
                        break;
                     }
                  }
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                "Is phi postponed? " + (phi_postponed ? std::string("YES") : std::string("NO")));
                  if(phi_postponed)
                  {
                     // std::cerr << "phi postponed 0" << std::endl;
                     generic_objRef fu_src_obj;
                     if(state_info->moved_op_use_set.find(tree_var) != state_info->moved_op_use_set.end() &&
                        state_info->moved_op_def_set.find(tree_temp) == state_info->moved_op_def_set.end())
                     {
                        auto src_storage_value = HLS->storage_value_information->get_storage_value_index(
                            fetch_previous(state, tgt_state), tree_temp);
                        auto src_r_index = HLS->Rreg->get_register(src_storage_value);
                        fu_src_obj = HLS->Rreg->get(src_r_index);
                     }
                     else
                     {
                        vertex src_def_op = HLS->Rliv->get_op_where_defined(tree_temp);
                        fu_src_obj = HLS->Rfu->get(src_def_op);
                     }
                     HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index,
                                                   data_transfer(tree_temp, precision, state, tgt_state, op));
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "       - add data transfer from "
                             << fu_src_obj->get_string() << " to " << fu_obj->get_string() << " port "
                             << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                             << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " +
                                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                        tree_temp));
                  }
                  else
                  {
                     THROW_ASSERT(
                         HLS->storage_value_information->is_a_storage_value(fetch_previous(state, tgt_state), tree_var),
                         "it has to be a register");
                     auto storage_value = HLS->storage_value_information->get_storage_value_index(
                         fetch_previous(state, tgt_state), tree_var);
                     auto r_index = HLS->Rreg->get_register(storage_value);
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "       - register: "
                             << r_index << " from "
                             << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " +
                                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                        tree_var));
                     reg_obj = HLS->Rreg->get(r_index);
                     if(reg_obj != fu_obj)
                     {
                        HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index,
                                                      data_transfer(tree_var, precision, state, tgt_state, op));
                        PRINT_DBG_MEX(
                            DEBUG_LEVEL_PEDANTIC, debug_level,
                            "       - add data transfer from "
                                << reg_obj->get_string() << " to " << fu_obj->get_string() << " port "
                                << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) +
                                       " for " +
                                       HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                           tree_var));
                     }
                  }
               }
               else
               {
                  THROW_ASSERT(
                      HLS->storage_value_information->is_a_storage_value(fetch_previous(state, tgt_state), tree_var),
                      "it has to be a register");
                  auto storage_value = HLS->storage_value_information->get_storage_value_index(
                      fetch_previous(state, tgt_state), tree_var);
                  auto r_index = HLS->Rreg->get_register(storage_value);
                  PRINT_DBG_MEX(
                      DEBUG_LEVEL_PEDANTIC, debug_level,
                      "       - register: "
                          << r_index << " from "
                          << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " +
                                 HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                  reg_obj = HLS->Rreg->get(r_index);
                  if(reg_obj != fu_obj)
                  {
                     HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index,
                                                   data_transfer(tree_var, precision, state, tgt_state, op));
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "       - add data transfer from "
                             << reg_obj->get_string() << " to " << fu_obj->get_string() << " port "
                             << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                             << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " +
                                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                        tree_var));
                  }
               }
            }
         }
      }
      if(HLSMgr->CGetFunctionBehavior(funId)->is_simple_pipeline())
      {
         vertex target;
         vertex previous;
         vertex def_op;
         vertex top;
         unsigned int origin_idx;
         unsigned int target_idx;
         unsigned int origin_reg_idx;
         unsigned int target_reg_idx;
         generic_objRef origin_reg;
         generic_objRef target_reg;
         CustomOrderedSet<unsigned int> in_vars = HLS->Rliv->get_live_in(state);
         for(auto& var : in_vars)
         {
            target = state;
            previous = fetch_previous(HLS->STG->get_entry_state(), target);
            def_op = HLS->Rliv->get_op_where_defined(var);
            THROW_ASSERT(HLS->Rliv->get_state_where_end(def_op).size() == 1,
                         "A pipelined operation has more than one ending state");
            top = *HLS->Rliv->get_state_where_end(def_op).begin();
            THROW_ASSERT(target != top, "State defining a variable cannot have it as a live in variable");
            THROW_ASSERT(HLS->storage_value_information->is_a_storage_value(target, var),
                         "There is a live in variable without any register");
            while(previous != top)
            {
               THROW_ASSERT(HLS->storage_value_information->is_a_storage_value(previous, var),
                            "There is a live in variable without any register");
               THROW_ASSERT(HLS->Rliv->get_live_in(previous).find(var) != HLS->Rliv->get_live_in(previous).end(),
                            "The variable is not in live-in");
               THROW_ASSERT(HLS->Rliv->get_live_out(previous).find(var) != HLS->Rliv->get_live_out(previous).end(),
                            "The variable is not in live-out");
               origin_idx = HLS->storage_value_information->get_storage_value_index(previous, var);
               target_idx = HLS->storage_value_information->get_storage_value_index(target, var);
               origin_reg_idx = HLS->Rreg->get_register(origin_idx);
               target_reg_idx = HLS->Rreg->get_register(target_idx);
               origin_reg = HLS->Rreg->get(origin_reg_idx);
               target_reg = HLS->Rreg->get(target_reg_idx);
               // Always add a data transfer on port 0 since it's always an input to reg_STD
               HLS->Rconn->add_data_transfer(origin_reg, target_reg, 0, 0,
                                             data_transfer(var, precision, previous, target, op));
               PRINT_DBG_MEX(
                   DEBUG_LEVEL_PEDANTIC, debug_level,
                   "       - add data transfer from "
                       << origin_reg->get_string() << " to " << target_reg->get_string() << " port "
                       << std::to_string(0) << ":" << std::to_string(0) << " from state "
                       << HLS->Rliv->get_name(previous) + " to state " + HLS->Rliv->get_name(target) + " for " +
                              HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
               target = previous;
               previous = fetch_previous(HLS->STG->get_entry_state(), target);
            }
         }
      }
   }
}

void mux_connection_binding::add_conversion(unsigned int num, vertex op, unsigned int form_par_type,
                                            unsigned long long form_par_bitsize, unsigned int port_index,
                                            const generic_objRef fu_obj, const OpGraphConstRef data,
                                            const tree_managerRef TreeM, unsigned int tree_var)
{
   const HLS_manager::io_binding_type& varObj = HLS_manager::io_binding_type(tree_var, 0);
   const auto var = TreeM->GetTreeNode(tree_var);
   const auto parm = TreeM->GetTreeNode(form_par_type);
   const auto size_tree_var = tree_helper::Size(var);
   const auto inIP = tree_helper::IsSignedIntegerType(var);
   const auto inUP =
       tree_helper::IsUnsignedIntegerType(var) || tree_helper::IsBooleanType(var) || tree_helper::IsPointerType(var);
   const auto outIP = tree_helper::IsSignedIntegerType(parm);
   const auto outUP =
       tree_helper::IsUnsignedIntegerType(parm) || tree_helper::IsBooleanType(parm) || tree_helper::IsPointerType(parm);
   THROW_ASSERT(((inIP || inUP) && (outIP || outUP)) || (!inIP && !inUP && !outIP && !outUP), "unexpected conversion");
   if(((inIP || inUP) && (outIP || outUP)) && (form_par_bitsize != size_tree_var))
   {
      generic_objRef conv_port;
      HLS_manager::check_bitwidth(form_par_bitsize);
      auto out_bitsize = static_cast<unsigned>(form_par_bitsize);
      auto ctype = uu_conv;
      if(inIP && outUP)
      {
         ctype = iu_conv;
      }
      else if(inIP && outIP)
      {
         ctype = ii_conv;
      }
      else if(inUP && outIP)
      {
         ctype = ui_conv;
      }
      auto key = std::make_tuple(out_bitsize, ctype, varObj);
      if(connCache.find(key) == connCache.end())
      {
         if(inIP && outUP)
         {
            conv_port = generic_objRef(new iu_conv_conn_obj("iu_conv_conn_obj_" + STR(id++)));
            GetPointer<iu_conv_conn_obj>(conv_port)->add_bitsize(out_bitsize);
         }
         else if(inIP && outIP)
         {
            conv_port = generic_objRef(new ii_conv_conn_obj("ii_conv_conn_obj_" + STR(id++)));
            GetPointer<ii_conv_conn_obj>(conv_port)->add_bitsize(out_bitsize);
         }
         else if(inUP && outUP)
         {
            conv_port = generic_objRef(new uu_conv_conn_obj("uu_conv_conn_obj_" + STR(id++)));
            GetPointer<uu_conv_conn_obj>(conv_port)->add_bitsize(out_bitsize);
         }
         else if(inUP && outIP)
         {
            conv_port = generic_objRef(new ui_conv_conn_obj("ui_conv_conn_obj_" + STR(id++)));
            GetPointer<ui_conv_conn_obj>(conv_port)->add_bitsize(out_bitsize);
         }
         else
         {
            THROW_ERROR("unexpected");
         }
         if(isConstantObj(std::get<0>(varObj), TreeM))
         {
            connCache[key] = conv_port;
         }
         HLS->Rconn->add_sparse_logic(conv_port);
         HLS_manager::check_bitwidth(size_tree_var);
         determine_connection(op, varObj, conv_port, 0, 0, data, static_cast<unsigned>(size_tree_var));
      }
      else
      {
         conv_port = connCache.find(key)->second;
      }
      auto is_not_a_phi = (GET_TYPE(data, op) & TYPE_PHI) == 0;
      create_single_conn(data, op, conv_port, fu_obj, num, port_index, tree_var, out_bitsize, is_not_a_phi);
   }
   else if(form_par_bitsize != size_tree_var)
   {
      generic_objRef conv_port = generic_objRef(new ff_conv_conn_obj("ff_conv_conn_obj_" + STR(id++)));
      if(parameters->isOption(OPT_soft_float) && parameters->getOption<bool>(OPT_soft_float))
      {
         technology_nodeRef current_fu;
         AllocationInformation::extract_bambu_provided_name(size_tree_var, form_par_bitsize, HLSMgr, current_fu);
      }
      HLS->Rconn->add_sparse_logic(conv_port);
      HLS_manager::check_bitwidth(form_par_bitsize);
      auto out_bitsize = static_cast<unsigned>(form_par_bitsize);
      HLS_manager::check_bitwidth(size_tree_var);
      GetPointer<ff_conv_conn_obj>(conv_port)->add_bitsize_in(static_cast<unsigned>(size_tree_var));
      GetPointer<ff_conv_conn_obj>(conv_port)->add_bitsize_out(out_bitsize);
      determine_connection(op, varObj, conv_port, 0, 0, data, static_cast<unsigned>(size_tree_var));
      auto is_not_a_phi = (GET_TYPE(data, op) & TYPE_PHI) == 0;
      create_single_conn(data, op, conv_port, fu_obj, num, port_index, tree_var, out_bitsize, is_not_a_phi);
   }
   else
   {
      HLS_manager::check_bitwidth(size_tree_var);
      determine_connection(op, varObj, fu_obj, num, port_index, data, static_cast<unsigned>(size_tree_var));
   }
}

void mux_connection_binding::create_connections()
{
   const auto TreeM = HLSMgr->get_tree_manager();
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();
   const auto data = FB->CGetOpGraph(FunctionBehavior::FDFG);
   const auto bus_addr_bitsize = HLSMgr->get_address_bitsize();
   const auto astg = HLS->STG->CGetAstg();
   if(parameters->getOption<int>(OPT_memory_banks_number) > 1 && !parameters->isOption(OPT_context_switch))
   {
      HLS->Rconn = conn_bindingRef(new ParallelMemoryConnBinding(BH, parameters));
   }
   else
   {
      HLS->Rconn = conn_bindingRef(conn_binding::create_conn_binding(HLSMgr, HLS, BH, parameters));
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Starting execution of interconnection binding");

   for(const auto& state2mu : HLS->STG->get_mu_ctrls())
   {
      const auto& mu = state2mu.second;
      const auto mu_mod = mu->get_structural_obj();
      const auto mut = GetPointer<multi_unbounded_obj>(mu);
      const auto en_port =
          HLS->Rconn->bind_selector_port(conn_binding::IN, commandport_obj::MULTI_UNBOUNDED_ENABLE, mu, 0);
      mut->set_mu_enable(en_port);
   }

   const auto num_regs = HLS->Rreg->get_used_regs();
   for(auto r = 0U; r < num_regs; r++)
   {
      const auto reg_obj = HLS->Rreg->get(r);
      const auto sel_port = HLS->Rconn->bind_selector_port(conn_binding::IN, commandport_obj::WRENABLE, reg_obj, r);
      GetPointer<register_obj>(reg_obj)->set_wr_enable(sel_port);
   }
   for(const auto i : HLS->Rfu->get_allocation_list())
   {
      // number of instance functional unit i
      const auto num = HLS->Rfu->get_number(i);
      for(unsigned int fu_num = 0; fu_num < num; fu_num++)
      {
         // get the functional unit object associated to i and fu_num (id and index)
         const auto tmp_Fu = HLS->Rfu->get(i, fu_num);
         std::vector<technology_nodeRef> tmp_ops_node =
             GetPointer<functional_unit>(HLS->allocation_information->get_fu(i))->get_operations();

         if(tmp_ops_node.size() > 1)
         {
            // check all operations associated to functional unit tmp_Fu
            for(unsigned int oper = 0; oper < tmp_ops_node.size(); oper++)
            {
               const auto sel_port =
                   HLS->Rconn->bind_selector_port(conn_binding::IN, commandport_obj::SELECTOR, tmp_Fu, oper);
               GetPointer<funit_obj>(tmp_Fu)->add_selector_op(sel_port, tmp_ops_node.at(oper)->get_name());
            }
         }
      }
   }

   /// add the ports representing the parameters
   add_parameter_ports();

   BOOST_FOREACH(vertex op, boost::vertices(*data))
   {
      /// check for required and produced values
      if(GET_TYPE(data, op) & TYPE_VPHI)
      {
         continue; /// virtual phis are skipped
      }
      auto fu = HLS->Rfu->get_assign(op);
      auto idx = HLS->Rfu->get_index(op);
      auto n_channels = HLS->allocation_information->get_number_channels(fu);
      if((GET_TYPE(data, op) & TYPE_PHI) == 0) /// phis are skipped
      {
         unsigned int port_index = n_channels < 2 ? 0 : idx % n_channels;
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                       "  * Operation: " + GET_NAME(data, op) << " " + data->CGetOpNodeInfo(op)->GetOperation());
         HLS->Rconn->bind_command_port(op, conn_binding::IN, commandport_obj::OPERATION, data);

         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                       "     - FU: " + HLS->allocation_information->get_fu_name(fu).first);
         /// adding activation's state of selector related to operation op
         const auto tmp_ops_node_size =
             GetPointer<functional_unit>(HLS->allocation_information->get_fu(fu))->get_operations().size();
         if(tmp_ops_node_size > 1U)
         {
            if(!GetPointer<funit_obj>(HLS->Rfu->get(fu, idx)))
            {
               THROW_ERROR("Functional unit " + HLS->allocation_information->get_string_name(fu) +
                           " does not have an instance " + STR(idx));
            }
            const auto selector_obj =
                GetPointer<funit_obj>(HLS->Rfu->get(fu, idx))->GetSelector_op(data->CGetOpNodeInfo(op)->GetOperation());
            if(!selector_obj)
            {
               THROW_ERROR("Functional unit " + HLS->allocation_information->get_string_name(fu) +
                           " does not have selector " + data->CGetOpNodeInfo(op)->GetOperation() + "(" + STR(idx) +
                           ") Operation: " + STR(data->CGetOpNodeInfo(op)->GetNodeId()));
            }
            const CustomOrderedSet<vertex>& running_states = HLS->Rliv->get_state_where_run(op);
            for(const auto state : running_states)
            {
               bool is_starting_operation = std::find(astg->CGetStateInfo(state)->starting_operations.begin(),
                                                      astg->CGetStateInfo(state)->starting_operations.end(),
                                                      op) != astg->CGetStateInfo(state)->starting_operations.end();
               if(!(GET_TYPE(data, op) & (TYPE_LOAD | TYPE_STORE)) || is_starting_operation)
               {
                  GetPointer<commandport_obj>(selector_obj)
                      ->add_activation(
                          commandport_obj::transition(state, NULL_VERTEX, commandport_obj::data_operation_pair(0, op)));
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                "       - add activation for " + selector_obj->get_string() + " in state "
                                    << HLS->Rliv->get_name(state));
               }
            }
         }

         const generic_objRef fu_obj = HLS->Rfu->get(op);
         std::vector<HLS_manager::io_binding_type> var_read = HLSMgr->get_required_values(HLS->functionId, op);
#ifndef NDEBUG
         unsigned int index = 0;
#endif
         for(auto& num : var_read)
         {
            if(std::get<0>(num) == 0)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "     - " << index << ". Read: " + STR(std::get<1>(num)));
            }
            else
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "     - " << index << ". Read: " + BH->PrintVariable(std::get<0>(num)));
            }
#ifndef NDEBUG
            ++index;
#endif
         }
         if(GET_TYPE(data, op) & (TYPE_LOAD | TYPE_STORE))
         {
            auto node_id = data->CGetOpNodeInfo(op)->GetNodeId();
            const tree_nodeRef node = TreeM->GetTreeNode(node_id);
            auto* gm = GetPointer<gimple_assign>(node);
            THROW_ASSERT(gm, "only gimple_assign's are allowed as memory operations");

            if(HLS->allocation_information->is_direct_access_memory_unit(fu) ||
               HLS->allocation_information->is_indirect_access_memory_unit(fu)) /// MEMORY REFERENCES
            {
               unsigned int alignment = 0;
               tree_nodeRef var_node;
               unsigned int size_var;
               tree_nodeConstRef tn;
               unsigned int var_node_idx;
               unsigned long long Prec = 0;
               const auto type = tree_helper::CGetType(gm->op0);
               if(type && (type->get_kind() == integer_type_K))
               {
                  Prec = GetPointerS<const integer_type>(type)->prec;
               }
               else if(type && (type->get_kind() == boolean_type_K))
               {
                  Prec = 8;
               }
               else if(type && (type->get_kind() == enumeral_type_K))
               {
                  Prec = GetPointerS<const enumeral_type>(type)->prec;
               }
               unsigned int algn = 0;
               if(type && (type->get_kind() == integer_type_K))
               {
                  algn = GetPointerS<const integer_type>(type)->algn;
               }
               else if(type && (type->get_kind() == boolean_type_K))
               {
                  algn = 8;
               }
#if USE_ALIGNMENT_INFO
               if(type && GetPointer<const type_node>(type))
               {
                  algn = alignment = GetPointerS<const type_node>(type)->algn;
               }
#endif
               if(GET_TYPE(data, op) & TYPE_STORE)
               {
                  size_var = std::get<0>(var_read[0]);
                  tn = tree_helper::CGetType(TreeM->GetTreeNode(size_var));
                  var_node = gm->op0;
                  var_node_idx = gm->op0->index;

                  if(size_var)
                  {
                     THROW_ASSERT(tree_helper::GetConstValue(GetPointerS<const type_node>(tn)->size) >= 0, "");
                     const auto IR_var_bitsize =
                         static_cast<unsigned int>(tree_helper::GetConstValue(GetPointerS<const type_node>(tn)->size));
                     unsigned int var_bitsize;
                     if(Prec != algn && Prec % algn)
                     {
                        HLS_manager::check_bitwidth(Prec);
                        var_bitsize = static_cast<unsigned int>(Prec);
                     }
                     else
                     {
                        HLS_manager::check_bitwidth(IR_var_bitsize);
                        var_bitsize = IR_var_bitsize;
                     }
                     generic_objRef conv_port;
                     auto varObj = var_read[0];
                     if(tree_helper::is_int(TreeM, size_var))
                     {
                        auto key = std::make_tuple(var_bitsize, iu_conv, varObj);
                        if(connCache.find(key) == connCache.end())
                        {
                           conv_port = generic_objRef(new iu_conv_conn_obj("iu_conv_conn_obj_" + STR(id++)));
                           if(isConstantObj(std::get<0>(varObj), TreeM))
                           {
                              connCache[key] = conv_port;
                           }
                           HLS->Rconn->add_sparse_logic(conv_port);
                           GetPointer<iu_conv_conn_obj>(conv_port)->add_bitsize(var_bitsize);
                           determine_connection(op, varObj, conv_port, 0, 0, data, var_bitsize);
                        }
                        else
                        {
                           conv_port = connCache.find(key)->second;
                        }
                     }
                     else
                     {
                        auto key = std::make_tuple(var_bitsize, uu_conv, varObj);
                        if(connCache.find(key) == connCache.end())
                        {
                           conv_port = generic_objRef(new uu_conv_conn_obj("uu_conv_conn_obj_" + STR(id++)));
                           if(isConstantObj(std::get<0>(varObj), TreeM))
                           {
                              connCache[key] = conv_port;
                           }
                           HLS->Rconn->add_sparse_logic(conv_port);
                           GetPointer<uu_conv_conn_obj>(conv_port)->add_bitsize(var_bitsize);
                           determine_connection(op, varObj, conv_port, 0, 0, data, var_bitsize);
                        }
                        else
                        {
                           conv_port = connCache.find(key)->second;
                        }
                     }
                     create_single_conn(data, op, conv_port, fu_obj, 0, port_index, size_var, var_bitsize, true);
                  }
                  else
                  {
                     auto prec = object_bitsize(TreeM, var_read[0]);
                     HLS_manager::check_bitwidth(prec);
                     determine_connection(op, var_read[0], fu_obj, 0, port_index, data, static_cast<unsigned>(prec));
                  }
               }
               else
               {
                  size_var = HLSMgr->get_produced_value(HLS->functionId, op);
                  tn = tree_helper::CGetType(TreeM->GetTreeNode(size_var));
                  var_node = gm->op1;
                  var_node_idx = gm->op1->index;
               }
#ifndef NDEBUG
               if(var_node->get_kind() == ssa_name_K)
               {
                  THROW_ASSERT(tree_helper::CGetType(var_node)->get_kind() == complex_type_K,
                               "only complex objects are considered");
               }
#endif
               auto is_dual = HLS->allocation_information->is_dual_port_memory(fu);
               auto port_offset = [&](unsigned pi) -> unsigned int {
                  if(is_dual)
                  {
                     return (GET_TYPE(data, op) & TYPE_LOAD) ? pi * 2 - 1 : pi * 2;
                  }
                  else
                  {
                     return pi;
                  }
               };

               THROW_ASSERT(!gm->predicate || tree_helper::Size(gm->predicate) == 1, STR(gm->predicate));
               auto var = gm->predicate ? HLS_manager::io_binding_type(gm->predicate->index, 0) :
                                          HLS_manager::io_binding_type(0, 1);
               /// connect predicate port
               determine_connection(op, var, fu_obj, port_offset(3), port_index, data, 1);

               THROW_ASSERT(var_node->get_kind() == mem_ref_K, "MEMORY REFERENCE/LOAD-STORE type not supported: " +
                                                                   var_node->get_kind_text() + " " + STR(node_id));

               /// connect address port
               determine_connection(op, HLS_manager::io_binding_type(var_node_idx, 0), fu_obj, port_offset(1),
                                    port_index, data, bus_addr_bitsize, alignment);
               /// connect size port
               const auto IR_var_bitsize = tree_helper::SizeAlloc(tn);
               HLS_manager::check_bitwidth(IR_var_bitsize);
               unsigned int var_bitsize;
               var_bitsize = static_cast<unsigned int>(IR_var_bitsize);
               determine_connection(
                   op, HLS_manager::io_binding_type(0, var_bitsize), fu_obj, port_offset(2), port_index, data,
                   static_cast<unsigned>(object_bitsize(TreeM, HLS_manager::io_binding_type(0, var_bitsize))));
            }
            else
            {
               THROW_ERROR("Unit " + HLS->allocation_information->get_fu_name(fu).first + " not supported");
            }
         }
         else if(data->CGetOpNodeInfo(op)->GetOperation() == MULTI_READ_COND)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - " << var_read.size() << " reads");
            for(unsigned int num = 0; num < var_read.size(); num++)
            {
               auto prec = object_bitsize(TreeM, var_read[num]);
               HLS_manager::check_bitwidth(prec);
               determine_connection(op, var_read[num], fu_obj, 0, num, data, static_cast<unsigned>(prec));
            }
         }
         else
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - " << var_read.size() << " reads");
            tree_nodeConstRef first_valid;
            if(HLS->Rfu->get_ports_are_swapped(op))
            {
               THROW_ASSERT(var_read.size() == 2, "unexpected condition");
               std::swap(var_read[0], var_read[1]);
            }
            for(unsigned int port_num = 0; port_num < var_read.size(); port_num++)
            {
               const auto tree_var = std::get<0>(var_read[port_num]);
               const auto tree_var_node = tree_var == 0 ? nullptr : TreeM->GetTreeNode(tree_var);
               const auto& node = data->CGetOpNodeInfo(op)->node;
               const auto form_par_type = tree_helper::GetFormalIth(node, port_num);
               auto size_form_par = form_par_type ? tree_helper::Size(form_par_type) : 0;
               const auto OperationType = data->CGetOpNodeInfo(op)->GetOperation();
               if(tree_var && !first_valid)
               {
                  first_valid = tree_var_node;
               }
               if((OperationType == "cond_expr" || OperationType == "vec_cond_expr") && port_num != 0 && tree_var)
               {
                  first_valid = tree_var_node;
               }

               if(tree_var == 0)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                "     - " << port_num << ". Read: " + STR(std::get<1>(var_read[port_num])));
               }
               else
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                "     - " << port_num << ". Read: " + BH->PrintVariable(tree_var));
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "          * " + tree_var_node->get_kind_text());
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                "          * bitsize " + STR(object_bitsize(TreeM, var_read[port_num])));
               }
               if(tree_var && HLSMgr->Rmem->is_actual_parm_loaded(tree_var))
               {
                  THROW_ERROR("LOADING of actual parameter not yet implemented");
               }
               else if(form_par_type && tree_var &&
                       ((tree_helper::IsSignedIntegerType(tree_var_node) &&
                         (tree_helper::IsUnsignedIntegerType(form_par_type) ||
                          tree_helper::IsBooleanType(form_par_type))) ||
                        ((tree_helper::IsUnsignedIntegerType(tree_var_node) ||
                          (tree_helper::IsBooleanType(form_par_type))) &&
                         tree_helper::IsSignedIntegerType(form_par_type)) ||
                        (tree_helper::IsRealType(tree_var_node) && tree_helper::IsRealType(form_par_type))))
               {
                  add_conversion(port_num, op, form_par_type->index, size_form_par, port_index, fu_obj, data, TreeM,
                                 tree_var);
               }
               else if(first_valid && tree_var && first_valid->index != tree_var_node->index && !form_par_type &&
                       OperationType != "rshift_expr" && OperationType != "lshift_expr" &&
                       OperationType != "extract_bit_expr" && OperationType != "rrotate_expr" &&
                       OperationType != "lrotate_expr" &&
                       ((tree_helper::IsSignedIntegerType(tree_var_node) &&
                         tree_helper::IsUnsignedIntegerType(first_valid)) ||
                        (tree_helper::IsUnsignedIntegerType(tree_var_node) &&
                         tree_helper::IsSignedIntegerType(first_valid))))
               {
                  // we only need type conversion and not size conversion, so we pass the same size for both
                  size_form_par = tree_helper::Size(tree_var_node);
                  add_conversion(port_num, op, first_valid->index, size_form_par, port_index, fu_obj, data, TreeM,
                                 tree_var);
               }
               else
               {
                  auto prec = object_bitsize(TreeM, var_read[port_num]);
                  HLS_manager::check_bitwidth(prec);
                  determine_connection(op, var_read[port_num], fu_obj, port_num, port_index, data,
                                       static_cast<unsigned>(prec));
               }
            }
         }
      }

      if(GET_TYPE(data, op) & TYPE_PHI)
      {
         /// phi must be differently managed
         auto var_written = HLSMgr->get_produced_value(HLS->functionId, op);
         CustomOrderedSet<unsigned int> source_already_analyzed;
         const CustomOrderedSet<vertex>& ending_states = HLS->Rliv->get_state_where_end(op);
         THROW_ASSERT(ending_states.size() == 1 || is_PC ||
                          HLS->STG->GetStg()->CGetStateInfo(*ending_states.begin())->is_duplicated,
                      "phis cannot run in more than one state");
         for(const auto estate : ending_states)
         {
            const StateInfoConstRef state_info =
                is_PC ? StateInfoConstRef() : HLS->STG->GetStg()->CGetStateInfo(estate);
            const auto gp = GetPointer<const gimple_phi>(TreeM->GetTreeNode(data->CGetOpNodeInfo(op)->GetNodeId()));
            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               auto tree_temp = def_edge.first->index;
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "Pre-Managing phi operation " + GET_NAME(data, op) + " ending in state " +
                                 HLS->Rliv->get_name(estate) +
                                 (tree_temp ? " for variable " + def_edge.first->ToString() : ""));
               bool phi_postponed = false;
               if(state_info && state_info->is_duplicated && !state_info->all_paths)
               {
                  auto bbID = def_edge.second;
                  if(!state_info->isOriginalState && bbID != state_info->sourceBb)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not original state and not source BB");
                     continue;
                  }
                  else if(state_info->isOriginalState && bbID == state_info->sourceBb)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Original state and source BB");
                     continue;
                  }
                  else if(state_info->moved_op_def_set.find(tree_temp) != state_info->moved_op_def_set.end())
                  {
                     phi_postponed = true;
                  }
                  else if(state_info->moved_op_use_set.find(var_written) != state_info->moved_op_use_set.end())
                  {
                     phi_postponed = true;
                  }
               }
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "Is phi postponed? " + (phi_postponed ? std::string("YES") : std::string("NO")));
               if(!phi_postponed)
               {
                  if(source_already_analyzed.find(tree_temp) == source_already_analyzed.end())
                  {
                     source_already_analyzed.insert(tree_temp);
                  }
                  else
                  {
                     continue;
                  }
               }
               cur_phi_tree_var = tree_temp;
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "Pre-Managing phi operation2 " + GET_NAME(data, op) + " ending in state " +
                                 HLS->Rliv->get_name(estate) +
                                 (cur_phi_tree_var ? " for variable " + BH->PrintVariable(cur_phi_tree_var) : ""));
               THROW_ASSERT(cur_phi_tree_var, "something wrong happened");
               THROW_ASSERT(!HLSMgr->Rmem->has_base_address(tree_temp),
                            "phi cannot manage memory objects: @" + STR(tree_temp));
               THROW_ASSERT(!HLSMgr->Rmem->has_base_address(var_written),
                            "phi cannot manage memory objects: @" + STR(var_written));
               THROW_ASSERT(TreeM->GetTreeNode(tree_temp)->get_kind() != array_ref_K, "unexpected phi use");
               THROW_ASSERT(TreeM->GetTreeNode(tree_temp)->get_kind() != indirect_ref_K, "unexpected phi use");
               THROW_ASSERT(TreeM->GetTreeNode(tree_temp)->get_kind() != misaligned_indirect_ref_K,
                            "unexpected phi use");

               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "Managing phi operation " + GET_NAME(data, op) + " ending in state " +
                                 HLS->Rliv->get_name(estate) +
                                 (cur_phi_tree_var ? " for variable " + BH->PrintVariable(cur_phi_tree_var) : ""));
               if(HLS->storage_value_information->is_a_storage_value(estate, var_written))
               {
                  auto storage_value = HLS->storage_value_information->get_storage_value_index(estate, var_written);
                  auto r_index = HLS->Rreg->get_register(storage_value);
                  auto in_bitsize = object_bitsize(TreeM, HLS_manager::io_binding_type(tree_temp, 0));
                  HLS_manager::check_bitwidth(in_bitsize);
                  auto out_bitsize = object_bitsize(TreeM, HLS_manager::io_binding_type(var_written, 0));
                  HLS_manager::check_bitwidth(out_bitsize);
                  generic_objRef tgt_reg_obj = HLS->Rreg->get(r_index);
                  THROW_ASSERT(tree_helper::IsSameType(TreeM->GetTreeNode(tree_temp), TreeM->GetTreeNode(var_written)),
                               "conversion required");
                  if(phi_postponed)
                  {
                     if(HLS->Rliv->has_state_out(estate, op, var_written))
                     {
                        if(in_bitsize != out_bitsize)
                        {
                           if(tree_helper::is_unsigned(TreeM, var_written) ||
                              tree_helper::is_a_pointer(TreeM, var_written) || tree_helper::is_bool(TreeM, var_written))
                           {
                              generic_objRef conv_port =
                                  generic_objRef(new u_assign_conn_obj("u_assign_conn_obj_" + STR(id++)));
                              HLS->Rconn->add_sparse_logic(conv_port);
                              GetPointer<u_assign_conn_obj>(conv_port)->add_bitsize(static_cast<unsigned>(in_bitsize));
                              generic_objRef fu_src_obj;
                              if(state_info &&
                                 state_info->moved_op_use_set.find(var_written) != state_info->moved_op_use_set.end() &&
                                 state_info->moved_op_def_set.find(cur_phi_tree_var) ==
                                     state_info->moved_op_def_set.end())
                              {
                                 auto src_storage_value =
                                     HLS->storage_value_information->get_storage_value_index(estate, cur_phi_tree_var);
                                 auto src_r_index = HLS->Rreg->get_register(src_storage_value);
                                 fu_src_obj = HLS->Rreg->get(src_r_index);
                              }
                              else
                              {
                                 vertex src_def_op = HLS->Rliv->get_op_where_defined(cur_phi_tree_var);
                                 fu_src_obj = HLS->Rfu->get(src_def_op);
                              }
                              const CustomOrderedSet<vertex>& states_out =
                                  HLS->Rliv->get_state_out(estate, op, var_written);
                              for(const auto state_out : states_out)
                              {
                                 HLS->Rconn->add_data_transfer(fu_src_obj, conv_port, 0, 0,
                                                               data_transfer(cur_phi_tree_var,
                                                                             static_cast<unsigned>(in_bitsize), estate,
                                                                             state_out, op));
                                 HLS->Rconn->add_data_transfer(conv_port, tgt_reg_obj, 0, 0,
                                                               data_transfer(cur_phi_tree_var,
                                                                             static_cast<unsigned>(in_bitsize), estate,
                                                                             state_out, op));
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                               "       - add data transfer from "
                                                   << fu_src_obj->get_string() << " to " << conv_port->get_string()
                                                   << " port 0:0 from state "
                                                   << HLS->Rliv->get_name(estate) + " to state " +
                                                          HLS->Rliv->get_name(state_out) + " for " +
                                                          HLSMgr->CGetFunctionBehavior(funId)
                                                              ->CGetBehavioralHelper()
                                                              ->PrintVariable(cur_phi_tree_var));
                                 generic_objRef enable_obj = GetPointer<register_obj>(tgt_reg_obj)->get_wr_enable();
                                 GetPointer<commandport_obj>(enable_obj)
                                     ->add_activation(commandport_obj::transition(
                                         estate, state_out,
                                         commandport_obj::data_operation_pair(cur_phi_tree_var, op)));
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                               "       - write enable for " + tgt_reg_obj->get_string() + " from "
                                                   << HLS->Rliv->get_name(estate) + " to state " +
                                                          HLS->Rliv->get_name(state_out));
                              }
                           }
                           else if(tree_helper::is_int(TreeM, var_written))
                           {
                              generic_objRef conv_port =
                                  generic_objRef(new i_assign_conn_obj("i_assign_conn_obj_phi" + STR(id++)));
                              HLS->Rconn->add_sparse_logic(conv_port);
                              GetPointer<i_assign_conn_obj>(conv_port)->add_bitsize(static_cast<unsigned>(in_bitsize));
                              generic_objRef fu_src_obj;
                              if(state_info &&
                                 state_info->moved_op_use_set.find(var_written) != state_info->moved_op_use_set.end() &&
                                 state_info->moved_op_def_set.find(cur_phi_tree_var) ==
                                     state_info->moved_op_def_set.end())
                              {
                                 auto src_storage_value =
                                     HLS->storage_value_information->get_storage_value_index(estate, cur_phi_tree_var);
                                 auto src_r_index = HLS->Rreg->get_register(src_storage_value);
                                 fu_src_obj = HLS->Rreg->get(src_r_index);
                              }
                              else
                              {
                                 vertex src_def_op = HLS->Rliv->get_op_where_defined(cur_phi_tree_var);
                                 fu_src_obj = HLS->Rfu->get(src_def_op);
                              }

                              const CustomOrderedSet<vertex>& states_out =
                                  HLS->Rliv->get_state_out(estate, op, var_written);
                              const CustomOrderedSet<vertex>::const_iterator s_out_it_end = states_out.end();
                              for(auto s_out_it = states_out.begin(); s_out_it != s_out_it_end; ++s_out_it)
                              {
                                 HLS->Rconn->add_data_transfer(fu_src_obj, conv_port, 0, 0,
                                                               data_transfer(cur_phi_tree_var,
                                                                             static_cast<unsigned>(in_bitsize), estate,
                                                                             *s_out_it, op));
                                 HLS->Rconn->add_data_transfer(conv_port, tgt_reg_obj, 0, 0,
                                                               data_transfer(cur_phi_tree_var,
                                                                             static_cast<unsigned>(in_bitsize), estate,
                                                                             *s_out_it, op));
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                               "       - add data transfer from "
                                                   << fu_src_obj->get_string() << " to " << conv_port->get_string()
                                                   << " port 0:0 from state "
                                                   << HLS->Rliv->get_name(estate) + " to state " +
                                                          HLS->Rliv->get_name(*s_out_it) + " for " +
                                                          HLSMgr->CGetFunctionBehavior(funId)
                                                              ->CGetBehavioralHelper()
                                                              ->PrintVariable(cur_phi_tree_var));
                                 generic_objRef enable_obj = GetPointer<register_obj>(tgt_reg_obj)->get_wr_enable();
                                 GetPointer<commandport_obj>(enable_obj)
                                     ->add_activation(commandport_obj::transition(
                                         estate, *s_out_it,
                                         commandport_obj::data_operation_pair(cur_phi_tree_var, op)));
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                               "       - write enable for " + tgt_reg_obj->get_string() + " from "
                                                   << HLS->Rliv->get_name(estate) + " to state " +
                                                          HLS->Rliv->get_name(*s_out_it));
                              }
                           }
                           else if(tree_helper::is_real(TreeM, var_written))
                           {
                              generic_objRef conv_port =
                                  generic_objRef(new f_assign_conn_obj("f_assign_conn_obj_" + STR(id++)));
                              HLS->Rconn->add_sparse_logic(conv_port);
                              GetPointer<f_assign_conn_obj>(conv_port)->add_bitsize(static_cast<unsigned>(in_bitsize));
                              generic_objRef fu_src_obj;
                              if(state_info &&
                                 state_info->moved_op_use_set.find(var_written) != state_info->moved_op_use_set.end() &&
                                 state_info->moved_op_def_set.find(cur_phi_tree_var) ==
                                     state_info->moved_op_def_set.end())
                              {
                                 auto src_storage_value =
                                     HLS->storage_value_information->get_storage_value_index(estate, cur_phi_tree_var);
                                 auto src_r_index = HLS->Rreg->get_register(src_storage_value);
                                 fu_src_obj = HLS->Rreg->get(src_r_index);
                              }
                              else
                              {
                                 vertex src_def_op = HLS->Rliv->get_op_where_defined(cur_phi_tree_var);
                                 fu_src_obj = HLS->Rfu->get(src_def_op);
                              }

                              const CustomOrderedSet<vertex>& states_out =
                                  HLS->Rliv->get_state_out(estate, op, var_written);
                              const CustomOrderedSet<vertex>::const_iterator s_out_it_end = states_out.end();
                              for(auto s_out_it = states_out.begin(); s_out_it != s_out_it_end; ++s_out_it)
                              {
                                 HLS->Rconn->add_data_transfer(fu_src_obj, conv_port, 0, 0,
                                                               data_transfer(cur_phi_tree_var,
                                                                             static_cast<unsigned>(in_bitsize), estate,
                                                                             *s_out_it, op));
                                 HLS->Rconn->add_data_transfer(conv_port, tgt_reg_obj, 0, 0,
                                                               data_transfer(cur_phi_tree_var,
                                                                             static_cast<unsigned>(in_bitsize), estate,
                                                                             *s_out_it, op));
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                               "       - add data transfer from "
                                                   << fu_src_obj->get_string() << " to " << conv_port->get_string()
                                                   << " port 0:0 from state "
                                                   << HLS->Rliv->get_name(estate) + " to state " +
                                                          HLS->Rliv->get_name(*s_out_it) + " for " +
                                                          HLSMgr->CGetFunctionBehavior(funId)
                                                              ->CGetBehavioralHelper()
                                                              ->PrintVariable(cur_phi_tree_var));
                                 generic_objRef enable_obj = GetPointer<register_obj>(tgt_reg_obj)->get_wr_enable();
                                 GetPointer<commandport_obj>(enable_obj)
                                     ->add_activation(commandport_obj::transition(
                                         estate, *s_out_it,
                                         commandport_obj::data_operation_pair(cur_phi_tree_var, op)));
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                               "       - write enable for " + tgt_reg_obj->get_string() + " from "
                                                   << HLS->Rliv->get_name(estate) + " to state " +
                                                          HLS->Rliv->get_name(*s_out_it));
                              }
                           }
                           else
                           {
                              THROW_ERROR("not expected conversion " + STR(cur_phi_tree_var) + " " + STR(in_bitsize) +
                                          " " + STR(out_bitsize) + " " +
                                          TreeM->GetTreeNode(data->CGetOpNodeInfo(op)->GetNodeId())->ToString());
                           }
                        }
                        else
                        {
                           generic_objRef fu_src_obj;
                           if(state_info &&
                              state_info->moved_op_use_set.find(var_written) != state_info->moved_op_use_set.end() &&
                              state_info->moved_op_def_set.find(cur_phi_tree_var) == state_info->moved_op_def_set.end())
                           {
                              auto src_storage_value =
                                  HLS->storage_value_information->get_storage_value_index(estate, cur_phi_tree_var);
                              auto src_r_index = HLS->Rreg->get_register(src_storage_value);
                              fu_src_obj = HLS->Rreg->get(src_r_index);
                           }
                           else
                           {
                              vertex src_def_op = HLS->Rliv->get_op_where_defined(cur_phi_tree_var);
                              fu_src_obj = HLS->Rfu->get(src_def_op);
                           }
                           const CustomOrderedSet<vertex>& states_out =
                               HLS->Rliv->get_state_out(estate, op, var_written);
                           const CustomOrderedSet<vertex>::const_iterator s_out_it_end = states_out.end();
                           for(auto s_out_it = states_out.begin(); s_out_it != s_out_it_end; ++s_out_it)
                           {
                              HLS->Rconn->add_data_transfer(fu_src_obj, tgt_reg_obj, 0, 0,
                                                            data_transfer(cur_phi_tree_var,
                                                                          static_cast<unsigned>(in_bitsize), estate,
                                                                          *s_out_it, op));
                              PRINT_DBG_MEX(
                                  DEBUG_LEVEL_PEDANTIC, debug_level,
                                  "       - add data transfer from "
                                      << fu_src_obj->get_string() << " to " << tgt_reg_obj->get_string()
                                      << " port 0:0 from state "
                                      << HLS->Rliv->get_name(estate) + " to state " + HLS->Rliv->get_name(*s_out_it) +
                                             " for " +
                                             HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                                 cur_phi_tree_var));
                              generic_objRef enable_obj = GetPointer<register_obj>(tgt_reg_obj)->get_wr_enable();
                              GetPointer<commandport_obj>(enable_obj)
                                  ->add_activation(commandport_obj::transition(
                                      estate, *s_out_it, commandport_obj::data_operation_pair(cur_phi_tree_var, op)));
                              PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                            "       - write enable for " + tgt_reg_obj->get_string() + " from "
                                                << HLS->Rliv->get_name(estate) + " to state " +
                                                       HLS->Rliv->get_name(*s_out_it));
                           }
                        }
                     }
                  }
                  else
                  {
                     if(in_bitsize != out_bitsize)
                     {
                        add_conversion(0, op, tree_helper::CGetType(TreeM->GetTreeNode(var_written))->index,
                                       out_bitsize, 0, tgt_reg_obj, data, TreeM, tree_temp);
                     }
                     else
                     {
                        determine_connection(op, HLS_manager::io_binding_type(tree_temp, 0), tgt_reg_obj, 0, 0, data,
                                             static_cast<unsigned>(in_bitsize));
                     }
                  }
               }
               cur_phi_tree_var = 0;
            }
         }
      }
      else
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  * Ending Operation: " + GET_NAME(data, op));
         HLS->Rconn->bind_command_port(op, conn_binding::IN, commandport_obj::OPERATION, data);

         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                       "     - FU: " + HLS->allocation_information->get_fu_name(HLS->Rfu->get_assign(op)).first);
         const generic_objRef fu_obj = HLS->Rfu->get(op);
         const auto var_written = HLSMgr->get_produced_value(HLS->functionId, op);
         if((GET_TYPE(data, op) & TYPE_MULTIIF) != 0)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: (multi-way if value)");
            auto node_id = data->CGetOpNodeInfo(op)->GetNodeId();
            std::vector<HLS_manager::io_binding_type> var_read = HLSMgr->get_required_values(HLS->functionId, op);
            generic_objRef TargetPort =
                HLS->Rconn->bind_selector_port(conn_binding::OUT, commandport_obj::MULTIIF, op, data);
            const CustomOrderedSet<vertex>& ending_states = HLS->Rliv->get_state_where_end(op);
            for(const auto estate : ending_states)
            {
               HLS->Rconn->add_data_transfer(fu_obj, TargetPort, 0, 0,
                                             data_transfer(node_id, var_read.size(), estate, NULL_VERTEX, op));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add data transfer from "
                                 << fu_obj->get_string() << " to " << TargetPort->get_string() << " in state "
                                 << HLS->Rliv->get_name(estate) + " for " + STR(node_id));
               GetPointer<commandport_obj>(TargetPort)
                   ->add_activation(commandport_obj::transition(estate, NULL_VERTEX,
                                                                commandport_obj::data_operation_pair(node_id, op)));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add activation for " + TargetPort->get_string() + " in state "
                                 << HLS->Rliv->get_name(estate));
            }
         }
         else if(var_written == 0)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: (no value produced)");
         }
         else if((GET_TYPE(data, op) & TYPE_IF) != 0)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: (boolean value)");
            generic_objRef TargetPort =
                HLS->Rconn->bind_selector_port(conn_binding::OUT, commandport_obj::CONDITION, op, data);
            const CustomOrderedSet<vertex>& ending_states = HLS->Rliv->get_state_where_end(op);
            for(const auto estate : ending_states)
            {
               HLS->Rconn->add_data_transfer(fu_obj, TargetPort, 0, 0,
                                             data_transfer(var_written,
                                                           tree_helper::Size(TreeM->GetTreeNode(var_written)), estate,
                                                           NULL_VERTEX, op));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add data transfer from " << fu_obj->get_string() << " to "
                                                                << TargetPort->get_string() << " in state "
                                                                << HLS->Rliv->get_name(estate) + "for condition");
               GetPointer<commandport_obj>(TargetPort)
                   ->add_activation(commandport_obj::transition(estate, NULL_VERTEX,
                                                                commandport_obj::data_operation_pair(var_written, op)));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add activation for " + TargetPort->get_string() + " in state "
                                 << HLS->Rliv->get_name(estate));
            }
         }
         else if((GET_TYPE(data, op) & TYPE_SWITCH) != 0)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: (switch value)");
            generic_objRef TargetPort =
                HLS->Rconn->bind_selector_port(conn_binding::OUT, commandport_obj::SWITCH, op, data);
            const CustomOrderedSet<vertex>& ending_states = HLS->Rliv->get_state_where_end(op);
            for(const auto estate : ending_states)
            {
               HLS->Rconn->add_data_transfer(fu_obj, TargetPort, 0, 0,
                                             data_transfer(var_written,
                                                           tree_helper::Size(TreeM->GetTreeNode(var_written)), estate,
                                                           NULL_VERTEX, op));
               PRINT_DBG_MEX(
                   DEBUG_LEVEL_PEDANTIC, debug_level,
                   "       - add data transfer from "
                       << fu_obj->get_string() << " to " << TargetPort->get_string() << " in state "
                       << HLS->Rliv->get_name(estate) + " for " +
                              HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(var_written));
               GetPointer<commandport_obj>(TargetPort)
                   ->add_activation(commandport_obj::transition(estate, NULL_VERTEX,
                                                                commandport_obj::data_operation_pair(var_written, op)));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add activation for " + TargetPort->get_string() + " in state "
                                 << HLS->Rliv->get_name(estate));
            }
         }
         else
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: " + BH->PrintVariable(var_written));
            const CustomOrderedSet<vertex>& ending_states = HLS->Rliv->get_state_where_end(op);
            for(const auto estate : ending_states)
            {
               if(HLS->Rliv->has_state_out(estate, op, var_written))
               {
                  const CustomOrderedSet<vertex>& states_out = HLS->Rliv->get_state_out(estate, op, var_written);
                  const CustomOrderedSet<vertex>::const_iterator s_out_it_end = states_out.end();
                  for(auto s_out_it = states_out.begin(); s_out_it != s_out_it_end; ++s_out_it)
                  {
                     auto storage_value =
                         HLS->storage_value_information->get_storage_value_index(*s_out_it, var_written);
                     auto r_index = HLS->Rreg->get_register(storage_value);
                     generic_objRef tgt_reg_obj = HLS->Rreg->get(r_index);
                     HLS->Rconn->add_data_transfer(fu_obj, tgt_reg_obj, 0, 0,
                                                   data_transfer(var_written,
                                                                 tree_helper::Size(TreeM->GetTreeNode(var_written)),
                                                                 estate, *s_out_it, op));
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "       - add data transfer from "
                             << fu_obj->get_string() << " to " << tgt_reg_obj->get_string() << " from state "
                             << HLS->Rliv->get_name(estate) + " to state " + HLS->Rliv->get_name(*s_out_it) + " for " +
                                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(
                                        var_written));
                     generic_objRef enable_obj = GetPointer<register_obj>(tgt_reg_obj)->get_wr_enable();
                     GetPointer<commandport_obj>(enable_obj)
                         ->add_activation(commandport_obj::transition(
                             estate, *s_out_it, commandport_obj::data_operation_pair(var_written, op)));
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - write enable for " + tgt_reg_obj->get_string() + " from "
                                       << HLS->Rliv->get_name(estate) + " to state " + HLS->Rliv->get_name(*s_out_it));
                  }
               }
               else
               {
                  /// if the variable does not belong to the live-out set, it means that it is used inside the state
                  /// (chaining) and this situation is managed above
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - write in a data_transfer");
               }
            }
         }
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Ended execution of interconnection binding");
}

unsigned int mux_connection_binding::mux_interconnection()
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Starting datapath interconnection based on mux architecture");

   unsigned int allocated_mux = 0;
   unsigned int iteration = 0;

   for(const auto& connection : HLS->Rconn->get_data_transfers())
   {
      const generic_objRef unit = std::get<0>(connection.first);
      auto operand = std::get<1>(connection.first);
      auto port_index = std::get<2>(connection.first);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                    "Unit: " + unit->get_string() + "(" + std::to_string(operand) + ":" + std::to_string(port_index) +
                        "): " + std::to_string(connection.second.size()) + " connections");
      allocated_mux += input_logic(connection.second, unit, operand, port_index, iteration);
      ++iteration;
   }

   return allocated_mux;
}

unsigned int mux_connection_binding::input_logic(const conn_binding::ConnectionSources& srcs, const generic_objRef tgt,
                                                 unsigned int op, unsigned int port_index, unsigned int iteration)
{
   static unsigned int used_mux = 0;
   unsigned int starting_value = used_mux;

   /// if it's a one-to-one connection, a directed link can be used
   if(srcs.size() == 1)
   {
      generic_objRef op1 = srcs.begin()->first;
      THROW_ASSERT(op1, "Target \"" + tgt->get_string() + "\" connected with an undefined source");
      connection_objRef conn_obj = connection_objRef(new direct_conn(srcs.begin()->second));
      HLS->Rconn->AddConnectionCB(op1, tgt, op, port_index, conn_obj);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "  - Direct connection between " + op1->get_string() + " and " + tgt->get_string() + "(" + STR(op) +
                        ":" + STR(port_index) + ")");
      return 0;
   }

   /// map between the source object and the resulting tree of multiplexers to the current target object
   std::map<generic_objRef, std::vector<std::pair<generic_objRef, unsigned int>>> src_mux_tree;

   /// map between a data_transfer and object associated at the moment
   std::map<data_transfer, generic_objRef> var2obj;
   /// map between a generic_obj and list of data_transfer associated with this object
   std::map<generic_objRef, std::list<data_transfer>> obj2var;
   /// map between a data_transfer and original source object
   std::map<data_transfer, generic_objRef> var2src;

#ifndef NDEBUG
   std::map<std::pair<vertex, vertex>, generic_objRef> check_sources;
#endif

   std::list<generic_objRef> to_allocate;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  - Connection from: ");
   for(const auto& src : srcs)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     * Source: " + src.first->get_string() + " ");
      const CustomOrderedSet<data_transfer>& vars = src.second;
      THROW_ASSERT(vars.size(), "A connection should contain at least one data-transfer");
      for(const auto& var : vars)
      {
         if(std::get<0>(var) == INFINITE_UINT)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                          "       - var: (bool) from. " + HLS->Rliv->get_name(std::get<2>(var)) + " to " +
                              HLS->Rliv->get_name(std::get<3>(var)));
         }
         else if(std::get<0>(var) != 0)
         {
            PRINT_DBG_MEX(
                DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                "       - var: " +
                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(std::get<0>(var)) +
                    " of size " + STR(std::get<1>(var)) + " from. " + HLS->Rliv->get_name(std::get<2>(var)) + " to " +
                    HLS->Rliv->get_name(std::get<3>(var)));
         }
         else
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                          "       - size: " + STR(std::get<1>(var)) + " from. " +
                              HLS->Rliv->get_name(std::get<2>(var)) + " to " + HLS->Rliv->get_name(std::get<3>(var)));
         }

         var2obj[var] = src.first;
         var2src[var] = src.first;
         obj2var[src.first].push_back(var);
#ifndef NDEBUG
         if(check_sources.find(std::make_pair(std::get<2>(var), std::get<3>(var))) != check_sources.end() &&
            check_sources.find(std::make_pair(std::get<2>(var), std::get<3>(var)))->second != src.first)
         {
            THROW_ERROR("two different sources for the same transition: from. " +
                        HLS->Rliv->get_name(std::get<2>(var)) + " to " + HLS->Rliv->get_name(std::get<3>(var)) +
                        " source 1 " + src.first->get_string() + " source 2 " +
                        check_sources.find(std::make_pair(std::get<2>(var), std::get<3>(var)))->second->get_string());
         }
         else if(check_sources.find(std::make_pair(std::get<2>(var), std::get<3>(var))) == check_sources.end())
         {
            check_sources[std::make_pair(std::get<2>(var), std::get<3>(var))] = src.first;
         }
#endif
      }
      if(src.first->get_type() != generic_obj::REGISTER)
      {
         to_allocate.push_back(src.first);
      }
      else
      {
         to_allocate.push_front(src.first);
      }
   }

   std::string tgt_string = tgt->get_string() + "_" + STR(op);
   if(to_allocate.size() > 1)
   {
      unsigned int level = 0;
      std::map<unsigned int, unsigned int> level_map;
      do
      {
         /// all the inputs are connected with 2:1 multiplexers. Two inputs are taken at each time.
         generic_objRef first = to_allocate.front();
         to_allocate.pop_front();
         generic_objRef second = to_allocate.front();
         to_allocate.pop_front();

         if(GetPointer<mux_obj>(first))
         {
            level = GetPointer<mux_obj>(first)->get_level() + 1;
         }

         std::string mux_name =
             "MUX_" + STR(iteration) + "_" + tgt_string + "_" + STR(level) + "_" + STR(level_map[level]++);
         generic_objRef mux = generic_objRef(new mux_obj(first, second, level, mux_name, tgt));
         ++used_mux;
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GetPointer<mux_obj>(mux)->get_string());

         generic_objRef sel_port = HLS->Rconn->bind_selector_port(conn_binding::IN, commandport_obj::SELECTOR, mux, 0);
         GetPointer<mux_obj>(mux)->set_selector(sel_port);

         to_allocate.push_back(mux);

         std::list<data_transfer>::iterator v;

         /// stuff for the first input
         for(v = obj2var[first].begin(); v != obj2var[first].end(); ++v)
         {
            if(GetPointer<mux_obj>(var2obj[*v]))
            {
               GetPointer<mux_obj>(var2obj[*v])->set_target(mux);
            }

            var2obj[*v] = mux;
            obj2var[mux].push_back(*v);

            if(std::find(src_mux_tree[var2src[*v]].begin(), src_mux_tree[var2src[*v]].end(),
                         std::make_pair(mux, T_COND)) == src_mux_tree[var2src[*v]].end())
            {
               src_mux_tree[var2src[*v]].push_back(std::make_pair(mux, T_COND));
            }

            GetPointer<mux_obj>(mux)->add_bitsize(std::get<1>(*v));

            GetPointer<commandport_obj>(sel_port)->add_activation(
                commandport_obj::transition(std::get<2>(*v), std::get<3>(*v),
                                            commandport_obj::data_operation_pair(std::get<0>(*v), std::get<4>(*v))));
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                          "       - add mux activation for " + sel_port->get_string() + " from state "
                              << HLS->Rliv->get_name(std::get<2>(*v)) + " to state " +
                                     HLS->Rliv->get_name(std::get<3>(*v)));
         }
         /// stuff for the second input
         for(v = obj2var[second].begin(); v != obj2var[second].end(); ++v)
         {
            if(GetPointer<mux_obj>(var2obj[*v]))
            {
               GetPointer<mux_obj>(var2obj[*v])->set_target(mux);
            }

            var2obj[*v] = mux;
            obj2var[mux].push_back(*v);

            if(std::find(src_mux_tree[var2src[*v]].begin(), src_mux_tree[var2src[*v]].end(),
                         std::make_pair(mux, F_COND)) == src_mux_tree[var2src[*v]].end())
            {
               src_mux_tree[var2src[*v]].push_back(std::make_pair(mux, F_COND));
            }

            GetPointer<mux_obj>(mux)->add_bitsize(std::get<1>(*v));
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                          "       - FALSE input for " + sel_port->get_string() + " from state "
                              << HLS->Rliv->get_name(std::get<2>(*v)) + " to state " +
                                     HLS->Rliv->get_name(std::get<3>(*v)));
         }
      } while(to_allocate.size() > 1);

      /// specialize connections between sources and target
      for(const auto& src : srcs)
      {
         connection_objRef conn_obj = connection_objRef(new mux_conn(src.second, src_mux_tree[src.first]));
         HLS->Rconn->AddConnectionCB(src.first, tgt, op, port_index, conn_obj);
      }
   }
   else
   {
      THROW_ERROR("no mux to_allocate" + STR(to_allocate.size()));
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "");
   return used_mux - starting_value;
}

unsigned long long mux_connection_binding::object_bitsize(const tree_managerRef TreeM,
                                                          const HLS_manager::io_binding_type& obj) const
{
   const auto first = std::get<0>(obj);
   const auto second = std::get<1>(obj);
   if(first)
   {
      const auto type = tree_helper::CGetType(TreeM->GetTreeNode(first));
      const auto bus_addr_bitsize = HLSMgr->get_address_bitsize();

      if(tree_helper::IsArrayType(type) || tree_helper::IsStructType(type) ||
         tree_helper::IsUnionType(type) /*|| tree_helper::IsComplexType(type)*/)
      {
         return bus_addr_bitsize;
      }
      else
      {
         return tree_helper::Size(TreeM->GetTreeNode(first));
      }
   }
   else
   {
      if(second)
      {
         unsigned int count;
         for(count = 1; second >= (1u << count); ++count)
         {
            ;
         }
         return count + 1;
      }
      else
      {
         return 1;
      }
   }
}
