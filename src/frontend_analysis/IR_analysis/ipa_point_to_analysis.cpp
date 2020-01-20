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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file ipa_point_to_analysis.cpp
 * @brief Perform an inter-procedural flow sensitive point-to analysis.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "ipa_point_to_analysis.hpp"
#include "config_HAVE_PRAGMA_BUILT.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// Parameter include
#include "Parameter.hpp"
#include "module_interface.hpp"

/// Tree include
#include "behavioral_helper.hpp"
#include "ext_tree_node.hpp"
#include "op_graph.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// boost include
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include <boost/range/adaptors.hpp>

REF_FORWARD_DECL(function_information);

struct function_information
{
   /// topological order identifier
   unsigned int topo_id;
   /// a function is non-preserving if it operates on pointer-type
   /// variables or passes them to another function
   bool preserving;
   /// called functions, directly or through function pointers
   CustomOrderedSet<unsigned int> called_functions;
   /// list of statements to be processed
   std::list<unsigned int> stmts_list;
   explicit function_information(unsigned int _topo_id) : topo_id(_topo_id), preserving(true)
   {
   }
};

void ipa_point_to_analysis::compute_function_topological_order(std::list<unsigned int>& sort_list)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "compute function topological order...");
   std::list<vertex> topology_sorted_vertex;
   const CallGraphManagerConstRef CG = AppM->CGetCallGraphManager();
   CG->CGetCallGraph()->TopologicalSort(topology_sorted_vertex);

   CustomOrderedSet<unsigned> reachable_functions;

   /// check if the root function is an empty function: compiler optimizations may kill everything
   auto top_functions = CG->GetRootFunctions();
   /// the analysis has to be performed only on the reachable functions
   if(parameters->isOption(OPT_top_design_name)) // top design function become the top_vertex
   {
      const auto saved_top_functions = top_functions;
      top_functions.clear();
      const auto top_function = AppM->get_tree_manager()->function_index(parameters->getOption<std::string>(OPT_top_design_name));
      if(top_function)
      {
         if(tree_helper::is_a_nop_function_decl(GetPointer<function_decl>(AppM->get_tree_manager()->get_tree_node_const(top_function))))
         {
            THROW_ERROR("the top function is empty or the compiler killed all the statements");
         }
         top_functions.insert(top_function);
      }
      else
      {
         if(output_level > OUTPUT_LEVEL_VERBOSE)
            THROW_WARNING("Top RTL name refers to a HDL-only module");
         top_functions = saved_top_functions;
      }
      reachable_functions = CG->GetReachedBodyFunctionsFrom(*(top_functions.begin()));
   }
   else
      reachable_functions = CG->GetReachedBodyFunctions();

   for(auto v : topology_sorted_vertex)
   {
      auto fun_id = CG->get_function(v);
      if(reachable_functions.find(fun_id) != reachable_functions.end())
         sort_list.push_back(fun_id);
   }
}

ipa_point_to_analysis::ipa_point_to_analysis(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, IPA_POINT_TO_ANALYSIS, _design_flow_manager, _parameters), TM(_AppM->get_tree_manager())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

