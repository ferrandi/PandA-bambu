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
 * @file fun_dominator_allocation.cpp
 * @brief Function allocation based on the dominator tree of the call graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_HAVE_PRAGMA_BUILT.hpp"

///. include
#include "Parameter.hpp"
#include "constant_strings.hpp"

/// Header include
#include "fun_dominator_allocation.hpp"

/// algorithms/dominance include
#include "Dominance.hpp"

/// behavior include
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// design_flows/codesign include
#if HAVE_PRAGMA_BUILT && HAVE_EXPERIMENTAL
#include "actor_graph_flow_step.hpp"
#include "actor_graph_flow_step_factory.hpp"
#endif

/// frontend_analysis includes
#include "application_frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"

/// HLS include
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// HLS/function_allocation includes
#include "functions.hpp"

/// technology include
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#include "technology_manager.hpp"

/// technology/physical_library includes
#include "library_manager.hpp"
#include "technology_node.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

/// utility include
#include "cpu_time.hpp"

#include "string_manipulation.hpp" // for GET_CLASS
#include <boost/range/adaptor/reversed.hpp>

fun_dominator_allocation::fun_dominator_allocation(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : function_allocation(_parameters, _HLSMgr, _design_flow_manager, _hls_flow_step_type), already_executed(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

fun_dominator_allocation::~fun_dominator_allocation() = default;

void fun_dominator_allocation::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_EXPERIMENTAL && HAVE_PRAGMA_BUILT
         if(parameters->isOption(OPT_parse_pragma) and parameters->getOption<bool>(OPT_parse_pragma) and relationship_type == PRECEDENCE_RELATIONSHIP)
         {
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const ActorGraphFlowStepFactory* actor_graph_flow_step_factory = GetPointer<const ActorGraphFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("ActorGraph"));
            const std::string actor_graph_creator_signature = ActorGraphFlowStep::ComputeSignature(ACTOR_GRAPHS_CREATOR, input_function, 0, "");
            const vertex actor_graph_creator_step = design_flow_manager.lock()->GetDesignFlowStep(actor_graph_creator_signature);
            const DesignFlowStepRef design_flow_step =
                actor_graph_creator_step ? design_flow_graph->CGetDesignFlowStepInfo(actor_graph_creator_step)->design_flow_step : actor_graph_flow_step_factory->CreateActorGraphStep(ACTOR_GRAPHS_CREATOR, input_function);
            relationship.insert(design_flow_step);
         }
      }
#endif
      break;
   }
   case DEPENDENCE_RELATIONSHIP:
   {
      const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
      const auto* frontend_flow_step_factory = GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"));
      const std::string frontend_flow_signature = ApplicationFrontendFlowStep::ComputeSignature(BAMBU_FRONTEND_FLOW);
      const vertex frontend_flow_step = design_flow_manager.lock()->GetDesignFlowStep(frontend_flow_signature);
      const DesignFlowStepRef design_flow_step = frontend_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(frontend_flow_step)->design_flow_step : frontend_flow_step_factory->CreateApplicationFrontendFlowStep(BAMBU_FRONTEND_FLOW);
      relationship.insert(design_flow_step);

      const auto* technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
      const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
      const vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
      const DesignFlowStepRef technology_design_flow_step =
          technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step : technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
      relationship.insert(technology_design_flow_step);
      break;
   }
   case INVALIDATION_RELATIONSHIP:
   {
      break;
   }
   default:
      THROW_UNREACHABLE("");
}
HLS_step::ComputeRelationships(relationship, relationship_type);
}

