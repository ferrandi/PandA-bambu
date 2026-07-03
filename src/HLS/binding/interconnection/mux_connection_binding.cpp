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
 * @file mux_connection_binding.cpp
 * @brief Implementation of mux_connection_binding class
 *
 * Implementation of mux_connection_binding class. In this class all data-structures have been filled and
 * then datapath circuit is created.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "mux_connection_binding.hpp"

#include "CompilerWrapper.hpp"
#include "Parameter.hpp"
#include "allocation.hpp"
#include "allocation_information.hpp"
#include "behavioral_helper.hpp"
#include "commandport_obj.hpp"
#include "conn_binding.hpp"
#include "connection_obj.hpp"
#include "conv_conn_obj.hpp"
#include "cpu_time.hpp"
#include "dataport_obj.hpp"
#include "dbgPrintHelper.hpp"
#include "direct_conn.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "funit_obj.hpp"
#include "graph.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "liveVariables.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "memory_symbol.hpp"
#include "multi_unbounded_obj.hpp"
#include "mux_conn.hpp"
#include "mux_obj.hpp"
#include "op_graph.hpp"
#include "reg_binding.hpp"
#include "register_obj.hpp"
#include "storage_value_information.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "technology_node.hpp"
#include "utility.hpp"

#define USE_ALIGNMENT_INFO 1

namespace
{
   ir_nodeConstRef getCallPredicate(const ir_nodeConstRef& node)
   {
      if(const auto* gc = GetPointer<const call_stmt>(node))
      {
         return gc->predicate;
      }
      if(const auto* ga = GetPointer<const assign_stmt>(node))
      {
         if(ga->op1->get_kind() == call_node_K)
         {
            return ga->predicate;
         }
      }
      return {};
   }

   ir_nodeConstRef getDynamicCallPredicate(const ir_nodeConstRef& node)
   {
      const auto predicate = getCallPredicate(node);
      if(!predicate)
      {
         return {};
      }
      if(predicate->get_kind() == constant_int_val_node_K)
      {
         return {};
      }
      return predicate;
   }

   bool needsCommandPredicatePort(const hlsRef& HLS, const OpGraph& data, const gc_vertex_descriptor op)
   {
      const auto fu_id = HLS->Rfu->get_assign(op);
      const auto fu_type = HLS->allocation_information->get_fu(fu_id);
      const auto op_name = ir_helper::NormalizeTypename(data.CGetNodeInfo(op).GetOperation());
      const auto op_descr = GetPointer<functional_unit>(fu_type)->get_operation(op_name);
      const auto* operation_descr = GetPointer<operation>(op_descr);
      THROW_ASSERT(operation_descr, "Missing operation descriptor for " + data.CGetNodeInfo(op).vertex_name);
      if(!operation_descr->is_bounded())
      {
         return true;
      }
      const auto CM = GetPointer<functional_unit>(fu_type)->CM;
      if(!CM)
      {
         return false;
      }
      const auto top = CM->get_circ();
      if(!top)
      {
         return false;
      }
      const auto* fu_module = GetPointer<module_o>(top);
      THROW_ASSERT(fu_module, "expected");
      return fu_module->find_member(START_PORT_NAME, port_o_K, top) != nullptr;
   }
} // namespace

mux_connection_binding::mux_connection_binding(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                               unsigned int _funId, const DesignFlowManager& _design_flow_manager)
    : conn_binding_creator(_parameters, _HLSMgr, _funId, _design_flow_manager,
                           HLSFlowStep_Type::MUX_INTERCONNECTION_BINDING),
      id(0),
      cur_phi_ir_var(0)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

void mux_connection_binding::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->Rconn = conn_bindingRef();
   regs_in.clear();
   chained_in.clear();
   module_in.clear();
   id = 0;
   cur_phi_ir_var = 0;
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
                         HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->GetFunctionName() + ":");
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

