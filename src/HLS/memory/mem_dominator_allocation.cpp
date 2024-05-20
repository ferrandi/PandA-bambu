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
 * @file mem_dominator_allocation.cpp
 * @brief Memory allocation based on the dominator tree of the call graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "mem_dominator_allocation.hpp"

#include "Dominance.hpp"
#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "cpu_time.hpp"
#include "function_behavior.hpp"
#include "functions.hpp"
#include "generic_device.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "op_graph.hpp"
#include "polixml.hpp"
#include "string_manipulation.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "var_pp_functor.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

/// STD includes
#include <algorithm>
#include <filesystem>
#include <limits>
#include <list>
#include <string>
#include <utility>

#include "custom_map.hpp"
#include "custom_set.hpp"

mem_dominator_allocation::mem_dominator_allocation(
    const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
    const DesignFlowManagerConstRef _design_flow_manager,
    const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization, const HLSFlowStep_Type _hls_flow_step_type)
    : memory_allocation(_parameters, _HLSMgr, _design_flow_manager, _hls_flow_step_type, _hls_flow_step_specialization),
      user_defined_base_address(UINT64_MAX)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

mem_dominator_allocation::~mem_dominator_allocation() = default;

bool mem_dominator_allocation::is_internal_obj(unsigned int var_id, unsigned int fun_id, bool multiple_top_call_graph)
{
   const auto TM = HLSMgr->get_tree_manager();
   const auto FB = HLSMgr->CGetFunctionBehavior(fun_id);
   const auto BH = FB->CGetBehavioralHelper();
   const auto var_name = BH->PrintVariable(var_id);
   const auto fun_name = BH->get_function_name();
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
      const auto tn = TM->GetTreeNode(var_id);
      switch(memory_allocation_policy)
      {
         case MemoryAllocation_Policy::LSS:
         {
            const auto vd = GetPointer<const var_decl>(tn);
            if(vd && (vd->static_flag || (vd->scpe && vd->scpe->get_kind() != translation_unit_decl_K)))
            {
               is_internal = true;
            }
            if(GetPointer<const string_cst>(tn))
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
            const auto vd = GetPointer<const var_decl>(tn);
            if(vd && (vd->static_flag || !vd->scpe || vd->scpe->get_kind() == translation_unit_decl_K))
            {
               is_internal = true;
            }
            if(GetPointer<const string_cst>(tn))
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
      if(GetPointer<const gimple_call>(tn))
      {
         is_internal = true;
      }
   }
   return is_internal;
}

/// check if current_vertex is a proxied function
static vertex get_remapped_vertex(vertex current_vertex, const CallGraphManagerConstRef CG, const HLS_managerRef HLSMgr)
{
   const auto current_function_ID = CG->get_function(current_vertex);
   const auto current_function_name = functions::GetFUName(current_function_ID, HLSMgr);
   if(HLSMgr->Rfuns->is_a_proxied_function(current_function_name))
   {
      return CG->GetVertex(HLSMgr->Rfuns->get_proxy_mapping(current_function_name));
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
   const auto TM = HLSMgr->get_tree_manager();
   const auto HLS_D = HLSMgr->get_HLS_device();

   const auto initial_internal_address_p = parameters->isOption(OPT_initial_internal_address);
   const auto initial_internal_address = initial_internal_address_p ?
                                             parameters->getOption<unsigned int>(OPT_initial_internal_address) :
                                             std::numeric_limits<unsigned int>::max();
   const auto unaligned_access_p =
       parameters->isOption(OPT_unaligned_access) && parameters->getOption<bool>(OPT_unaligned_access);
   const auto assume_aligned_access_p =
       parameters->isOption(OPT_aligned_access) && parameters->getOption<bool>(OPT_aligned_access);
   const auto max_bram = HLS_D->get_parameter<unsigned int>("BRAM_bitsize_max");
   /// TODO: to be fixed with information coming out from the target platform description
   HLSMgr->base_address = user_defined_base_address != UINT64_MAX ?
                              user_defined_base_address :
                              parameters->getOption<unsigned long long int>(OPT_base_address);
   const auto null_pointer_check = [&]() {
      if(parameters->isOption(OPT_gcc_optimizations))
      {
         const auto gcc_parameters = parameters->getOption<CustomSet<std::string>>(OPT_gcc_optimizations);
         if(gcc_parameters.find("no-delete-null-pointer-checks") != gcc_parameters.end())
         {
            return false;
         }
      }
      return true;
   }();
   /// information about memory allocation to be shared across the functions
   const memoryRef prevRmem = HLSMgr->Rmem;
   HLSMgr->Rmem =
       memory::create_memory(parameters, TM, HLSMgr->base_address, max_bram, null_pointer_check,
                             initial_internal_address_p, initial_internal_address, HLSMgr->Rget_address_bitsize());
   setup_memory_allocation();

   const auto CGM = HLSMgr->CGetCallGraphManager();
   /// the analysis has to be performed only on the reachable functions
   CustomMap<top_id_t, CallGraphConstRef> cg;
   CustomMap<top_id_t, refcount<dominance<graph>>> cg_dominators;
   OrderedMapStd<top_id_t, CustomUnorderedSet<vertex>> reachable_vertices;
   CustomUnorderedSet<vertex> all_reachable_vertices;
   CustomMap<top_id_t, CustomUnorderedMapStable<vertex, vertex>> cg_dominator_map;
   std::set<func_id_t> top_functions;
   CustomMap<top_id_t, std::vector<func_id_t>> function_allocation_order;

   const auto& root_functions = CGM->GetRootFunctions();
   top_functions.insert(root_functions.begin(), root_functions.end());
   const auto subgraph_from = [&](const vertex& top_vertex, bool is_addr = false, bool include_shared = true) {
      const auto top_function = CGM->get_function(top_vertex);
      CustomUnorderedSet<vertex> preset;
      preset.insert(top_vertex);
      const auto reached_from_top = CGM->GetReachedFunctionsFrom(top_function);
      const auto is_dominated = [&](const vertex& v) {
         BOOST_FOREACH(EdgeDescriptor e, boost::in_edges(v, *CGM->CGetCallGraph()))
         {
            if(!reached_from_top.count(CGM->get_function(boost::source(e, *CGM->CGetCallGraph()))))
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
            const auto funV = CGM->GetVertex(funID);
            if(include_shared || is_dominated(funV))
            {
               preset.insert(funV);
            }
         }
      }
      const auto presub = CGM->CGetCallSubGraph(preset);
      CustomUnorderedSet<vertex> subset;
      subset.insert(top_vertex);
      const auto update_vertices = !is_addr;
      if(update_vertices)
      {
         reachable_vertices[top_function].insert(top_vertex);
         all_reachable_vertices.insert(top_vertex);
      }
      for(const auto& v : preset)
      {
         if(presub->IsReachable(top_vertex, v))
         {
            subset.insert(v);
            if(update_vertices)
            {
               reachable_vertices[top_function].insert(v);
               all_reachable_vertices.insert(v);
            }
         }
      }
      return CGM->CGetCallSubGraph(subset);
   };
   const auto push_allocation_order = [&](std::vector<func_id_t>& allocation_order, top_id_t top_function,
                                          bool include_shared = true) {
      const auto top_vertex = CGM->GetVertex(top_function);
      const auto subgraph = subgraph_from(top_vertex, false, include_shared);
      cg[top_function] = subgraph;
      /// we do not need the exit vertex since the post-dominator graph is not used
      cg_dominators[top_function] =
          refcount<dominance<graph>>(new dominance<graph>(*subgraph, top_vertex, NULL_VERTEX, parameters));
      cg_dominators.at(top_function)->calculate_dominance_info(dominance<graph>::CDI_DOMINATORS);
      cg_dominator_map[top_function] = cg_dominators.at(top_function)->get_dominator_map();
      std::list<vertex> sorted;
      subgraph->TopologicalSort(sorted);
      for(const auto& v : sorted)
      {
         allocation_order.push_back(CGM->get_function(v));
      }
   };
   for(const auto& top_function : root_functions)
   {
      push_allocation_order(function_allocation_order[top_function], top_function);
   }
   for(const auto& addr_func : CGM->GetAddressedFunctions())
   {
      const auto top_vertex = CGM->GetVertex(addr_func);
      const auto subgraph = subgraph_from(top_vertex, true);
      cg[addr_func] = subgraph;
      std::list<vertex> sorted;
      subgraph->TopologicalSort(sorted);
      for(const auto& v : sorted)
      {
         const auto v_id = CGM->get_function(v);
         if(!std::count_if(function_allocation_order.begin(), function_allocation_order.end(),
                           [&](const std::pair<top_id_t, std::vector<func_id_t>>& p) {
                              return std::count(p.second.begin(), p.second.end(), v_id);
                           }))
         {
            function_allocation_order[addr_func].push_back(v_id);
         }
      }
   }

#if HAVE_ASSERTS
   for(const auto& f_order : function_allocation_order)
   {
      CustomSet<unsigned int> check_order;
      for(const auto& f : f_order.second)
      {
         const auto fname = HLSMgr->GetFunctionBehavior(f)->CGetBehavioralHelper()->get_function_name();
         THROW_ASSERT(func_list.count(f), "Function " + fname + " not present in function list");
         THROW_ASSERT(check_order.insert(f).second, "Duplicate fuction in allocation order: " + STR(f) + " " + fname);
      }
   }
#endif

   CustomMap<top_id_t, CustomMap<var_id_t, CustomOrderedSet<vertex>>> var_map;
   CustomMap<top_id_t, CustomMap<var_id_t, CustomOrderedSet<func_id_t>>> where_used;
   const auto compute_var_usage = [&](const func_id_t f_id, const top_id_t top_id) {
      const auto function_behavior = HLSMgr->CGetFunctionBehavior(f_id);
      const auto BH = function_behavior->CGetBehavioralHelper();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Analyzing function: " + BH->get_function_name());
      CustomSet<vertex> vert_dominator;
      const auto current_vertex = get_remapped_vertex(CGM->GetVertex(f_id), CGM, HLSMgr);
      const auto projection_in_degree = boost::in_degree(current_vertex, *cg.at(top_id));

      if(projection_in_degree != 1)
      {
         if(cg_dominator_map.at(top_id).find(current_vertex) != cg_dominator_map.at(top_id).end())
         {
            vert_dominator.insert(get_remapped_vertex(cg_dominator_map.at(top_id).at(current_vertex), CGM, HLSMgr));
         }
      }
      else
      {
         vert_dominator.insert(current_vertex);
      }

      const auto& function_mem = function_behavior->get_function_mem();
      for(const auto v : function_mem)
      {
         if(function_behavior->is_a_state_variable(v))
         {
            var_map[top_id][v].insert(vert_dominator.begin(), vert_dominator.end());
         }
         else
         {
            var_map[top_id][v].insert(current_vertex);
         }
         where_used[top_id][v].insert(f_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Variable : " + BH->PrintVariable(v) + " used in function " +
                            function_behavior->CGetBehavioralHelper()->get_function_name());
         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dominator Vertex: " +
         // HLSMgr->CGetFunctionBehavior(CG->get_function(vert_dominator))->CGetBehavioralHelper()->get_function_name()
         // + " - Variable to be stored: " + BH->PrintVariable(*v));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Analyzed function: " + BH->get_function_name());
   };
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Variable use analysis...");
   for(const auto& top_order : function_allocation_order)
   {
      const auto& top_id = top_order.first;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                     "-->Analyzing top function: " +
                         HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->get_function_name());
      for(const auto& f_id : top_order.second)
      {
         compute_var_usage(f_id, top_id);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                     "<--Analyzed top function: " +
                         HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->get_function_name());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Variable use analysis completed");

   bool all_pointers_resolved = true;
   CustomMap<var_id_t, unsigned long long> var_size;
   CustomMap<var_id_t, CustomMap<func_id_t, CustomOrderedSet<vertex>>> var_referring_vertex_map;
   CustomMap<var_id_t, CustomMap<func_id_t, CustomOrderedSet<vertex>>> var_load_vertex_map;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Pointers classification...");
   for(const auto& fun_id : func_list)
   {
      const auto function_behavior = HLSMgr->CGetFunctionBehavior(fun_id);
      const auto BH = function_behavior->CGetBehavioralHelper();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Analyzing function: " + BH->get_function_name());
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

      const auto g = function_behavior->CGetOpGraph(FunctionBehavior::CFG);
      BOOST_FOREACH(const vertex v, boost::vertices(*g))
      {
         const auto current_op = g->CGetOpNodeInfo(v)->GetOperation();
         /// custom function like printf may create problem to the pointer resolution
         if(current_op == "__builtin_printf" || current_op == BUILTIN_WAIT_CALL || current_op == MEMSET ||
            current_op == MEMCMP || current_op == MEMCPY)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                           "---Pointers not resolved: it uses printf/builtin-wait-call/memset/memcpy/memcmp");
            all_pointers_resolved = false;
         }
         if(GET_TYPE(g, v) & (TYPE_LOAD | TYPE_STORE))
         {
            const auto curr_tn = TM->GetTreeNode(g->CGetOpNodeInfo(v)->GetNodeId());
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "-->Analyzing statement " + GET_NAME(g, v) + ": " + STR(curr_tn));
            const auto me = GetPointer<const gimple_assign>(curr_tn);
            THROW_ASSERT(me, "only gimple_assign's are allowed as memory operations");
            CustomOrderedSet<unsigned int> res_set;

            if(GET_TYPE(g, v) & TYPE_STORE)
            {
               if(!tree_helper::IsPointerResolved(me->op0, res_set))
               {
                  const auto var = tree_helper::GetBaseVariable(me->op0);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---var:" + STR(var));
                  if(var && function_behavior->is_variable_mem(var->index))
                  {
                     res_set.insert(var->index);
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                                    "---Pointers not resolved: point-to-set not resolved");
                     all_pointers_resolved = false;
                  }
               }
            }
            else
            {
               if(!tree_helper::IsPointerResolved(me->op1, res_set))
               {
                  const auto var = tree_helper::GetBaseVariable(me->op1);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---var:" + STR(var));
                  if(var && function_behavior->is_variable_mem(var->index))
                  {
                     res_set.insert(var->index);
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                                    "---Pointers not resolved: point-to-set not resolved " + me->ToString());
                     all_pointers_resolved = false;
                  }
               }
            }
            for(const auto& var : res_set)
            {
               THROW_ASSERT(var, "");
               THROW_ASSERT(function_behavior->is_variable_mem(var),
                            "unexpected condition: " + STR(TM->GetTreeNode(var)) + " is not a memory variable");
               if(HLSMgr->Rmem->has_sds_var(var) && !HLSMgr->Rmem->is_sds_var(var))
               {
                  continue;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable is " + STR(var));
               THROW_ASSERT(g->CGetOpNodeInfo(v), "unexpected condition");
               const auto node_id = g->CGetOpNodeInfo(v)->GetNodeId();
               const auto node = TM->GetTreeNode(node_id);
               const auto gm = GetPointer<const gimple_assign>(node);
               THROW_ASSERT(gm, "only gimple_assign's are allowed as memory operations");
               unsigned long long value_bitsize;
               unsigned long long alignment = 8ULL;
               if(GET_TYPE(g, v) & TYPE_STORE)
               {
                  auto n_last_zerobits = 0u;
                  if(gm->op0->get_kind() == mem_ref_K)
                  {
                     const auto mr = GetPointer<mem_ref>(gm->op0);
                     THROW_ASSERT(GetPointer<integer_cst>(mr->op1), "unexpected condition");
                     THROW_ASSERT(tree_helper::GetConstValue(mr->op1) == 0, "unexpected condition");
                     if(mr->op0->get_kind() == ssa_name_K)
                     {
                        const auto ssa_addr = GetPointer<const ssa_name>(mr->op0);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---STORE SSA pointer " + mr->op0->ToString() +
                                           " bit_values=" + ssa_addr->bit_values);
                        if(ssa_addr->bit_values.find_first_not_of('0') == std::string::npos)
                        {
                           n_last_zerobits = 60; // infinite alignment
                        }
                        else
                        {
                           for(auto it = ssa_addr->bit_values.rbegin(); it != ssa_addr->bit_values.rend(); ++it)
                           {
                              if(*it == '0' || *it == 'X')
                              {
                                 ++n_last_zerobits;
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
                     THROW_ERROR("unexpected condition" + gm->op0->get_kind_text() + " " + gm->ToString());
                  }
                  alignment = (1ull << n_last_zerobits) * 8;
                  const auto var_read = HLSMgr->get_required_values(fun_id, v);
                  const auto size_var = std::get<0>(var_read[0]);
                  const auto size_type = tree_helper::CGetType(TM->GetTreeNode(size_var));
                  value_bitsize = tree_helper::SizeAlloc(size_type);
                  const auto fd = GetPointer<const field_decl>(size_type);
                  if(!fd || !fd->is_bitfield())
                  {
                     value_bitsize = std::max(8ull, value_bitsize);
                  }
                  HLSMgr->Rmem->add_source_value(var, size_var);
               }
               else
               {
                  auto n_last_zerobits = 0u;
                  if(gm->op1->get_kind() == mem_ref_K)
                  {
                     const auto mr = GetPointer<const mem_ref>(gm->op1);
                     THROW_ASSERT(GetPointer<const integer_cst>(mr->op1), "unexpected condition");
                     THROW_ASSERT(tree_helper::GetConstValue(mr->op1) == 0, "unexpected condition");
                     if(mr->op0->get_kind() == ssa_name_K)
                     {
                        const auto ssa_addr = GetPointer<const ssa_name>(mr->op0);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---LOAD SSA pointer " + mr->op0->ToString() +
                                           " bit_values=" + ssa_addr->bit_values);
                        if(ssa_addr->bit_values.find_first_not_of('0') == std::string::npos)
                        {
                           n_last_zerobits = 60; // infinite alignment
                        }
                        else
                        {
                           for(auto it = ssa_addr->bit_values.rbegin(); it != ssa_addr->bit_values.rend(); ++it)
                           {
                              if(*it == '0' || *it == 'X')
                              {
                                 ++n_last_zerobits;
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
                     THROW_ERROR("unexpected condition " + gm->op1->get_kind_text() + " " + gm->ToString());
                  }
                  alignment = (1ull << n_last_zerobits) * 8;
                  const auto size_var = HLSMgr->get_produced_value(fun_id, v);
                  const auto size_type = tree_helper::CGetType(TM->GetTreeNode(size_var));
                  value_bitsize = tree_helper::SizeAlloc(size_type);
                  const auto fd = GetPointer<const field_decl>(size_type);
                  if(!fd || !fd->is_bitfield())
                  {
                     value_bitsize = std::max(8ull, value_bitsize);
                  }
               }

               if(var_size.find(var) == var_size.end())
               {
                  const auto type_node = tree_helper::CGetType(TM->GetTreeNode(var));
                  const auto is_a_struct_union =
                      ((tree_helper::IsStructType(type_node) || tree_helper::IsUnionType(type_node)) &&
                       !tree_helper::IsArrayEquivType(type_node)) ||
                      tree_helper::IsComplexType(type_node);
                  const auto elmt_bitsize = tree_helper::AccessedMaximumBitsize(type_node, 1);
                  const auto min_elmt_bitsize = tree_helper::AccessedMinimunBitsize(type_node, elmt_bitsize);
                  const auto elts_size = tree_helper::IsArrayEquivType(type_node) ?
                                             tree_helper::GetArrayElementSize(type_node) :
                                             elmt_bitsize;
                  if(unaligned_access_p)
                  {
                     if(assume_aligned_access_p)
                     {
                        THROW_ERROR("Option --aligned-access have been specified on a function with unaligned "
                                    "accesses:\n\tVariable " +
                                    BH->PrintVariable(var) + " could be accessed in unaligned way");
                     }
                     HLSMgr->Rmem->set_sds_var(var, false);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Variable " + STR(var) + " not sds because unaligned_access option specified");
                  }
                  else if(min_elmt_bitsize != elmt_bitsize || is_a_struct_union || elts_size != elmt_bitsize)
                  {
                     if(assume_aligned_access_p)
                     {
                        THROW_ERROR("Option --aligned-access have been specified on a function with unaligned "
                                    "accesses:\n\tVariable " +
                                    BH->PrintVariable(var) + " could be accessed in unaligned way");
                     }
                     HLSMgr->Rmem->set_sds_var(var, false);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Variable " + STR(var) + " not sds " + STR(elmt_bitsize) + " vs " +
                                        STR(min_elmt_bitsize) + " vs " + STR(elts_size));
                  }
                  else if(value_bitsize != elmt_bitsize)
                  {
                     if(assume_aligned_access_p)
                     {
                        THROW_ERROR("Option --aligned-access have been specified on a function with unaligned "
                                    "accesses:\n\tVariable " +
                                    BH->PrintVariable(var) +
                                    " could be accessed in unaligned way: " + curr_tn->ToString());
                     }
                     HLSMgr->Rmem->set_sds_var(var, false);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Variable " + STR(var) + " not sds " + STR(value_bitsize) + " vs " +
                                        STR(elmt_bitsize));
                  }
                  else if(alignment < value_bitsize)
                  {
                     if(assume_aligned_access_p)
                     {
                        THROW_WARNING("Option --aligned-access have been specified on a function with not "
                                      "compiler-proved unaligned accesses:\n\tVariable " +
                                      BH->PrintVariable(var) + " could be accessed in unaligned way");
                        THROW_WARNING("\tStatement is " + gm->ToString());
                     }
                     else
                     {
                        HLSMgr->Rmem->set_sds_var(var, false);
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
                                    "---Variable " + STR(var) + " sds " + STR(value_bitsize) + " vs " +
                                        STR(elmt_bitsize));
                     HLSMgr->Rmem->set_sds_var(var, true);
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
                     HLSMgr->Rmem->set_sds_var(var, false);
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
                        THROW_WARNING("\tStatement is " + gm->ToString());
                     }
                     else
                     {
                        HLSMgr->Rmem->set_sds_var(var, false);
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
               if(GET_TYPE(g, v) & TYPE_LOAD)
               {
                  var_load_vertex_map[var][fun_id].insert(v);
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement " + GET_NAME(g, v));
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Analyzed function: " + BH->get_function_name());
   }
   HLSMgr->Rmem->set_all_pointers_resolved(all_pointers_resolved);

   if(all_pointers_resolved)
   {
      for(const auto fun_id : func_list)
      {
         const auto function_behavior = HLSMgr->CGetFunctionBehavior(fun_id);
         const auto BH = function_behavior->CGetBehavioralHelper();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Analyzing function: " + BH->get_function_name());
         const auto g = function_behavior->CGetOpGraph(FunctionBehavior::CFG);
         BOOST_FOREACH(const vertex v, boost::vertices(*g))
         {
            if(GET_TYPE(g, v) & (TYPE_LOAD | TYPE_STORE))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing statement " + GET_NAME(g, v));
               const auto curr_tn = TM->GetTreeNode(g->CGetOpNodeInfo(v)->GetNodeId());
               const auto me = GetPointer<const gimple_assign>(curr_tn);
               THROW_ASSERT(me, "only gimple_assign's are allowed as memory operations");
               tree_nodeRef expr;
               if(GET_TYPE(g, v) & TYPE_STORE)
               {
                  expr = me->op0;
               }
               else
               {
                  expr = me->op1;
               }
               CustomOrderedSet<unsigned int> used_set;
               if(!tree_helper::IsPointerResolved(expr, used_set))
               {
                  const auto var = tree_helper::GetBaseVariable(expr);
                  THROW_ASSERT(var, "unexpected condition");
                  used_set.insert(var->index);
               }
               THROW_ASSERT(!used_set.empty(), "");
               if(used_set.size() > 1)
               {
                  for(const auto used_var : used_set)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Variable need the bus for loads and stores " + BH->PrintVariable(used_var));
                     HLSMgr->Rmem->add_need_bus(used_var);
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement " + GET_NAME(g, v));
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Analyzed function: " + BH->get_function_name());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Pointers classification completed");

   /// compute the number of instances for each function
   CustomMap<top_id_t, CustomMap<vertex, unsigned int>> num_instances;
   CustomMap<top_id_t, CustomMap<func_id_t, std::vector<std::pair<var_id_t, bool>>>> memory_allocation_map;
   for(const auto root_function : root_functions)
   {
      num_instances[root_function][CGM->GetVertex(root_function)] = 1;
   }
   for(const auto addr_function : CGM->GetAddressedFunctions())
   {
      num_instances[addr_function][CGM->GetVertex(addr_function)] = 1;
   }
   const auto compute_instances = [&](const unsigned int f_id, const unsigned int top_id) {
      memory_allocation_map[top_id][f_id];
      const auto top_cg = cg.at(top_id);
      const auto f_v = CGM->GetVertex(f_id);
      if(boost::out_degree(f_v, *top_cg))
      {
         THROW_ASSERT(HLSMgr->get_HLS(f_id),
                      "Missing HLS initialization for " +
                          HLSMgr->CGetFunctionBehavior(f_id)->CGetBehavioralHelper()->get_function_name());
         const auto HLS_C = HLSMgr->get_HLS(f_id)->HLS_C;
         THROW_ASSERT(num_instances.at(top_id).count(f_v),
                      "missing number of instances of function " +
                          HLSMgr->CGetFunctionBehavior(f_id)->CGetBehavioralHelper()->get_function_name());
         const auto cur_instances = num_instances.at(top_id).at(f_v);
         BOOST_FOREACH(EdgeDescriptor e, boost::out_edges(f_v, *top_cg))
         {
            const auto tgt = boost::target(e, *top_cg);
            const auto tgt_fu_name = functions::GetFUName(CGM->get_function(tgt), HLSMgr);
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
               const auto n_call_points =
                   static_cast<unsigned int>(top_cg->CGetFunctionEdgeInfo(e)->direct_call_points.size());
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
   for(const auto& top_vars : var_map)
   {
      const auto& top_id = top_vars.first;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                     "-->Analyzing top function: " +
                         HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->get_function_name());
      for(const auto& var_uses : top_vars.second)
      {
         const auto& var_id = var_uses.first;
         THROW_ASSERT(var_id, "null var index unexpected");
         const auto& uses = var_uses.second;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Finding common dominator for " + STR(TM->GetTreeNode(var_id)) + " (id: " + STR(var_id) +
                            ", uses: " + STR(uses.size()) + ")");
         CustomSet<unsigned int> filtered_top_functions;
         for(const auto& reachable_vertex : reachable_vertices)
         {
            for(const auto use_vertex : uses)
            {
               if(reachable_vertex.second.count(use_vertex))
               {
                  filtered_top_functions.insert(reachable_vertex.first);
               }
            }
         }
         auto funID = top_id;
         bool multiple_top_call_graph = false;
         if(no_local_mem)
         {
            if(filtered_top_functions.size() != 1)
            {
               multiple_top_call_graph = true;
            }
         }
         else
         {
            const auto is_written = HLSMgr->get_written_objects().count(var_id) || no_private_mem;
            if(uses.size() == 1)
            {
               auto cur = *uses.begin();
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Current function(0): " + HLSMgr->CGetFunctionBehavior(CGM->get_function(cur))
                                                               ->CGetBehavioralHelper()
                                                               ->get_function_name());
               const auto fun_behavior = HLSMgr->CGetFunctionBehavior(CGM->get_function(cur));
               /// look for a single instance function in case the object is not a ROM and not a local var
               if(is_written && fun_behavior->is_a_state_variable(var_id))
               {
                  while(num_instances.at(top_id).at(cur) != 1)
                  {
                     cur = get_remapped_vertex(cg_dominator_map.at(top_id).at(cur), CGM, HLSMgr);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Current function(1): " + HLSMgr->CGetFunctionBehavior(CGM->get_function(cur))
                                                                     ->CGetBehavioralHelper()
                                                                     ->get_function_name());
                  }
               }
               funID = CGM->get_function(cur);
            }
            else
            {
               if(filtered_top_functions.size() != 1)
               {
                  multiple_top_call_graph = true;
               }
               else
               {
                  const auto top_vertex = CGM->GetVertex(top_id);
                  const auto vert_it_end = uses.end();
                  auto vert_it = uses.begin();
                  std::list<vertex> dominator_list1;
                  auto cur = *vert_it;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Current function(2a): " + HLSMgr->CGetFunctionBehavior(CGM->get_function(cur))
                                                                   ->CGetBehavioralHelper()
                                                                   ->get_function_name());
                  dominator_list1.push_front(cur);
                  do
                  {
                     cur = get_remapped_vertex(cg_dominator_map.at(top_id).at(cur), CGM, HLSMgr);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Current function(2b): " + HLSMgr->CGetFunctionBehavior(CGM->get_function(cur))
                                                                      ->CGetBehavioralHelper()
                                                                      ->get_function_name());
                     dominator_list1.push_front(cur);
                  } while(cur != top_vertex);
                  ++vert_it;
                  auto last = dominator_list1.end();
                  for(; vert_it != vert_it_end; ++vert_it)
                  {
                     std::list<vertex> dominator_list2;
                     cur = *vert_it;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Current function(2c): " + HLSMgr->CGetFunctionBehavior(CGM->get_function(cur))
                                                                      ->CGetBehavioralHelper()
                                                                      ->get_function_name());
                     dominator_list2.push_front(cur);
                     do
                     {
                        cur = get_remapped_vertex(cg_dominator_map.at(top_id).at(cur), CGM, HLSMgr);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Current function(2d): " +
                                           HLSMgr->CGetFunctionBehavior(CGM->get_function(cur))
                                               ->CGetBehavioralHelper()
                                               ->get_function_name());
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
                                           HLSMgr->CGetFunctionBehavior(CGM->get_function(cur))
                                               ->CGetBehavioralHelper()
                                               ->get_function_name() +
                                           " num instances: " + STR(num_instances.at(top_id).at(cur)));
                        ++dl1_it;
                        cur_last = dl1_it;
                        ++dl2_it;
                     }
                     last = cur_last;
                     funID = CGM->get_function(cur);
                     if(cur == top_vertex)
                     {
                        break;
                     }
                  }
               }
            }
         }
         THROW_ASSERT(funID, "null function id index unexpected");
         const auto is_internal = is_internal_obj(var_id, funID, multiple_top_call_graph);
         memory_allocation_map.at(top_id)[funID].push_back(std::make_pair(var_id, is_internal));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Found common dominator for " +
                            HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->get_function_name() + " -> " +
                            STR(TM->GetTreeNode(var_id)) + " (scope: " + (is_internal ? "internal" : "external") + ")");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                     "<--Analyzed top function: " +
                         HLSMgr->CGetFunctionBehavior(top_id)->CGetBehavioralHelper()->get_function_name());
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
               const auto wiu_it_end = where_used.at(top_id).at(var_id).end();
               for(auto wiu_it = where_used.at(top_id).at(var_id).begin();
                   wiu_it != wiu_it_end && !is_dynamic_address_used; ++wiu_it)
               {
                  const auto cur_function_behavior = HLSMgr->CGetFunctionBehavior(*wiu_it);
                  if(cur_function_behavior->get_dynamic_address().find(var_id) !=
                         cur_function_behavior->get_dynamic_address().end() ||
                     (GetPointer<const var_decl>(TM->GetTreeNode(var_id)) &&
                      GetPointerS<const var_decl>(TM->GetTreeNode(var_id))->addr_taken))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Found dynamic use of variable: " +
                                        cur_function_behavior->CGetBehavioralHelper()->PrintVariable(var_id) + " - " +
                                        STR(var_id) + " - " +
                                        HLSMgr->CGetFunctionBehavior(*(where_used.at(top_id).at(var_id).begin()))
                                            ->CGetBehavioralHelper()
                                            ->PrintVariable(var_id) +
                                        " in function " +
                                        cur_function_behavior->CGetBehavioralHelper()->get_function_name());
                     is_dynamic_address_used = true;
                  }
               }

               if(is_dynamic_address_used && !all_pointers_resolved && !assume_aligned_access_p)
               {
                  HLSMgr->Rmem->set_sds_var(var_id, false);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var_id) + " not sds-A");
               }

               if(!HLSMgr->Rmem->has_sds_var(var_id) && assume_aligned_access_p)
               {
                  HLSMgr->Rmem->set_sds_var(var_id, true);
               }
               else if(!HLSMgr->Rmem->has_sds_var(var_id))
               {
                  HLSMgr->Rmem->set_sds_var(var_id, false);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var_id) + " not sds-B");
               }

               if(!no_private_mem && !no_local_mem)
               {
                  if(!is_dynamic_address_used && /// we never have &(var_id_object)
                     HLSMgr->get_written_objects().find(var_id) ==
                         HLSMgr->get_written_objects().end() /// read only memory
                  )
                  {
                     if(!GetPointer<const gimple_call>(TM->GetTreeNode(var_id)))
                     {
                        HLSMgr->Rmem->add_private_memory(var_id);
                     }
                  }
                  else if(CGM->ExistsAddressedFunction())
                  {
                     if(var_map.at(top_id).at(var_id).size() == 1 && where_used.at(top_id).at(var_id).size() == 1 &&
                        !is_dynamic_address_used &&                              /// we never have &(var_id_object)
                        (*(where_used.at(top_id).at(var_id).begin()) == f_id) && /// used in a single place
                        !GetPointer<const gimple_call>(TM->GetTreeNode(var_id)))
                     {
                        HLSMgr->Rmem->add_private_memory(var_id);
                     }
                  }
                  else
                  {
                     if(!is_dynamic_address_used && /// we never have &(var_id_object)
                        !GetPointer<const gimple_call>(TM->GetTreeNode(var_id)))
                     {
                        HLSMgr->Rmem->add_private_memory(var_id);
                     }
                  }
               }
            }
            else
            {
               is_dynamic_address_used = true;
               if(assume_aligned_access_p)
               {
                  HLSMgr->Rmem->set_sds_var(var_id, true);
               }
               else
               {
                  HLSMgr->Rmem->set_sds_var(var_id, false);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var_id) + " not sds-C");
               }
            }
            const auto vd = GetPointer<const var_decl>(TM->GetTreeNode(var_id));
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
                  const auto curr_size = compute_n_bytes(tree_helper::SizeAlloc(TM->GetTreeNode(var_id)));
                  max_byte_size = std::max(static_cast<unsigned long long>(curr_size), max_byte_size);
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
   const auto allocate_function_mem = [&](const func_id_t f_id, const top_id_t top_id, const memoryRef& Rmem) {
#ifndef NDEBUG
      const auto dbg_lvl = Rmem == HLSMgr->Rmem ? debug_level : DEBUG_LEVEL_NONE;
#endif
      const auto out_lvl = Rmem == HLSMgr->Rmem ? output_level : OUTPUT_LEVEL_NONE;
      THROW_ASSERT(memory_allocation_map.count(top_id), "Invalid top function id.");
      const auto func_mem_map = memory_allocation_map.at(top_id).find(f_id);
      if(func_mem_map != memory_allocation_map.at(top_id).end())
      {
         for(const auto& mem_map : func_mem_map->second)
         {
            const auto var_id = mem_map.first;
            THROW_ASSERT(var_id, "null var index unexpected");
            const auto var_node = TM->GetTreeNode(var_id);
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
               for(auto wiu_it = where_used.at(top_id).at(var_id).begin();
                   wiu_it != wiu_it_end && !is_dynamic_address_used; ++wiu_it)
               {
                  const auto cur_function_behavior = HLSMgr->CGetFunctionBehavior(*wiu_it);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, dbg_lvl,
                                 "---Analyzing function " +
                                     cur_function_behavior->CGetBehavioralHelper()->get_function_name());
                  if(cur_function_behavior->get_dynamic_address().count(var_id) ||
                     (GetPointer<const var_decl>(var_node) && GetPointerS<const var_decl>(var_node)->addr_taken))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, dbg_lvl,
                                    "---Found dynamic use of variable: " +
                                        cur_function_behavior->CGetBehavioralHelper()->PrintVariable(var_id) + " - " +
                                        STR(var_id) + " - " + var_id_string + " in function " +
                                        cur_function_behavior->CGetBehavioralHelper()->get_function_name());
                     is_dynamic_address_used = true;
                  }
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
                                        " - " + var_id_string + " in function " + cur_BH->get_function_name());
                     Rmem->add_internal_variable(*wiu_it, var_id, cur_BH->PrintVariable(var_id));
                  }
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "-->");
               }
               else if(CGM->ExistsAddressedFunction())
               {
                  const auto cur_function_behavior = HLSMgr->CGetFunctionBehavior(f_id);
                  const auto cur_BH = cur_function_behavior->CGetBehavioralHelper();
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                                 "-->Internal variable: " + cur_BH->PrintVariable(var_id) + " - " + STR(var_id) +
                                     " - " + var_id_string + " in function " + cur_BH->get_function_name());
                  Rmem->add_internal_variable(f_id, var_id, cur_BH->PrintVariable(var_id));
               }
               else
               {
                  const auto cur_function_behavior = HLSMgr->CGetFunctionBehavior(f_id);
                  const auto cur_BH = cur_function_behavior->CGetBehavioralHelper();
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                                 "-->Internal variable: " + cur_BH->PrintVariable(var_id) + " - " + STR(var_id) +
                                     " - " + var_id_string + " in function " + cur_BH->get_function_name());
                  Rmem->add_internal_variable(f_id, var_id, cur_BH->PrintVariable(var_id));
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
            const auto is_packed = GetPointer<const decl_node>(var_node) && tree_helper::IsPackedType(var_node);
            Rmem->set_packed_vars(is_packed);

            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl, "---Id: " + STR(var_id));
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                           "---Base Address: " + STR(Rmem->get_base_address(var_id, f_id)));
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                           "---Size: " + STR(compute_n_bytes(tree_helper::SizeAlloc(var_node))));
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
                              "---The variable is always accessed with the same data size");
               const auto vd = GetPointer<const var_decl>(var_node);
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
                  max_references =
                      max_references > fun_vertex_set.second.size() ? max_references : fun_vertex_set.second.size();
               }
               Rmem->set_maximum_references(var_id, max_references);
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, out_lvl,
                              "---Maximum number of references per function: " + STR(max_references));
            }
            if(var_load_vertex_map.count(var_id))
            {
               size_t max_loads = 0;
               for(const auto& fun_vertex_set : var_load_vertex_map.at(var_id))
               {
                  max_loads = max_loads > fun_vertex_set.second.size() ? max_loads : fun_vertex_set.second.size();
               }
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
      if(f_id && (top_functions.count(f_id) ? memory_mapped_top_if : HLSMgr->hasToBeInterfaced(f_id)))
      {
         allocate_parameters(f_id, Rmem);
      }
   };

   // Allocate memory for root functions
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Memory allocation phase...");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                  "Internal memory address base: " + STR(HLSMgr->Rmem->get_next_internal_base_address()));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Top functions");
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