ipa_point_to_analysis::~ipa_point_to_analysis() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> ipa_point_to_analysis::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(IR_LOWERING, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(USE_COUNTING, ALL_FUNCTIONS));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BUILD_VIRTUAL_PHI, ALL_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CSE_STEP, ALL_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(FANOUT_OPT, ALL_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(HLS_DIV_CG_EXT, ALL_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(MEM_CG_EXT, ALL_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SOFT_FLOAT_CG_EXT, ALL_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BIT_VALUE_OPT, ALL_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(LUT_TRANSFORMATION, ALL_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTRACT_PATTERNS, ALL_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(FUNCTION_CALL_TYPE_CLEANUP, ALL_FUNCTIONS));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SPLIT_RETURN, ALL_FUNCTIONS));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

class worklist_queue
{
 public:
   worklist_queue() = default;
   ~worklist_queue() = default;

   /**
    * @brief insert a statement in the priority queue
    * @param statement_id
    * @param priority_value
    */
   void insert(unsigned int statement_id, unsigned int priority_value)
   {
      priority_queue_data.push(std::make_pair(statement_id, priority_value));
   }

   /// return the first element in the priority queue
   unsigned int get()
   {
      THROW_ASSERT(!empty(), "work list is empty");

      std::pair<unsigned int, unsigned int> el = priority_queue_data.top();
      priority_queue_data.pop();
      while(!priority_queue_data.empty() && el.second == priority_queue_data.top().second)
         priority_queue_data.pop();

      return el.first;
   }

   /// check if the priority queue is empty
   bool empty() const
   {
      return priority_queue_data.empty();
   }

 private:
   struct comp_functor : std::binary_function<const std::pair<unsigned int, unsigned int>, const std::pair<unsigned int, unsigned int>, bool>
   {
      bool operator()(const std::pair<unsigned int, unsigned int>& a, const std::pair<unsigned int, unsigned int>& b) const
      {
         return (a.second < b.second);
      }
   };

   std::priority_queue<std::pair<unsigned int, unsigned int>, std::vector<std::pair<unsigned int, unsigned int>>, comp_functor> priority_queue_data;
};

DesignFlowStep_Status ipa_point_to_analysis::Exec()
{
   CustomUnorderedSet<unsigned int> pointing_to_ssa_vars;
   CustomUnorderedMapUnstable<unsigned int, unsigned int> topo_stmt_order;
   // CustomUnorderedMap<unsigned int,unsigned int> topo_func_parameter_order;
   unsigned int curr_topo_order = 0;
   /// array storing the functions for which the address has been taken by some gimple node.
   CustomOrderedSet<unsigned int> addr_taken_functions;
   worklist_queue wl;

   std::map<unsigned int, function_informationRef> function_information_map;

   const CallGraphManagerConstRef CG = AppM->CGetCallGraphManager();
   const CallGraphConstRef cg = CG->CGetCallGraph();

   /// compute ssa variables that may work as a pointer variables
   /// first compute seeds
   std::list<unsigned int> functions;
   compute_function_topological_order(functions);
   unsigned int topo_func_order = 0;
   for(const auto function : functions)
   {
      const tree_nodeRef curr_tn = TM->get_tree_node_const(function);
      auto* fd = GetPointer<function_decl>(curr_tn);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining function " + tree_helper::name_function(TM, function));
      function_information_map[function] = function_informationRef(new function_information(topo_func_order));
      topo_func_order++;
      OutEdgeIterator ei, ei_end;
      for(boost::tie(ei, ei_end) = boost::out_edges(CG->GetVertex(function), *cg); ei != ei_end; ++ei)
      {
         vertex tgt_vrtx = boost::target(*ei, *cg);
         auto called_id = CG->get_function(tgt_vrtx);
         function_information_map[function]->called_functions.insert(called_id);
         if(function_information_map.find(called_id) != function_information_map.end())
            function_information_map[function]->preserving &= function_information_map[called_id]->preserving;
         else
         {
            std::string fun_name = tree_helper::name_function(TM, called_id);
            if(fun_name == "open") /// may use pointers
            {
               function_information_map[function]->preserving &= false; // check if this is actually equal to true
            }
            else if(fun_name != "signbit" && fun_name != "__builtin_signbit" && fun_name != "signbitf" && fun_name != "__builtin_signbitf" && fun_name != "fabs" && fun_name != "fabsf" && fun_name != "llabs" && fun_name != "labs" && fun_name != "abs" &&
                    fun_name != "putchar" && fun_name != "__builtin_putchar" && fun_name != "__bambu_read4c" && fun_name != "__bambu_readc" && fun_name != "abort" && fun_name != "exit")
               function_information_map[function]->preserving = false;
         }
      }

      /// reserve priorities for the function parameters
      // for(auto par : fd->list_of_args)
      //{
      //   topo_func_parameter_order[GET_INDEX_NODE(par)] = curr_topo_order++;
      //}
      auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
      std::map<unsigned int, blocRef>& blocks = sl->list_of_bloc;
      std::map<unsigned int, blocRef>::iterator it, it_end;
      it_end = blocks.end();
      for(it = blocks.begin(); it != it_end; ++it)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + boost::lexical_cast<std::string>(it->first));
         for(auto stmt : it->second->CGetStmtList())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + GET_NODE(stmt)->ToString());
            topo_stmt_order[GET_INDEX_NODE(stmt)] = curr_topo_order++;
            if(GET_NODE(stmt)->get_kind() == gimple_assign_K)
            {
               auto* ga = GetPointer<gimple_assign>(GET_NODE(stmt));
               unsigned int output_uid = GET_INDEX_NODE(ga->op0);
               auto* ssa = GetPointer<ssa_name>(GET_NODE(ga->op0));
               if(ssa)
               {
                  if(GET_NODE(ga->op1)->get_kind() == addr_expr_K)
                  {
                     pointing_to_ssa_vars.insert(output_uid);
                     wl.insert(GET_INDEX_NODE(stmt), topo_stmt_order.find(GET_INDEX_NODE(stmt))->second);
                     auto* ae = GetPointer<addr_expr>(GET_NODE(ga->op1));
                     if(GET_NODE(ae->op)->get_kind() == var_decl_K || GET_NODE(ae->op)->get_kind() == parm_decl_K || GET_NODE(ae->op)->get_kind() == function_decl_K || GET_NODE(ae->op)->get_kind() == string_cst_K)
                     {
                        ssa->use_set = PointToSolutionRef(new PointToSolution());
                        ssa->use_set->Add(ae->op);
                        if(GET_NODE(ae->op)->get_kind() == function_decl_K)
                           addr_taken_functions.insert(GET_INDEX_NODE(ae->op));
                     }
                     else if(GET_NODE(ae->op)->get_kind() == mem_ref_K)
                     {
                        auto* MR = GetPointer<mem_ref>(GET_NODE(ae->op));
                        THROW_ASSERT(GET_NODE(MR->op0)->get_kind() == ssa_name_K && GetPointer<integer_cst>(GET_NODE(MR->op1)) && tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(MR->op1))) == 0,
                                     "expected a ssa_name as first operand of a mem_ref " + GET_NODE(stmt)->ToString() + " but " + STR(GET_NODE(MR->op0)) + " is a " + GET_NODE(MR->op0)->get_kind_text() +
                                         "; "
                                         "second operand " +
                                         STR(GET_NODE(MR->op1)) + " is a " + GET_NODE(MR->op1)->get_kind_text());
                        pointing_to_ssa_vars.insert(GET_INDEX_NODE(MR->op0));
                     }
                     else
                     {
                        THROW_ERROR("unexpected pattern: addr_expr " + STR(ga) + " takes address of " + GET_NODE(ae->op)->get_kind_text());
                     }
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == pointer_plus_expr_K)
                  {
                     pointing_to_ssa_vars.insert(output_uid);
                     wl.insert(GET_INDEX_NODE(stmt), topo_stmt_order.find(GET_INDEX_NODE(stmt))->second);
                     THROW_ASSERT(GET_NODE(GetPointer<pointer_plus_expr>(GET_NODE(ga->op1))->op0)->get_kind() == ssa_name_K || GET_NODE(GetPointer<pointer_plus_expr>(GET_NODE(ga->op1))->op0)->get_kind() == integer_cst_K,
                                  "expected a ssa_name as first operand of a pointer plus expression" + GET_NODE(stmt)->ToString());
                     if(GET_NODE(GetPointer<pointer_plus_expr>(GET_NODE(ga->op1))->op0)->get_kind() == ssa_name_K)
                        pointing_to_ssa_vars.insert(GET_INDEX_NODE(GetPointer<pointer_plus_expr>(GET_NODE(ga->op1))->op0));
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == mem_ref_K)
                  {
                     auto* MR = GetPointer<mem_ref>(GET_NODE(ga->op1));
                     THROW_ASSERT(MR, STR(GET_NODE(ga->op1)) + " is not a mem_ref but a " + GET_NODE(ga->op1)->get_kind_text());
                     if(GET_NODE(MR->op0)->get_kind() != integer_cst_K)
                     {
                        THROW_ASSERT(GET_NODE(MR->op0)->get_kind() == ssa_name_K && GetPointer<integer_cst>(GET_NODE(MR->op1)) && tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(MR->op1))) == 0,
                                     "expected a ssa_name as first operand of a mem_ref " + GET_NODE(stmt)->ToString() + " but " + STR(GET_NODE(MR->op0)) + " is a " + GET_NODE(MR->op0)->get_kind_text() +
                                         "; "
                                         "second operand " +
                                         STR(GET_NODE(MR->op1)) + " is a " + GET_NODE(MR->op1)->get_kind_text());
                        pointing_to_ssa_vars.insert(GET_INDEX_NODE(MR->op0));
                     }
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == call_expr_K || GET_NODE(ga->op1)->get_kind() == aggr_init_expr_K)
                  {
                     auto* ce = GetPointer<call_expr>(GET_NODE(ga->op1));
                     if(GET_NODE(ce->fn)->get_kind() == ssa_name_K)
                     {
                        pointing_to_ssa_vars.insert(GET_INDEX_NODE(ce->fn));
                     }
                     auto* ae = GetPointer<addr_expr>(GET_NODE(ce->fn));
                     THROW_ASSERT(ae, "unexpected pattern" + GET_NODE(stmt)->ToString());
                     if(function_information_map[function]->preserving)
                     {
                        for(auto par : ce->args)
                        {
                           if(GET_NODE(par)->get_kind() == ssa_name_K)
                           {
                              if(tree_helper::is_a_pointer(TM, GET_INDEX_NODE(par)))
                              {
                                 function_information_map[function]->preserving = false;
                                 break;
                              }
                           }
                        }
                     }

                     auto* called_fd = GetPointer<function_decl>(GET_NODE(ae->op));
                     if(not called_fd->body)
                     {
                        std::string fun_name = tree_helper::print_function_name(TM, called_fd);
                        if(fun_name == "open") /// may use pointers
                        {
                           THROW_ASSERT(ce->args.size() >= 1, "unexpected pattern");
                           THROW_ASSERT(GET_NODE(ce->args.at(0))->get_kind() == ssa_name_K, "unexpected pattern");
                           pointing_to_ssa_vars.insert(GET_INDEX_NODE(ce->args.at(0)));
                        }
                        else if(fun_name != "signbit" && fun_name != "__builtin_signbit" && fun_name != "signbitf" && fun_name != "__builtin_signbitf" && fun_name != "fabs" && fun_name != "fabsf" && fun_name != "llabs" && fun_name != "labs" &&
                                fun_name != "abs" && fun_name != "putchar" && fun_name != "__builtin_putchar" && fun_name != "__bambu_read4c" && fun_name != "__bambu_readc")
                        {
                           if(output_level > OUTPUT_LEVEL_VERBOSE)
                              THROW_WARNING("function " + fun_name + " does not have a source C body so we have to be very restrictive on its parameters and on the usage of the value returned...");
                           for(auto par : ce->args)
                              if(GET_NODE(par)->get_kind() == ssa_name_K)
                              {
                                 pointing_to_ssa_vars.insert(GET_INDEX_NODE(par));
                              }
                           pointing_to_ssa_vars.insert(GET_INDEX_NODE(ga->op0));
                        }
                     }
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == target_mem_ref461_K)
                  {
                     THROW_ERROR("unexpected pattern" + GET_NODE(stmt)->ToString());
                  }
               }
               else if(GET_NODE(ga->op0)->get_kind() == mem_ref_K)
               {
                  auto* MR = GetPointer<mem_ref>(GET_NODE(ga->op0));
                  THROW_ASSERT(MR, STR(GET_NODE(ga->op0)) + " is not a mem_ref but a " + GET_NODE(ga->op0)->get_kind_text());
                  if(GET_NODE(MR->op0)->get_kind() != integer_cst_K)
                  {
                     THROW_ASSERT(GET_NODE(MR->op0)->get_kind() == ssa_name_K && GetPointer<integer_cst>(GET_NODE(MR->op1)) && tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(MR->op1))) == 0,
                                  "expected a ssa_name as first operand of a mem_ref" + GET_NODE(stmt)->ToString() + " but it's a " + GET_NODE(MR->op0)->get_kind_text());
                     pointing_to_ssa_vars.insert(GET_INDEX_NODE(MR->op0));
                  }
                  function_information_map[function]->preserving = false;
               }
               else if(GET_NODE(ga->op0)->get_kind() == imagpart_expr_K || GET_NODE(ga->op0)->get_kind() == realpart_expr_K)
               {
                  /// do nothing
               }
               else if(ga->clobber)
               {
                  function_information_map[function]->preserving = false;
               }
               else if(!ga->init_assignment)
               {
                  THROW_ERROR("unexpected pattern: lhs of gimple_assign " + STR(ga) + " is a " + GET_NODE(ga->op0)->get_kind_text());
               }
            }
            else if(GET_NODE(stmt)->get_kind() == gimple_call_K)
            {
               auto* ce = GetPointer<gimple_call>(GET_NODE(stmt));
               if(GET_NODE(ce->fn)->get_kind() == ssa_name_K)
               {
                  pointing_to_ssa_vars.insert(GET_INDEX_NODE(ce->fn));
               }
               if(function_information_map[function]->preserving)
               {
                  for(auto par : ce->args)
                  {
                     if(GET_NODE(par)->get_kind() == ssa_name_K)
                     {
                        if(tree_helper::is_a_pointer(TM, GET_INDEX_NODE(par)))
                        {
                           function_information_map[function]->preserving = false;
                           break;
                        }
                     }
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + boost::lexical_cast<std::string>(GET_INDEX_NODE(stmt)));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + boost::lexical_cast<std::string>(it->first));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined function " + STR(function));
   }

   if(not pointing_to_ssa_vars.empty() && debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      /// print all ssa vars pointing to something
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->SSA vars pointing to something (size:" + STR(pointing_to_ssa_vars.size()) + "):");
      for(auto var : pointing_to_ssa_vars)
      {
         const tree_nodeRef curr_tn = TM->get_tree_node_const(var);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Var " + curr_tn->ToString() + ((GetPointer<ssa_name>(curr_tn) && GetPointer<ssa_name>(curr_tn)->use_set) ? (" { " + GetPointer<ssa_name>(curr_tn)->use_set->ToString() + "}") : ""));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   return DesignFlowStep_Status::SUCCESS;
}