void mux_connection_binding::create_single_conn(OpGraph::vertex_descriptor op, generic_objRef fu_obj_src,
                                                generic_objRef fu_obj, unsigned int port_num, unsigned int port_index,
                                                unsigned int ir_var, unsigned int precision, bool is_not_a_phi,
                                                FSMInfo::state_descriptor state_src,
                                                FSMInfo::state_descriptor state_tgt)
{
   HLS->Rconn->add_data_transfer(fu_obj_src, fu_obj, port_num, port_index,
                                 data_transfer(ir_var, precision, state_src, state_tgt, op));
   PRINT_DBG_MEX(
       DEBUG_LEVEL_PEDANTIC, debug_level,
       "       - add data transfer from " + fu_obj_src->get_string() + " to " + fu_obj->get_string() + " port " +
           std::to_string(port_num) + ":" + std::to_string(port_index) + " from state " +
           HLS->fsm_info->getState(state_src).name + " to state " +
           (state_tgt == FSMInfo::invalidState ? std::string("") : HLS->fsm_info->getState(state_tgt).name) +
           (ir_var ? (" for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(ir_var)) :
                     ""));
   if(GetPointer<register_obj>(fu_obj) && !is_not_a_phi)
   {
      generic_objRef enable_obj = GetPointer<register_obj>(fu_obj)->get_wr_enable();
      GetPointer<commandport_obj>(enable_obj)
          ->add_activation(commandport_obj::transition(
              state_src, state_tgt, commandport_obj::data_operation_pair(cur_phi_ir_var, OpGraph::null_vertex())));
      PRINT_DBG_MEX(
          DEBUG_LEVEL_PEDANTIC, debug_level,
          "       - write enable for " + fu_obj->get_string() + " from state " +
              HLS->fsm_info->getState(state_src).name + " to state " +
              (state_tgt == FSMInfo::invalidState ? std::string("") : HLS->fsm_info->getState(state_tgt).name));
   }
   else
   {
      THROW_ASSERT(!GetPointer<register_obj>(fu_obj), "unexpected condition");
   }
}

unsigned int mux_connection_binding::address_precision(unsigned int precision, OpGraph::vertex_descriptor op,
                                                       const OpGraph& data, const ir_managerRef IRM)
{
   auto fu_type = HLS->Rfu->get_assign(op);
   auto node_id = data.CGetNodeInfo(op).GetNodeId();
   const auto node = IRM->GetIRNode(node_id);
   const auto gm = GetPointer<const assign_stmt>(node);
   bool right_addr_node = false;
   if(gm && GetPointer<const addr_node>(gm->op1))
   {
      right_addr_node = true;
   }
   bool is_load_store = data.CGetNodeInfo(op).node_type & (TYPE_LOAD | TYPE_STORE);
   if(!right_addr_node && is_load_store && HLS->allocation_information->is_direct_access_memory_unit(fu_type))
   {
      unsigned var = HLS->allocation_information->is_memory_unit(fu_type) ?
                         HLS->allocation_information->get_memory_var(fu_type) :
                         HLS->allocation_information->get_proxy_memory_var(fu_type);
      if(var && HLSMgr->Rmem->is_private_memory(var))
      {
         unsigned long long int max_addr =
             HLSMgr->Rmem->get_base_address(var, HLS->functionId) + ir_helper::SizeAlloc(IRM->GetIRNode(var)) / 8;
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

bool mux_connection_binding::isConstantObj(unsigned int ir_node_index, const ir_managerRef IRM)
{
   if(ir_node_index == 0)
   {
      return true;
   }
   auto tn = IRM->GetIRNode(ir_node_index);
   if(GetPointer<constant_int_val_node>(tn))
   {
      return true;
   }
   else
   {
      return false;
   }
}

void mux_connection_binding::determine_connection(OpGraph::vertex_descriptor op,
                                                  const HLS_manager::io_binding_type& _var, generic_objRef fu_obj,
                                                  unsigned int port_num, unsigned int port_index, const OpGraph& data,
                                                  unsigned int precision, unsigned int alignment,
                                                  FSMInfo::state_descriptor state_src,
                                                  FSMInfo::state_descriptor state_tgt, unsigned src_phi_bb_index)
{
   bool is_not_a_phi = (data.CGetNodeInfo(op).node_type & TYPE_PHI) == 0;
   auto ir_var = std::get<0>(_var);
   unsigned long long int constant_value = std::get<1>(_var);
   auto bus_addr_bitsize = HLSMgr->get_address_bitsize();
   bus_addr_bitsize = std::min(precision, bus_addr_bitsize);
   memory_symbolRef m_sym;
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();
   bool is_signed = false;

   if(ir_var)
   {
      is_signed = BH->is_int(ir_var);
      const auto IRM = HLSMgr->get_ir_manager();
      auto tn = IRM->GetIRNode(ir_var);
      switch(tn->get_kind())
      {
         case addr_node_K:
         {
            auto* ae = GetPointer<addr_node>(tn);
            auto node_id = data.CGetNodeInfo(op).GetNodeId();
            const auto node = IRM->GetIRNode(node_id);
            auto* gm = GetPointer<const assign_stmt>(node);
            const auto type = ir_helper::CGetType(ae->op);
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
               const auto ref_var = ir_helper::GetBaseVariable(gm->op0);
               auto local_precision = bus_addr_bitsize;
               if(FB->is_variable_mem(ref_var->index))
               {
                  unsigned long long int max_addr = HLSMgr->Rmem->get_base_address(ref_var->index, HLS->functionId) +
                                                    ir_helper::SizeAlloc(ref_var) / 8;
                  for(local_precision = 1; max_addr > (1ull << local_precision); ++local_precision)
                  {
                     ;
                  }
               }
               determine_connection(op, HLS_manager::io_binding_type(ae->op->index, 0), fu_obj, port_num, port_index,
                                    data, local_precision, alignment, state_src, state_tgt, src_phi_bb_index);
            }
            else
            {
               determine_connection(op, HLS_manager::io_binding_type(ae->op->index, 0), fu_obj, port_num, port_index,
                                    data, precision, alignment, state_src, state_tgt, src_phi_bb_index);
            }
            return;
         }
         case mem_access_node_K:
         {
            auto* mr = GetPointer<mem_access_node>(tn);
            auto base_index = mr->op->index;
            generic_objRef current_operand;
            auto local_precision = address_precision(precision, op, data, IRM);
            determine_connection(op, HLS_manager::io_binding_type(base_index, 0), fu_obj, port_num, port_index, data,
                                 local_precision, alignment, state_src, state_tgt, src_phi_bb_index);
            return;
         }
         case constant_int_val_node_K:
         case constant_fp_val_node_K:
         case constant_vector_val_node_K:
         case ssa_node_K:
         case variable_val_node_K:
         {
            if(HLSMgr->Rmem->has_base_address(ir_var))
            {
               m_sym = HLSMgr->Rmem->get_symbol(ir_var, HLS->functionId);
               constant_value = HLSMgr->Rmem->get_base_address(ir_var, HLS->functionId);
               ir_var = 0;
               precision = bus_addr_bitsize;
            }
            // else a direct connection is considered
            break;
         }
         case function_val_node_K:
         {
            m_sym = HLSMgr->Rmem->get_symbol(ir_var, ir_var);
            constant_value = HLSMgr->Rmem->get_base_address(ir_var, ir_var);
            ir_var = 0;
            precision = bus_addr_bitsize;
            break;
         }
         case argument_val_node_K:
         case bitcast_node_K:
         case constructor_node_K:
         case unaligned_mem_access_node_K:
         case abs_node_K:
         case not_node_K:
         case fptoi_node_K:
         case itofp_node_K:
         case lut_node_K:
         case neg_node_K:
         case nop_node_K:
         case and_node_K:
         case or_node_K:
         case xor_node_K:
         case eq_node_K:
         case ge_node_K:
         case gt_node_K:
         case le_node_K:
         case shl_node_K:
         case lt_node_K:
         case max_node_K:
         case min_node_K:
         case sub_node_K:
         case mul_node_K:
         case ne_node_K:
         case add_node_K:
         case gep_node_K:
         case fdiv_node_K:
         case shr_node_K:
         case idiv_node_K:
         case irem_node_K:
         case widen_mul_node_K:
         case shufflevector_node_K:
         case field_val_node_K:
         case module_unit_node_K:
         case CASE_NODE_STMTS:
         case call_node_K:
         case CASE_FAKE_NODES:
         case identifier_node_K:
         case statement_list_node_K:
         case select_node_K:
         case ternary_add_node_K:
         case ternary_as_node_K:
         case ternary_sa_node_K:
         case ternary_ss_node_K:
         case fshl_node_K:
         case fshr_node_K:
         case concat_bit_node_K:
         case extract_bit_node_K:
         case add_sat_node_K:
         case sub_sat_node_K:
         case extractvalue_node_K:
         case insertvalue_node_K:
         case extractelement_node_K:
         case insertelement_node_K:
         case frem_node_K:
         case CASE_TYPE_NODES:
         default:
            THROW_ERROR("determine_connection pattern not supported: " + tn->get_kind_text() + " @" + STR(ir_var));
      }
   }
   if(ir_var == 0)
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
         /// parameters are expected to be defined as 32/64bit unsigned integers
         is_signed = false;
         precision = static_cast<unsigned>(CompilerWrapper::CGetPointerSize(parameters));
      }
      const auto C_obj = HLS->Rconn->get_constant_obj(string_value, param_name, precision, is_signed);
      create_single_conn(op, C_obj, fu_obj, port_num, port_index, 0, precision, is_not_a_phi, state_src, state_tgt);
      return;
   }
   if(BH->is_a_constant(ir_var))
   {
      THROW_ASSERT(precision, "a precision greater than 0 is expected: " + STR(precision));
      const auto C_value = HLSMgr->get_constant_string(ir_var, precision);
      const auto C_obj = HLS->Rconn->get_constant_obj(C_value, "", precision, is_signed);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - IR constant value: " + BH->PrintVariable(ir_var));
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "         - " + C_value);
      create_single_conn(op, C_obj, fu_obj, port_num, port_index, ir_var, precision, is_not_a_phi, state_src,
                         state_tgt);
      return;
   }
   connect_to_registers(op, data, fu_obj, port_num, port_index, ir_var, precision, is_not_a_phi, state_src, state_tgt,
                        src_phi_bb_index);
}

