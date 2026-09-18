/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file mem_dominator_allocation.cpp
 * @brief Memory allocation based on the dominator tree of the call graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "mem_dominator_allocation.hpp"

#include "Parameter.hpp"
#include "SemiNCADominance.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "cpu_time.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "function_behavior.hpp"
#include "functions.hpp"
#include "generic_device.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "kmp_bambu_names.h"
#include "math_function.hpp"
#include "memory.hpp"
#include "op_graph.hpp"
#include "polixml.hpp"
#include "string_manipulation.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "var_pp_functor.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"
#include <algorithm>
#include <filesystem>
#include <limits>
#include <list>
#include <string>
#include <utility>

#define VAL(str) #str
#define TOSTRING(str) VAL(str)

mem_dominator_allocation::mem_dominator_allocation(
    const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManager& _design_flow_manager,
    const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization, const HLSFlowStep_Type _hls_flow_step_type)
    : memory_allocation(_parameters, _HLSMgr, _design_flow_manager, _hls_flow_step_type, _hls_flow_step_specialization),
      user_defined_base_address(UINT64_MAX)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

bool mem_dominator_allocation::is_internal_obj(unsigned int var_id, unsigned int fun_id, bool multiple_top_call_graph)
{
   const auto TM = HLSMgr->get_ir_manager();
   const auto FB = HLSMgr->CGetFunctionBehavior(fun_id);
   const auto BH = FB->CGetBehavioralHelper();
   const auto var_name = BH->PrintVariable(var_id);
   const auto fun_name = BH->GetFunctionName();
   const auto memory_allocation_policy = FB->GetMemoryAllocationPolicy();

   bool is_internal = false;
   if(user_external_objects.count(fun_name) && user_external_objects.at(fun_name).count(var_name))
   {
      return false;
   }
   if(user_external_objects.count("*") && user_external_objects.at("*").count(var_name))
   {
      return false;
   }
   if(user_internal_objects.count(fun_name) && user_internal_objects.at(fun_name).count(var_name))
   {
      return true;
   }
   if(!multiple_top_call_graph)
   {
      const auto tn = TM->GetIRNode(var_id);
      switch(memory_allocation_policy)
      {
         case MemoryAllocation_Policy::GLSS:
         {
            const auto vd = GetPointer<const variable_val_node>(tn);
            if(vd)
            {
               is_internal = true;
            }
            if(HLSMgr->Rmem->is_parm_decl_copied(var_id) || HLSMgr->Rmem->is_parm_decl_stored(var_id))
            {
               is_internal = true;
            }
            break;
         }
         case MemoryAllocation_Policy::LSS:
         {
            const auto vd = GetPointer<const variable_val_node>(tn);
            if(vd && (vd->static_flag || (vd->parent && vd->parent->get_kind() != module_unit_node_K)))
            {
               is_internal = true;
            }
            if(HLSMgr->Rmem->is_parm_decl_copied(var_id) || HLSMgr->Rmem->is_parm_decl_stored(var_id))
            {
               is_internal = true;
            }
            break;
         }
         case MemoryAllocation_Policy::GSS:
         {
            const auto vd = GetPointer<const variable_val_node>(tn);
            if(vd && (vd->static_flag || !vd->parent || vd->parent->get_kind() == module_unit_node_K))
            {
               is_internal = true;
            }
            if(HLSMgr->Rmem->is_parm_decl_copied(var_id) || HLSMgr->Rmem->is_parm_decl_stored(var_id))
            {
               is_internal = true;
            }
            break;
         }
         case MemoryAllocation_Policy::ALL_BRAM:
         {
            is_internal = true;
            break;
         }
         case MemoryAllocation_Policy::EXT_PIPELINED_BRAM:
         case MemoryAllocation_Policy::NO_BRAM:
         {
            is_internal = false;
            break;
         }
         case MemoryAllocation_Policy::NONE:
         default:
            THROW_UNREACHABLE("not supported memory allocation policy");
      }
      // The address of the call site is internal.
      // We are using the address of the call site in the notification
      // mechanism of the hw call between accelerators.
      if(GetPointer<const call_stmt>(tn))
      {
         is_internal = true;
      }
   }
   return is_internal;
}

/// check if current_vertex is a proxied function
static CallGraph::vertex_descriptor get_remapped_vertex(CallGraph::vertex_descriptor current_vertex,
                                                        const CallGraphManager& CGM, const HLS_managerRef HLSMgr)
{
   const auto current_function_ID = CGM.get_function(current_vertex);
   const auto current_function_name = functions::GetFUName(current_function_ID, HLSMgr);
   if(HLSMgr->Rfuns->is_a_proxied_function(current_function_name))
   {
      return CGM.GetVertex(HLSMgr->Rfuns->get_proxy_mapping(current_function_name));
   }
   return current_vertex;
}

