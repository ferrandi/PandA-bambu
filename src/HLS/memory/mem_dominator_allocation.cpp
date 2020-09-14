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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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

#include "functions.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "memory.hpp"
#include "target_device.hpp"
#include "technology_manager.hpp"

#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

#include "Dominance.hpp"
#include "Parameter.hpp"
#include "constant_strings.hpp"
#include "cpu_time.hpp"
#include "math_function.hpp"
#include "module_interface.hpp"

/// STD includes
#include <limits>
#include <string>

/// STL includes
#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <list>
#include <utility>

#include "custom_map.hpp"
#include "custom_set.hpp"

/// technology include
#include "technology_node.hpp"

/// tree includes
#include "ext_tree_node.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

mem_dominator_allocation::mem_dominator_allocation(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization,
                                                   const HLSFlowStep_Type _hls_flow_step_type)
    : memory_allocation(_parameters, _HLSMgr, _design_flow_manager, _hls_flow_step_type, _hls_flow_step_specialization), user_defined_base_address(static_cast<unsigned long long>(-1LL))
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

mem_dominator_allocation::~mem_dominator_allocation() = default;

static void buildAllocationOrderRecursively(const HLS_managerRef HLSMgr, std::vector<unsigned int>& List, unsigned int topFunction)
{
   const CustomOrderedSet<unsigned int> calledSet = HLSMgr->CGetCallGraphManager()->get_called_by(topFunction);
   List.push_back(topFunction);
   for(unsigned int Itr : calledSet)
   {
      if(not HLSMgr->hasToBeInterfaced(Itr) && std::find(List.begin(), List.end(), Itr) == List.end())
      {
         buildAllocationOrderRecursively(HLSMgr, List, Itr);
      }
   }
}