unsigned int mux_connection_binding::extract_parm_decl(unsigned int ir_var, const ir_managerRef IRM)
{
   unsigned int base_index;
   ir_nodeRef node = IRM->GetIRNode(ir_var);
   if(GetPointer<argument_val_node>(node))
   {
      base_index = ir_var;
   }
   else
   {
      auto* sn = GetPointer<ssa_node>(node);
      base_index = sn->var->index;
   }
   return base_index;
}

void mux_connection_binding::connect_to_registers(OpGraph::vertex_descriptor op, const OpGraph& data,
                                                  generic_objRef fu_obj, unsigned int port_num, unsigned int port_index,
                                                  unsigned int ir_var, unsigned long long precision,
                                                  const bool is_not_a_phi, FSMInfo::state_descriptor state_src,
                                                  FSMInfo::state_descriptor state_tgt, unsigned src_phi_bb_index)
{
   THROW_ASSERT(ir_var, "a non-null IR var is expected");
   const auto IRM = HLSMgr->get_ir_manager();
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();
   const auto state_tgt_name =
       state_tgt == FSMInfo::invalidState ? std::string("") : HLS->fsm_info->getState(state_tgt).name;
   bool is_param = ir_helper::IsParameter(IRM->GetIRNode(ir_var));

   if(is_param)
   {
      auto stepOp = FB->is_function_pipelined() ? HLS->fsm_info->GetStepOp(data, state_src, op) : 0;
      if(stepOp)
      {
         auto storage_value = HLS->storage_value_information->get_storage_value_index(state_src, ir_var, stepOp);
         auto r_index = HLS->Rreg->get_register(storage_value);
         auto reg_obj = HLS->Rreg->get(r_index);
         HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index,
                                       data_transfer(ir_var, precision, state_src, state_tgt, op));
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                       "       - add data transfer from2 " + reg_obj->get_string() + " to " + fu_obj->get_string() +
                           " port " + std::to_string(port_num) + ":" + std::to_string(port_index) + " from state " +
                           HLS->fsm_info->getState(state_src).name + " to state " + state_tgt_name + " for " +
                           HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(ir_var));
      }
      else
      {
         auto base_index = extract_parm_decl(ir_var, IRM);
         const generic_objRef fu_src_obj = input_ports[base_index];
         THROW_ASSERT(fu_src_obj, "unexpected condition");
         HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index,
                                       data_transfer(ir_var, precision, state_src, state_tgt, op));
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                       "       - add data transfer from primary input " + fu_src_obj->get_string() + " to " +
                           fu_obj->get_string() + " port " + std::to_string(port_num) + ":" +
                           std::to_string(port_index) + " from state " + HLS->fsm_info->getState(state_src).name +
                           " to state " + state_tgt_name + " for " +
                           HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(ir_var));
      }
   }

   generic_objRef reg_obj;
   if(!is_not_a_phi)
   {
      if(!is_param)
      {
         auto def_op = getDefOp(data, ir_var);
         const auto& def_op_ending_states = HLS->fsm_info->operationEndingStates.at(def_op);
         if((data.CGetNodeInfo(def_op).node_type & TYPE_PHI) == 0 &&
            def_op_ending_states.find(state_src) != def_op_ending_states.end())
         {
            const generic_objRef fu_src_obj = HLS->Rfu->get(def_op);
            HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index,
                                          data_transfer(ir_var, precision, state_src, state_tgt, op));
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                          "       - add data transfer from " + fu_src_obj->get_string() + " to " +
                              fu_obj->get_string() + " port " + std::to_string(port_num) + ":" +
                              std::to_string(port_index) + " from state " + HLS->fsm_info->getState(state_src).name +
                              " to state " + state_tgt_name + " for " +
                              HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(ir_var));
         }
         else
         {
            const auto& src_state_info = HLS->fsm_info->getState(state_src);
            auto src_state_BB_index = src_state_info.bbId;
            auto stepIn = HLS->fsm_info->GetStepPhiIn(data, op, ir_var, src_phi_bb_index, src_state_BB_index, state_src,
                                                      HLS->Rsch);
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                          "        step " + (ir_var ? "for variable " + BH->PrintVariable(ir_var) : "") + "=" +
                              STR(stepIn));
            if(HLS->storage_value_information->is_a_storage_value(state_src, ir_var, stepIn))
            {
               auto storage_value = HLS->storage_value_information->get_storage_value_index(state_src, ir_var, stepIn);
               auto r_index = HLS->Rreg->get_register(storage_value);
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - register: " + std::to_string(r_index) + " from " +
                                 HLS->fsm_info->getState(state_src).name + " to state " + state_tgt_name + " for " +
                                 HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(ir_var));
               reg_obj = HLS->Rreg->get(r_index);
               if(reg_obj != fu_obj)
               {
                  HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index,
                                                data_transfer(ir_var, precision, state_src, state_tgt, op));
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                "       - add data transfer from " + reg_obj->get_string() + " to " +
                                    fu_obj->get_string() + " port " + std::to_string(port_num) + ":" +
                                    std::to_string(port_index) + " from state " +
                                    HLS->fsm_info->getState(state_src).name + " to state " + state_tgt_name + " for " +
                                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(ir_var));
               }
            }
            else
            {
               THROW_ERROR("not expected from " + HLS->fsm_info->getState(state_src).name + " to " + state_tgt_name +
                           " " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(ir_var) +
                           " step=" + STR(stepIn));
            }
         }
      }
      if(GetPointer<register_obj>(fu_obj) && (!reg_obj || reg_obj != fu_obj))
      {
         generic_objRef enable_obj = GetPointer<register_obj>(fu_obj)->get_wr_enable();
         GetPointer<commandport_obj>(enable_obj)
             ->add_activation(commandport_obj::transition(
                 state_src, state_tgt, commandport_obj::data_operation_pair(ir_var, OpGraph::null_vertex())));
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                       "       - write enable for " + fu_obj->get_string() + " from state " +
                           HLS->fsm_info->getState(state_src).name + " to state " + state_tgt_name);
      }
   }
   else
   {
      if(!is_param)
      {
         auto def_op = getDefOp(data, ir_var);
         const auto& def_op_ending_states = HLS->fsm_info->operationEndingStates.at(def_op);
         if((data.CGetNodeInfo(def_op).node_type & TYPE_PHI) == 0)
         {
            bool same_stage = true;
            if(HLS->fsm_info->notSameStep(state_src, def_op, op))
            {
               same_stage = false;
            }
            if(def_op_ending_states.find(state_src) != def_op_ending_states.end() && same_stage)
            {
               const auto fu_src_obj = HLS->Rfu->get(def_op);
               HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index,
                                             data_transfer(ir_var, precision, state_src, state_tgt, op));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add data transfer from2 " + fu_src_obj->get_string() + " to " +
                                 fu_obj->get_string() + " port " + std::to_string(port_num) + ":" +
                                 std::to_string(port_index) + " from state " + HLS->fsm_info->getState(state_src).name +
                                 " to state " + state_tgt_name + " for " +
                                 HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(ir_var));
            }
            else
            {
               const bool ir_var_register_compatible = HLSMgr->is_register_compatible(ir_var);
               auto stepIn = HLS->fsm_info->GetStep(data, state_src, op, ir_var, true, ir_var_register_compatible);
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "        step " + (ir_var ? "for variable " + BH->PrintVariable(ir_var) : "") + "=" +
                                 STR(stepIn));
               if(HLS->storage_value_information->is_a_storage_value(state_src, ir_var, stepIn))
               {
                  auto storage_value =
                      HLS->storage_value_information->get_storage_value_index(state_src, ir_var, stepIn);
                  auto r_index = HLS->Rreg->get_register(storage_value);
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                "       - register: " + std::to_string(r_index) + " from " +
                                    HLS->fsm_info->getState(state_src).name + " to state " + state_tgt_name + " for " +
                                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(ir_var));
                  reg_obj = HLS->Rreg->get(r_index);
                  if(reg_obj != fu_obj)
                  {
                     HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index,
                                                   data_transfer(ir_var, precision, state_src, state_tgt, op));
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "       - add data transfer from " + reg_obj->get_string() + " to " + fu_obj->get_string() +
                             " port " + std::to_string(port_num) + ":" + std::to_string(port_index) + " from state " +
                             HLS->fsm_info->getState(state_src).name + " to state " + state_tgt_name + " for " +
                             HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(ir_var));
                  }
               }
               else
               {
                  THROW_UNREACHABLE("not expected from " + HLS->fsm_info->getState(state_src).name + " to " +
                                    state_tgt_name + " " + HLSMgr->get_ir_manager()->GetIRNode(ir_var)->ToString());
               }
            }
         }
         else
         {
            const bool ir_var_register_compatible = HLSMgr->is_register_compatible(ir_var);
            auto stepIn = HLS->fsm_info->GetStep(data, state_src, op, ir_var, true, ir_var_register_compatible);
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                          "        step " + (ir_var ? "for variable " + BH->PrintVariable(ir_var) : "") + "=" +
                              STR(stepIn));
            THROW_ASSERT(HLS->storage_value_information->is_a_storage_value(state_src, ir_var, stepIn),
                         "it has to be a register");
            auto storage_value = HLS->storage_value_information->get_storage_value_index(state_src, ir_var, stepIn);
            auto r_index = HLS->Rreg->get_register(storage_value);
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                          "       - register: " + std::to_string(r_index) + " from " +
                              HLS->fsm_info->getState(state_src).name + " to state " + state_tgt_name + " for " +
                              HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(ir_var));
            reg_obj = HLS->Rreg->get(r_index);
            if(reg_obj != fu_obj)
            {
               HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index,
                                             data_transfer(ir_var, precision, state_src, state_tgt, op));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add data transfer from " + reg_obj->get_string() + " to " +
                                 fu_obj->get_string() + " port " + std::to_string(port_num) + ":" +
                                 std::to_string(port_index) + " from state " + HLS->fsm_info->getState(state_src).name +
                                 " to state " + state_tgt_name + " for " +
                                 HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(ir_var));
            }
         }
      }
   }
}