DesignFlowStep_Status fun_dominator_allocation::Exec()
{
   already_executed = true;
   const CallGraphManagerConstRef CG = HLSMgr->CGetCallGraphManager();
   auto root_functions = CG->GetRootFunctions();
   if(parameters->isOption(OPT_top_design_name)) // top design function become the top_vertex
   {
      const auto top_rtldesign_function_id = HLSMgr->get_tree_manager()->function_index(parameters->getOption<std::string>(OPT_top_design_name));
      if(top_rtldesign_function_id != 0 and root_functions.find(top_rtldesign_function_id) != root_functions.end())
      {
         root_functions.clear();
         root_functions.insert(top_rtldesign_function_id);
      }
   }
   if(parameters->isOption(OPT_disable_function_proxy) and parameters->getOption<bool>(OPT_disable_function_proxy))
      return DesignFlowStep_Status::UNCHANGED;

   CustomOrderedSet<unsigned int> reached_fu_ids;
   for(const auto f_id : root_functions)
      for(const auto reached_f_id : CG->GetReachedBodyFunctionsFrom(f_id))
         reached_fu_ids.insert(reached_f_id);

   if(!parameters->isOption(OPT_top_functions_names))
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
      for(const auto f_id : root_functions)
      {
         const auto function_behavior = HLSMgr->CGetFunctionBehavior(f_id);
         const auto BH = function_behavior->CGetBehavioralHelper();
         auto fname = BH->get_function_name();
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---The top function inferred from the specification is: " + fname);
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
   }

   /// the analysis has to be performed only on the reachable functions
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Functions to be synthesized:");
   for(const auto& funID : reached_fu_ids)
   {
      const FunctionBehaviorConstRef function_behavior = HLSMgr->CGetFunctionBehavior(funID);
      const BehavioralHelperConstRef BH = function_behavior->CGetBehavioralHelper();
      std::string fname = BH->get_function_name();
      if(tree_helper::is_a_nop_function_decl(GetPointer<function_decl>(HLSMgr->get_tree_manager()->get_tree_node_const(funID))))
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Warning: " + fname + " is empty or the compiler killed all the statements");
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---" + fname);
      if(not function_behavior->build_simple_pipeline())
         HLSMgr->global_resource_constraints[std::make_pair(fname, WORK_LIBRARY)] = 1;
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");

   THROW_ASSERT(root_functions.size() == 1, "Multiple top functions");
   const auto top_fun_id = *(root_functions.begin());
   vertex top_vertex = CG->GetVertex(top_fun_id);

   CustomUnorderedSet<vertex> vertex_subset;
   for(const auto& funID : reached_fu_ids)
      vertex_subset.insert(CG->GetVertex(funID));

   refcount<dominance<graph>> cg_dominators;
   const CallGraphConstRef subgraph = CG->CGetCallSubGraph(vertex_subset);
   /// we do not need the exit vertex since the post-dominator graph is not used
   cg_dominators = refcount<dominance<graph>>(new dominance<graph>(*subgraph, top_vertex, NULL_VERTEX, parameters));
   cg_dominators->calculate_dominance_info(dominance<graph>::CDI_DOMINATORS);
   const auto& cg_dominator_map = cg_dominators->get_dominator_map();
   std::map<std::string, CustomOrderedSet<vertex>> fun_dom_map;
   std::map<std::string, CustomOrderedSet<unsigned int>> where_used;
   std::map<std::string, bool> indirectlyCalled;
   const HLS_constraintsRef HLS_C = HLS_constraintsRef(new HLS_constraints(HLSMgr->get_parameter(), ""));

   const CallGraphConstRef cg = CG->CGetCallGraph();
   for(const auto& funID : reached_fu_ids)
   {
      vertex vert_dominator, current_vertex;
      current_vertex = CG->GetVertex(funID);
      THROW_ASSERT(cg_dominator_map.find(current_vertex) != cg_dominator_map.end(), "Dominator vertex not in the dominator tree: " + HLSMgr->CGetFunctionBehavior(CG->get_function(current_vertex))->CGetBehavioralHelper()->get_function_name());
      if(boost::in_degree(current_vertex, *cg) != 1)
         vert_dominator = cg_dominator_map.find(current_vertex)->second;
      else
         vert_dominator = current_vertex;
      BOOST_FOREACH(EdgeDescriptor eo, boost::out_edges(current_vertex, *cg))
      {
         std::string called_fu_name = functions::get_function_name_cleaned(CG->get_function(boost::target(eo, *cg)), HLSMgr);
         if(HLS_C->get_number_fu(called_fu_name, WORK_LIBRARY) == INFINITE_UINT || // without constraints
            HLS_C->get_number_fu(called_fu_name, WORK_LIBRARY) == 1)               // or single instance functions
         {
            fun_dom_map[called_fu_name].insert(vert_dominator);
            const auto* info = Cget_edge_info<FunctionEdgeInfo, const CallGraph>(eo, *cg);

            if(info->direct_call_points.size())
               where_used[called_fu_name].insert(funID);
            else
               where_used[called_fu_name];

            if(info->indirect_call_points.size() or info->function_addresses.size())
               indirectlyCalled[called_fu_name] = true;
            else
               indirectlyCalled[called_fu_name] = false;
         }
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Dominator Vertex: " + HLSMgr->CGetFunctionBehavior(CG->get_function(vert_dominator))->CGetBehavioralHelper()->get_function_name() + " - function " + called_fu_name);
      }
   }
   /// compute the number of instances for each function
   std::map<std::string, unsigned int> num_instances;
   num_instances[functions::get_function_name_cleaned(CG->get_function(top_vertex), HLSMgr)] = 1;
   std::map<unsigned int, std::vector<std::string>> function_allocation_map;
   std::list<vertex> topology_sorted_vertex;
   cg->TopologicalSort(topology_sorted_vertex);
   for(const auto& cur : topology_sorted_vertex)
   {
      unsigned int funID = CG->get_function(cur);
      if(reached_fu_ids.find(funID) == reached_fu_ids.end())
         continue;
      THROW_ASSERT(num_instances.find(functions::get_function_name_cleaned(funID, HLSMgr)) != num_instances.end(), "missing number of instances of function " + HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->get_function_name());
      function_allocation_map[funID];
      unsigned int cur_instances = num_instances.at(functions::get_function_name_cleaned(funID, HLSMgr));
      BOOST_FOREACH(EdgeDescriptor eo, boost::out_edges(cur, *cg))
      {
         vertex tgt = boost::target(eo, *cg);
         unsigned int n_call_points = static_cast<unsigned int>(Cget_edge_info<FunctionEdgeInfo, const CallGraph>(eo, *cg)->direct_call_points.size());
         if(num_instances.find(functions::get_function_name_cleaned(CG->get_function(tgt), HLSMgr)) == num_instances.end())
            num_instances[functions::get_function_name_cleaned(CG->get_function(tgt), HLSMgr)] = cur_instances * n_call_points;
         else
            num_instances[functions::get_function_name_cleaned(CG->get_function(tgt), HLSMgr)] += cur_instances * n_call_points;
      }
   }

   THROW_ASSERT(num_instances.at(functions::get_function_name_cleaned(CG->get_function(top_vertex), HLSMgr)) == 1, "top function cannot be called from some other function");
   /// find the common dominator and decide where to allocate
   for(const auto& dom_map : fun_dom_map)
   {
      unsigned int funID = 0;
      if(dom_map.second.size() == 1)
      {
         vertex cur = *dom_map.second.begin();
         while(num_instances.at(functions::get_function_name_cleaned(CG->get_function(cur), HLSMgr)) != 1)
            cur = cg_dominator_map.at(cur);
         funID = CG->get_function(cur);
      }
      else
      {
         if(dom_map.second.find(top_vertex) != dom_map.second.end())
            funID = CG->get_function(top_vertex);
         else
         {
            auto vert_it = dom_map.second.begin();
            auto vert_it_end = dom_map.second.end();
            std::list<vertex> dominator_list1;
            vertex cur = *vert_it;
            dominator_list1.push_front(cur);
            do
            {
               cur = cg_dominator_map.at(cur);
               dominator_list1.push_front(cur);
            } while(cur != top_vertex);
            ++vert_it;
            std::list<vertex>::const_iterator last = dominator_list1.end();
            for(; vert_it != vert_it_end; ++vert_it)
            {
               std::list<vertex> dominator_list2;
               cur = *vert_it;
               dominator_list2.push_front(cur);
               do
               {
                  cur = cg_dominator_map.find(cur)->second;
                  dominator_list2.push_front(cur);
               } while(cur != top_vertex);
               /// find the common dominator between two candidates
               std::list<vertex>::const_iterator dl1_it = dominator_list1.begin(), dl2_it = dominator_list2.begin(), dl2_it_end = dominator_list2.end(), cur_last = dominator_list1.begin();
               while(dl1_it != last && dl2_it != dl2_it_end && *dl1_it == *dl2_it && (num_instances.at(functions::get_function_name_cleaned(CG->get_function(*dl1_it), HLSMgr)) == 1))
               {
                  cur = *dl1_it;
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
      THROW_ASSERT(funID, "null function id index unexpected");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "function " + dom_map.first + " has to be allocated in " + HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->get_function_name());
      function_allocation_map[funID].push_back(dom_map.first);
   }

   /// really allocate
   const HLS_targetRef HLS_T = HLSMgr->get_HLS_target();
   const technology_managerRef TM = HLS_T->get_technology_manager();

   for(const auto& funID : reached_fu_ids)
   {
      for(const auto& fun_name : function_allocation_map.at(funID))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, fun_name + " has to be allocated in " + HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->get_function_name());
         THROW_ASSERT(where_used.at(fun_name).size() > 0 or indirectlyCalled.at(fun_name), fun_name + " function not used anywhere: used size = " + STR(where_used.at(fun_name).size()) + " indirectlyCalled = " + STR(indirectlyCalled.at(fun_name)));
         if(where_used.at(fun_name).size() == 1)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Skipped " + fun_name + " because called only by one other function");
            continue;
         }
         if(HLSMgr->hasToBeInterfaced(HLSMgr->get_tree_manager()->function_index(fun_name)))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Skipped " + fun_name + " because it has to be interfaced");
            continue;
         }
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT
         if(fun_name == "panda_pthread_mutex")
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Skipped " + fun_name);
            continue;
         }
#endif
         std::string library_name = TM->get_library(fun_name);
         if(library_name != "")
         {
            const library_managerRef libraryManager = TM->get_library_manager(library_name);
            technology_nodeRef techNode_obj = libraryManager->get_fu(fun_name);
            if(GetPointer<functional_unit_template>(techNode_obj))
               continue;
         }

         HLSMgr->Rfuns->map_shared_function(funID, fun_name);

         const FunctionBehaviorConstRef cur_function_behavior = HLSMgr->CGetFunctionBehavior(funID);
         const BehavioralHelperConstRef cur_BH = cur_function_behavior->CGetBehavioralHelper();
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Adding proxy wrapper: " + fun_name + " in function " + cur_BH->get_function_name());
         /// add proxies
         for(const auto& wu_id : where_used.at(fun_name))
         {
            if(wu_id != funID)
            {
               const FunctionBehaviorConstRef wiu_function_behavior = HLSMgr->CGetFunctionBehavior(wu_id);
               const BehavioralHelperConstRef wiu_BH = wiu_function_behavior->CGetBehavioralHelper();
               PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Adding proxy function: " + fun_name + " in function " + wiu_BH->get_function_name());
               HLSMgr->Rfuns->add_shared_function_proxy(wu_id, fun_name);
            }
         }
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}

bool fun_dominator_allocation::HasToBeExecuted() const
{
   return not already_executed;
}