void mem_dominator_allocation::Initialize()
{
   std::vector<std::string> xml_files;
   if(parameters->isOption(OPT_xml_memory_allocation))
   {
      xml_files.push_back(parameters->getOption<std::string>(OPT_xml_memory_allocation));
   }
   else
   {
      /// load xml memory allocation file
      for(auto source_file : HLSMgr->input_files)
      {
         const std::string output_temporary_directory = parameters->getOption<std::string>(OPT_output_temporary_directory);
         std::string leaf_name = source_file.second == "-" ? "stdin-" : GetLeafFileName(source_file.second);
         auto XMLfilename = output_temporary_directory + "/" + leaf_name + ".memory_allocation.xml";
         if((boost::filesystem::exists(boost::filesystem::path(XMLfilename))))
         {
            xml_files.push_back(XMLfilename);
         }
      }
   }
   for(auto XMLfilename : xml_files)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->parsing " + XMLfilename);
      XMLDomParser parser(XMLfilename);
      parser.Exec();
      if(parser)
      {
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
         const xml_node::node_list list = node->get_children();
         for(const auto& l : list)
         {
            const xml_element* child = GetPointer<xml_element>(l);
            if(!child)
               continue;
            if(child->get_name() == "memory_allocation")
            {
               unsigned long long int base_address = static_cast<unsigned long long int>(-1LL);
               if(CE_XVM(base_address, child))
                  LOAD_XVM(base_address, child);
               user_defined_base_address = base_address;
               for(const auto& it : child->get_children())
               {
                  const xml_element* mem_node = GetPointer<xml_element>(it);
                  if(!mem_node)
                     continue;
                  if(mem_node->get_name() == "object")
                  {
                     std::string is_internal;
                     if(!CE_XVM(is_internal, mem_node))
                        THROW_ERROR("expected the is_internal attribute");
                     LOAD_XVM(is_internal, mem_node);
                     if(is_internal == "T")
                     {
                        if(!CE_XVM(scope, mem_node))
                           THROW_ERROR("expected the scope attribute when the object is internal");
                        std::string scope;
                        LOAD_XVM(scope, mem_node);
                        if(!CE_XVM(name, mem_node))
                           THROW_ERROR("expected the name attribute");
                        std::string name;
                        LOAD_XVM(name, mem_node);
                        user_internal_objects[scope].insert(name);
                     }
                     else if(is_internal == "F")
                     {
                        std::string scope;
                        if(CE_XVM(scope, mem_node))
                           LOAD_XVM(scope, mem_node);
                        else
                           scope = "*";
                        if(!CE_XVM(name, mem_node))
                           THROW_ERROR("expected the name attribute");
                        std::string name;
                        LOAD_XVM(name, mem_node);
                        user_external_objects[scope].insert(name);
                     }
                     else
                        THROW_ERROR("unexpected value for is_internal attribute");
                  }
               }
            }
         }
         /// check xml consistency
         for(auto user_obj : user_internal_objects)
         {
            for(auto var_obj : user_obj.second)
            {
               if(user_external_objects.find(user_obj.first) != user_external_objects.end() && user_external_objects.find(user_obj.first)->second.find(var_obj) != user_external_objects.find(user_obj.first)->second.end())
                  THROW_ERROR("An allocated object cannot be both internal and external: " + var_obj + " in function " + user_obj.first);
               if(user_external_objects.find("*") != user_external_objects.end() && user_external_objects.find("*")->second.find(var_obj) != user_external_objects.find("*")->second.end())
                  THROW_ERROR("An allocated object cannot be both internal and external: " + var_obj + " in function " + user_obj.first);
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parsed file " + XMLfilename);
   }
}

std::vector<unsigned int> mem_dominator_allocation::getFunctionAllocationOrder(CustomOrderedSet<unsigned int> top_functions)
{
   std::vector<unsigned int> functionAllocationOrder;
   for(const auto top_function : top_functions)
   {
      buildAllocationOrderRecursively(HLSMgr, functionAllocationOrder, top_function);
   }

   const CallGraphManagerConstRef CG = HLSMgr->CGetCallGraphManager();
   const CustomOrderedSet<unsigned int> additional_tops = CG->GetAddressedFunctions();
   for(auto const additional_top : additional_tops)
   {
      buildAllocationOrderRecursively(HLSMgr, functionAllocationOrder, additional_top);
   }
   return functionAllocationOrder;
}

bool mem_dominator_allocation::is_internal_obj(unsigned int var_index, const std::string& var_name, const std::string& fun_name, bool multiple_top_call_graph, const tree_managerRef TreeM)
{
   bool is_internal = false;
   if(user_external_objects.find(fun_name) != user_external_objects.end() && user_external_objects.find(fun_name)->second.find(var_name) != user_external_objects.find(fun_name)->second.end())
      return false;
   if(user_external_objects.find("*") != user_external_objects.end() && user_external_objects.find("*")->second.find(var_name) != user_external_objects.find("*")->second.end())
      return false;
   if(user_internal_objects.find(fun_name) != user_internal_objects.end() && user_internal_objects.find(fun_name)->second.find(var_name) != user_internal_objects.find(fun_name)->second.end())
      return true;
   if(not multiple_top_call_graph)
   {
      const tree_nodeRef tn = TreeM->get_tree_node_const(var_index);
      switch(memory_allocation_policy)
      {
         case MemoryAllocation_Policy::LSS:
         {
            auto* vd = GetPointer<var_decl>(tn);
            if(vd && (vd->static_flag || (vd->scpe && GET_NODE(vd->scpe)->get_kind() != translation_unit_decl_K)))
               is_internal = true;
            if(GetPointer<string_cst>(tn))
               is_internal = true;
            if(HLSMgr->Rmem->is_parm_decl_copied(var_index) || HLSMgr->Rmem->is_parm_decl_stored(var_index))
               is_internal = true;
            break;
         }
         case MemoryAllocation_Policy::GSS:
         {
            auto* vd = GetPointer<var_decl>(tn);
            if(vd && (vd->static_flag || !vd->scpe || GET_NODE(vd->scpe)->get_kind() == translation_unit_decl_K))
               is_internal = true;
            if(GetPointer<string_cst>(tn))
               is_internal = true;
            if(HLSMgr->Rmem->is_parm_decl_copied(var_index) || HLSMgr->Rmem->is_parm_decl_stored(var_index))
               is_internal = true;
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
      if(GetPointer<gimple_call>(tn))
         is_internal = true;
   }
   return is_internal;
}

/// check if current_vertex is a proxied function
static vertex get_remapped_vertex(vertex current_vertex, const CallGraphManagerConstRef CG, const HLS_managerRef HLSMgr)
{
   unsigned int current_function_ID = CG->get_function(current_vertex);
   std::string current_function_name = functions::get_function_name_cleaned(current_function_ID, HLSMgr);
   if(HLSMgr->Rfuns->is_a_proxied_function(current_function_name))
   {
      return CG->GetVertex(HLSMgr->Rfuns->get_proxy_mapping(current_function_name));
   }
   return current_vertex;
}

DesignFlowStep_Status mem_dominator_allocation::InternalExec()
{
   long int step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      START_TIME(step_time);
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Memory allocation information:");
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();

   const HLS_targetRef HLS_T = HLSMgr->get_HLS_target();
   /// TODO: to be fixed with information coming out from the target platform description
   auto base_address = parameters->getOption<unsigned long long int>(OPT_base_address);
   if(user_defined_base_address != static_cast<unsigned long long>(-1))
      base_address = user_defined_base_address;
   bool initial_internal_address_p = parameters->isOption(OPT_initial_internal_address);
   unsigned int initial_internal_address = initial_internal_address_p ? parameters->getOption<unsigned int>(OPT_initial_internal_address) : std::numeric_limits<unsigned int>::max();
   auto max_bram = HLS_T->get_target_device()->get_parameter<unsigned int>("BRAM_bitsize_max");
   HLSMgr->base_address = base_address;
   bool null_pointer_check = true;
   if(parameters->isOption(OPT_gcc_optimizations))
   {
      const auto gcc_parameters = parameters->getOption<const CustomSet<std::string>>(OPT_gcc_optimizations);
      if(gcc_parameters.find("no-delete-null-pointer-checks") != gcc_parameters.end())
         null_pointer_check = false;
   }
   /// information about memory allocation to be shared across the functions
   memoryRef prevRmem = HLSMgr->Rmem;
   HLSMgr->Rmem = memoryRef(memory::create_memory(parameters, TreeM, base_address, max_bram, null_pointer_check, initial_internal_address_p, initial_internal_address, HLSMgr->Rget_address_bitsize()));
   setup_memory_allocation();

   const CallGraphManagerConstRef CG = HLSMgr->CGetCallGraphManager();
   /// the analysis has to be performed only on the reachable functions
   const CallGraphConstRef cg = CG->CGetCallGraph();
   CustomMap<unsigned int, refcount<dominance<graph>>> cg_dominators;
   OrderedMapStd<unsigned int, CustomUnorderedSet<vertex>> reachable_vertices;
   CustomUnorderedSet<vertex> all_reachable_vertices;
   CustomMap<unsigned int, CustomUnorderedMapStable<vertex, vertex>> cg_dominator_map;
   auto top_functions = CG->GetRootFunctions();
   for(const auto top_function : top_functions)
   {
      vertex top_vertex = CG->GetVertex(top_function);
      CustomOrderedSet<unsigned int> temp = CG->GetReachedBodyFunctionsFrom(top_function);
      for(const auto temp_int : temp)
      {
         auto cVertex = CG->GetVertex(temp_int);
         reachable_vertices[top_function].insert(cVertex);
         all_reachable_vertices.insert(cVertex);
      }
      const CallGraphConstRef subgraph = CG->CGetCallSubGraph(reachable_vertices[top_function]);
      /// we do not need the exit vertex since the post-dominator graph is not used
      cg_dominators[top_function] = refcount<dominance<graph>>(new dominance<graph>(*subgraph, top_vertex, NULL_VERTEX, parameters));
      cg_dominators[top_function]->calculate_dominance_info(dominance<graph>::CDI_DOMINATORS);
      cg_dominator_map[top_function] = cg_dominators[top_function]->get_dominator_map();
   }

   std::map<unsigned, std::map<unsigned int, CustomOrderedSet<vertex>>> var_referring_vertex_map;
   std::map<unsigned, std::map<unsigned int, CustomOrderedSet<vertex>>> var_load_vertex_map;
   std::map<unsigned int, CustomOrderedSet<vertex>> var_map;
   std::map<unsigned int, CustomOrderedSet<unsigned int>> where_used;
   bool all_pointers_resolved = true;
   bool unaligned_access_p = parameters->isOption(OPT_unaligned_access) && parameters->getOption<bool>(OPT_unaligned_access);
   bool assume_aligned_access_p = parameters->isOption(OPT_aligned_access) && parameters->getOption<bool>(OPT_aligned_access);
   if(unaligned_access_p && assume_aligned_access_p)
      THROW_ERROR("Both --unaligned-access and --aligned-access have been specified");
   std::map<unsigned int, unsigned int> var_size;

   for(const auto fun_id : func_list)
   {
      const FunctionBehaviorConstRef function_behavior = HLSMgr->CGetFunctionBehavior(fun_id);
      const BehavioralHelperConstRef BH = function_behavior->CGetBehavioralHelper();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Analyzing function: " + BH->get_function_name());
      //      if(function_behavior->get_has_globals())
      //      {
      //         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Pointers not resolved: it has global variables");
      //         all_pointers_resolved = false;
      //      }
      if(function_behavior->get_has_undefined_function_receiving_pointers())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Pointers not resolved: it has undefined function receiving pointers");
         all_pointers_resolved = false;
      }
      CustomSet<vertex> vert_dominator;
      vertex current_vertex = get_remapped_vertex(CG->GetVertex(fun_id), CG, HLSMgr);
      unsigned int projection_in_degree = 0;
      InEdgeIterator ei, ei_end;
      for(boost::tie(ei, ei_end) = boost::in_edges(current_vertex, *cg); ei != ei_end; ++ei)
      {
         vertex src_vrtx = boost::source(*ei, *cg);
         if(func_list.find(CG->get_function(src_vrtx)) != func_list.end())
            ++projection_in_degree;
      }

      if(projection_in_degree != 1)
      {
         for(const auto& top_function : cg_dominator_map)
         {
            if(top_function.second.find(current_vertex) != top_function.second.end())
            {
               vert_dominator.insert(get_remapped_vertex(top_function.second.find(current_vertex)->second, CG, HLSMgr));
            }
         }
      }
      else
      {
         vert_dominator.insert(current_vertex);
      }

      const CustomOrderedSet<unsigned int>& function_mem = function_behavior->get_function_mem();
      for(unsigned int v : function_mem)
      {
         if(function_behavior->is_a_state_variable(v))
         {
            var_map[v].insert(vert_dominator.begin(), vert_dominator.end());
         }
         else
         {
            var_map[v].insert(current_vertex);
         }
         where_used[v].insert(fun_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable : " + BH->PrintVariable(v) + " used in function " + function_behavior->CGetBehavioralHelper()->get_function_name());
         // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Dominator Vertex: " + HLSMgr->CGetFunctionBehavior(CG->get_function(vert_dominator))->CGetBehavioralHelper()->get_function_name() + " - Variable to be stored: " +
         // BH->PrintVariable(*v));
      }
      const OpGraphConstRef g = function_behavior->CGetOpGraph(FunctionBehavior::CFG);
      graph::vertex_iterator v, v_end;
      for(boost::tie(v, v_end) = boost::vertices(*g); v != v_end; ++v)
      {
         std::string current_op = g->CGetOpNodeInfo(*v)->GetOperation();
         /// custom function like printf may create problem to the pointer resolution
         if(current_op == "__builtin_printf" || current_op == BUILTIN_WAIT_CALL || current_op == MEMSET || current_op == MEMCMP || current_op == MEMCPY)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Pointers not resolved: it uses printf/builtin-wait-call/memset/memcpy/memcmp");
            all_pointers_resolved = false;
         }
         if(GET_TYPE(g, *v) & (TYPE_LOAD | TYPE_STORE))
         {
            const tree_nodeRef curr_tn = TreeM->get_tree_node_const(g->CGetOpNodeInfo(*v)->GetNodeId());
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing statement " + GET_NAME(g, *v) + ": " + STR(curr_tn));
            auto* me = GetPointer<gimple_assign>(curr_tn);
            THROW_ASSERT(me, "only gimple_assign's are allowed as memory operations");
            unsigned int expr_index;
            CustomOrderedSet<unsigned int> res_set;

            if(GET_TYPE(g, *v) & TYPE_STORE)
            {
               expr_index = GET_INDEX_NODE(me->op0);
               if(!tree_helper::is_fully_resolved(TreeM, expr_index, res_set))
               {
                  const auto var = tree_helper::get_base_index(TreeM, expr_index);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---var:" + STR(var));
                  if(var != 0 && function_behavior->is_variable_mem(var))
                  {
                     res_set.insert(var);
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Pointers not resolved: point-to-set not resolved");
                     all_pointers_resolved = false;
                  }
               }
            }
            else
            {
               expr_index = GET_INDEX_NODE(me->op1);
               if(!tree_helper::is_fully_resolved(TreeM, expr_index, res_set))
               {
                  auto var = tree_helper::get_base_index(TreeM, expr_index);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---var:" + STR(var));
                  if(var != 0 && function_behavior->is_variable_mem(var))
                  {
                     res_set.insert(var);
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Pointers not resolved: point-to-set not resolved");
                     all_pointers_resolved = false;
                  }
               }
            }
            for(auto var : res_set)
            {
               THROW_ASSERT(var, "");
               THROW_ASSERT(function_behavior->is_variable_mem(var), "unexpected condition: " + STR(TreeM->CGetTreeNode(var)) + " is not a memory variable");
               if(HLSMgr->Rmem->has_sds_var(var) && !HLSMgr->Rmem->is_sds_var(var))
                  continue;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable is " + STR(var));
               unsigned int value_bitsize;
               THROW_ASSERT(g->CGetOpNodeInfo(*v), "unexpected condition");
               unsigned int node_id = g->CGetOpNodeInfo(*v)->GetNodeId();
               const tree_nodeRef node = TreeM->get_tree_node_const(node_id);
               auto* gm = GetPointer<gimple_assign>(node);
               THROW_ASSERT(gm, "only gimple_assign's are allowed as memory operations");
               auto alignment = 8ull;
               if(GET_TYPE(g, *v) & TYPE_STORE)
               {
                  auto n_last_zerobits = 0u;
                  if(GET_NODE(gm->op0)->get_kind() == mem_ref_K)
                  {
                     auto* mr = GetPointer<mem_ref>(GET_NODE(gm->op0));
                     THROW_ASSERT(GetPointer<integer_cst>(GET_NODE(mr->op1)), "unexpected condition");
                     THROW_ASSERT(tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(mr->op1))) == 0, "unexpected condition");
                     if(GET_NODE(mr->op0)->get_kind() == ssa_name_K)
                     {
                        auto ssa_addr = GetPointer<ssa_name>(GET_NODE(mr->op0));
                        for(auto it = ssa_addr->bit_values.rbegin(); it != ssa_addr->bit_values.rend(); ++it)
                        {
                           if(*it == '0' || *it == 'X')
                              ++n_last_zerobits;
                           else
                              break;
                        }
                     }
                     else
                        THROW_ERROR("unexpected condition");
                  }
                  else
                     THROW_ERROR("unexpected condition" + GET_NODE(gm->op0)->get_kind_text() + " " + gm->ToString());
                  alignment = (1ull << n_last_zerobits) * 8;
                  std::vector<HLS_manager::io_binding_type> var_read = HLSMgr->get_required_values(fun_id, *v);
                  unsigned int size_var = std::get<0>(var_read[0]);
                  unsigned int size_type_index = tree_helper::get_type_index(TreeM, size_var);
                  value_bitsize = tree_helper::size(TreeM, size_type_index);
                  auto* fd = GetPointer<field_decl>(TreeM->get_tree_node_const(size_type_index));
                  if(!fd or !fd->is_bitfield())
                     value_bitsize = std::max(8u, value_bitsize);
                  HLSMgr->Rmem->add_source_value(var, size_var);
               }
               else
               {
                  auto n_last_zerobits = 0u;
                  if(GET_NODE(gm->op1)->get_kind() == mem_ref_K)
                  {
                     auto* mr = GetPointer<mem_ref>(GET_NODE(gm->op1));
                     THROW_ASSERT(GetPointer<integer_cst>(GET_NODE(mr->op1)), "unexpected condition");
                     THROW_ASSERT(tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(mr->op1))) == 0, "unexpected condition");
                     if(GET_NODE(mr->op0)->get_kind() == ssa_name_K)
                     {
                        auto ssa_addr = GetPointer<ssa_name>(GET_NODE(mr->op0));
                        for(auto it = ssa_addr->bit_values.rbegin(); it != ssa_addr->bit_values.rend(); ++it)
                        {
                           if(*it == '0' || *it == 'X')
                              ++n_last_zerobits;
                           else
                              break;
                        }
                     }
                     else
                        THROW_ERROR("unexpected condition");
                  }
                  else
                     THROW_ERROR("unexpected condition " + GET_NODE(gm->op1)->get_kind_text() + " " + gm->ToString());
                  alignment = (1ull << n_last_zerobits) * 8;
                  unsigned int size_var = HLSMgr->get_produced_value(fun_id, *v);
                  unsigned int size_type_index = tree_helper::get_type_index(TreeM, size_var);
                  value_bitsize = tree_helper::size(TreeM, size_type_index);
                  auto* fd = GetPointer<field_decl>(TreeM->get_tree_node_const(size_type_index));
                  if(!fd or !fd->is_bitfield())
                     value_bitsize = std::max(8u, value_bitsize);
               }

               if(var_size.find(var) == var_size.end())
               {
                  unsigned int elmt_bitsize = 1;
                  unsigned int type_index = tree_helper::get_type_index(TreeM, var);
                  bool is_a_struct_union = ((tree_helper::is_a_struct(TreeM, type_index)) && !tree_helper::is_an_array(TreeM, type_index)) || tree_helper::is_an_union(TreeM, type_index) || tree_helper::is_a_complex(TreeM, type_index);
                  tree_nodeRef type_node = TreeM->get_tree_node_const(type_index);
                  tree_helper::accessed_greatest_bitsize(TreeM, type_node, type_index, elmt_bitsize);
                  unsigned int mim_elmt_bitsize = elmt_bitsize;
                  tree_helper::accessed_minimum_bitsize(TreeM, type_node, type_index, mim_elmt_bitsize);
                  unsigned int elts_size = elmt_bitsize;
                  if(tree_helper::is_an_array(TreeM, type_index))
                     elts_size = tree_helper::get_array_data_bitsize(TreeM, type_index);
                  if(unaligned_access_p)
                  {
                     if(assume_aligned_access_p)
                        THROW_ERROR("Option --aligned-access have been specified on a function with unaligned accesses:\n\tVariable " + BH->PrintVariable(var) + " could be accessed in unaligned way");
                     HLSMgr->Rmem->set_sds_var(var, false);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var) + " not sds because unaligned_access option specified");
                  }
                  else if(mim_elmt_bitsize != elmt_bitsize || is_a_struct_union || elts_size != elmt_bitsize)
                  {
                     if(assume_aligned_access_p)
                        THROW_ERROR("Option --aligned-access have been specified on a function with unaligned accesses:\n\tVariable " + BH->PrintVariable(var) + " could be accessed in unaligned way");
                     HLSMgr->Rmem->set_sds_var(var, false);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var) + " not sds " + STR(elmt_bitsize) + " vs " + STR(mim_elmt_bitsize) + " vs " + STR(elts_size));
                  }
                  else if(value_bitsize != elmt_bitsize)
                  {
                     if(assume_aligned_access_p)
                        THROW_ERROR("Option --aligned-access have been specified on a function with unaligned accesses:\n\tVariable " + BH->PrintVariable(var) + " could be accessed in unaligned way: " + curr_tn->ToString());
                     HLSMgr->Rmem->set_sds_var(var, false);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var) + " not sds " + STR(value_bitsize) + " vs " + STR(elmt_bitsize));
                  }
                  else if(alignment < value_bitsize)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Variable " + STR(var) + " not sds because alignment " + STR(alignment) + " is less than the value loaded or written or than the size of the array elements " + STR(value_bitsize));
                     if(assume_aligned_access_p)
                     {
                        THROW_WARNING("Option --aligned-access have been specified on a function with not compiler-proved unaligned accesses:\n\tVariable " + BH->PrintVariable(var) + " could be accessed in unaligned way");
                        THROW_WARNING("\tStatement is " + gm->ToString());
                     }
                     else
                     {
                        HLSMgr->Rmem->set_sds_var(var, false);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Variable " + STR(var) + " not sds " + STR(value_bitsize) + " vs " + STR(var_size.find(var)->second) + " alignment=" + STR(alignment) + " value_bitsize1=" + STR(value_bitsize));
                     }
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var) + " sds " + STR(value_bitsize) + " vs " + STR(elmt_bitsize));
                     HLSMgr->Rmem->set_sds_var(var, true);
                  }
                  var_size[var] = value_bitsize;
               }
               else
               {
                  if(var_size.find(var)->second != value_bitsize)
                  {
                     if(assume_aligned_access_p)
                        THROW_ERROR("Option --aligned-access have been specified on a function with unaligned accesses:\n\tVariable " + BH->PrintVariable(var) + " could be accessed in unaligned way");
                     HLSMgr->Rmem->set_sds_var(var, false);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var) + " not sds " + STR(value_bitsize) + " vs " + STR(var_size.find(var)->second));
                  }
                  else if(alignment < var_size.find(var)->second)
                  {
                     if(assume_aligned_access_p)
                     {
                        THROW_WARNING("Option --aligned-access have been specified on a function with not compiler-proved unaligned accesses:\n\tVariable " + BH->PrintVariable(var) + " could be accessed in unaligned way");
                        THROW_WARNING("\tStatement is " + gm->ToString());
                     }
                     else
                     {
                        HLSMgr->Rmem->set_sds_var(var, false);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Variable " + STR(var) + " not sds " + STR(value_bitsize) + " vs " + STR(var_size.find(var)->second) + " alignment=" + STR(alignment) + " value_bitsize2=" + STR(var_size.find(var)->second));
                     }
                  }
               }
               /// var referring vertex map
               var_referring_vertex_map[var][fun_id].insert(*v);
               if(GET_TYPE(g, *v) & TYPE_LOAD)
                  var_load_vertex_map[var][fun_id].insert(*v);
               ;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement " + GET_NAME(g, *v));
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Analyzed function: " + BH->get_function_name());
   }
   HLSMgr->Rmem->set_all_pointers_resolved(all_pointers_resolved);

   if(all_pointers_resolved)
   {
      for(const auto fun_id : func_list)
      {
         const FunctionBehaviorConstRef function_behavior = HLSMgr->CGetFunctionBehavior(fun_id);
         const BehavioralHelperConstRef BH = function_behavior->CGetBehavioralHelper();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Analyzing function: " + BH->get_function_name());
         const OpGraphConstRef g = function_behavior->CGetOpGraph(FunctionBehavior::CFG);
         graph::vertex_iterator v, v_end;
         for(boost::tie(v, v_end) = boost::vertices(*g); v != v_end; ++v)
         {
            if(GET_TYPE(g, *v) & (TYPE_LOAD | TYPE_STORE))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing statement " + GET_NAME(g, *v));
               const tree_nodeRef curr_tn = TreeM->get_tree_node_const(g->CGetOpNodeInfo(*v)->GetNodeId());
               auto* me = GetPointer<gimple_assign>(curr_tn);
               THROW_ASSERT(me, "only gimple_assign's are allowed as memory operations");
               unsigned int expr_index;
               if(GET_TYPE(g, *v) & TYPE_STORE)
               {
                  expr_index = GET_INDEX_NODE(me->op0);
               }
               else
               {
                  expr_index = GET_INDEX_NODE(me->op1);
               }
               CustomOrderedSet<unsigned int> used_set;
               bool resolved = tree_helper::is_fully_resolved(TreeM, expr_index, used_set);
               if(!resolved)
               {
                  auto var = tree_helper::get_base_index(TreeM, expr_index);
                  THROW_ASSERT(var, "unexpected condition");
                  used_set.insert(var);
                  resolved = true;
               }
               assert(resolved && !used_set.empty());
               if(resolved && used_set.size() > 1)
               {
                  for(auto used_var : used_set)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable need the bus for loads and stores " + BH->PrintVariable(used_var));
                     HLSMgr->Rmem->add_need_bus(used_var);
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement " + GET_NAME(g, *v));
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Analyzed function: " + BH->get_function_name());
      }
   }

   /// compute the number of instances for each function
   std::map<vertex, unsigned int> num_instances;
   for(const auto top_function : top_functions)
   {
      vertex top_vertex = CG->GetVertex(top_function);
      num_instances[top_vertex] = 1;
   }
   std::map<unsigned int, std::vector<std::pair<unsigned int, bool>>> memory_allocation_map;
   const HLS_constraintsRef HLS_C = HLS_constraintsRef(new HLS_constraints(HLSMgr->get_parameter(), ""));
   const CallGraphConstRef cg_projection = CG->CGetCallSubGraph(all_reachable_vertices);
   std::list<vertex> topology_sorted_vertex;
   cg_projection->TopologicalSort(topology_sorted_vertex);
   const auto& reached_fu_ids = CG->GetReachedBodyFunctions();
   for(const auto cur : topology_sorted_vertex)
   {
      const unsigned int funID = CG->get_function(cur);
      if(reached_fu_ids.find(funID) == reached_fu_ids.end())
         continue;
      memory_allocation_map[funID];
      THROW_ASSERT(num_instances.find(cur) != num_instances.end(), "missing number of instances of function " + HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->get_function_name());
      unsigned int cur_instances = num_instances.find(cur)->second;
      OutEdgeIterator eo, eo_end;
      for(boost::tie(eo, eo_end) = boost::out_edges(cur, *cg); eo != eo_end; ++eo)
      {
         vertex tgt = boost::target(*eo, *cg);
         std::string tgt_fu_name = functions::get_function_name_cleaned(CG->get_function(tgt), HLSMgr);
         if(HLSMgr->Rfuns->is_a_proxied_function(tgt_fu_name) || (parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::WB4_INTERFACE_GENERATION && HLSMgr->hasToBeInterfaced(funID)))
         {
            num_instances[tgt] = 1;
         }
         else if(HLS_C->get_number_fu(tgt_fu_name, WORK_LIBRARY) == 1)
         {
            num_instances[tgt] = 1;
         }
         else
         {
            unsigned int n_call_points = static_cast<unsigned int>(Cget_edge_info<FunctionEdgeInfo, const CallGraph>(*eo, *cg)->direct_call_points.size());
            if(num_instances.find(tgt) == num_instances.end())
               num_instances[tgt] = cur_instances * n_call_points;
            else
               num_instances[tgt] += cur_instances * n_call_points;
         }
      }
   }
   // THROW_ASSERT(num_instances.find(top_vertex)->second == 1, "something of wrong happened");

   /// find the common dominator and decide where to allocate
   const std::map<unsigned int, CustomOrderedSet<vertex>>::const_iterator it_end = var_map.end();
   for(std::map<unsigned int, CustomOrderedSet<vertex>>::const_iterator it = var_map.begin(); it != it_end; ++it)
   {
      bool multiple_top_call_graph = false;
      unsigned int funID = 0;
      unsigned int var_index = it->first;
      THROW_ASSERT(var_index, "null var index unexpected");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Finding common dominator for variable " + STR(var_index));
      CustomSet<unsigned int> filtered_top_functions;
      for(const auto& reachable_vertex : reachable_vertices)
      {
         for(const auto use_vertex : it->second)
         {
            if(reachable_vertex.second.find(use_vertex) != reachable_vertex.second.end())
               filtered_top_functions.insert(reachable_vertex.first);
         }
      }
      if(parameters->IsParameter("no-local-mem") && parameters->GetParameter<int>("no-local-mem") == 1)
      {
         if(filtered_top_functions.size() != 1)
         {
            funID = *(filtered_top_functions.begin());
            multiple_top_call_graph = true;
         }
         else
            funID = *(filtered_top_functions.begin());
      }
      else
      {
         bool is_written = HLSMgr->get_written_objects().find(var_index) != HLSMgr->get_written_objects().end() || (parameters->IsParameter("no-private-mem") && parameters->GetParameter<int>("no-private-mem") == 1);
         if(it->second.size() == 1)
         {
            vertex cur = *it->second.begin();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Current function(0): " + HLSMgr->CGetFunctionBehavior(CG->get_function(cur))->CGetBehavioralHelper()->get_function_name());
            const auto filtered_top_function = (*(filtered_top_functions.begin()));
            const FunctionBehaviorConstRef fun_behavior = HLSMgr->CGetFunctionBehavior(CG->get_function(cur));
            /// look for a single instance function in case the object is not a ROM and not a local var
            if(is_written && fun_behavior->is_a_state_variable(var_index))
            {
               while(num_instances.find(cur)->second != 1)
               {
                  cur = get_remapped_vertex(cg_dominator_map[filtered_top_function].find(cur)->second, CG, HLSMgr);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Current function(1): " + HLSMgr->CGetFunctionBehavior(CG->get_function(cur))->CGetBehavioralHelper()->get_function_name());
               }
            }
            funID = CG->get_function(cur);
         }
         else
         {
            if(filtered_top_functions.size() != 1)
            {
               funID = *(filtered_top_functions.begin());
               multiple_top_call_graph = true;
            }
            else
            {
               const auto top_id = (*(filtered_top_functions.begin()));
               const auto top_vertex = CG->GetVertex(top_id);
               auto vert_it_end = it->second.end();
               auto vert_it = it->second.begin();
               std::list<vertex> dominator_list1;
               vertex cur = *vert_it;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Current function(2a): " + HLSMgr->CGetFunctionBehavior(CG->get_function(cur))->CGetBehavioralHelper()->get_function_name());
               dominator_list1.push_front(cur);
               do
               {
                  cur = get_remapped_vertex(cg_dominator_map[top_id].find(cur)->second, CG, HLSMgr);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Current function(2b): " + HLSMgr->CGetFunctionBehavior(CG->get_function(cur))->CGetBehavioralHelper()->get_function_name());
                  dominator_list1.push_front(cur);
               } while(cur != top_vertex);
               ++vert_it;
               std::list<vertex>::const_iterator last = dominator_list1.end();
               for(; vert_it != vert_it_end; ++vert_it)
               {
                  std::list<vertex> dominator_list2;
                  cur = *vert_it;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Current function(2c): " + HLSMgr->CGetFunctionBehavior(CG->get_function(cur))->CGetBehavioralHelper()->get_function_name());
                  dominator_list2.push_front(cur);
                  do
                  {
                     cur = get_remapped_vertex(cg_dominator_map[top_id].find(cur)->second, CG, HLSMgr);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Current function(2d): " + HLSMgr->CGetFunctionBehavior(CG->get_function(cur))->CGetBehavioralHelper()->get_function_name());
                     dominator_list2.push_front(cur);
                  } while(cur != top_vertex);
                  /// find the common dominator between two candidates
                  std::list<vertex>::const_iterator dl1_it = dominator_list1.begin(), dl2_it = dominator_list2.begin(), dl2_it_end = dominator_list2.end(), cur_last = dominator_list1.begin();
                  while(dl1_it != last && dl2_it != dl2_it_end && *dl1_it == *dl2_it && (num_instances.find(*dl1_it)->second == 1 || !is_written))
                  {
                     cur = *dl1_it;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Current function(2e): " + HLSMgr->CGetFunctionBehavior(CG->get_function(cur))->CGetBehavioralHelper()->get_function_name() + " num instances: " + STR(num_instances.find(cur)->second));
                     ++dl1_it;
                     cur_last = dl1_it;
                     ++dl2_it;
                  }
                  last = cur_last;
                  funID = CG->get_function(cur);
                  if(cur == top_vertex)
                     break;
               }
            }
         }
      }
      THROW_ASSERT(funID, "null function id index unexpected");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Found common dominator for variable " + STR(var_index) + ":" + HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->get_function_name());

      bool is_internal = is_internal_obj(var_index, HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->PrintVariable(var_index), HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->get_function_name(), multiple_top_call_graph, TreeM);

      memory_allocation_map[funID].push_back(std::make_pair(var_index, is_internal));
   }

   /// classify variable
   for(const auto& funID : getFunctionAllocationOrder(top_functions))
   {
      memory_allocation_map[funID];
      for(auto& mem_map : memory_allocation_map.at(funID))
      {
         unsigned int var_index = mem_map.first;
         THROW_ASSERT(var_index, "null var index unexpected");
         bool is_internal = mem_map.second;
         bool is_dynamic_address_used = false;

         if(is_internal)
         {
            THROW_ASSERT(where_used[var_index].size() > 0, "variable not used anywhere");
            const FunctionBehaviorConstRef function_behavior = HLSMgr->CGetFunctionBehavior(*(where_used[var_index].begin()));
            const BehavioralHelperConstRef BH = function_behavior->CGetBehavioralHelper();
            /// check dynamic address use
            auto wiu_it_end = where_used[var_index].end();
            for(auto wiu_it = where_used[var_index].begin(); wiu_it != wiu_it_end && !is_dynamic_address_used; ++wiu_it)
            {
               const FunctionBehaviorConstRef cur_function_behavior = HLSMgr->CGetFunctionBehavior(*wiu_it);
               if(cur_function_behavior->get_dynamic_address().find(var_index) != cur_function_behavior->get_dynamic_address().end() ||
                  (GetPointer<var_decl>(TreeM->get_tree_node_const(var_index)) && GetPointer<var_decl>(TreeM->get_tree_node_const(var_index))->addr_taken))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Found dynamic use of variable: " + cur_function_behavior->CGetBehavioralHelper()->PrintVariable(var_index) + " - " + STR(var_index) + " - " + BH->PrintVariable(var_index) + " in function " +
                                     cur_function_behavior->CGetBehavioralHelper()->get_function_name());
                  is_dynamic_address_used = true;
               }
            }

            if(is_dynamic_address_used && !all_pointers_resolved && !assume_aligned_access_p)
            {
               HLSMgr->Rmem->set_sds_var(var_index, false);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var_index) + " not sds-A");
            }

            if(!HLSMgr->Rmem->has_sds_var(var_index) && assume_aligned_access_p)
               HLSMgr->Rmem->set_sds_var(var_index, true);
            else if(!HLSMgr->Rmem->has_sds_var(var_index))
            {
               HLSMgr->Rmem->set_sds_var(var_index, false);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var_index) + " not sds-B");
            }

            if((!parameters->IsParameter("no-private-mem") || parameters->GetParameter<int>("no-private-mem") == 0) && (!parameters->IsParameter("no-local-mem") || parameters->GetParameter<int>("no-local-mem") == 0))
            {
               if(!is_dynamic_address_used &&                                                          /// we never have &(var_index_object)
                  HLSMgr->get_written_objects().find(var_index) == HLSMgr->get_written_objects().end() /// read only memory
               )
               {
                  if(!GetPointer<const gimple_call>(TreeM->CGetTreeNode(var_index)))
                     HLSMgr->Rmem->add_private_memory(var_index);
               }
               else if(CG->ExistsAddressedFunction())
               {
                  if(var_map[var_index].size() == 1 && where_used[var_index].size() == 1 && !is_dynamic_address_used && /// we never have &(var_index_object)
                     (*(where_used[var_index].begin()) == funID) &&                                                     /// used in a single place
                     !GetPointer<const gimple_call>(TreeM->CGetTreeNode(var_index)))
                  {
                     HLSMgr->Rmem->add_private_memory(var_index);
                  }
               }
               else
               {
                  if(!is_dynamic_address_used && /// we never have &(var_index_object)
                     !GetPointer<const gimple_call>(TreeM->CGetTreeNode(var_index)))
                  {
                     HLSMgr->Rmem->add_private_memory(var_index);
                  }
               }
            }
         }
         else
         {
            const FunctionBehaviorConstRef function_behavior = HLSMgr->CGetFunctionBehavior(funID);
            const BehavioralHelperConstRef BH = function_behavior->CGetBehavioralHelper();
            is_dynamic_address_used = true;
            if(assume_aligned_access_p)
               HLSMgr->Rmem->set_sds_var(var_index, true);
            else
            {
               HLSMgr->Rmem->set_sds_var(var_index, false);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable " + STR(var_index) + " not sds-C");
            }
         }
         const tree_nodeRef curr_tn = TreeM->get_tree_node_const(var_index);
         auto* vd = GetPointer<var_decl>(curr_tn);
         if((vd && vd->readonly_flag) || (HLSMgr->get_written_objects().find(var_index) == HLSMgr->get_written_objects().end() && !is_dynamic_address_used))
         {
            HLSMgr->Rmem->add_read_only_variable(var_index);
         }
      }
   }

   /// change the alignment in case is requested
   if(parameters->isOption(OPT_sparse_memory) && parameters->getOption<bool>(OPT_sparse_memory))
   {
      /// change the internal alignment to improve the decoding logic
      unsigned int max_byte_size = HLSMgr->Rmem->get_internal_base_address_alignment();
      for(const auto& mem_map : memory_allocation_map)
      {
         for(const auto& pair : mem_map.second)
         {
            unsigned int var_index = pair.first;
            THROW_ASSERT(var_index, "null var index unexpected");
            if(pair.second && (!HLSMgr->Rmem->is_private_memory(var_index) || null_pointer_check))
            {
               unsigned int curr_size = compute_n_bytes(tree_helper::size(TreeM, var_index));
               max_byte_size = std::max(curr_size, max_byte_size);
            }
         }
      }
      /// Round up to the next highest power of 2
      max_byte_size--;
      max_byte_size |= max_byte_size >> 1;
      max_byte_size |= max_byte_size >> 2;
      max_byte_size |= max_byte_size >> 4;
      max_byte_size |= max_byte_size >> 8;
      max_byte_size |= max_byte_size >> 16;
      max_byte_size++;
      HLSMgr->Rmem->set_internal_base_address_alignment(max_byte_size);
   }

   /// really allocate
   for(const auto& funID : getFunctionAllocationOrder(top_functions))
   {
      memory_allocation_map[funID];
      for(const auto& mem_map : memory_allocation_map.at(funID))
      {
         unsigned int var_index = mem_map.first;
         THROW_ASSERT(var_index, "null var index unexpected");
         bool is_internal = mem_map.second;
         std::string var_index_string;
         bool is_dynamic_address_used = false;

         if(is_internal)
         {
            THROW_ASSERT(where_used[var_index].size() > 0, "variable not used anywhere");
            const FunctionBehaviorConstRef function_behavior = HLSMgr->CGetFunctionBehavior(*(where_used[var_index].begin()));
            const BehavioralHelperConstRef BH = function_behavior->CGetBehavioralHelper();
            var_index_string = BH->PrintVariable(var_index);
            /// check dynamic address use
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Check dynamic use for var " + var_index_string);
            auto wiu_it_end = where_used[var_index].end();
            for(auto wiu_it = where_used[var_index].begin(); wiu_it != wiu_it_end && !is_dynamic_address_used; ++wiu_it)
            {
               const FunctionBehaviorConstRef cur_function_behavior = HLSMgr->CGetFunctionBehavior(*wiu_it);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing function " + cur_function_behavior->CGetBehavioralHelper()->get_function_name());
               if(cur_function_behavior->get_dynamic_address().find(var_index) != cur_function_behavior->get_dynamic_address().end() ||
                  (GetPointer<var_decl>(TreeM->get_tree_node_const(var_index)) && GetPointer<var_decl>(TreeM->get_tree_node_const(var_index))->addr_taken))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Found dynamic use of variable: " + cur_function_behavior->CGetBehavioralHelper()->PrintVariable(var_index) + " - " + STR(var_index) + " - " + var_index_string + " in function " +
                                     cur_function_behavior->CGetBehavioralHelper()->get_function_name());
                  is_dynamic_address_used = true;
               }
            }

            if(!is_dynamic_address_used &&                                                          /// we never have &(var_index_object)
               HLSMgr->get_written_objects().find(var_index) == HLSMgr->get_written_objects().end() /// read only memory
               && ((!parameters->IsParameter("no-private-mem") || parameters->GetParameter<int>("no-private-mem") == 0) && (!parameters->IsParameter("no-local-mem") || parameters->GetParameter<int>("no-local-mem") == 0)))
            {
               for(auto wiu_it = where_used[var_index].begin(); wiu_it != wiu_it_end; ++wiu_it)
               {
                  const FunctionBehaviorConstRef cur_function_behavior = HLSMgr->CGetFunctionBehavior(*wiu_it);
                  const BehavioralHelperConstRef cur_BH = cur_function_behavior->CGetBehavioralHelper();
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Internal variable: " + cur_BH->PrintVariable(var_index) + " - " + STR(var_index) + " - " + var_index_string + " in function " + cur_BH->get_function_name());
                  HLSMgr->Rmem->add_internal_variable(*wiu_it, var_index, cur_BH->PrintVariable(var_index));
               }
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->");
            }
            else if(CG->ExistsAddressedFunction())
            {
               const FunctionBehaviorConstRef cur_function_behavior = HLSMgr->CGetFunctionBehavior(funID);
               const BehavioralHelperConstRef cur_BH = cur_function_behavior->CGetBehavioralHelper();
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Internal variable: " + cur_BH->PrintVariable(var_index) + " - " + STR(var_index) + " - " + var_index_string + " in function " + cur_BH->get_function_name());
               HLSMgr->Rmem->add_internal_variable(funID, var_index, cur_BH->PrintVariable(var_index));
            }
            else
            {
               const FunctionBehaviorConstRef cur_function_behavior = HLSMgr->CGetFunctionBehavior(funID);
               const BehavioralHelperConstRef cur_BH = cur_function_behavior->CGetBehavioralHelper();
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Internal variable: " + cur_BH->PrintVariable(var_index) + " - " + STR(var_index) + " - " + var_index_string + " in function " + cur_BH->get_function_name());
               HLSMgr->Rmem->add_internal_variable(funID, var_index, cur_BH->PrintVariable(var_index));
               /// add proxies
               if((!parameters->IsParameter("no-private-mem") || parameters->GetParameter<int>("no-private-mem") == 0) && (!parameters->IsParameter("no-local-mem") || parameters->GetParameter<int>("no-local-mem") == 0))
               {
                  for(const auto& wu_id : where_used[var_index])
                     if(wu_id != funID)
                        HLSMgr->Rmem->add_internal_variable_proxy(wu_id, var_index);
               }
            }
         }
         else
         {
            const FunctionBehaviorConstRef function_behavior = HLSMgr->CGetFunctionBehavior(funID);
            const BehavioralHelperConstRef BH = function_behavior->CGetBehavioralHelper();
            var_index_string = BH->PrintVariable(var_index);
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Variable external to the top module: " + BH->PrintVariable(var_index) + " - " + STR(var_index) + " - " + var_index_string);
            HLSMgr->Rmem->add_external_variable(var_index, BH->PrintVariable(var_index));
            is_dynamic_address_used = true;
         }
         bool is_packed = GetPointer<decl_node>(TreeM->get_tree_node_const(var_index)) && tree_helper::is_packed(TreeM, var_index);
         HLSMgr->Rmem->set_packed_vars(is_packed);

         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Id: " + STR(var_index));
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Base Address: " + STR(HLSMgr->Rmem->get_base_address(var_index, funID)));
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Size: " + STR(compute_n_bytes(tree_helper::size(TreeM, var_index))));
         if(HLSMgr->Rmem->is_private_memory(var_index))
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Is a private memory");
         if(HLSMgr->Rmem->is_a_proxied_variable(var_index))
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Has proxied accesses");
         if(HLSMgr->Rmem->is_read_only_variable(var_index))
         {
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Is a Read Only Memory");
         }
         if(HLSMgr->Rmem->is_parm_decl_copied(var_index))
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Is a parm decl copied");
         if(HLSMgr->Rmem->is_actual_parm_loaded(var_index))
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Is an actual parm decl loaded");
         if(HLSMgr->Rmem->is_parm_decl_stored(var_index))
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Is a parm decl stored");
         if(is_dynamic_address_used)
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Used &(object)");
         if(HLSMgr->Rmem->is_sds_var(var_index))
         {
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---The variable is always accessed with the same data size");
            const tree_nodeRef curr_tn = TreeM->get_tree_node_const(var_index);
            auto* vd = GetPointer<var_decl>(curr_tn);
            if(vd && vd->bit_values.size() != 0)
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---The variable has been trimmed to bitsize: " + STR(vd->bit_values.size()) + " with bit-value pattern: " + vd->bit_values);
         }
         if(var_referring_vertex_map.find(var_index) != var_referring_vertex_map.end())
         {
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Number of functions in which is used: " + STR(var_referring_vertex_map.find(var_index)->second.size()));
            size_t max_references = 0;
            for(auto fun_vertex_set : var_referring_vertex_map.find(var_index)->second)
               max_references = max_references > static_cast<size_t>(fun_vertex_set.second.size()) ? max_references : static_cast<size_t>(fun_vertex_set.second.size());
            HLSMgr->Rmem->set_maximum_references(var_index, max_references);
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Maximum number of references per function: " + STR(max_references));
         }
         if(var_load_vertex_map.find(var_index) != var_load_vertex_map.end())
         {
            size_t max_loads = 0;
            for(auto fun_vertex_set : var_load_vertex_map.find(var_index)->second)
               max_loads = max_loads > static_cast<size_t>(fun_vertex_set.second.size()) ? max_loads : static_cast<size_t>(fun_vertex_set.second.size());
            HLSMgr->Rmem->set_maximum_loads(var_index, max_loads);
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Maximum number of loads per function: " + STR(max_loads));
         }
         if(is_packed)
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Variable is packed");
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
      }
      if(funID and HLSMgr->hasToBeInterfaced(funID) and top_functions.find(funID) == top_functions.end())
      {
         allocate_parameters(funID);
      }
      else

          if(funID and top_functions.find(funID) != top_functions.end() and (parameters->getOption<bool>(OPT_memory_mapped_top) || (parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::WB4_INTERFACE_GENERATION)))
      {
         allocate_parameters(funID);
      }
   }

   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      STOP_TIME(step_time);
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   finalize_memory_allocation();
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Time to perform memory allocation: " + print_cpu_time(step_time) + " seconds");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   bool changed = HLSMgr->Rmem->notEQ(prevRmem);
   if(changed)
   {
      HLSMgr->UpdateMemVersion();
      /// clean proxy library
      auto TM = HLS_T->get_technology_manager();
      TM->erase_library(PROXY_LIBRARY);
      TM->erase_library(WORK_LIBRARY);
   }
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