void mux_connection_binding::connect_pipelined_registers(FSMInfo::state_descriptor state, const OpGraph& data)
{
   const auto& state_data = HLS->fsm_info->getState(state);
   if(state_data.isPipelinedState)
   {
      const ir_managerRef IRM = HLSMgr->get_ir_manager();
      const auto& in_vars = HLS->Rliv->getLiveInFsmVariables(state);
      const auto& out_vars = HLS->Rliv->getLiveOutFsmVariables(state);

      for(const auto& var : in_vars)
      {
         auto is_parameter = ir_helper::IsParameter(IRM->GetIRNode(var.first));
         // std::cerr << HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(var.first) << "\n";
         if(out_vars.find(std::make_pair(var.first, var.second + 1)) != out_vars.end())
         {
            auto origin_idx = HLS->storage_value_information->get_storage_value_index(state, var.first, var.second);
            auto def_op = is_parameter ? OpGraph::null_vertex() : getDefOp(data, var.first);

            for(const auto out_state : HLS->fsm_info->successors(state))
            {
               const auto& out_state_in_vars = HLS->Rliv->getLiveInFsmVariables(out_state);
               if(out_state_in_vars.find(std::make_pair(var.first, var.second + 1)) != out_state_in_vars.end())
               {
                  // std::cerr << "src state "
                  //           << " " << HLS->fsm_info->getState(state).name << " tgt state" <<
                  //           HLS->fsm_info->getState(out_state).name
                  //           << " step: " << var.second << "\n";
                  if(HLS->storage_value_information->is_a_storage_value(out_state, var.first, var.second + 1))
                  {
                     auto next_idx =
                         HLS->storage_value_information->get_storage_value_index(state, var.first, var.second + 1);
                     auto origin_reg_idx = HLS->Rreg->get_register(origin_idx);
                     auto next_reg_idx = HLS->Rreg->get_register(next_idx);
                     if(origin_reg_idx != next_reg_idx)
                     {
                        auto origin_reg = HLS->Rreg->get(origin_reg_idx);
                        auto next_reg = HLS->Rreg->get(next_reg_idx);

                        HLS->Rconn->add_data_transfer(
                            origin_reg, next_reg, 0, 0,
                            data_transfer(var.first, object_bitsize(IRM, HLS_manager::io_binding_type(var.first, 0)),
                                          state, out_state, def_op));
                        PRINT_DBG_MEX(
                            DEBUG_LEVEL_PEDANTIC, debug_level,
                            "    * Add pipelined register data transfer from " + origin_reg->get_string() + " to " +
                                next_reg->get_string() + " port " + std::to_string(0) + ":" + std::to_string(0) +
                                " from state " + HLS->fsm_info->getState(state).name + " to state " +
                                HLS->fsm_info->getState(out_state).name + " for " +
                                HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(var.first));
                        generic_objRef enable_obj = GetPointer<register_obj>(next_reg)->get_wr_enable();
                        GetPointer<commandport_obj>(enable_obj)
                            ->add_activation(commandport_obj::transition(
                                state, out_state,
                                commandport_obj::data_operation_pair(var.first, OpGraph::null_vertex())));
                        PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                      "       - write enable for " + next_reg->get_string() + " from " +
                                          HLS->fsm_info->getState(state).name + " to state " +
                                          HLS->fsm_info->getState(out_state).name);
                     }
                  }
               }
            }
         }
         if(is_parameter && var.second == 1)
         {
            for(const auto& in_state : HLS->fsm_info->predecessors(state))
            {
               const auto& in_state_out_vars = HLS->Rliv->getLiveOutFsmVariables(in_state);
               if(in_state_out_vars.find(std::make_pair(var.first, var.second)) != in_state_out_vars.end())
               {
                  if(HLS->storage_value_information->is_a_storage_value(state, var.first, var.second))
                  {
                     auto par_idx =
                         HLS->storage_value_information->get_storage_value_index(state, var.first, var.second);
                     auto par_reg_idx = HLS->Rreg->get_register(par_idx);
                     auto par_reg = HLS->Rreg->get(par_reg_idx);
                     auto base_index = extract_parm_decl(var.first, IRM);
                     const generic_objRef fu_src_obj = input_ports[base_index];
                     THROW_ASSERT(fu_src_obj, "unexpected condition");
                     HLS->Rconn->add_data_transfer(
                         fu_src_obj, par_reg, 0, 0,
                         data_transfer(var.first, object_bitsize(IRM, HLS_manager::io_binding_type(var.first, 0)),
                                       in_state, state, OpGraph::null_vertex()));
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "    * Add pipelined register data transfer from primary input " + fu_src_obj->get_string() +
                             " to " + par_reg->get_string() + " port 0:0 from state " +
                             HLS->fsm_info->getState(in_state).name + " to state " +
                             HLS->fsm_info->getState(state).name + " for " +
                             HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(var.first));
                     generic_objRef enable_obj = GetPointer<register_obj>(par_reg)->get_wr_enable();
                     GetPointer<commandport_obj>(enable_obj)
                         ->add_activation(commandport_obj::transition(
                             in_state, state, commandport_obj::data_operation_pair(var.first, OpGraph::null_vertex())));
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - write enable for " + par_reg->get_string() + " from " +
                                       HLS->fsm_info->getState(in_state).name + " to state " +
                                       HLS->fsm_info->getState(state).name);
                  }
               }
            }
         }
      }
   }
}

