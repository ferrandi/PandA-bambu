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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */
#include "VcdSignalSelection.hpp"

#include "Discrepancy.hpp"
#include "HDL_manager.hpp"
#include "Parameter.hpp"
#include "allocation_information.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "cpu_time.hpp"
#include "design_flow_manager.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "generic_obj.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "language_writer.hpp"
#include "memory.hpp"
#include "reg_binding.hpp"
#include "storage_value_information.hpp"
#include "string_manipulation.hpp"
#include "structural_objects.hpp"
#include "technology_node.hpp"

VcdSignalSelection::VcdSignalSelection(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                       const DesignFlowManager& _design_flow_manager)
    : HLS_step(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::VCD_SIGNAL_SELECTION),
      TM(_HLSMgr->get_ir_manager()),
      Discr(HLSMgr->RDiscr),
      present_state_name("present_state")
{
   THROW_ASSERT(parameters->isOption(OPT_discrepancy) && parameters->getOption<bool>(OPT_discrepancy),
                "Step " + STR(__PRETTY_FUNCTION__) + " should not be added without discrepancy");
   THROW_ASSERT(HLSMgr->RDiscr, "Discr data structure is not correctly initialized");
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

bool VcdSignalSelection::IsAddressType(const ir_nodeRef& _tn) const
{
   const auto tn = ir_helper::CGetType(_tn);
   return ir_helper::IsPointerType(tn) || ir_helper::IsArrayEquivType(tn) ||
          (ir_helper::IsVectorType(tn) && ir_helper::IsPointerType(ir_helper::CGetElements(tn)));
}

HLS_step::HLSRelationships
VcdSignalSelection::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HW_PATH_COMPUTATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::CALL_GRAPH_UNFOLDING, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      case(PRECEDENCE_RELATIONSHIP):
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

void VcdSignalSelection::SelectInitialAddrParam(const CustomOrderedSet<unsigned int>& reached_body_fun_ids,
                                                CustomUnorderedMap<unsigned int, IRNodeSet>& address_parameters)
{
   for(const unsigned int fun_id : reached_body_fun_ids)
   {
      const auto fun_decl_node = TM->GetIRNode(fun_id);
      THROW_ASSERT(fun_decl_node->get_kind() == function_val_node_K,
                   fun_decl_node->ToString() + " is of kind " + ir_node::GetString(fun_decl_node->get_kind()));
      const auto fu_dec = GetPointer<const function_val_node>(fun_decl_node);
      for(const auto& parm_decl_node : fu_dec->list_of_args)
      {
         THROW_ASSERT(parm_decl_node->get_kind() == argument_val_node_K,
                      parm_decl_node->ToString() + " is of kind " + ir_node::GetString(parm_decl_node->get_kind()));
         if(IsAddressType(parm_decl_node))
         {
            address_parameters[fun_id].insert(parm_decl_node);
         }
      }
   }
}

void VcdSignalSelection::InitialSsaIsAddress(
    const ir_nodeConstRef& tn, const CustomUnorderedSet<unsigned int>& addr_fun_ids,
    const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& call_id_to_called_id)
{
   THROW_ASSERT(tn->get_kind() == assign_stmt_K, tn->ToString() + " is of kind " + ir_node::GetString(tn->get_kind()));
   const auto* g_as_node = GetPointer<const assign_stmt>(tn);
   /* check if the left value is an ssa_node_K */
   const auto assigned_ssa = g_as_node->op0;
   if(assigned_ssa->get_kind() != ssa_node_K)
   {
      return;
   }
   // complexes and reals are never marked as addresses
   if(ir_helper::IsRealType(assigned_ssa))
   {
      return;
   }

   const auto rhs = g_as_node->op1;
   const auto rhs_kind = rhs->get_kind();
   /*
    * If the user does not specify OPT_discrepancy_no_load_pointers, all the
    * values loaded from memory must be treated as if thy were addresses. This
    * happens because we don't know where these data come from. Even if the load
    * is from a memory used to store integers, we don't know if somobody used it
    * to store a pointer, so we must treat it as if it were an address.
    */
   if(rhs_kind == mem_access_node_K)
   {
      if(not parameters->isOption(OPT_discrepancy_no_load_pointers) or
         not parameters->getOption<bool>(OPT_discrepancy_no_load_pointers))
      {
         Discr->address_ssa.insert(assigned_ssa);
      }
   }

   /*
    * if this statements assign to the ssa the result of a function call, we must
    * check if the function returns a value that can be an address. if so, the
    * assigned ssa must be marked as address as well
    */
   if(rhs_kind == call_node_K)
   {
      /*
       * if the called function returns an address value also the ssa is marked
       */
      THROW_ASSERT(call_id_to_called_id.find(tn->index) != call_id_to_called_id.end(),
                   "call id " + STR(tn->index) + " does not call any function");
      for(unsigned int i : call_id_to_called_id.at(tn->index))
      {
         if(addr_fun_ids.find(i) != addr_fun_ids.end())
         {
            Discr->address_ssa.insert(assigned_ssa);
         }
      }
      return;
   }
   /*
    * check the ir_nodeRef for the right-hand side of the assignement. if it's
    * a cast of some pointer types, then the ssa is marked as address too
    */
   if(rhs_kind == nop_node_K || rhs_kind == bitcast_node_K)
   {
      const auto* ue = GetPointerS<const unary_node>(rhs);
      if(IsAddressType(ue->op))
      {
         Discr->address_ssa.insert(assigned_ssa);
      }
   }
}

void VcdSignalSelection::InitialPhiResIsAddress(const ir_nodeConstRef& tn)
{
   THROW_ASSERT(tn->get_kind() == phi_stmt_K, tn->ToString() + " is of kind " + ir_node::GetString(tn->get_kind()));
   const auto phi = GetPointer<const phi_stmt>(tn);
   auto phi_edges = phi->CGetDefEdgesList();
   const auto is_address = [&](const std::pair<ir_nodeRef, unsigned int>& p) -> bool {
      return (p.first->get_kind() == addr_node_K) || (Discr->address_ssa.find(p.first) != Discr->address_ssa.end());
   };
   if(std::find_if(phi_edges.begin(), phi_edges.end(), is_address) != phi_edges.end())
   {
      THROW_ASSERT(phi->res->get_kind() == ssa_node_K,
                   "phi node id: " + STR(tn->index) + " result node id: " + STR(phi->res->index) + "\n");
      Discr->address_ssa.insert(phi->res);
   }
}

void VcdSignalSelection::SelectInitialSsa(
    const CustomOrderedSet<unsigned int>& reached_body_fun_ids, const CustomUnorderedSet<unsigned int>& addr_fun_ids,
    const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& call_id_to_called_id)
{
   for(const auto fid : reached_body_fun_ids)
   {
      const auto FB = HLSMgr->CGetFunctionBehavior(fid);
      const auto op_graph = FB->GetOpGraph(FunctionBehavior::FCFG);
      const auto BH = FB->CGetBehavioralHelper();
      for(const auto vi : op_graph.vertices())
      {
         const auto st_tn_id = op_graph.CGetNodeInfo(vi).GetNodeId();
         if(st_tn_id == ENTRY_ID || st_tn_id == EXIT_ID)
         {
            continue;
         }
         THROW_ASSERT(st_tn_id, "operation vertex has id = 0");
         const auto curr_tn = TM->GetIRNode(st_tn_id);
         const auto assigned_ir_node_id = HLSMgr->get_produced_value(fid, vi);
         if(assigned_ir_node_id == 0)
         {
            continue;
         }
         const auto assigned_ssa_ir_node = TM->GetIRNode(assigned_ir_node_id);
         if(assigned_ssa_ir_node->get_kind() != ssa_node_K)
         {
            continue;
         }
         /*
          * if the ssa_node is never used it should be skipped. if we don't do this
          * the vcd signal selector will try to select the output signal. but if the
          * ssa is not used, the output wire is not placed by the interconnection and
          * the simulator dies when it tries to find it
          */
         const auto ssa = GetPointer<const ssa_node>(assigned_ssa_ir_node);
         if(ssa->CGetUseStmts().empty())
         {
            Discr->ssa_to_skip.insert(assigned_ssa_ir_node);
            if(IsAddressType(assigned_ssa_ir_node))
            {
               Discr->address_ssa.insert(assigned_ssa_ir_node);
            }
            continue;
         }
         /*
          * Never mark an ssa as address if it is a complex or floating point.
          * This assumption is kind of arbitrary because if somebody does some dirty
          * tricks with pointers it is possible to fool the checks in this way.
          * However, dirty pointer tricks with floating points and with complex values
          * are more rare than with integers, so for now I make this assumption.
          */
         if(ir_helper::IsRealType(assigned_ssa_ir_node))
         {
            continue;
         }
         /*
          * check the type of the ssa variable.
          */
         if(IsAddressType(assigned_ssa_ir_node))
         {
            Discr->address_ssa.insert(assigned_ssa_ir_node);
            continue;
         }
         if(curr_tn->get_kind() == assign_stmt_K)
         {
            InitialSsaIsAddress(curr_tn, addr_fun_ids, call_id_to_called_id);
         }
         else if(curr_tn->get_kind() == phi_stmt_K)
         {
            InitialPhiResIsAddress(curr_tn);
         }
      }
   }
}

void VcdSignalSelection::SingleStepPropagateParamToSsa(const IRNodeMap<size_t>& used_ssa,
                                                       const IRNodeSet& address_parameters)
{
   for(const auto& [ssa_ref, use_count] : used_ssa)
   {
      const auto ssa = GetPointer<const ssa_node>(ssa_ref);
      if(!ssa->var)
      {
         continue;
      }
      if(address_parameters.find(ssa->var) != address_parameters.end())
      {
         const auto def = ssa->GetDefStmt();
         if(def->get_kind() == nop_stmt_K)
         {
            Discr->address_ssa.insert(ssa_ref);
            Discr->ssa_to_skip.insert(ssa_ref);
         }
      }
   }
}

void VcdSignalSelection::PropagateAddrParamToSsa(const CustomUnorderedMap<unsigned int, IRNodeSet>& address_parameters,
                                                 const CustomOrderedSet<unsigned int>& reached_body_fun_ids)
{
   for(const auto fid : reached_body_fun_ids)
   {
      const auto addrp_it = address_parameters.find(fid);
      bool has_addr_param = (addrp_it != address_parameters.end());
      if(!has_addr_param)
      {
         continue;
      }
      const auto FB = HLSMgr->CGetFunctionBehavior(fid);
      const auto op_graph = FB->GetOpGraph(FunctionBehavior::FCFG);
      const auto BH = FB->CGetBehavioralHelper();
      for(const auto vi : op_graph.vertices())
      {
         const auto& op_info = op_graph.CGetNodeInfo(vi);
         const auto st_tn_id = op_info.GetNodeId();
         if(st_tn_id == ENTRY_ID || st_tn_id == EXIT_ID)
         {
            continue;
         }
         THROW_ASSERT(st_tn_id, "operation vertex has id = 0");
         const auto curr_tn = TM->GetIRNode(st_tn_id);
         if(curr_tn->get_kind() == assign_stmt_K)
         {
            const auto g_as_node = GetPointer<const assign_stmt>(curr_tn);
            const auto used_ssa = ir_helper::ComputeSsaUses(g_as_node->op1);
            SingleStepPropagateParamToSsa(used_ssa, addrp_it->second);
         }
         else
         {
            const auto used_ssa = ir_helper::ComputeSsaUses(op_info.node);
            SingleStepPropagateParamToSsa(used_ssa, addrp_it->second);
         }
      }
   }
}

void VcdSignalSelection::SingleStepPropagateAddrSsa(const ir_nodeRef& tn, IRNodeSet& new_address_ssa)
{
   if(tn->get_kind() == assign_stmt_K)
   {
      const auto g_as_node = GetPointer<const assign_stmt>(tn);
      /* check if the left value is an ssa_node_K */
      if(ir_helper::IsRealType(g_as_node->op0))
      {
         return;
      }
      if(g_as_node->op0->get_kind() == ssa_node_K)
      {
         const auto rhs_kind = g_as_node->op1->get_kind();

         const bool rhs_is_comparison = rhs_kind == lt_node_K || rhs_kind == le_node_K || rhs_kind == gt_node_K ||
                                        rhs_kind == ge_node_K || rhs_kind == eq_node_K || rhs_kind == ne_node_K;

         bool rhs_is_load_candidate = rhs_kind == unaligned_mem_access_node_K || rhs_kind == mem_access_node_K;

         if(rhs_kind == bitcast_node_K)
         {
            const auto bitcast_expr = GetPointer<const bitcast_node>(g_as_node->op1);
            const auto bitcast_kind = ir_helper::CGetType(bitcast_expr->op)->get_kind();
            if(bitcast_kind == struct_ty_node_K)
            {
               rhs_is_load_candidate = true;
            }
            if(bitcast_kind == array_ty_node_K && g_as_node->op0->get_kind() == vector_ty_node_K)
            {
               rhs_is_load_candidate = true;
            }
         }

         if(!rhs_is_comparison && !rhs_is_load_candidate && rhs_kind != call_node_K)
         {
            new_address_ssa.insert(g_as_node->op0);
         }
      }
   }
   else if(tn->get_kind() == phi_stmt_K)
   {
      const auto phi = GetPointer<const phi_stmt>(tn);
      const auto phi_edges = phi->CGetDefEdgesList();
      const auto is_address = [&](const phi_stmt::DefEdge& p) -> bool {
         return (p.first->get_kind() == addr_node_K) || (Discr->address_ssa.find(p.first) != Discr->address_ssa.end());
      };
      if(std::find_if(phi_edges.begin(), phi_edges.end(), is_address) != phi_edges.end())
      {
         THROW_ASSERT(phi->res->get_kind() == ssa_node_K,
                      "phi node id: " + STR(tn->index) + " result node id: " + STR(phi->res->index) + "\n");
         new_address_ssa.insert(phi->res);
      }
   }
}

void VcdSignalSelection::PropagateAddrSsa()
{
   size_t previous_address_ssa_size;
   do
   {
      previous_address_ssa_size = Discr->address_ssa.size();
      IRNodeSet new_address_ssa;
      for(const auto& addr : Discr->address_ssa)
      {
         THROW_ASSERT(addr->get_kind() == ssa_node_K,
                      addr->ToString() + " is of kind " + ir_node::GetString(addr->get_kind()));
         for(const auto& [ssa_node, use_count] : GetPointer<const ssa_node>(addr)->CGetUseStmts())
         {
            SingleStepPropagateAddrSsa(ssa_node, new_address_ssa);
         }
      }
      Discr->address_ssa.insert(new_address_ssa.begin(), new_address_ssa.end());
   } while(previous_address_ssa_size != Discr->address_ssa.size());
}

void VcdSignalSelection::DetectInvalidReturns(const CustomOrderedSet<unsigned int>& reached_body_functions,
                                              CustomUnorderedSet<unsigned int>& addr_fun_ids)
{
   for(const unsigned int i : reached_body_functions)
   {
      if(addr_fun_ids.find(i) != addr_fun_ids.end())
      {
         continue;
      }
      const auto FB = HLSMgr->CGetFunctionBehavior(i);
      const auto op_graph = FB->GetOpGraph(FunctionBehavior::FCFG);
      const auto exit_vertex = op_graph.CGetGraphInfo().exit_vertex;
      for(const auto& edge : op_graph.CGetInEdges(exit_vertex))
      {
         const auto op = op_graph.source(edge);
         const unsigned int node_id = op_graph.CGetNodeInfo(op).GetNodeId();
         if(node_id == ENTRY_ID or node_id == EXIT_ID)
         {
            continue;
         }
         const auto tn = TM->GetIRNode(node_id);
         if(tn->get_kind() == return_stmt_K)
         {
            const auto gr = GetPointer<const return_stmt>(tn);
            if((gr->op != nullptr) &&
               (IsAddressType(gr->op) ||
                ((gr->op->get_kind() == ssa_node_K) && (Discr->address_ssa.find(gr->op) != Discr->address_ssa.end()))))
            {
               addr_fun_ids.insert(i);
               break;
            }
         }
      }
   }
}

void VcdSignalSelection::InProcedurePropagateAddr(const CustomUnorderedMap<unsigned int, IRNodeSet>& address_parameters,
                                                  const CustomOrderedSet<unsigned int>& reached_body_functions,
                                                  CustomUnorderedSet<unsigned int>& addr_fun_ids)
{
   PropagateAddrParamToSsa(address_parameters, reached_body_functions);
   PropagateAddrSsa();
   DetectInvalidReturns(reached_body_functions, addr_fun_ids);
}

void VcdSignalSelection::CrossPropagateAddrSsa(
    CustomUnorderedMap<unsigned int, IRNodeSet>& address_parameters,
    const CustomOrderedSet<unsigned int>& reached_body_functions, const CustomUnorderedSet<unsigned int>& addr_fun_ids,
    const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& fu_id_to_call_ids,
    const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& call_id_to_called_id)
{
   for(const unsigned int caller_fun_id : reached_body_functions)
   {
      const auto FB = HLSMgr->CGetFunctionBehavior(caller_fun_id);
      const auto op_graph = FB->GetOpGraph(FunctionBehavior::FCFG);
      const auto BH = FB->CGetBehavioralHelper();
      THROW_ASSERT(fu_id_to_call_ids.find(caller_fun_id) != fu_id_to_call_ids.end(),
                   "caller_id = " + STR(caller_fun_id));
      for(const unsigned int callid : fu_id_to_call_ids.at(caller_fun_id))
      {
         THROW_ASSERT(call_id_to_called_id.find(callid) != call_id_to_called_id.end(), "callid = " + STR(callid));
         for(const unsigned int called_id : call_id_to_called_id.at(callid))
         {
            /*
             * if this an artificial call (it happens only for memcpy),
             * then we don't cross propagate inside
             */
            if(callid == 0)
            {
               THROW_ASSERT(HLSMgr->CGetFunctionBehavior(called_id)->CGetBehavioralHelper()->GetFunctionName() ==
                                MEMCPY,
                            "artificial calls to " +
                                HLSMgr->CGetFunctionBehavior(called_id)->CGetBehavioralHelper()->GetFunctionName() +
                                " should not happen");
               continue;
            }
            /*
             * Cross propagation downwards, from return statements of the called
             * function to the ssa of the caller.
             * If the return value of an address function is assigned to an ssa,
             * then the ssa itself must be marked as address
             */
            const auto call_node = TM->GetIRNode(callid);
            const auto called_BH = HLSMgr->CGetFunctionBehavior(called_id)->CGetBehavioralHelper();
            if(call_node->get_kind() == assign_stmt_K)
            {
               const auto g_as = GetPointer<const assign_stmt>(call_node);
               const auto ssa_node = g_as->op0;
               const auto rhs = g_as->op1;
               if(ssa_node->get_kind() == ssa_node_K && (rhs->get_kind() == call_node_K))
               {
                  /*
                   * if the called function returns an address value the ssa is an address
                   */
                  if(addr_fun_ids.find(called_id) != addr_fun_ids.end())
                  {
                     Discr->address_ssa.insert(ssa_node);
                     continue;
                  }
                  /*
                   * don't check the return values of the open() system call,
                   * because they are integers representing file descriptors and
                   * for this reason they cannot be compared to values in HDL
                   */
                  if(called_BH->GetFunctionName() == "open" && called_BH->is_operating_system_function(called_id))
                  {
                     Discr->ssa_to_skip.insert(ssa_node);
                  }
               }
            }

            if(not called_BH->has_implementation() or not called_BH->function_has_to_be_printed(called_id))
            {
               /*
                * also don't propagate inside functions without body
                */
               continue;
            }
            /*
             * Cross propagation downwards, from ssa of the caller function to the
             * parameters of the called function. If a function is called passing
             * as parameter an address ssa, then the parameter itself is marked as
             * representing address
             */
            if(op_graph.CGetGraphInfo().ir_node_to_operation.find(callid) ==
               op_graph.CGetGraphInfo().ir_node_to_operation.end())
            {
               THROW_WARNING("cannot find call for interprocedural address propagation:\n\t"
                             "caller id = " +
                             STR(caller_fun_id) +
                             "\n\t"
                             "caller name = " +
                             BH->GetFunctionName() +
                             "\n\t"
                             "called id = " +
                             STR(called_id) +
                             "\n\t"
                             "called name = " +
                             called_BH->GetFunctionName() +
                             "\n\t"
                             "call id " +
                             STR(callid) + "\ncall was probably removed by dead code elimination\n");
               return;
            }
            /*
             * retrieve the OpNodeInfo related to the IR node corresponding to
             * the call id
             */
            const auto& callopinfo = op_graph.CGetNodeInfo(op_graph.CGetGraphInfo().ir_node_to_operation.at(callid));
            if(callopinfo.called.size() == 0)
            {
               continue;
            }
            THROW_ASSERT(callopinfo.called.size() == 1,
                         "call id " + STR(callid) + " called.size() = " + STR(callopinfo.called.size()));
            /*
             * now we analyze two different function declarations.
             * called_fun_decl_node is the fun_decl of the function called
             * according to the call graph. directly_called_fun_decl_node is the
             * fun_decl of the function called according to the IR
             * representation. They may not be the same only in one case: when
             * there is an indirect call (through function pointers). In this
             * case the call graph has the correct information on the real
             * called function, while the IR representation has a call to
             * __builtin_wait_call().  we retrieve both the declaration to tell
             * the indirect call apart and propagate correctly the parameters
             */
            const unsigned int direct_called_id = *callopinfo.called.begin();
            const auto direct_called_fun_decl_node = TM->GetIRNode(direct_called_id);
#if HAVE_ASSERTS
            const auto* direct_fu_dec = GetPointer<const function_val_node>(direct_called_fun_decl_node);
#endif
            const auto called_fun_decl_node = TM->GetIRNode(called_id);
            const auto fu_dec = GetPointer<const function_val_node>(called_fun_decl_node);
            std::list<unsigned int>::const_iterator par_id_it, par_id_end;
            if(called_id == direct_called_id)
            {
               /* it's a direct call */
               THROW_ASSERT(called_BH->is_var_args() or
                                fu_dec->list_of_args.size() == callopinfo.actual_parameters.size() or
                                callopinfo.actual_parameters.empty(),
                            "fun decl " + STR(called_fun_decl_node->index) +
                                ", "
                                "call id " +
                                STR(callid) + ", called id " + STR(called_id) +
                                "\n"
                                "list_of_args.size() = " +
                                STR(fu_dec->list_of_args.size()) +
                                ", "
                                "actual_parameters.size() = " +
                                STR(callopinfo.actual_parameters.size()) + "\n");
               par_id_it = callopinfo.actual_parameters.cbegin();
               par_id_end = callopinfo.actual_parameters.cend();
            }
            else
            {
               /* it's indirect call */
               THROW_ASSERT(GetPointer<const identifier_node>(direct_fu_dec->name)->strg == BUILTIN_WAIT_CALL,
                            GetPointer<const identifier_node>(direct_fu_dec->name)->strg +
                                " called_id=" + STR(called_id) + " direct_called_id=" + STR(direct_called_id));
               THROW_ASSERT(callopinfo.actual_parameters.size() == fu_dec->list_of_args.size() + 2 or
                                callopinfo.actual_parameters.size() == fu_dec->list_of_args.size() + 3,
                            "fun decl " + STR(called_fun_decl_node->index) +
                                ", "
                                "call id " +
                                STR(callid) + ", called id " + STR(called_id) +
                                "\n"
                                "list_of_args.size() = " +
                                STR(fu_dec->list_of_args.size()) +
                                ", "
                                "actual_parameters.size() = " +
                                STR(callopinfo.actual_parameters.size()) + "\n");
               par_id_it = std::next(callopinfo.actual_parameters.cbegin(), 2);
               par_id_end = par_id_it;
               std::advance(par_id_end, fu_dec->list_of_args.size());
            }
            auto par_decl_it = fu_dec->list_of_args.cbegin();
            const auto par_decl_end = fu_dec->list_of_args.cend();
            for(; (par_id_it != par_id_end) && (par_decl_it != par_decl_end); ++par_id_it, ++par_decl_it)
            {
               const auto ssa_node = TM->GetIRNode(*par_id_it);
               if(ssa_node->get_kind() == ssa_node_K)
               {
                  if(Discr->address_ssa.find(ssa_node) != Discr->address_ssa.end())
                  {
                     /*
                      * the function called in callop was called with an argument
                      * representing an address. we have to propagate this
                      * information in the called  function
                      */
                     address_parameters[called_id].insert(*par_decl_it);
                  }
               }
            }
         }
      }
   }
}

void VcdSignalSelection::SelectAddrSsa(
    const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& fu_id_to_call_ids,
    const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& call_id_to_called_id)
{
   const auto& CGM = HLSMgr->CGetCallGraphManager();

   /*
    * initialize the set of fun_ids representing an address
    */
   CustomUnorderedSet<unsigned int> addr_fun_ids;
   const auto reached_body_fun_ids = CGM.GetReachedBodyFunctions();
   for(auto f_id : reached_body_fun_ids)
   {
      const auto return_type = ir_helper::GetFunctionReturnType(TM->GetIRNode(f_id));
      if(return_type && IsAddressType(TM->GetIRNode(return_type->index)))
      {
         addr_fun_ids.insert(f_id);
      }
   }

   /*
    * initialize the parameters representing addresses for every reached functions.
    */
   CustomUnorderedMap<unsigned int, IRNodeSet> address_parameters;
   SelectInitialAddrParam(reached_body_fun_ids, address_parameters);
   /*
    * get the initial ssa_node representing addresses for every function.
    * these are basically in the following cathegories:
    *    - variables representing addresses (pointers, arrays, vectors)
    *    - integer variables resulting from pointers casted to integer
    *    - ssa_node referred to a function's parameter marked as address
    *    - ssa_node to which is assigned the return value of a function returning an address
    * also marks ssa to skip if they are not used
    */
   SelectInitialSsa(reached_body_fun_ids, addr_fun_ids, call_id_to_called_id);
   /*
    * now we have the ssas representing addresses. we have to propagate the information to all the
    * other statements which use the ssa to skip, in the same function and across function calls
    */
   InProcedurePropagateAddr(address_parameters, reached_body_fun_ids, addr_fun_ids);

   size_t previous_address_ssa_n;
   do
   {
      previous_address_ssa_n = Discr->address_ssa.size();
      /*
       * now we have to propagate addresses across the functional units. this is
       * necessary because we can have for example a function taking an integer
       * argument wich is called passing to it a pointer casted to integer. if this
       * happens the InProcedurePropagateAddr is not enough.
       */
      CrossPropagateAddrSsa(address_parameters, reached_body_fun_ids, addr_fun_ids, fu_id_to_call_ids,
                            call_id_to_called_id);
      /*
       * then we have to propagate again internally to the procedures
       */
      InProcedurePropagateAddr(address_parameters, reached_body_fun_ids, addr_fun_ids);
   } while(previous_address_ssa_n != Discr->address_ssa.size());
}

void VcdSignalSelection::SelectInternalSignals(
    CustomUnorderedMap<unsigned int, UnorderedSetStdStable<std::string>>& fun_id_to_sig_names) const
{
   const auto& fun_with_body = HLSMgr->CGetCallGraphManager().GetReachedBodyFunctions();
   for(const unsigned int f_id : fun_with_body)
   {
      const auto FB = HLSMgr->CGetFunctionBehavior(f_id);
      /*
       * if the function is not printed in C there's no need to select signals
       * in vcd, because there will not be C assignments to compare them with
       */
      if(!FB->CGetBehavioralHelper()->has_implementation() ||
         !FB->CGetBehavioralHelper()->function_has_to_be_printed(f_id))
      {
         continue;
      }
      fun_id_to_sig_names[f_id];
      // get the opgraph and information on binding and allocation of this function
      const auto op_graph = FB->GetOpGraph(FunctionBehavior::FCFG);
      const auto& fu_bind = HLSMgr->get_HLS(f_id)->Rfu;
      const auto& alloc_info = HLSMgr->get_HLS(f_id)->allocation_information;
      // loop on the opgraph
      for(const auto v : op_graph.vertices())
      {
         const auto& op_info = op_graph.CGetNodeInfo(v);
         const auto op_node_id = op_info.GetNodeId();
         if(op_node_id == ENTRY_ID || op_node_id == EXIT_ID)
         {
            continue;
         }
         const auto assigned_ir_node_id = HLSMgr->get_produced_value(f_id, v);
         if(assigned_ir_node_id == 0)
         {
            continue;
         }
         const auto assigned_var_ir_node = TM->GetIRNode(assigned_ir_node_id);
         if(assigned_var_ir_node->get_kind() != ssa_node_K || Discr->ssa_to_skip.count(assigned_var_ir_node))
         {
            continue;
         }
         const auto op_node = TM->GetIRNode(op_node_id);
         if(op_node->get_kind() == assign_stmt_K)
         {
            const auto fu_type_id = fu_bind->get_assign(v);
            const auto fu_instance_id = fu_bind->get_index(v);
            auto to_select = "out_" + fu_bind->get_fu_name(v) + "_i" + STR(fu_bind->get_index(v)) + "_";

            if(alloc_info->is_direct_access_memory_unit(fu_type_id) && alloc_info->is_memory_unit(fu_type_id))
            {
               to_select += "array_" + STR(alloc_info->get_memory_var(fu_type_id)) + "_" +
                            STR(fu_bind->get_index(v) / alloc_info->get_number_channels(fu_type_id));
            }
            else if((alloc_info->is_direct_proxy_memory_unit(fu_type_id)) ||
                    alloc_info->is_indirect_access_memory_unit(fu_type_id))
            {
               to_select += fu_bind->get_fu_name(v) + "_i" +
                            STR(fu_bind->get_index(v) / alloc_info->get_number_channels(fu_type_id));
            }
            else if(alloc_info->is_proxy_wrapped_unit(fu_type_id))
            {
               const auto fu_unit_name = fu_bind->get_fu_name(v);
               if(starts_with(fu_unit_name, WRAPPED_PROXY_PREFIX))
               {
                  to_select += alloc_info->get_fu_name(fu_type_id).first.substr(sizeof(WRAPPED_PROXY_PREFIX) - 1);
               }
               else
               {
                  to_select += fu_unit_name;
               }
               to_select += "_instance";
            }
            else if(alloc_info->get_number_channels(fu_type_id) > 0)
            {
               to_select += fu_bind->get_fu_name(v) + "_i" +
                            STR(fu_bind->get_index(v) / alloc_info->get_number_channels(fu_type_id));
            }
            else if(fu_bind->get_operations(fu_type_id, fu_instance_id).size() == 1)
            {
               to_select += "fu_" + op_info.vertex_name;
            }
            else
            {
               to_select += fu_bind->get_fu_name(v) + "_i" + STR(fu_bind->get_index(v));
            }
            Discr->opid_to_outsignal[op_node_id] = HDL_manager::convert_to_identifier(to_select);
            fun_id_to_sig_names[f_id].insert(HDL_manager::convert_to_identifier(to_select));
         }
         else if(op_node->get_kind() == phi_stmt_K)
         {
            const auto phi = GetPointerS<const phi_stmt>(op_node);
            if(!phi->virtual_flag)
            {
               const auto& storage_val_info = HLSMgr->get_HLS(f_id)->storage_value_information;
               THROW_ASSERT(
                   storage_val_info->is_a_storage_value(0, assigned_ir_node_id, 0),
                   " variable " +
                       HLSMgr->CGetFunctionBehavior(f_id)->CGetBehavioralHelper()->PrintVariable(assigned_ir_node_id) +
                       " with IR node index " + STR(assigned_ir_node_id) + " has to be a storage value");
               const auto storage_index = storage_val_info->get_storage_value_index(0, assigned_ir_node_id, 0);
               const auto& regbind = HLSMgr->get_HLS(f_id)->Rreg;
               const auto reg_name = regbind->get(regbind->get_register(storage_index))->get_string();
               const auto reg_outsig_name = HDL_manager::convert_to_identifier("out_" + reg_name + "_" + reg_name);
               fun_id_to_sig_names[f_id].insert(reg_outsig_name);
               Discr->opid_to_outsignal[op_node_id] = reg_outsig_name;
               const auto reg_wrenable_sig_name = HDL_manager::convert_to_identifier("wrenable_" + reg_name);
               fun_id_to_sig_names[f_id].insert(reg_wrenable_sig_name);
            }
         }
      }
   }
}

DesignFlowStep_Status VcdSignalSelection::Exec()
{
   THROW_ASSERT(Discr, "Discr data structure is not correctly initialized");
   /* select the ssa representing addresses and ssa to skip in discrepancy analysis */
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Selecting discrepancy variables");
   SelectAddrSsa(Discr->call_sites_info.fu_id_to_call_ids, Discr->call_sites_info.call_id_to_called_id);
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--Selected discrepancy variables");
   /* Calculate the internal signal names for every function */
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Selecting internal signals in functions");
   CustomUnorderedMap<unsigned int, UnorderedSetStdStable<std::string>> fun_ids_to_local_sig_names;
   SelectInternalSignals(fun_ids_to_local_sig_names);
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--Selected internal signals in functions");
   // helper strings
   const std::string controller_str = "Controller_i" + STR(HIERARCHY_SEPARATOR);
   const std::string datapath_str = "Datapath_i" + STR(HIERARCHY_SEPARATOR);
   /* generate the scoped signal names and insert them in the selected signals*/
   for(const auto& [v, scope] : Discr->unfolded_v_to_scope)
   {
      const auto& node_info = Discr->DiscrepancyCallGraph.CGetNodeInfo(v);
      const auto f_id = node_info.f_id;
      const auto BH = node_info.behavior->CGetBehavioralHelper();
      if(!BH->has_implementation() || !BH->function_has_to_be_printed(f_id))
      {
         continue;
      }
      auto& datapath_scope = Discr->selected_vcd_signals[scope + datapath_str];
      auto& controller_scope = Discr->selected_vcd_signals[scope + controller_str];
      THROW_ASSERT(not controller_scope.empty() or datapath_scope.empty(),
                   "controller_scope size " + STR(controller_scope.size()) + " datapath_scope size " +
                       STR(datapath_scope.size()));
      if(not controller_scope.empty())
      { // this scope was already added by another vertex
         continue;
      }
      // select start, done and present_state signals in the fsm
      controller_scope.insert(START_PORT_NAME);
      controller_scope.insert(DONE_PORT_NAME);
      controller_scope.insert(present_state_name);
      // select the clock signal (only in the root function)
      if(v == Discr->unfolded_root_v)
      {
         controller_scope.insert(CLOCK_PORT_NAME);
      }
      /*
       * if the current function was not included with --discrepancy-only we're
       * done and we don't need to add the signals in the datapath.
       * the signals for the controller were necessary anyway because they are
       * used to reconstruct the sequence of events, the clock, and the duration
       * of the unbounded functions
       */
      if(parameters->isOption(OPT_discrepancy_only))
      {
         const auto discrepancy_functions = parameters->getOption<CustomSet<std::string>>(OPT_discrepancy_only);
         std::string fu_name = BH->GetFunctionName();
         if(not discrepancy_functions.empty() && discrepancy_functions.find(fu_name) == discrepancy_functions.end())
         {
            continue;
         }
      }
      // select local signals in the datapath
      THROW_ASSERT(fun_ids_to_local_sig_names.find(f_id) != fun_ids_to_local_sig_names.end(), "f_id = " + STR(f_id));
      for(const auto& local_sig_name : fun_ids_to_local_sig_names.at(f_id))
      {
         datapath_scope.insert(local_sig_name);
      }
   }
#ifndef NDEBUG
   if(debug_level > DEBUG_LEVEL_PEDANTIC)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Selected vcd signals");
      for(const auto& sig_scope : Discr->selected_vcd_signals)
      {
         for(const auto& sig_name : sig_scope.second)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "SIGNAL: " + sig_scope.first + sig_name);
         }
      }
   }
#endif

   std::size_t pointers_n = 0;
   std::size_t fully_resolved_n = 0;
   for(const auto& s : Discr->address_ssa)
   {
      const auto ssa = GetPointer<const ssa_node>(s);
      if(ssa && IsAddressType(s))
      {
         pointers_n++;
         const unsigned int ssa_base_index = ir_helper::GetBaseVariable(s)->index;
         if((ssa_base_index != 0 && HLSMgr->Rmem->has_base_address(ssa_base_index)) || ssa->use_set.is_fully_resolved())
         {
            fully_resolved_n++;
         }
      }
      else
      {
         continue;
      }
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                  "DISCREPANCY RESOLVED: " + STR(fully_resolved_n) + "/" + STR(pointers_n));
   return DesignFlowStep_Status::SUCCESS;
}

bool VcdSignalSelection::HasToBeExecuted() const
{
   return true;
}