void mem_dominator_allocation::Initialize()
{
   if(parameters->isOption(OPT_xml_memory_allocation))
   {
      const auto XMLfilename = parameters->getOption<std::string>(OPT_xml_memory_allocation);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->parsing " + XMLfilename);
      XMLDomParser parser(XMLfilename);
      parser.Exec();
      if(parser)
      {
         const auto node = parser.get_document()->get_root_node(); // deleted by DomParser.
         const auto& list = node->get_children();
         for(const auto& l : list)
         {
            const auto child = GetPointer<xml_element>(l);
            if(!child)
            {
               continue;
            }
            if(child->get_name() == "memory_allocation")
            {
               auto base_address = UINT64_MAX;
               if(CE_XVM(base_address, child))
               {
                  LOAD_XVM(base_address, child);
               }
               user_defined_base_address = base_address;
               for(const auto& it : child->get_children())
               {
                  const auto mem_node = GetPointer<xml_element>(it);
                  if(!mem_node)
                  {
                     continue;
                  }
                  if(mem_node->get_name() == "object")
                  {
                     std::string is_internal;
                     if(!CE_XVM(is_internal, mem_node))
                     {
                        THROW_ERROR("expected the is_internal attribute");
                     }
                     LOAD_XVM(is_internal, mem_node);
                     if(is_internal == "T")
                     {
                        if(!CE_XVM(scope, mem_node))
                        {
                           THROW_ERROR("expected the scope attribute when the object is internal");
                        }
                        std::string scope;
                        LOAD_XVM(scope, mem_node);
                        if(!CE_XVM(name, mem_node))
                        {
                           THROW_ERROR("expected the name attribute");
                        }
                        std::string name;
                        LOAD_XVM(name, mem_node);
                        user_internal_objects[scope].insert(name);
                     }
                     else if(is_internal == "F")
                     {
                        std::string scope;
                        if(CE_XVM(scope, mem_node))
                        {
                           LOAD_XVM(scope, mem_node);
                        }
                        else
                        {
                           scope = "*";
                        }
                        if(!CE_XVM(name, mem_node))
                        {
                           THROW_ERROR("expected the name attribute");
                        }
                        std::string name;
                        LOAD_XVM(name, mem_node);
                        user_external_objects[scope].insert(name);
                     }
                     else
                     {
                        THROW_ERROR("unexpected value for is_internal attribute");
                     }
                  }
               }
            }
         }
         /// check xml consistency
         for(const auto& user_obj : user_internal_objects)
         {
            for(const auto& var_obj : user_obj.second)
            {
               if(user_external_objects.find(user_obj.first) != user_external_objects.end() &&
                  user_external_objects.find(user_obj.first)->second.find(var_obj) !=
                      user_external_objects.find(user_obj.first)->second.end())
               {
                  THROW_ERROR("An allocated object cannot be both internal and external: " + var_obj + " in function " +
                              user_obj.first);
               }
               if(user_external_objects.find("*") != user_external_objects.end() &&
                  user_external_objects.find("*")->second.find(var_obj) !=
                      user_external_objects.find("*")->second.end())
               {
                  THROW_ERROR("An allocated object cannot be both internal and external: " + var_obj + " in function " +
                              user_obj.first);
               }
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parsed file " + XMLfilename);
   }
}

DesignFlowStep_Status mem_dominator_allocation::InternalExec()
{
   /// For better understanding of containers' structure
   using func_id_t = unsigned int;
   using top_id_t = func_id_t;
   using var_id_t = unsigned int;

   long int step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM && output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      START_TIME(step_time);
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Memory allocation information:");
   const auto TM = HLSMgr->get_ir_manager();
   const auto HLS_D = HLSMgr->get_HLS_device();

   const auto initial_internal_address_p = parameters->isOption(OPT_initial_internal_address);
   const auto initial_internal_address = initial_internal_address_p ?
                                             parameters->getOption<unsigned int>(OPT_initial_internal_address) :
                                             std::numeric_limits<unsigned int>::max();
   const auto unaligned_access_p =
       parameters->isOption(OPT_unaligned_access) && parameters->getOption<bool>(OPT_unaligned_access);
   const auto assume_aligned_access_p =
       parameters->isOption(OPT_aligned_access) && parameters->getOption<bool>(OPT_aligned_access);
   const auto max_bram =
       HLSMgr->GetParameterFromParameterOrDeviceOrDefault<unsigned int>("BRAM_bitsize_max", HLS_D, 0U);
   /// TODO: to be fixed with information coming out from the target platform description
   HLSMgr->base_address = user_defined_base_address != UINT64_MAX ?
                              user_defined_base_address :
                              parameters->getOption<unsigned long long int>(OPT_base_address);
   const auto null_pointer_check = [&]() {
      if(parameters->isOption(OPT_cc_optimizations))
      {
         const auto cc_parameters = parameters->getOption<CustomSet<std::string>>(OPT_cc_optimizations);
         if(cc_parameters.find("no-delete-null-pointer-checks") != cc_parameters.end())
         {
            return false;
         }
      }
      return true;
   }();
   /// information about memory allocation to be shared across the functions
   const auto prevRmem = std::move(HLSMgr->Rmem);
   HLSMgr->Rmem =
       memory::create_memory(parameters, TM, HLSMgr->base_address, max_bram, null_pointer_check,
                             initial_internal_address_p, initial_internal_address, HLSMgr->Rget_address_bitsize());
   setup_memory_allocation();

   const auto& CGM = HLSMgr->CGetCallGraphManager();
   /// the analysis has to be performed only on the reachable functions
   CustomMap<top_id_t, CallGraph> cg;
   CustomMap<top_id_t, std::unique_ptr<dominance<CallGraph>>> cg_dominators;
   OrderedMapStd<top_id_t, CustomUnorderedSet<CallGraph::vertex_descriptor>> reachable_vertices;
   const auto top_cmp = [&](const unsigned int& a, const unsigned int& b) -> bool {
      const auto a_omp = CGM.IsOMPLambdaFunction(a);
      const auto b_omp = CGM.IsOMPLambdaFunction(b);
      if(a_omp ^ b_omp)
      {
         return b_omp;
      }
      return a < b;
   };
   std::set<func_id_t, decltype(top_cmp)> top_functions(top_cmp);
   CustomMap<top_id_t, std::vector<func_id_t>> function_allocation_order;
   CustomMap<top_id_t, std::vector<func_id_t>> omp_allocation_order;

   const auto& root_functions = CGM.GetRootFunctions();
   const auto omp_functions = CGM.GetOMPLambdaFunctions();
   const auto CG = CGM.GetCallGraph();
   top_functions.insert(root_functions.begin(), root_functions.end());
   const auto subgraph_from = [&](CallGraph::vertex_descriptor top_vertex, bool is_addr = false,
                                  bool include_shared = true, bool stop_at_fork = false) {
      const auto top_function = CGM.get_function(top_vertex);
      CustomUnorderedSet<CallGraph::vertex_descriptor> preset;
      preset.insert(top_vertex);
      const auto reached_from_top = CGM.GetReachedFunctionsFrom(top_function);
      const auto is_dominated = [&](CallGraph::vertex_descriptor v) {
         for(const auto& e : CG.in_edges(v))
         {
            if(!reached_from_top.count(CGM.get_function(CG.source(e))))
            {
               return false;
            }
         }
         return true;
      };
      for(const auto& funID : reached_from_top)
      {
         if(!top_functions.count(funID))
         {
            const auto funV = CGM.GetVertex(funID);
            if((include_shared || is_dominated(funV)) &&
               (!stop_at_fork ||
                !boost::starts_with(ir_helper::GetFunctionName(TM->GetIRNode(funID)), TOSTRING(KMP_FORK_CALL))))
            {
               preset.insert(funV);
            }
         }
      }
      const auto presub = CGM.CGetCallSubGraph(preset);
      CustomUnorderedSet<CallGraph::vertex_descriptor> subset;
      subset.insert(top_vertex);
      const auto update_vertices = !is_addr && !omp_functions.count(top_function);
      if(update_vertices)
      {
         reachable_vertices[top_function].insert(top_vertex);
      }
      for(const auto& v : preset)
      {
         if(presub.IsReachable(top_vertex, v))
         {
            subset.insert(v);
            if(update_vertices)
            {
               reachable_vertices[top_function].insert(v);
            }
         }
      }
      return CGM.CGetCallSubGraph(subset);
   };
   const auto push_allocation_order = [&](std::vector<func_id_t>& allocation_order, top_id_t top_function,
                                          bool include_shared = true, bool stop_at_fork = false) {
      const auto top_vertex = CGM.GetVertex(top_function);
      cg.emplace(top_function, subgraph_from(top_vertex, false, include_shared, stop_at_fork));
      const auto& subgraph = cg.at(top_function);
      /// we do not need the exit vertex since the post-dominator graph is not used
      cg_dominators[top_function] =
          std::make_unique<dominance<CallGraph>>(subgraph, top_vertex, CallGraph::null_vertex());
      std::list<CallGraph::vertex_descriptor> sorted;
      subgraph.TopologicalSort(sorted);
      for(const auto& v : sorted)
      {
         allocation_order.push_back(CGM.get_function(v));
      }
   };
   for(const auto& top_function : root_functions)
   {
      push_allocation_order(function_allocation_order[top_function], top_function);
   }
   for(const auto& addr_func : CGM.GetAddressedFunctions())
   {
      const auto top_vertex = CGM.GetVertex(addr_func);
      cg.emplace(addr_func, subgraph_from(top_vertex, true));
      const auto& subgraph = cg.at(addr_func);
      std::list<CallGraph::vertex_descriptor> sorted;
      subgraph.TopologicalSort(sorted);
      for(const auto& v : sorted)
      {
         const auto v_id = CGM.get_function(v);
         if(!std::count_if(function_allocation_order.begin(), function_allocation_order.end(),
                           [&](const std::pair<top_id_t, std::vector<func_id_t>>& p) {
                              return std::count(p.second.begin(), p.second.end(), v_id);
                           }))
         {
            function_allocation_order[addr_func].push_back(v_id);
         }
      }
   }
   for(const auto& omp_function : omp_functions)
   {
      auto& allocation_order = omp_allocation_order[omp_function];
      push_allocation_order(allocation_order, omp_function, false, true);
   }

#if HAVE_ASSERTS
   for(const auto& f_order : function_allocation_order)
   {
      CustomSet<unsigned int> check_order;
      for(const auto& f : f_order.second)
      {
         const auto fname = HLSMgr->GetFunctionBehavior(f)->CGetBehavioralHelper()->GetFunctionName();
         THROW_ASSERT(func_list.count(f), "Function " + fname + " not present in function list");
         THROW_ASSERT(check_order.insert(f).second, "Duplicate fuction in allocation order: " + STR(f) + " " + fname);
      }
   }
   for(const auto& f_order : omp_allocation_order)
   {
      CustomSet<unsigned int> check_order;
      for(const auto& f : f_order.second)
      {
         const auto fname = HLSMgr->GetFunctionBehavior(f)->CGetBehavioralHelper()->GetFunctionName();
         THROW_ASSERT(func_list.count(f), "Function " + fname + " not present in function list");
         THROW_ASSERT(check_order.insert(f).second, "Duplicate fuction in allocation order: " + STR(f) + " " + fname);
      }
   }
#endif

   CustomMap<top_id_t, CustomMap<var_id_t, CustomOrderedSet<CallGraph::vertex_descriptor>>> var_map;
   CustomMap<top_id_t, CustomMap<var_id_t, CustomOrderedSet<func_id_t>>> where_used;
   const auto compute_var_usage = [&](const func_id_t f_id, const top_id_t top_id) {
      const auto function_behavior = HLSMgr->CGetFunctionBehavior(f_id);
      const auto BH = function_behavior->CGetBehavioralHelper();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Analyzing function: " + BH->GetFunctionName());
      const auto& dom_tree = *cg_dominators.at(top_id);
      const auto current_vertex = get_remapped_vertex(CGM.GetVertex(f_id), CGM, HLSMgr);

      auto current_dom = current_vertex;
      const auto projection_in_degree = cg.at(top_id).in_degree(current_vertex);
      if(projection_in_degree != 1)
      {
         current_dom = get_remapped_vertex(dom_tree.getImmediateDominator(current_vertex), CGM, HLSMgr);
      }

      const auto& function_mem = function_behavior->get_function_mem();
      for(const auto v : function_mem)
      {
         if(function_behavior->is_a_state_variable(v))
         {
            var_map[top_id][v].insert(current_dom);
         }
         else
         {
            var_map[top_id][v].insert(current_vertex);
         }
         where_used[top_id][v].insert(f_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Variable : " + BH->PrintVariable(v) + " used in function " +
                            function_behavior->CGetBehavioralHelper()->GetFunctionName());
         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dominator Vertex: " +
         // HLSMgr->CGetFunctionBehavior(CG->get_function(vert_dominator))->CGetBehavioralHelper()->GetFunctionName()
         // + " - Variable to be stored: " + BH->PrintVariable(*v));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Analyzed function: " + BH->GetFunctionName());
   };
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Variable use analysis...");
   for(const auto& top_order : function_allocation_order)
   {
      const auto& top_id = top_order.first;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                     "-->Analyzing top function: " +
                         HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetFunctionName());
      for(const auto& f_id : top_order.second)
      {
         compute_var_usage(f_id, top_id);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                     "<--Analyzed top function: " +
                         HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetFunctionName());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Variable use analysis completed");

   bool all_pointers_resolved = true;
   CustomMap<var_id_t, unsigned long long> var_size;
   CustomMap<var_id_t, CustomMap<func_id_t, CustomOrderedSet<CallGraph::vertex_descriptor>>> var_referring_vertex_map;
   CustomMap<var_id_t, CustomMap<func_id_t, CustomOrderedSet<CallGraph::vertex_descriptor>>> var_load_vertex_map;
   CustomMap<var_id_t, CustomOrderedSet<func_id_t>> concrete_store_functions;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Pointers classification...");
   for(const auto& fun_id : func_list)
   {
      const auto function_behavior = HLSMgr->CGetFunctionBehavior(fun_id);
      const auto BH = function_behavior->CGetBehavioralHelper();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Analyzing function: " + BH->GetFunctionName());
      //      if(function_behavior->get_has_globals())
      //      {
      //         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Pointers not resolved: it has global variables");
      //         all_pointers_resolved = false;
      //      }
      if(function_behavior->get_has_undefined_function_receiving_pointers())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                        "---Pointers not resolved: it has undefined function receiving pointers");
         all_pointers_resolved = false;
      }

      const auto g = function_behavior->GetOpGraph(FunctionBehavior::CFG);
      for(const auto v : g.vertices())
      {
         const auto op_info = g.CGetNodeInfo(v);
         const auto current_op = op_info.GetOperation();
         /// custom function like printf may create problem to the pointer resolution
         if(current_op == "__builtin_printf" || current_op == BUILTIN_WAIT_CALL || current_op == MEMSET ||
            current_op == MEMCMP || current_op == MEMCPY)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                           "---Pointers not resolved: it uses printf/builtin-wait-call/memset/memcpy/memcmp");
            all_pointers_resolved = false;
         }
         if(op_info.node_type & (TYPE_LOAD | TYPE_STORE))
         {
            const auto curr_tn = TM->GetIRNode(op_info.GetNodeId());
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "-->Analyzing statement " + op_info.vertex_name + ": " + STR(curr_tn));
            THROW_ASSERT(curr_tn->get_kind() == assign_stmt_K, "only assign_stmt's are allowed as memory operations");
            const auto me = GetPointerS<const assign_stmt>(curr_tn);
            const auto& ptr_node = op_info.node_type & TYPE_STORE ? me->op0 : me->op1;
            const auto& pts = ir_helper::GetPointToSet(ptr_node);
            std::vector<ir_nodeRef> res_set = pts.variables;
            if(!pts.is_fully_resolved())
            {
               const auto var = ir_helper::GetBaseVariable(ptr_node);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                              "---var:" + (var ? STR(var->index) : std::string("unknown")));
               if(var && function_behavior->is_variable_mem(var->index))
               {
                  if(std::find(res_set.begin(), res_set.end(), var) == res_set.end())
                  {
                     res_set.push_back(var);
                  }
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                                 "---Pointers not resolved: point-to-set not resolved " + me->ToString());
                  all_pointers_resolved = false;
               }
            }
            for(const auto& var_node : res_set)
            {
               const auto var = var_node->index;
               if(op_info.node_type & TYPE_STORE)
               {
                  concrete_store_functions[var].insert(fun_id);
               }
               if(HLSMgr->Rmem->has_sds_var(var) && !HLSMgr->Rmem->is_sds_var(var))
               {
                  continue;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable is " + STR(var));
               const auto var_ptr_node = op_info.node_type & TYPE_STORE ? me->op0 : me->op1;
               auto alignment = 8ULL;
               if(var_ptr_node->get_kind() == mem_access_node_K)
               {
                  const auto mr = GetPointerS<const mem_access_node>(var_ptr_node);
                  if(mr->op->get_kind() == ssa_node_K)
                  {
                     const auto ssa_addr = GetPointerS<const ssa_node>(mr->op);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---" + std::string(op_info.node_type & TYPE_STORE ? "STORE" : "LOAD") +
                                        " SSA pointer " + mr->op->ToString() + " bit_values=" + ssa_addr->bit_values);
                     if(ssa_addr->bit_values.find_first_not_of('0') == std::string::npos)
                     {
                        alignment <<= 60; // infinite alignment
                     }
                     else
                     {
                        for(auto it = ssa_addr->bit_values.rbegin(); it != ssa_addr->bit_values.rend(); ++it)
                        {
                           if(*it == '0' || *it == 'X')
                           {
                              alignment <<= 1;
                           }
                           else
                           {
                              break;
                           }
                        }
                     }
                  }
                  else
                  {
                     THROW_ERROR("unexpected condition");
                  }
               }
               else
               {
                  THROW_ERROR("unexpected condition" + var_ptr_node->get_kind_text() + " " + me->ToString());
               }
               const auto size_var = [&]() {
                  if(op_info.node_type & TYPE_STORE)
                  {
                     const auto var_read = HLSMgr->get_required_values(fun_id, v);
                     return std::get<0>(var_read[0]);
                  }
                  return HLSMgr->get_produced_value(fun_id, v);
               }();
               const auto size_type = ir_helper::CGetType(TM->GetIRNode(size_var));
               auto value_bitsize = ir_helper::SizeAlloc(size_type);
               const auto fd = GetPointer<const field_val_node>(size_type);
               if(!fd || !fd->bitfield)
               {
                  value_bitsize = std::max(8ull, value_bitsize);
               }
               if(op_info.node_type & TYPE_STORE)
               {
                  HLSMgr->Rmem->add_source_value(var, size_var);
               }

               if(var_size.find(var) == var_size.end())
               {
                  const auto type_node = ir_helper::CGetType(TM->GetIRNode(var));
                  const auto is_a_struct_union =
                      ((ir_helper::IsStructType(type_node)) && !ir_helper::IsArrayEquivType(type_node));
                  if(unaligned_access_p)
                  {
                     if(assume_aligned_access_p)
                     {
                        THROW_ERROR("Option --aligned-access have been specified on a function with unaligned "
                                    "accesses:\n\tVariable " +
                                    BH->PrintVariable(var) + " could be accessed in unaligned way");
                     }
                     HLSMgr->Rmem->set_sds_var(var, false, 0);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Variable " + STR(var) + " not sds because unaligned_access option specified");
                  }
                  else if(is_a_struct_union)
                  {
                     if(assume_aligned_access_p)
                     {
                        THROW_ERROR("Option --aligned-access have been specified on a function with unaligned access "
                                    ":\n\tVariable " +
                                    BH->PrintVariable(var) + " could be accessed in unaligned way");
                     }
                     HLSMgr->Rmem->set_sds_var(var, false, 0);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Variable " + STR(var) + " could be accessed in unaligned way");
                  }
                  else if(alignment < value_bitsize)
                  {
                     if(assume_aligned_access_p)
                     {
                        THROW_WARNING("Option --aligned-access have been specified on a function with not "
                                      "compiler-proved unaligned accesses:\n\tVariable " +
                                      BH->PrintVariable(var) + " could be accessed in unaligned way");
                        THROW_WARNING("\tStatement is " + me->ToString());
                     }
                     else
                     {
                        HLSMgr->Rmem->set_sds_var(var, false, 0);
                        INDENT_DBG_MEX(
                            DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                            "---Variable " + STR(var) + " not sds because alignment=" + STR(alignment) +
                                " is less than the value loaded or written or than the size of the array elements=" +
                                STR(value_bitsize));
                     }
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Variable " + STR(var) + " sds " + STR(value_bitsize));
                     HLSMgr->Rmem->set_sds_var(var, true, value_bitsize);
                  }
                  var_size[var] = value_bitsize;
               }
               else
               {
                  if(var_size.at(var) != value_bitsize)
                  {
                     if(assume_aligned_access_p)
                     {
                        THROW_ERROR("Option --aligned-access have been specified on a function with unaligned "
                                    "accesses:\n\tVariable " +
                                    BH->PrintVariable(var) + " could be accessed in unaligned way");
                     }
                     HLSMgr->Rmem->set_sds_var(var, false, 0);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Variable " + STR(var) + " not sds " + STR(value_bitsize) + " vs " +
                                        STR(var_size.at(var)));
                  }
                  else if(alignment < value_bitsize)
                  {
                     if(assume_aligned_access_p)
                     {
                        THROW_WARNING("Option --aligned-access have been specified on a function with not "
                                      "compiler-proved unaligned accesses:\n\tVariable " +
                                      BH->PrintVariable(var) + " could be accessed in unaligned way");
                        THROW_WARNING("\tStatement is " + me->ToString());
                     }
                     else
                     {
                        HLSMgr->Rmem->set_sds_var(var, false, 0);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Variable " + STR(var) + " not sds " + STR(value_bitsize) + " vs " +
                                           STR(var_size.at(var)) + " alignment=" + STR(alignment) +
                                           " value_bitsize2=" + STR(var_size.at(var)));
                     }
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Variable " + STR(var) + " sds " + STR(value_bitsize));
                  }
               }
               /// var referring vertex map
               var_referring_vertex_map[var][fun_id].insert(v);
               if(op_info.node_type & TYPE_LOAD)
               {
                  var_load_vertex_map[var][fun_id].insert(v);
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement " + op_info.vertex_name);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Analyzed function: " + BH->GetFunctionName());
   }
   HLSMgr->Rmem->set_all_pointers_resolved(all_pointers_resolved);

   if(all_pointers_resolved)
   {
      for(const auto fun_id : func_list)
      {
         const auto function_behavior = HLSMgr->CGetFunctionBehavior(fun_id);
         const auto BH = function_behavior->CGetBehavioralHelper();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Analyzing function: " + BH->GetFunctionName());
         const auto g = function_behavior->GetOpGraph(FunctionBehavior::CFG);
         for(const auto v : g.vertices())
         {
            const auto op_info = g.CGetNodeInfo(v);
            if(op_info.node_type & (TYPE_LOAD | TYPE_STORE))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing statement " + op_info.vertex_name);
               const auto curr_tn = TM->GetIRNode(op_info.GetNodeId());
               THROW_ASSERT(curr_tn->get_kind() == assign_stmt_K, "only assign_stmt is allowed as memory operation");
               const auto me = GetPointerS<const assign_stmt>(curr_tn);
               const auto& ptr_node = op_info.node_type & TYPE_STORE ? me->op0 : me->op1;
               const auto& pts = ir_helper::GetPointToSet(ptr_node);
               std::vector<ir_nodeRef> used_set = pts.variables;
               if(!pts.is_fully_resolved())
               {
                  const auto var = ir_helper::GetBaseVariable(ptr_node);
                  if(std::find(used_set.begin(), used_set.end(), var) == used_set.end())
                  {
                     used_set.push_back(var);
                  }
               }
               if(used_set.size() > 1)
               {
                  for(const auto& var : used_set)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Variable needs the bus for loads and stores " + BH->PrintVariable(var->index));
                     HLSMgr->Rmem->add_need_bus(var->index);
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement " + op_info.vertex_name);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Analyzed function: " + BH->GetFunctionName());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Pointers classification completed");

   HLSMgr->clear_ding_dong_buffer_candidates();
   HLSMgr->clear_dataflow_read_only_memory_candidates();
   const auto is_dataflow_top_function = [&](const unsigned int function_id) {
      const auto function_name = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->GetFunctionName();
      const auto func_arch = HLSMgr->module_arch->GetArchitecture(function_name);
      return func_arch && func_arch->attrs.count(FunctionArchitecture::func_dataflow_top) &&
             func_arch->attrs.at(FunctionArchitecture::func_dataflow_top) == "1";
   };
   const auto is_dataflow_module_function = [&](const unsigned int function_id) {
      const auto function_name = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->GetFunctionName();
      const auto func_arch = HLSMgr->module_arch->GetArchitecture(function_name);
      return func_arch && func_arch->attrs.count(FunctionArchitecture::func_dataflow_module) &&
             func_arch->attrs.at(FunctionArchitecture::func_dataflow_module) == "1";
   };
   const auto get_dataflow_bundle_names = [&](const unsigned int top_id, const unsigned int var_id,
                                              const CustomOrderedSet<unsigned int>& dataflow_functions) {
      std::set<std::string> bundle_names;
      const auto expected_prefix = "DF_bambu_" + STR(top_id) + "_" + STR(var_id) + "FO";
      for(const auto function_id : dataflow_functions)
      {
         const auto function_name =
             HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->GetFunctionName();
         const auto func_arch = HLSMgr->module_arch->GetArchitecture(function_name);
         if(!func_arch)
         {
            continue;
         }
         for(const auto& [_, parm_attrs] : func_arch->parms)
         {
            const auto bundle_it = parm_attrs.find(FunctionArchitecture::parm_bundle);
            if(bundle_it == parm_attrs.end() || !starts_with(bundle_it->second, expected_prefix))
            {
               continue;
            }
            const auto iface_it = func_arch->ifaces.find(bundle_it->second);
            if(iface_it == func_arch->ifaces.end())
            {
               continue;
            }
            const auto mode_it = iface_it->second.find(FunctionArchitecture::iface_mode);
            if(mode_it != iface_it->second.end() && (mode_it->second == "array" || mode_it->second == "default"))
            {
               bundle_names.insert(bundle_it->second);
            }
         }
      }
      return bundle_names;
   };

   const auto has_dynamic_address_use = [&](const unsigned int top_id, const unsigned int var_id) {
      const auto top_uses_it = where_used.find(top_id);
      if(top_uses_it == where_used.end())
      {
         return false;
      }
      const auto users_it = top_uses_it->second.find(var_id);
      if(users_it == top_uses_it->second.end())
      {
         return false;
      }
      const auto& users = users_it->second;
      return std::any_of(users.begin(), users.end(), [&](const auto function_id) {
         return HLSMgr->CGetFunctionBehavior(function_id)->get_dynamic_address().count(var_id);
      });
   };

   const auto is_initialized_read_only_dataflow_memory = [&](const unsigned int top_id, const unsigned int var_id) {
      // determine_memory_accesses conservatively records non-const pointer address uses as writes.  Record
      // only concrete TYPE_STORE operations here; interface directions still classify dataflow writers.
      (void)top_id;
      const auto vd = GetPointer<const variable_val_node>(TM->GetIRNode(var_id));
      return vd && vd->init && !concrete_store_functions.count(var_id);
   };

   // DATAFLOW modules can be promoted to separate roots.  Recover their concrete shared objects by walking the direct
   // call sites reachable from each DATAFLOW top, rather than looking for the callee formal in var_map.
   CustomMap<top_id_t, CustomOrderedSet<func_id_t>> dataflow_reachable_functions;
   CustomMap<top_id_t, CustomOrderedSet<var_id_t>> dataflow_reachable_objects;
   const auto& full_call_graph = CGM.GetCallGraph();
   for(const auto& [top_id, _] : function_allocation_order)
   {
      if(!is_dataflow_top_function(top_id))
      {
         continue;
      }
      CustomOrderedSet<CallGraph::vertex_descriptor> reached_vertices;
      reached_vertices.insert(CGM.GetVertex(top_id));
      bool changed = true;
      while(changed)
      {
         changed = false;
         for(const auto edge : full_call_graph.edges())
         {
            const auto caller_vertex = full_call_graph.source(edge);
            if(!reached_vertices.count(caller_vertex))
            {
               continue;
            }
            const auto& edge_info = full_call_graph.CGetEdgeInfo(edge);
            if(edge_info.direct_call_points.empty())
            {
               continue;
            }
            const auto callee_vertex = full_call_graph.target(edge);
            changed |= reached_vertices.insert(callee_vertex).second;
            for(const auto call_point : edge_info.direct_call_points)
            {
               const auto call_statement = TM->GetIRNode(call_point);
               std::vector<ir_nodeRef> actual_parameters;
               if(call_statement->get_kind() == assign_stmt_K)
               {
                  const auto assignment = GetPointerS<const assign_stmt>(call_statement);
                  if(assignment->op1->get_kind() != call_node_K)
                  {
                     continue;
                  }
                  actual_parameters = GetPointerS<const call_node>(assignment->op1)->args;
               }
               else if(call_statement->get_kind() == call_stmt_K)
               {
                  actual_parameters = GetPointerS<const call_stmt>(call_statement)->args;
               }
               else
               {
                  continue;
               }
               for(const auto& actual : actual_parameters)
               {
                  const auto base_actual = ir_helper::GetBaseVariable(actual);
                  if(base_actual && GetPointer<const variable_val_node>(TM->GetIRNode(base_actual->index)))
                  {
                     dataflow_reachable_objects[top_id].insert(base_actual->index);
                  }
               }
            }
         }
      }
      for(const auto vertex : reached_vertices)
      {
         dataflow_reachable_functions[top_id].insert(CGM.get_function(vertex));
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Ding-dong candidate classification...");
   for(const auto& [top_id, var_uses] : var_map)
   {
      if(!is_dataflow_top_function(top_id))
      {
         continue;
      }
      // Collect all candidate variables for this dataflow top: those directly in its var_map
      // plus those appearing in callee dataflow-module var_maps that are internal to this top.
      // The latter are needed in BAMBU-BALANCED mode where each callee is a separate root
      // function and therefore excluded from the top's own subgraph.
      CustomOrderedSet<var_id_t> candidate_vars;
      for(const auto& [var_id, _] : var_uses)
      {
         if(is_internal_obj(var_id, top_id, false))
         {
            candidate_vars.insert(var_id);
         }
      }
      for(const auto object_id : dataflow_reachable_objects[top_id])
      {
         if(is_internal_obj(object_id, top_id, false))
         {
            candidate_vars.insert(object_id);
         }
      }
      for(const auto var_id : candidate_vars)
      {
         CustomOrderedSet<unsigned int> writer_functions;
         CustomOrderedSet<unsigned int> reader_functions;
         CustomOrderedSet<unsigned int> direct_memory_reader_functions;
         CustomOrderedSet<unsigned int> dataflow_functions;
         const auto expected_prefix = "DF_bambu_" + STR(top_id) + "_" + STR(var_id) + "FO";
         for(const auto function_id : dataflow_reachable_functions[top_id])
         {
            if(function_id == top_id || !is_dataflow_module_function(function_id))
            {
               continue;
            }
            const auto function_name =
                HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->GetFunctionName();
            const auto func_arch = HLSMgr->module_arch->GetArchitecture(function_name);
            if(!func_arch)
            {
               continue;
            }
            auto function_has_matching_bundle = false;
            for(const auto& [parm_name, parm_attrs] : func_arch->parms)
            {
               (void)parm_name;
               const auto bundle_it = parm_attrs.find(FunctionArchitecture::parm_bundle);
               if(bundle_it == parm_attrs.end() || !starts_with(bundle_it->second, expected_prefix))
               {
                  continue;
               }
               const auto iface_it = func_arch->ifaces.find(bundle_it->second);
               if(iface_it == func_arch->ifaces.end())
               {
                  continue;
               }
               const auto mode_it = iface_it->second.find(FunctionArchitecture::iface_mode);
               const auto direction_it = iface_it->second.find(FunctionArchitecture::iface_direction);
               if(mode_it == iface_it->second.end() || direction_it == iface_it->second.end())
               {
                  continue;
               }
               if(mode_it->second != "array" && mode_it->second != "default")
               {
                  continue;
               }
               function_has_matching_bundle = true;
               if(direction_it->second == "OUT")
               {
                  writer_functions.insert(function_id);
               }
               else if(direction_it->second == "IN")
               {
                  reader_functions.insert(function_id);
                  if(mode_it->second == "array")
                  {
                     direct_memory_reader_functions.insert(function_id);
                  }
               }
            }
            if(function_has_matching_bundle)
            {
               dataflow_functions.insert(function_id);
            }
         }
         if(dataflow_functions.empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Skipping non-interface internal variable " + STR(TM->GetIRNode(var_id)) +
                               " while classifying ding-dong candidates");
            continue;
         }
         if(!writer_functions.empty() && !reader_functions.empty())
         {
            for(const auto writer_function_id : writer_functions)
            {
               if(reader_functions.count(writer_function_id))
               {
                  THROW_ERROR_USAGE(
                      "Ding-dong candidate " + STR(TM->GetIRNode(var_id)) +
                      " has the same function as both writer and reader: " +
                      HLSMgr->CGetFunctionBehavior(writer_function_id)->CGetBehavioralHelper()->GetFunctionName() +
                      ". Each dataflow module connected to a ding-dong buffer must be either a writer or "
                      "a reader.");
               }
            }
            const auto bundle_names = get_dataflow_bundle_names(top_id, var_id, dataflow_functions);
            if(bundle_names.empty())
            {
               THROW_ERROR_USAGE("Ding-dong candidate " + STR(TM->GetIRNode(var_id)) +
                                 " has no dataflow bundle names. "
                                 "Check that the variable is correctly connected to a dataflow interface bundle.");
            }
            const auto producer_function_id = *writer_functions.begin();
            const auto consumer_function_id = *reader_functions.begin();
            HLS_manager::ding_dong_buffer_info candidate_info{
                producer_function_id, consumer_function_id,
                std::set<unsigned int>(writer_functions.begin(), writer_functions.end()),
                std::set<unsigned int>(reader_functions.begin(), reader_functions.end()), bundle_names};
            HLSMgr->add_ding_dong_buffer_candidate(top_id, var_id, candidate_info);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Classified ding-dong candidate " + STR(TM->GetIRNode(var_id)) + " in dataflow top " +
                               HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetFunctionName() +
                               " writers=" + STR(writer_functions.size()) + " readers=" + STR(reader_functions.size()));
         }
         else if(writer_functions.empty() && !reader_functions.empty() &&
                 direct_memory_reader_functions.size() != reader_functions.size())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Skipping dataflow read-only candidate without direct array interfaces " +
                               STR(TM->GetIRNode(var_id)));
         }
         else if(writer_functions.empty() && !reader_functions.empty() &&
                 is_initialized_read_only_dataflow_memory(top_id, var_id))
         {
            const auto bundle_names = get_dataflow_bundle_names(top_id, var_id, dataflow_functions);
            if(bundle_names.empty())
            {
               THROW_ERROR_USAGE("Dataflow read-only memory " + STR(TM->GetIRNode(var_id)) +
                                 " has no dataflow bundle names. "
                                 "Check that the variable is correctly connected to a dataflow interface bundle.");
            }
            HLS_manager::dataflow_read_only_memory_info candidate_info{
                std::set<unsigned int>(reader_functions.begin(), reader_functions.end()), bundle_names};
            HLSMgr->add_dataflow_read_only_memory_candidate(top_id, var_id, candidate_info);
            HLSMgr->Rmem->add_read_only_variable(var_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Classified dataflow read-only memory " + STR(TM->GetIRNode(var_id)) +
                               " in dataflow top " +
                               HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetFunctionName() +
                               " consumers=" + STR(reader_functions.size()) + " bundles=" + STR(bundle_names.size()));
         }
         else if(writer_functions.empty() && !reader_functions.empty())
         {
            THROW_ERROR_USAGE("Reader-only dataflow memory " + STR(TM->GetIRNode(var_id)) +
                              " requires a static initializer and no synthesized writes");
         }
         else if(!writer_functions.empty() && reader_functions.empty())
         {
            THROW_ERROR_USAGE("Producer-only dataflow memory " + STR(TM->GetIRNode(var_id)) +
                              " has no consumer module");
         }
         else
         {
            THROW_ERROR_USAGE("Ding-dong candidate " + STR(TM->GetIRNode(var_id)) + " is used by " +
                              STR(writer_functions.size()) + " writers and " + STR(reader_functions.size()) +
                              " readers (expected at least 1 writer and 1 reader). "
                              "Ensure the variable is connected to producer and consumer dataflow modules.");
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Ding-dong candidate classification completed");
   CustomMap<var_id_t, top_id_t> dataflow_shared_memory_owner_map;
   // Build owner map from the registered candidates directly so that variables
   // that are not in var_map[top_id] are still captured.
   const auto add_dataflow_shared_memory_owners = [&](const auto& candidates) {
      for(const auto& [candidate_top_id, candidate_map] : candidates)
      {
         for(const auto& [candidate_var_id, _] : candidate_map)
         {
            const auto inserted =
                dataflow_shared_memory_owner_map.insert(std::make_pair(candidate_var_id, candidate_top_id));
            if(!(inserted.second || inserted.first->second == candidate_top_id))
            {
               THROW_ERROR_USAGE(
                   "Dataflow shared memory " + STR(TM->GetIRNode(candidate_var_id)) +
                   " is assigned to multiple dataflow tops: " +
                   HLSMgr->CGetFunctionBehavior(inserted.first->second)->CGetBehavioralHelper()->GetFunctionName() +
                   " and " + HLSMgr->CGetFunctionBehavior(candidate_top_id)->CGetBehavioralHelper()->GetFunctionName());
            }
         }
      }
   };
   add_dataflow_shared_memory_owners(HLSMgr->get_ding_dong_buffer_candidates());
   add_dataflow_shared_memory_owners(HLSMgr->get_dataflow_read_only_memory_candidates());
   const auto has_dataflow_ding_dong_candidate = [&](const var_id_t var_id) {
      for(const auto& [_, candidates] : HLSMgr->get_ding_dong_buffer_candidates())
      {
         if(candidates.count(var_id))
         {
            return true;
         }
      }
      return false;
   };
   const auto has_dataflow_read_only_memory_candidate = [&](const var_id_t var_id) {
      for(const auto& [_, candidates] : HLSMgr->get_dataflow_read_only_memory_candidates())
      {
         if(candidates.count(var_id))
         {
            return true;
         }
      }
      return false;
   };

   /// compute the number of instances for each function
   CustomMap<top_id_t, CustomMap<CallGraph::vertex_descriptor, unsigned int>> num_instances;
   CustomMap<top_id_t, CustomMap<func_id_t, std::vector<std::pair<var_id_t, bool>>>> memory_allocation_map;
   for(const auto root_function : root_functions)
   {
      num_instances[root_function][CGM.GetVertex(root_function)] = 1;
   }
   for(const auto addr_function : CGM.GetAddressedFunctions())
   {
      num_instances[addr_function][CGM.GetVertex(addr_function)] = 1;
   }
   const auto compute_instances = [&](const unsigned int f_id, const unsigned int top_id) {
      memory_allocation_map[top_id][f_id];
      const auto& top_cg = cg.at(top_id);
      const auto f_v = CGM.GetVertex(f_id);
      if(top_cg.out_degree(f_v))
      {
         THROW_ASSERT(HLSMgr->get_HLS(f_id),
                      "Missing HLS initialization for " +
                          HLSMgr->CGetFunctionBehavior(f_id)->CGetBehavioralHelper()->GetFunctionName());
         const auto HLS_C = HLSMgr->get_HLS(f_id)->HLS_C;
         THROW_ASSERT(num_instances.at(top_id).count(f_v),
                      "missing number of instances of function " +
                          HLSMgr->CGetFunctionBehavior(f_id)->CGetBehavioralHelper()->GetFunctionName());
         const auto cur_instances = num_instances.at(top_id).at(f_v);
         for(const auto& e : top_cg.out_edges(f_v))
         {
            const auto tgt = top_cg.target(e);
            const auto tgt_fu_name = functions::GetFUName(CGM.get_function(tgt), HLSMgr);
            if(HLSMgr->Rfuns->is_a_proxied_function(tgt_fu_name) ||
               (parameters->getOption<bool>(OPT_memory_mapped_top) && HLSMgr->hasToBeInterfaced(f_id)))
            {
               num_instances.at(top_id)[tgt] = 1;
            }
            else if(HLS_C->get_number_fu(tgt_fu_name, WORK_LIBRARY) == 1)
            {
               num_instances.at(top_id)[tgt] = 1;
            }
            else
            {
               const auto n_call_points = static_cast<unsigned int>(top_cg.CGetEdgeInfo(e).direct_call_points.size());
               if(!num_instances.at(top_id).count(tgt))
               {
                  num_instances.at(top_id)[tgt] = cur_instances * n_call_points;
               }
               else
               {
                  num_instances.at(top_id)[tgt] += cur_instances * n_call_points;
               }
            }
         }
      }
   };
   for(const auto& f_order : function_allocation_order)
   {
      const auto top_id = f_order.first;
      for(const auto f_id : f_order.second)
      {
         compute_instances(f_id, top_id);
      }
   }

   /// find the common dominator and decide where to allocate
   const auto no_private_mem =
       parameters->IsParameter("no-private-mem") && parameters->GetParameter<bool>("no-private-mem");
   const auto no_local_mem = parameters->IsParameter("no-local-mem") && parameters->GetParameter<bool>("no-local-mem");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Variable dominator computation...");
   for(const auto& [top_id, var_uses] : var_map)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                     "-->Analyzing top function: " +
                         HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetFunctionName());
      const auto& dom_tree = *cg_dominators.at(top_id);
      for(const auto& [var_id, uses] : var_uses)
      {
         THROW_ASSERT(var_id, "null var index unexpected");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Finding common dominator for " + STR(TM->GetIRNode(var_id)) + " (id: " + STR(var_id) +
                            ", uses: " + STR(uses.size()) + ")");
         auto funID = top_id;
         auto multiple_top_call_graph = false;
         const auto is_ding_dong_candidate = has_dataflow_ding_dong_candidate(var_id);
         const auto is_dataflow_read_only_memory = has_dataflow_read_only_memory_candidate(var_id);
         const auto is_dataflow_shared_memory = is_ding_dong_candidate || is_dataflow_read_only_memory;
         for(const auto& [fid, vu] : var_map)
         {
            if(fid != top_id && vu.count(var_id))
            {
               multiple_top_call_graph = true;
               break;
            }
         }
         if(is_dataflow_shared_memory)
         {
            const auto owner_it = dataflow_shared_memory_owner_map.find(var_id);
            THROW_ASSERT(owner_it != dataflow_shared_memory_owner_map.end(),
                         "Missing owner for dataflow shared memory " + STR(TM->GetIRNode(var_id)));
            const auto owner_top_id = owner_it->second;
            if(top_id == owner_top_id)
            {
               funID = top_id;
               multiple_top_call_graph = false;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Allocating dataflow shared memory " + STR(TM->GetIRNode(var_id)) +
                                  " in dataflow top " +
                                  HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetFunctionName());
            }
            else
            {
               INDENT_DBG_MEX(
                   DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                   "<--Skipping dataflow shared memory " + STR(TM->GetIRNode(var_id)) + " in function " +
                       HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetFunctionName() +
                       " (allocated in dataflow top " +
                       HLSMgr->CGetFunctionBehavior(owner_top_id)->CGetBehavioralHelper()->GetFunctionName() + ")");
               continue;
            }
         }
         if(multiple_top_call_graph)
         {
            if(multiple_top_call_graph)
            {
               const auto top_func_name =
                   HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetFunctionName();
               const auto func_arch = HLSMgr->module_arch->GetArchitecture(top_func_name);
               const auto is_df_top = func_arch && func_arch->attrs.count(FunctionArchitecture::func_dataflow_top) &&
                                      func_arch->attrs.at(FunctionArchitecture::func_dataflow_top) == "1";
               const auto is_df_module = func_arch &&
                                         func_arch->attrs.count(FunctionArchitecture::func_dataflow_module) &&
                                         func_arch->attrs.at(FunctionArchitecture::func_dataflow_module) == "1";
               if(is_df_top)
               {
                  /// Dataflow top function only passes variable addresses to sub-modules.
                  /// The memory will be allocated inside each sub-module, not here.
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Skipping variable " + STR(TM->GetIRNode(var_id)) +
                                     " in dataflow top (allocated in sub-modules)");
                  continue;
               }
               if(is_df_module)
               {
                  /// If the variable has a DF_bambu dataflow interface bundle in this function's
                  /// architecture, it is managed by the dataflow interconnect and should NOT be
                  /// allocated here. Memory bundles are handled by the ding-dong buffer mechanism;
                  /// stream bundles are handled by their channel interface.
                  const auto var_df_substring = "_" + STR(var_id) + "FO";
                  auto has_df_bambu_memory_bundle = false;
                  auto has_df_bambu_channel_bundle = false;
                  if(func_arch)
                  {
                     for(const auto& [parm_name, parm_attrs] : func_arch->parms)
                     {
                        (void)parm_name;
                        const auto bundle_it = parm_attrs.find(FunctionArchitecture::parm_bundle);
                        if(bundle_it != parm_attrs.end() && starts_with(bundle_it->second, "DF_bambu_") &&
                           bundle_it->second.find(var_df_substring) != std::string::npos)
                        {
                           const auto iface_it = func_arch->ifaces.find(bundle_it->second);
                           auto is_memory_bundle = false;
                           if(iface_it != func_arch->ifaces.end())
                           {
                              const auto mode_it = iface_it->second.find(FunctionArchitecture::iface_mode);
                              is_memory_bundle = mode_it != iface_it->second.end() && mode_it->second == "array";
                           }
                           has_df_bambu_memory_bundle |= is_memory_bundle;
                           has_df_bambu_channel_bundle |= !is_memory_bundle;
                        }
                     }
                  }
                  if(has_df_bambu_channel_bundle && !has_df_bambu_memory_bundle)
                  {
                     INDENT_DBG_MEX(
                         DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                         "---Allocating DF_bambu channel variable " + STR(TM->GetIRNode(var_id)) + " in df_module " +
                             HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetFunctionName() +
                             " locally (shared access is managed by dataflow channel interface)");
                     multiple_top_call_graph = false;
                  }
                  else
                  {
                     if(has_df_bambu_memory_bundle)
                     {
                        const auto& ding_dong_candidates = HLSMgr->get_ding_dong_buffer_candidates(top_id);
                        const auto& dataflow_read_only_memory_candidates =
                            HLSMgr->get_dataflow_read_only_memory_candidates(top_id);
                        if(ding_dong_candidates.count(var_id) || dataflow_read_only_memory_candidates.count(var_id))
                        {
                           INDENT_DBG_MEX(
                               DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                               "<--Skipping DF_bambu dataflow variable " + STR(TM->GetIRNode(var_id)) +
                                   " in df_module " +
                                   HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetFunctionName() +
                                   " (managed by shared dataflow memory mechanism)");
                           continue;
                        }
                        else
                        {
                           THROW_ERROR_USAGE(
                               "DF_bambu dataflow variable " + STR(TM->GetIRNode(var_id)) + " in df_module " +
                               HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetFunctionName() +
                               " is not registered as a ding-dong buffer or dataflow read-only memory candidate. "
                               "This usually happens when the variable does not follow the 1-writer-1-reader rule.");
                        }
                     }
                     /// Dataflow module: allocate the variable internally.
                     /// Check for concurrent writes across dataflow modules (race condition).
                     const auto is_written = HLSMgr->get_written_objects().count(var_id);
                     unsigned int df_module_count = 0;
                     for(const auto& [other_top, other_vu] : var_map)
                     {
                        if(other_vu.count(var_id))
                        {
                           const auto other_name =
                               HLSMgr->CGetFunctionBehavior(other_top)->CGetBehavioralHelper()->GetFunctionName();
                           const auto other_arch = HLSMgr->module_arch->GetArchitecture(other_name);
                           const auto other_is_df_module =
                               other_arch && other_arch->attrs.count(FunctionArchitecture::func_dataflow_module) &&
                               other_arch->attrs.at(FunctionArchitecture::func_dataflow_module) == "1";
                           if(other_is_df_module)
                           {
                              ++df_module_count;
                           }
                        }
                     }
                     if(is_written && df_module_count > 1)
                     {
                        THROW_ERROR("Variable " + STR(TM->GetIRNode(var_id)) +
                                    " is written by multiple concurrent dataflow modules. "
                                    "Concurrent writes to the same memory cause race conditions");
                     }
                     if(!is_written && df_module_count > 1)
                     {
                        THROW_WARNING("Read-only variable " + STR(TM->GetIRNode(var_id)) + " is duplicated across " +
                                      STR(df_module_count) + " concurrent dataflow modules");
                     }
                     /// Treat as single-top for internal allocation
                     multiple_top_call_graph = false;
                  }
               }
            }
         }
         if(!no_local_mem)
         {
            const auto is_written = HLSMgr->get_written_objects().count(var_id) || no_private_mem;
            if(uses.size() == 1)
            {
               auto cur = *uses.begin();
               INDENT_DBG_MEX(
                   DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                   "---Current function(0): " +
                       HLSMgr->CGetFunctionBehavior(CGM.get_function(cur))->CGetBehavioralHelper()->GetFunctionName());
               const auto fun_behavior = HLSMgr->CGetFunctionBehavior(CGM.get_function(cur));
               /// look for a single instance function in case the object is not a ROM and not a local var
               if(is_written && fun_behavior->is_a_state_variable(var_id))
               {
                  while(num_instances.at(top_id).at(cur) != 1)
                  {
                     cur = get_remapped_vertex(dom_tree.getImmediateDominator(cur), CGM, HLSMgr);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Current function(1): " + HLSMgr->CGetFunctionBehavior(CGM.get_function(cur))
                                                                     ->CGetBehavioralHelper()
                                                                     ->GetFunctionName());
                  }
               }
               funID = CGM.get_function(cur);
            }
            else if(!multiple_top_call_graph)
            {
               const auto top_vertex = CGM.GetVertex(top_id);
               const auto vert_it_end = uses.end();
               auto vert_it = uses.begin();
               std::list<CallGraph::vertex_descriptor> dominator_list1;
               auto cur = *vert_it;
               INDENT_DBG_MEX(
                   DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                   "---Current function(2a): " +
                       HLSMgr->CGetFunctionBehavior(CGM.get_function(cur))->CGetBehavioralHelper()->GetFunctionName());
               dominator_list1.push_front(cur);
               do
               {
                  cur = get_remapped_vertex(dom_tree.getImmediateDominator(cur), CGM, HLSMgr);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Current function(2b): " + HLSMgr->CGetFunctionBehavior(CGM.get_function(cur))
                                                                   ->CGetBehavioralHelper()
                                                                   ->GetFunctionName());
                  dominator_list1.push_front(cur);
               } while(cur != top_vertex);
               ++vert_it;
               auto last = dominator_list1.end();
               for(; vert_it != vert_it_end; ++vert_it)
               {
                  std::list<CallGraph::vertex_descriptor> dominator_list2;
                  cur = *vert_it;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Current function(2c): " + HLSMgr->CGetFunctionBehavior(CGM.get_function(cur))
                                                                   ->CGetBehavioralHelper()
                                                                   ->GetFunctionName());
                  dominator_list2.push_front(cur);
                  do
                  {
                     cur = get_remapped_vertex(dom_tree.getImmediateDominator(cur), CGM, HLSMgr);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Current function(2d): " + HLSMgr->CGetFunctionBehavior(CGM.get_function(cur))
                                                                      ->CGetBehavioralHelper()
                                                                      ->GetFunctionName());
                     dominator_list2.push_front(cur);
                  } while(cur != top_vertex);
                  /// find the common dominator between two candidates
                  auto dl1_it = dominator_list1.begin(), dl2_it = dominator_list2.begin(),
                       dl2_it_end = dominator_list2.end(), cur_last = dominator_list1.begin();
                  while(dl1_it != last && dl2_it != dl2_it_end && *dl1_it == *dl2_it &&
                        (num_instances.at(top_id).at(*dl1_it) == 1 || !is_written))
                  {
                     cur = *dl1_it;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Current function(2e): " +
                                        HLSMgr->CGetFunctionBehavior(CGM.get_function(cur))
                                            ->CGetBehavioralHelper()
                                            ->GetFunctionName() +
                                        " num instances: " + STR(num_instances.at(top_id).at(cur)));
                     ++dl1_it;
                     cur_last = dl1_it;
                     ++dl2_it;
                  }
                  last = cur_last;
                  funID = CGM.get_function(cur);
                  if(cur == top_vertex)
                  {
                     break;
                  }
               }
            }
         }
         THROW_ASSERT(funID, "null function id index unexpected");
         const auto is_internal = is_internal_obj(var_id, funID, multiple_top_call_graph);
         memory_allocation_map.at(top_id)[funID].push_back(std::make_pair(var_id, is_internal));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Found common dominator for " +
                            HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->GetFunctionName() + " -> " +
                            STR(TM->GetIRNode(var_id)) + " (scope: " + (is_internal ? "internal" : "external") + ")");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                     "<--Analyzed top function: " +
                         HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->GetFunctionName());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Variable dominator computation completed");

   const auto classify_variables = [&](const func_id_t f_id, const top_id_t top_id) {
      const auto it = memory_allocation_map.at(top_id).find(f_id);
      if(it != memory_allocation_map.at(top_id).end())
      {
         for(const auto& mem_map : it->second)
         {
            const auto var_id = mem_map.first;
            THROW_ASSERT(var_id, "null var index unexpected");
            const auto is_internal = mem_map.second;
            auto is_dynamic_address_used = false;

            if(is_internal)
            {
               THROW_ASSERT(where_used.at(top_id)[var_id].size() > 0, "variable not used anywhere");
               /// check dynamic address use
               is_dynamic_address_used = has_dynamic_address_use(top_id, var_id);
               if(is_dynamic_address_used)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Found dynamic use of variable " + STR(var_id));
               }

               if(is_dynamic_address_used && !all_pointers_resolved && !assume_aligned_access_p)
               {
                  HLSMgr->Rmem->set_sds_var(var_id, false, 0);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var_id) + " not sds-A");
               }

               if(!HLSMgr->Rmem->has_sds_var(var_id))
               {
                  HLSMgr->Rmem->set_sds_var(var_id, false, 0);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var_id) + " not sds-B");
               }

               if(!no_private_mem && !no_local_mem)
               {
                  if(!is_dynamic_address_used && /// we never have &(var_id_object)
                     HLSMgr->get_written_objects().find(var_id) ==
                         HLSMgr->get_written_objects().end() /// read only memory
                  )
                  {
                     if(!GetPointer<const call_stmt>(TM->GetIRNode(var_id)))
                     {
                        HLSMgr->Rmem->add_private_memory(var_id);
                     }
                  }
                  else if(CGM.ExistsAddressedFunction())
                  {
                     if(var_map.at(top_id).at(var_id).size() == 1 && where_used.at(top_id).at(var_id).size() == 1 &&
                        !is_dynamic_address_used &&                              /// we never have &(var_id_object)
                        (*(where_used.at(top_id).at(var_id).begin()) == f_id) && /// used in a single place
                        !GetPointer<const call_stmt>(TM->GetIRNode(var_id)))
                     {
                        HLSMgr->Rmem->add_private_memory(var_id);
                     }
                  }
                  else
                  {
                     if(!is_dynamic_address_used && /// we never have &(var_id_object)
                        !GetPointer<const call_stmt>(TM->GetIRNode(var_id)))
                     {
                        HLSMgr->Rmem->add_private_memory(var_id);
                     }
                  }
               }
            }
            else
            {
               is_dynamic_address_used = true;
               HLSMgr->Rmem->set_sds_var(var_id, false, 0);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var_id) + " not sds-C");
            }
            const auto vd = GetPointer<const variable_val_node>(TM->GetIRNode(var_id));
            if((HLSMgr->get_written_objects().find(var_id) == HLSMgr->get_written_objects().end()) &&
               (!is_dynamic_address_used || (vd && vd->readonly_flag)))
            {
               HLSMgr->Rmem->add_read_only_variable(var_id);
            }
         }
      }
   };

   /// classify variable
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Variable classification...");
   for(const auto& top_order : function_allocation_order)
   {
      const auto& top_id = top_order.first;
      if(memory_allocation_map.find(top_id) != memory_allocation_map.end())
      {
         for(const auto& f_id : top_order.second)
         {
            classify_variables(f_id, top_id);
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Variable classification completed");

   /// change the alignment in case is requested
   if(parameters->isOption(OPT_sparse_memory) && parameters->getOption<bool>(OPT_sparse_memory))
   {
      /// change the internal alignment to improve the decoding logic
      auto max_byte_size = HLSMgr->Rmem->get_internal_base_address_alignment();
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                     "memory alignment initialized to " + STR(max_byte_size) + " bytes");
      for(const auto& mem_map : memory_allocation_map)
      {
         for(const auto& f_pair : mem_map.second)
         {
            for(const auto& pair : f_pair.second)
            {
               const auto& var_id = pair.first;
               THROW_ASSERT(var_id, "null var index unexpected");
               if(pair.second && (!HLSMgr->Rmem->is_private_memory(var_id) || null_pointer_check))
               {
                  const auto curr_size = compute_n_bytes(ir_helper::SizeAlloc(TM->GetIRNode(var_id)));
                  max_byte_size = std::max(static_cast<unsigned long long>(curr_size), max_byte_size);
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                 "Compare memory alignment with " + STR(curr_size) + " bytes");
               }
            }
         }
      }
      /// Round up to the next highest power of 2
      max_byte_size = ceil_pow2(max_byte_size);
      HLSMgr->Rmem->set_internal_base_address_alignment(max_byte_size);
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                     "Sparse memory alignment set to " + STR(max_byte_size) + " bytes");
   }

   const auto memory_mapped_top_if = parameters->getOption<bool>(OPT_memory_mapped_top);
   const auto allocate_function_mem = [&](const func_id_t f_id, const top_id_t top_id,
                                          const std::unique_ptr<memory>& Rmem) {
#ifndef NDEBUG
      const auto dbg_lvl = Rmem == HLSMgr->Rmem ? debug_level : DEBUG_LEVEL_NONE;
#endif
      const auto out_lvl = Rmem == HLSMgr->Rmem ? output_level : OUTPUT_LEVEL_NONE;
      const auto align_omp_memory = f_id && HLSMgr->isOmpLambdaFunction(f_id);
      THROW_ASSERT(memory_allocation_map.count(top_id), "Invalid top function id.");
      const auto func_mem_map = memory_allocation_map.at(top_id).find(f_id);
      if(func_mem_map != memory_allocation_map.at(top_id).end())
      {
         for(const auto& mem_map : func_mem_map->second)
         {
            const auto var_id = mem_map.first;
            THROW_ASSERT(var_id, "null var index unexpected");
            const auto var_node = TM->GetIRNode(var_id);
            const auto is_internal = mem_map.second;
            auto is_dynamic_address_used = false;

            if(is_internal)
            {
               THROW_ASSERT(where_used.at(top_id)[var_id].size() > 0, "variable not used anywhere");
               const auto function_behavior = HLSMgr->CGetFunctionBehavior(*(where_used.at(top_id).at(var_id).begin()));
               const auto BH = function_behavior->CGetBehavioralHelper();
               const auto var_id_string = BH->PrintVariable(var_id);
               /// check dynamic address use
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, dbg_lvl, "---Check dynamic use for var " + var_id_string);
               const auto wiu_it_end = where_used.at(top_id).at(var_id).end();
               is_dynamic_address_used = has_dynamic_address_use(top_id, var_id);
               if(is_dynamic_address_used)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, dbg_lvl, "---Found dynamic use of variable " + STR(var_id));
               }

               if(!is_dynamic_address_used &&                  /// we never have &(var_id_object)
                  !HLSMgr->get_written_objects().count(var_id) /// read only memory
                  && (!no_private_mem && !no_local_mem))
               {
                  for(auto wiu_it = where_used.at(top_id).at(var_id).begin(); wiu_it != wiu_it_end; ++wiu_it)
                  {
                     const auto cur_function_behavior = HLSMgr->CGetFunctionBehavior(*wiu_it);
                     const auto cur_BH = cur_function_behavior->CGetBehavioralHelper();
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                                    "---Internal variable: " + cur_BH->PrintVariable(var_id) + " - " + STR(var_id) +
                                        " - " + var_id_string + " in function " + cur_BH->GetFunctionName());
                     Rmem->add_internal_variable(*wiu_it, var_id, cur_BH->PrintVariable(var_id),
                                                 align_omp_memory && !HLSMgr->Rmem->is_sds_var(var_id));
                  }
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "-->");
               }
               else if(CGM.ExistsAddressedFunction())
               {
                  const auto cur_function_behavior = HLSMgr->CGetFunctionBehavior(f_id);
                  const auto cur_BH = cur_function_behavior->CGetBehavioralHelper();
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                                 "-->Internal variable: " + cur_BH->PrintVariable(var_id) + " - " + STR(var_id) +
                                     " - " + var_id_string + " in function " + cur_BH->GetFunctionName());
                  Rmem->add_internal_variable(f_id, var_id, cur_BH->PrintVariable(var_id),
                                              align_omp_memory && !HLSMgr->Rmem->is_sds_var(var_id));
               }
               else
               {
                  const auto cur_function_behavior = HLSMgr->CGetFunctionBehavior(f_id);
                  const auto cur_BH = cur_function_behavior->CGetBehavioralHelper();
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                                 "-->Internal variable: " + cur_BH->PrintVariable(var_id) + " - " + STR(var_id) +
                                     " - " + var_id_string + " in function " + cur_BH->GetFunctionName());
                  Rmem->add_internal_variable(f_id, var_id, cur_BH->PrintVariable(var_id),
                                              align_omp_memory && !HLSMgr->Rmem->is_sds_var(var_id));
                  /// add proxies
                  if(!no_private_mem && !no_local_mem)
                  {
                     for(const auto& wu_id : where_used.at(top_id)[var_id])
                     {
                        if(wu_id != f_id)
                        {
                           Rmem->add_internal_variable_proxy(wu_id, var_id);
                        }
                     }
                  }
               }
            }
            else
            {
               const auto function_behavior = HLSMgr->CGetFunctionBehavior(f_id);
               const auto BH = function_behavior->CGetBehavioralHelper();
               const auto var_id_string = BH->PrintVariable(var_id);
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                              "-->Variable external to the top module: " + BH->PrintVariable(var_id) + " - " +
                                  STR(var_id) + " - " + var_id_string);
               Rmem->add_external_variable(var_id, BH->PrintVariable(var_id));
               is_dynamic_address_used = true;
            }
            const auto is_packed = GetPointer<const decl_node>(var_node) && ir_helper::IsPackedType(var_node);
            Rmem->set_packed_vars(is_packed);

            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Id: " + STR(var_id));
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                           "---Base Address: " + STR(Rmem->get_base_address(var_id, f_id)));
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                           "---Size: " + STR(compute_n_bytes(ir_helper::SizeAlloc(var_node))));
            if(HLSMgr->Rmem->is_private_memory(var_id))
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Is a private memory");
            }
            if(HLSMgr->Rmem->is_a_proxied_variable(var_id))
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Has proxied accesses");
            }
            if(HLSMgr->Rmem->is_read_only_variable(var_id))
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Is a Read Only Memory");
            }
            if(HLSMgr->Rmem->is_parm_decl_copied(var_id))
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Is a parm decl copied");
            }
            if(HLSMgr->Rmem->is_actual_parm_loaded(var_id))
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Is an actual parm decl loaded");
            }
            if(HLSMgr->Rmem->is_parm_decl_stored(var_id))
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Is a parm decl stored");
            }
            if(is_dynamic_address_used)
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Used &(object)");
            }
            if(HLSMgr->Rmem->is_sds_var(var_id))
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                              "---The variable is always accessed with the same data size: " +
                                  STR(Rmem->get_sds_var_size(var_id)));
               const auto vd = GetPointer<const variable_val_node>(var_node);
               if(vd && vd->bit_values.size() != 0)
               {
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                                 "---The variable has been trimmed to bitsize: " + STR(vd->bit_values.size()) +
                                     " with bit-value pattern: " + vd->bit_values);
               }
            }
            if(var_referring_vertex_map.count(var_id))
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                              "---Number of functions in which is used: " +
                                  STR(var_referring_vertex_map.at(var_id).size()));
               size_t max_references = 0;
               for(const auto& fun_vertex_set : var_referring_vertex_map.at(var_id))
               {
                  max_references = std::max(max_references, fun_vertex_set.second.size());
               }
               // TODO: check if ok also with omp lambda
               Rmem->set_maximum_references(var_id, max_references);
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                              "---Maximum number of references per function: " + STR(max_references));
            }
            if(var_load_vertex_map.count(var_id))
            {
               size_t max_loads = 0;
               for(const auto& fun_vertex_set : var_load_vertex_map.at(var_id))
               {
                  max_loads = std::max(max_loads, fun_vertex_set.second.size());
               }
               // TODO: check if ok also with omp lambda
               Rmem->set_maximum_loads(var_id, max_loads);
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                              "---Maximum number of loads per function: " + STR(max_loads));
            }
            if(is_packed)
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Variable is packed");
            }
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "<--");
         }
      }
      if(f_id && (HLSMgr->isOmpLambdaFunction(f_id) ||
                  (top_functions.count(f_id) ? memory_mapped_top_if : HLSMgr->hasToBeInterfaced(f_id))))
      {
         allocate_parameters(f_id, Rmem);
      }
   };

   // Allocate memory for root functions
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Memory allocation phase...");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                  "Internal memory address base: " + STR(HLSMgr->Rmem->get_next_internal_base_address()));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Top functions");
   struct ForkAllocationInfo
   {
      top_id_t top_fid;
      std::vector<std::vector<func_id_t>*> outlined_allocation;
   };
   CustomMap<unsigned int, ForkAllocationInfo> omp_fork_allocation;
   CustomSet<CallGraph::vertex_descriptor> fork_calls;
   for(const auto& f_id : omp_functions)
   {
      const auto f_v = CGM.GetVertex(f_id);
      for(const auto& e : CG.in_edges(f_v))
      {
         fork_calls.insert(CG.source(e));
      }
   }
   for(const auto& fork_v : fork_calls)
   {
      const auto fork_id = CGM.get_function(fork_v);
      for(const auto& e : CG.out_edges(fork_v))
      {
         const auto outlined_id = CGM.get_function(CG.target(e));
         if(HLSMgr->isOmpLambdaFunction(outlined_id))
         {
            THROW_ASSERT(omp_allocation_order.count(outlined_id),
                         "Expected initialized allocation order for current lambda: " + STR(outlined_id) + " " +
                             ir_helper::GetFunctionName(TM->GetIRNode(outlined_id)));
            const auto outlined_order = &(omp_allocation_order.at(outlined_id));
            auto& fork_info = omp_fork_allocation[fork_id];
            fork_info.outlined_allocation.push_back(outlined_order);
            fork_info.top_fid = [&]() {
               for(auto& top_order : function_allocation_order)
               {
                  auto& order = top_order.second;
                  if(std::find(order.cbegin(), order.cend(), outlined_order->front()) != order.cend())
                  {
                     order.erase(std::remove_if(order.begin(), order.end(),
                                                [&](const unsigned int f_id) {
                                                   return std::count(outlined_order->begin(), outlined_order->end(),
                                                                     f_id);
                                                }),
                                 order.end());
                     return top_order.first;
                  }
               }
               return 0U;
            }();
            THROW_ASSERT(fork_info.top_fid, "Unable to find OMP lambda allocation order");
         }
      }
   }

   for(const auto& top_order : function_allocation_order)
   {
      const auto& top_id = top_order.first;
      if(memory_allocation_map.find(top_id) != memory_allocation_map.end())
      {
         for(const auto& f_id : top_order.second)
         {
            allocate_function_mem(f_id, top_id, HLSMgr->Rmem);
         }
      }
   }

   if(omp_functions.size())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "OpenMP functions");
      const auto align_page = [&](const unsigned long long alignment, bool force_next_page = false) {
         const auto misalignment = HLSMgr->Rmem->get_next_internal_base_address() % alignment;
         const auto increment = misalignment ? (alignment - misalignment) : (force_next_page ? alignment : 0U);
         HLSMgr->Rmem->reserve_internal_space(increment);
      };
      // Compute max omp lambda page size for omp lambda functions
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "OpenMP smart allocation");
      unsigned long long max_fork_page_size = 0ULL;
      auto fork_number = 0U;
      for(const auto& fork_orders : omp_fork_allocation)
      {
         const auto& fork_id = fork_orders.first;
         const auto& omp_top = fork_orders.second.top_fid;
         const auto& outlined_orders = fork_orders.second.outlined_allocation;
         unsigned long long max_core_page_size = 0ULL;
         auto fork_nthread = 0U;
         for(const auto order : outlined_orders)
         {
            const auto outlined_id = order->front();
            const auto omp_info = HLSMgr->CGetFunctionBehavior(outlined_id)->GetOMPInfo();
            THROW_ASSERT(omp_info, "");
            fork_nthread += omp_info->context_count;
            const auto OMPRmem = memory::create_memory(parameters, TM, HLSMgr->base_address, max_bram,
                                                       null_pointer_check, initial_internal_address_p,
                                                       initial_internal_address, HLSMgr->Rget_address_bitsize());
            OMPRmem->set_internal_base_address_alignment(HLSMgr->Rmem->get_internal_base_address_alignment());
            for(const auto& f_id : *order)
            {
               allocate_function_mem(f_id, omp_top, OMPRmem);
            }
            const auto core_mem_size = OMPRmem->get_allocated_internal_memory();
            const auto core_page_size = ceil_pow2(core_mem_size);
            THROW_ASSERT(HLSMgr->Rmem->get_internal_base_address_alignment() <= core_page_size,
                         "Alignment larger then OMP core page size: " +
                             STR(HLSMgr->Rmem->get_internal_base_address_alignment()) + " > " + STR(core_page_size));
            max_core_page_size = std::max(max_core_page_size, core_page_size);
         }
         HLSMgr->Rmem->set_omp_allocation_info(
             fork_id, memory::OMPAllocationInfo(ceil_log2(max_core_page_size), ceil_log2(fork_nthread), fork_number));
         const auto fork_page_size = ceil_pow2(max_core_page_size * fork_nthread);
         max_fork_page_size = std::max(max_fork_page_size, fork_page_size);
         fork_number += 1;
      }

      const auto omp_page_size =
          ceil_pow2(max_fork_page_size * static_cast<unsigned long long>(omp_fork_allocation.size()));
      const auto root_page_size = ceil_pow2(HLSMgr->Rmem->get_allocated_internal_memory());
      const auto root_space_alignment = std::max({root_page_size, omp_page_size, 1ULL});
      align_page(root_space_alignment);
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---OMP allocation space is " + STR(root_space_alignment) + " bits");

      HLSMgr->Rmem->set_root_space_alignment(root_page_size > 0 ? ceil_log2(root_space_alignment) : 0);
      HLSMgr->Rmem->set_fork_page_id_start(ceil_log2(max_fork_page_size));
      HLSMgr->Rmem->set_fork_page_id_end(ceil_log2(omp_page_size));

      // Allocate memory for OMP lambda functions
      auto next_mem_address = HLSMgr->Rmem->get_next_internal_base_address();
      bool first = true;
      for(const auto& fork_orders : omp_fork_allocation)
      {
         const auto& fork_id = fork_orders.first;
         const auto& omp_top = fork_orders.second.top_fid;
         const auto& outlined_orders = fork_orders.second.outlined_allocation;
         const auto core_page_size = 1ULL << HLSMgr->Rmem->get_omp_allocation_info(fork_id).proc_addr_bitsize;
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "-->Analyzing OMP fork " + ir_helper::GetFunctionName(TM->GetIRNode(fork_id)));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---OMP cores  : " + STR(outlined_orders.size()));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---OMP memory  : " + STR(core_page_size) + " bytes (per thread)");

         // Align for a new OMP lambda memory page
         if(!first)
         {
            align_page(max_fork_page_size, next_mem_address == HLSMgr->Rmem->get_next_internal_base_address());
         }
         first = false;
         next_mem_address = HLSMgr->Rmem->get_next_internal_base_address();
         bool first_core = true;
         for(const auto& order : outlined_orders)
         {
            const auto outlined_id = order->front();
            const auto omp_info = HLSMgr->CGetFunctionBehavior(outlined_id)->GetOMPInfo();
            THROW_ASSERT(omp_info, "");
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                           "---Analyzing OMP core " + ir_helper::GetFunctionName(TM->GetIRNode(order->at(0))) +
                               " (threads: " + STR(omp_info->context_count) + ")");

            if(!first_core)
            {
               align_page(core_page_size, next_mem_address == HLSMgr->Rmem->get_next_internal_base_address());
            }
            first_core = false;

            next_mem_address = HLSMgr->Rmem->get_next_internal_base_address();
            for(const auto& f_id : *order)
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->");
               allocate_function_mem(f_id, omp_top, HLSMgr->Rmem);
               INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
            }
            omp_info->mem_page_size = core_page_size;
            // Reserve space for all the other OMP threads
            HLSMgr->Rmem->reserve_internal_space((omp_info->context_count - 1U) * core_page_size);
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                           "---Analyzed OMP core " + ir_helper::GetFunctionName(TM->GetIRNode(order->at(0))));
         }
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "<--Analyzed OMP fork " + ir_helper::GetFunctionName(TM->GetIRNode(fork_id)));
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Memory allocation phase completed");

   if(output_level >= OUTPUT_LEVEL_MINIMUM && output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      STOP_TIME(step_time);
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   finalize_memory_allocation();
   if(output_level >= OUTPUT_LEVEL_MINIMUM && output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---Time to perform memory allocation: " + print_cpu_time(step_time) + " seconds");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   const auto changed = HLSMgr->Rmem->notEQ(prevRmem);
   if(changed)
   {
      HLSMgr->UpdateMemVersion();
      /// clean proxy library
      const auto TechM = HLS_D->get_technology_manager();
      TechM->erase_library(PROXY_LIBRARY);
      TechM->erase_library(WORK_LIBRARY);
   }
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