void mux_connection_binding::create_connections()
{
   const auto IRM = HLSMgr->get_ir_manager();
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();
   const auto data = FB->GetOpGraph(FunctionBehavior::FDFG);
   const auto bus_addr_bitsize = HLSMgr->get_address_bitsize();
   HLS->Rconn = conn_bindingRef(conn_binding::create_conn_binding(HLSMgr, HLS, BH, parameters));

   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Starting execution of interconnection binding");

   for(const auto& state2mu : HLS->fsm_info->getMuCtrls())
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

   for(const auto op : data.vertices())
   {
      const auto& node_info = data.CGetNodeInfo(op);
      /// check for required and produced values
      if(node_info.node_type & (TYPE_VPHI | TYPE_EXIT | TYPE_ENTRY))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                       "  * Skipping special operation: " + node_info.vertex_name +
                           " (node_id=" + STR(node_info.GetNodeId()) + ", node_type=" + STR(node_info.node_type) + ")");
         continue; /// virtual phis are skipped
      }
      const auto op_node_id = node_info.GetNodeId();
      const auto op_node = (op_node_id && op_node_id != INFINITE_UINT) ? IRM->GetIRNode(op_node_id) : ir_nodeConstRef{};
      const auto call_predicate =
          ((node_info.node_type & TYPE_EXTERNAL) && op_node) ? getDynamicCallPredicate(op_node) : ir_nodeConstRef{};
      auto fu = HLS->Rfu->get_assign(op);
      auto idx = HLS->Rfu->get_index(op);
      auto n_channels = HLS->allocation_information->get_number_channels(fu);

      if((node_info.node_type & TYPE_PHI) == 0)
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                       "  * Operation: " + node_info.vertex_name + " " + data.CGetNodeInfo(op).GetOperation());
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                       "     - FU: " + HLS->allocation_information->get_fu_name(fu).first);
#ifndef NDEBUG
         unsigned int index = 0;
#endif
         std::vector<HLS_manager::io_binding_type> var_read = HLSMgr->get_required_values(HLS->functionId, op);
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - " + std::to_string(var_read.size()) + " reads");
         for(auto& num : var_read)
         {
            if(std::get<0>(num) == 0)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "     - " + std::to_string(index) + ". Read: " + std::to_string(std::get<1>(num)));
            }
            else
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "     - " + std::to_string(index) + ". Read: " + BH->PrintVariable(std::get<0>(num)));
            }
#ifndef NDEBUG
            ++index;
#endif
         }
      }
      const auto& running_states = HLS->fsm_info->operationExecutingStates.at(op);
      for(const auto rstate : running_states)
      {
         /// phis are not considered
         if((node_info.node_type & TYPE_PHI) == 0)
         {
            unsigned int port_index = n_channels < 2 ? 0 : idx % n_channels;

            HLS->Rconn->bind_command_port(op, conn_binding::IN, commandport_obj::OPERATION, data);

            /// adding activation's state of selector related to operation op
            auto state_info = HLS->fsm_info->getState(rstate);
            const auto tmp_ops_node_size =
                GetPointer<functional_unit>(HLS->allocation_information->get_fu(fu))->get_operations().size();
            bool is_starting_operation =
                std::find(state_info.startingOperations.begin(), state_info.startingOperations.end(), op) !=
                state_info.startingOperations.end();

            if(call_predicate && is_starting_operation && needsCommandPredicatePort(HLS, data, op))
            {
               THROW_ASSERT(ir_helper::Size(call_predicate) == 1, call_predicate->ToString());
               auto predicate_port = HLS->Rconn->get_command_predicate(op);
               if(!predicate_port)
               {
                  predicate_port = generic_objRef(new dataport_obj("predicate_" + node_info.vertex_name, 1U, false));
                  HLS->Rconn->bind_command_predicate(op, predicate_port);
               }
               determine_connection(op, HLS_manager::io_binding_type(call_predicate->index, 0), predicate_port, 0, 0,
                                    data, 1U, 0U, rstate, FSMInfo::invalidState, 0U);
            }

            if(tmp_ops_node_size > 1U && (!(node_info.node_type & (TYPE_LOAD | TYPE_STORE)) || is_starting_operation))
            {
               if(!GetPointer<funit_obj>(HLS->Rfu->get(fu, idx)))
               {
                  THROW_ERROR("Functional unit " + HLS->allocation_information->get_string_name(fu) +
                              " does not have an instance " + std::to_string(idx));
               }
               const auto selector_obj =
                   GetPointer<funit_obj>(HLS->Rfu->get(fu, idx))->GetSelector_op(data.CGetNodeInfo(op).GetOperation());
               if(!selector_obj)
               {
                  THROW_ERROR("Functional unit " + HLS->allocation_information->get_string_name(fu) +
                              " does not exist or it does not have selector " + data.CGetNodeInfo(op).GetOperation() +
                              "(" + std::to_string(idx) +
                              ") Operation: " + std::to_string(data.CGetNodeInfo(op).GetNodeId()));
               }
               GetPointer<commandport_obj>(selector_obj)
                   ->add_activation(commandport_obj::transition(
                       rstate, FSMInfo::invalidState, commandport_obj::data_operation_pair(0, OpGraph::null_vertex())));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add activation for " + selector_obj->get_string() + " in state " +
                                 HLS->fsm_info->getState(rstate).name);
            }

            const generic_objRef fu_obj = HLS->Rfu->get(op);
            std::vector<HLS_manager::io_binding_type> var_read = HLSMgr->get_required_values(HLS->functionId, op);

            if(node_info.node_type & (TYPE_LOAD | TYPE_STORE))
            {
               auto node_id = data.CGetNodeInfo(op).GetNodeId();
               const ir_nodeRef node = IRM->GetIRNode(node_id);
               auto* gm = GetPointer<assign_stmt>(node);
               THROW_ASSERT(gm, "only assign_stmt's are allowed as memory operations");

               if(HLS->allocation_information->is_direct_access_memory_unit(fu) ||
                  HLS->allocation_information->is_indirect_access_memory_unit(fu)) /// MEMORY REFERENCES
               {
                  unsigned int alignment = 0;
                  ir_nodeRef var_node;
                  unsigned int size_var;
                  ir_nodeConstRef tn;
                  unsigned int var_node_idx;
                  unsigned long long Prec = 0;
                  const auto type = ir_helper::CGetType(gm->op0);
                  if(type && (type->get_kind() == integer_ty_node_K))
                  {
                     Prec = GetPointerS<const integer_ty_node>(type)->bitsize;
                  }
                  unsigned int algn = 0;
                  if(type && (type->get_kind() == integer_ty_node_K))
                  {
                     algn = GetPointerS<const integer_ty_node>(type)->algn;
                  }
#if USE_ALIGNMENT_INFO
                  if(type && GetPointer<const type_node>(type))
                  {
                     algn = alignment = GetPointerS<const type_node>(type)->algn;
                  }
#endif
                  if(node_info.node_type & TYPE_STORE)
                  {
                     size_var = std::get<0>(var_read[0]);
                     tn = ir_helper::CGetType(IRM->GetIRNode(size_var));
                     var_node = gm->op0;
                     var_node_idx = gm->op0->index;

                     if(size_var)
                     {
                        const auto IR_var_bitsize = GetPointerS<const type_node>(tn)->bitsizealloc;
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
                        if(ir_helper::IsSignedIntegerType(IRM->GetIRNode(size_var)))
                        {
                           auto key = std::make_tuple(var_bitsize, iu_conv, varObj);
                           if(connCache.find(key) == connCache.end())
                           {
                              conv_port =
                                  generic_objRef(new iu_conv_conn_obj("iu_conv_conn_obj_" + std::to_string(id++)));
                              if(isConstantObj(std::get<0>(varObj), IRM))
                              {
                                 connCache[key] = conv_port;
                              }
                              HLS->Rconn->add_sparse_logic(conv_port);
                              GetPointer<iu_conv_conn_obj>(conv_port)->add_bitsize(var_bitsize);
                              determine_connection(op, varObj, conv_port, 0, 0, data, var_bitsize, 0, rstate,
                                                   FSMInfo::invalidState, 0);
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
                              conv_port =
                                  generic_objRef(new uu_conv_conn_obj("uu_conv_conn_obj_" + std::to_string(id++)));
                              if(isConstantObj(std::get<0>(varObj), IRM))
                              {
                                 connCache[key] = conv_port;
                              }
                              HLS->Rconn->add_sparse_logic(conv_port);
                              GetPointer<uu_conv_conn_obj>(conv_port)->add_bitsize(var_bitsize);
                              determine_connection(op, varObj, conv_port, 0, 0, data, var_bitsize, 0, rstate,
                                                   FSMInfo::invalidState, 0);
                           }
                           else
                           {
                              conv_port = connCache.find(key)->second;
                           }
                        }
                        create_single_conn(op, conv_port, fu_obj, 0, port_index, size_var, var_bitsize, true, rstate,
                                           FSMInfo::invalidState);
                     }
                     else
                     {
                        auto prec = object_bitsize(IRM, var_read[0]);
                        HLS_manager::check_bitwidth(prec);
                        determine_connection(op, var_read[0], fu_obj, 0, port_index, data, static_cast<unsigned>(prec),
                                             0, rstate, FSMInfo::invalidState, 0);
                     }
                  }
                  else
                  {
                     size_var = HLSMgr->get_produced_value(HLS->functionId, op);
                     tn = ir_helper::CGetType(IRM->GetIRNode(size_var));
                     var_node = gm->op1;
                     var_node_idx = gm->op1->index;
                  }
                  auto is_dual = HLS->allocation_information->is_dual_port_memory(fu);
                  auto port_offset = [&](unsigned pi) -> unsigned int {
                     if(is_dual)
                     {
                        return (node_info.node_type & TYPE_LOAD) ? pi * 2 - 1 : pi * 2;
                     }
                     else
                     {
                        return pi;
                     }
                  };

                  THROW_ASSERT(!gm->predicate || ir_helper::Size(gm->predicate) == 1, gm->predicate->ToString());
                  auto var = gm->predicate ? HLS_manager::io_binding_type(gm->predicate->index, 0) :
                                             HLS_manager::io_binding_type(0, 1);
                  determine_connection(op, var, fu_obj, port_offset(3), port_index, data, 1, 0, rstate,
                                       FSMInfo::invalidState, 0);

                  THROW_ASSERT(var_node->get_kind() == mem_access_node_K,
                               "MEMORY REFERENCE/LOAD-STORE type not supported: " + var_node->get_kind_text() + " " +
                                   std::to_string(node_id));

                  determine_connection(op, HLS_manager::io_binding_type(var_node_idx, 0), fu_obj, port_offset(1),
                                       port_index, data, bus_addr_bitsize, alignment, rstate, FSMInfo::invalidState, 0);
                  const auto IR_var_bitsize = ceil_pow2(ir_helper::SizeAlloc(tn));
                  HLS_manager::check_bitwidth(IR_var_bitsize);
                  unsigned int var_bitsize;
                  var_bitsize = static_cast<unsigned int>(IR_var_bitsize);
                  determine_connection(
                      op, HLS_manager::io_binding_type(0, var_bitsize), fu_obj, port_offset(2), port_index, data,
                      static_cast<unsigned>(object_bitsize(IRM, HLS_manager::io_binding_type(0, var_bitsize))), 0,
                      rstate, FSMInfo::invalidState, 0);
               }
               else
               {
                  THROW_ERROR("Unit " + HLS->allocation_information->get_fu_name(fu).first + " not supported");
               }
            }
            else if(data.CGetNodeInfo(op).GetOperation() == MULTI_READ_COND)
            {
               for(unsigned int num = 0; num < var_read.size(); num++)
               {
                  auto prec = object_bitsize(IRM, var_read[num]);
                  HLS_manager::check_bitwidth(prec);
                  determine_connection(op, var_read[num], fu_obj, 0, num, data, static_cast<unsigned>(prec), 0, rstate,
                                       FSMInfo::invalidState, 0);
               }
            }
            else
            {
               ir_nodeConstRef first_valid;
               if(HLS->Rfu->get_ports_are_swapped(op))
               {
                  THROW_ASSERT(var_read.size() == 2, "unexpected condition");
                  std::swap(var_read[0], var_read[1]);
               }
               for(unsigned int port_num = 0; port_num < var_read.size(); port_num++)
               {
                  const auto ir_var = std::get<0>(var_read[port_num]);
                  const auto ir_var_node = ir_var == 0 ? nullptr : IRM->GetIRNode(ir_var);
                  const auto& node = data.CGetNodeInfo(op).node;
                  const auto form_par_type = ir_helper::GetFormalIth(node, port_num);
                  const auto OperationType = data.CGetNodeInfo(op).GetOperation();
                  if(ir_var && !first_valid)
                  {
                     first_valid = ir_var_node;
                  }
                  if((OperationType == "select_node") && port_num != 0 && ir_var)
                  {
                     first_valid = ir_var_node;
                  }

                  if(ir_var == 0)
                  {
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "     - " + std::to_string(port_num) +
                                       ". Read: " + std::to_string(std::get<1>(var_read[port_num])));
                  }
                  else
                  {
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "     - " + std::to_string(port_num) + ". Read: " + BH->PrintVariable(ir_var));
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "          * " + ir_var_node->get_kind_text());
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "          * bitsize " + std::to_string(object_bitsize(IRM, var_read[port_num])));
                  }
                  if(ir_var && HLSMgr->Rmem->is_actual_parm_loaded(ir_var))
                  {
                     THROW_ERROR("LOADING of actual parameter not yet implemented");
                  }
                  auto prec = object_bitsize(IRM, var_read[port_num]);
                  HLS_manager::check_bitwidth(prec);
                  determine_connection(op, var_read[port_num], fu_obj, port_num, port_index, data,
                                       static_cast<unsigned>(prec), 0, rstate, FSMInfo::invalidState, 0);
               }
            }
         }
      }
      const auto& ending_states = HLS->fsm_info->operationEndingStates.at(op);
      for(const auto estate : ending_states)
      {
         if(node_info.node_type & TYPE_PHI)
         {
            THROW_ASSERT(ending_states.size() == 1 || HLS->fsm_info->getState(*ending_states.begin()).isPipelinedState,
                         "phis cannot run in more than one state");
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "    * Ending Operation: " + node_info.vertex_name);
            /// phi must be differently managed
            auto var_written = HLSMgr->get_produced_value(HLS->functionId, op);
            const auto& state_info = HLS->fsm_info->getState(estate);
            const auto gp = GetPointer<const phi_stmt>(IRM->GetIRNode(data.CGetNodeInfo(op).GetNodeId()));
            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               auto phi_input_ir_var = def_edge.first->index;
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "Pre-Managing phi operation " + node_info.vertex_name + " ending in state " +
                                 HLS->fsm_info->getState(estate).name +
                                 (phi_input_ir_var ? " for variable " + def_edge.first->ToString() : "") + " from BB" +
                                 STR(def_edge.second));
               bool phi_pipelined_state = false;
               if(state_info.isPipelinedState)
               {
                  phi_pipelined_state = true;
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "      pipelined state");
               }
               if(phi_pipelined_state && HLS->fsm_info->getVariableSourceStates(estate, op, phi_input_ir_var).empty())
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "      pipelined state: edge not active");
                  continue;
               }

               cur_phi_ir_var = phi_input_ir_var;
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "Pre-Managing phi operation2 " + node_info.vertex_name + " ending in state " +
                                 HLS->fsm_info->getState(estate).name +
                                 (cur_phi_ir_var ? " for variable " + BH->PrintVariable(cur_phi_ir_var) : ""));
               THROW_ASSERT(cur_phi_ir_var, "something wrong happened");
               THROW_ASSERT(!HLSMgr->Rmem->has_base_address(phi_input_ir_var),
                            "phi cannot manage memory objects: @" + STR(phi_input_ir_var));
               THROW_ASSERT(!HLSMgr->Rmem->has_base_address(var_written),
                            "phi cannot manage memory objects: @" + STR(var_written));
               THROW_ASSERT(IRM->GetIRNode(phi_input_ir_var)->get_kind() != unaligned_mem_access_node_K,
                            "unexpected phi use");

               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "Managing phi operation " + node_info.vertex_name + " ending in state " +
                                 HLS->fsm_info->getState(estate).name +
                                 (cur_phi_ir_var ? " for variable " + BH->PrintVariable(cur_phi_ir_var) : ""));
               const bool var_written_register_compatible = HLSMgr->is_register_compatible(var_written);
               auto step =
                   HLS->fsm_info->GetStep(data, estate, op, var_written, false, var_written_register_compatible);
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "        step " + (var_written ? "for variable " + BH->PrintVariable(var_written) : "") +
                                 "=" + STR(step));
               if(HLS->storage_value_information->is_a_storage_value(estate, var_written, step))
               {
                  auto storage_value =
                      HLS->storage_value_information->get_storage_value_index(estate, var_written, step);
                  auto r_index = HLS->Rreg->get_register(storage_value);
                  auto in_bitsize = object_bitsize(IRM, HLS_manager::io_binding_type(phi_input_ir_var, 0));
                  HLS_manager::check_bitwidth(in_bitsize);
                  auto out_bitsize = object_bitsize(IRM, HLS_manager::io_binding_type(var_written, 0));
                  HLS_manager::check_bitwidth(out_bitsize);
                  generic_objRef tgt_reg_obj = HLS->Rreg->get(r_index);
                  THROW_ASSERT(ir_helper::IsSameType(IRM->GetIRNode(phi_input_ir_var), IRM->GetIRNode(var_written)),
                               "conversion required");

                  const auto& states_in = HLS->fsm_info->getVariableSourceStates(estate, op, phi_input_ir_var);
                  for(const auto stateIn : states_in)
                  {
                     const auto& stateIn_state_info = HLS->fsm_info->getState(stateIn);
                     if(stateIn_state_info.bbId != def_edge.second &&
                        !(stateIn_state_info.bbId == state_info.bbId && state_info.isPipelinedState))
                     {
                        continue;
                     }
                     /// with phi no conversion is needed
                     determine_connection(op, HLS_manager::io_binding_type(phi_input_ir_var, 0), tgt_reg_obj, 0, 0,
                                          data, static_cast<unsigned>(in_bitsize), 0, stateIn, estate, def_edge.second);
                  }
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "      target reg_" + STR(r_index));
               }
               cur_phi_ir_var = 0;
            }
         }
         else
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  * Ending Operation: " + node_info.vertex_name);
            HLS->Rconn->bind_command_port(op, conn_binding::IN, commandport_obj::OPERATION, data);

            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                          "     - FU: " + HLS->allocation_information->get_fu_name(HLS->Rfu->get_assign(op)).first);
            const generic_objRef fu_obj = HLS->Rfu->get(op);
            const auto var_written = HLSMgr->get_produced_value(HLS->functionId, op);
            if((node_info.node_type & TYPE_MULTIIF) != 0)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: (multi-way if value)");
               auto node_id = data.CGetNodeInfo(op).GetNodeId();
               std::vector<HLS_manager::io_binding_type> var_read = HLSMgr->get_required_values(HLS->functionId, op);
               generic_objRef TargetPort =
                   HLS->Rconn->bind_selector_port(conn_binding::OUT, commandport_obj::MULTIIF, op, data);

               HLS->Rconn->add_data_transfer(
                   fu_obj, TargetPort, 0, 0,
                   data_transfer(node_id, var_read.size(), estate, FSMInfo::invalidState, op));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add data transfer from " + fu_obj->get_string() + " to " +
                                 TargetPort->get_string() + " in state " + HLS->fsm_info->getState(estate).name +
                                 " for " + STR(node_id));
               GetPointer<commandport_obj>(TargetPort)
                   ->add_activation(commandport_obj::transition(
                       estate, FSMInfo::invalidState,
                       commandport_obj::data_operation_pair(node_id, OpGraph::null_vertex())));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add activation for " + TargetPort->get_string() + " in state " +
                                 HLS->fsm_info->getState(estate).name);
            }
            else if(var_written == 0)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: (no value produced)");
            }
            else
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: " + BH->PrintVariable(var_written));
               const auto& states_out = HLS->Rliv->getVariableDestinationStates(estate, op, var_written);
               if(states_out.empty())
               {
                  /// if the variable does not belong to the live-out set, it means that it is used inside
                  /// the state (chaining) and this situation is managed above
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - write in a data_transfer");
               }
               else
               {
                  for(auto s_out : states_out)
                  {
                     auto step_write = HLS->fsm_info->GetStepWrite(data, op);
                     auto storage_value =
                         HLS->storage_value_information->get_storage_value_index(s_out, var_written, step_write);
                     auto r_index = HLS->Rreg->get_register(storage_value);
                     auto tgt_reg_obj = HLS->Rreg->get(r_index);
                     HLS->Rconn->add_data_transfer(
                         fu_obj, tgt_reg_obj, 0, 0,
                         data_transfer(var_written, ir_helper::Size(IRM->GetIRNode(var_written)), estate, s_out, op));
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "       - add data transfer from " + fu_obj->get_string() + " to " +
                             tgt_reg_obj->get_string() + " from state " + HLS->fsm_info->getState(estate).name +
                             " to state " + HLS->fsm_info->getState(s_out).name + " for " +
                             HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(var_written));
                     auto enable_obj = GetPointer<register_obj>(tgt_reg_obj)->get_wr_enable();
                     auto is_VL_op = HLS->fsm_info->getState(estate).isDummy;
                     GetPointer<commandport_obj>(enable_obj)
                         ->add_activation(
                             commandport_obj::transition(estate, s_out,
                                                         commandport_obj::data_operation_pair(
                                                             var_written, is_VL_op ? op : OpGraph::null_vertex())));
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - write enable for " + tgt_reg_obj->get_string() + " from " +
                                       HLS->fsm_info->getState(estate).name + " to state " +
                                       HLS->fsm_info->getState(s_out).name);
                  }
               }
            }
         }
      }
   }
   const auto states = HLS->fsm_info->vertices();
   for(const auto vIt : states)
   {
      connect_pipelined_registers(vIt, data);
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
   const auto stateName = [this](FSMInfo::state_descriptor s) -> std::string {
      return s == FSMInfo::invalidState ? std::string("") : HLS->fsm_info->getState(s).name;
   };

   std::map<std::pair<FSMInfo::state_descriptor, FSMInfo::state_descriptor>, generic_objRef> check_sources;
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
                          "       - var: (bool) from. " + stateName(std::get<2>(var)) + " to " +
                              stateName(std::get<3>(var)));
         }
         else if(std::get<0>(var) != 0)
         {
            PRINT_DBG_MEX(
                DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                "       - var: " +
                    HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(std::get<0>(var)) +
                    " of size " + STR(std::get<1>(var)) + " from. " + stateName(std::get<2>(var)) + " to " +
                    stateName(std::get<3>(var)));
         }
         else
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                          "       - size: " + STR(std::get<1>(var)) + " from. " + stateName(std::get<2>(var)) + " to " +
                              stateName(std::get<3>(var)));
         }

         var2obj[var] = src.first;
         var2src[var] = src.first;
         obj2var[src.first].push_back(var);
#ifndef NDEBUG
         if(check_sources.find(std::make_pair(std::get<2>(var), std::get<3>(var))) != check_sources.end() &&
            check_sources.find(std::make_pair(std::get<2>(var), std::get<3>(var)))->second != src.first)
         {
            THROW_ERROR("two different sources for the same transition: from. " + stateName(std::get<2>(var)) + " to " +
                        stateName(std::get<3>(var)) + " source 1 " + src.first->get_string() + " source 2 " +
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
                         std::make_pair(mux, static_cast<unsigned int>(mux_conn::MUX_T_PORT))) ==
               src_mux_tree[var2src[*v]].end())
            {
               src_mux_tree[var2src[*v]].push_back(
                   std::make_pair(mux, static_cast<unsigned int>(mux_conn::MUX_T_PORT)));
            }

            GetPointer<mux_obj>(mux)->add_bitsize(std::get<1>(*v));

            GetPointer<commandport_obj>(sel_port)->add_activation(commandport_obj::transition(
                std::get<2>(*v), std::get<3>(*v),
                commandport_obj::data_operation_pair(std::get<0>(*v), OpGraph::null_vertex())));
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                          "       - add mux activation for " + sel_port->get_string() + " from state " +
                              stateName(std::get<2>(*v)) + " to state " + stateName(std::get<3>(*v)));
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
                         std::make_pair(mux, static_cast<unsigned int>(mux_conn::MUX_F_PORT))) ==
               src_mux_tree[var2src[*v]].end())
            {
               src_mux_tree[var2src[*v]].push_back(
                   std::make_pair(mux, static_cast<unsigned int>(mux_conn::MUX_F_PORT)));
            }

            GetPointer<mux_obj>(mux)->add_bitsize(std::get<1>(*v));
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                          "       - FALSE input for " + sel_port->get_string() + " from state " +
                              stateName(std::get<2>(*v)) + " to state " + stateName(std::get<3>(*v)));
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

unsigned long long mux_connection_binding::object_bitsize(const ir_managerRef IRM,
                                                          const HLS_manager::io_binding_type& obj) const
{
   const auto first = std::get<0>(obj);
   const auto second = std::get<1>(obj);
   if(first)
   {
      const auto tn = IRM->GetIRNode(first);
      const auto type = ir_helper::CGetType(tn);
      const auto bus_addr_bitsize = HLSMgr->get_address_bitsize();

      if(ir_helper::IsArrayType(type) || ir_helper::IsStructType(type))
      {
         return bus_addr_bitsize;
      }
      return ir_helper::Size(tn);
   }
   if(second)
   {
      unsigned int count;
      for(count = 1; second >= (1u << count); ++count)
      {
         ;
      }
      return count + 1;
   }
   return 1;
}
