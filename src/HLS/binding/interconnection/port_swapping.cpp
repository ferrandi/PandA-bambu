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
 * @file port_swapping.cpp
 * @brief Implementation of the port swapping algorithm described in the following paper:
 *   Hao Cong, Song Chen and T. Yoshimura, "Port assignment for interconnect reduction in high-level synthesis," Proceedings of Technical Program of 2012 VLSI Design, Automation and Test, Hsinchu, 2012, pp. 1-4.
 *
 * @author Alessandro Comodi <alessandro.comodi@mail.polimi.it>
 * @author Davide Conficconi <davide.conficconi@mail.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
/// Header include
#include "port_swapping.hpp"

///. include
#include "Parameter.hpp"

/// boost include
#include <boost/graph/random_spanning_tree.hpp>

#include "allocation_information.hpp"
#include "fu_binding.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "liveness.hpp"
#include "reg_binding.hpp"
#include "storage_value_information.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// STD include
#include <random>

#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_helper.hpp"

#define SET_A 0
#define SET_B 1
#define SET_AB 2

port_swapping::port_swapping(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::PORT_SWAPPING)
{
   debug_level = _Param->get_class_debug_level(GET_CLASS(*this));
}

port_swapping::~port_swapping() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> port_swapping::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->module_binding_algorithm, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_register_allocation_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
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

//
//  This function calculates the levels of all the vertices in a spanning tree
//
void port_swapping::vertex_levels(std::vector<PSE>& spt_edges, PSVertex root, size_t num_vertices_g, std::vector<PSVSet>& vset)
{
   PSVSet set;
   set.v = root;
   set.level = 0;
   set.belongs = SET_A;
   vset.push_back(set);

   bool flag = false;
   while(vset.size() < num_vertices_g)
   {
      for(auto i : spt_edges)
      {
         for(auto j : vset)
         {
            if(j.v == i.first)
            {
               set.level = j.level + 1;
               set.v = i.second;
               flag = true;
               break;
            }
         }
         for(auto lvl : vset)
            if(lvl.v == set.v)
               flag = false;
         if(flag)
         {
            if(set.level % 2 == 0)
               set.belongs = SET_A;
            else
               set.belongs = SET_B;
            vset.push_back(set);
         }
         flag = false;
      }
   }
}

//
//  This function calculates the distances between two vertices in a spanning tree
//
int port_swapping::vertex_distance(std::vector<PSE>& spt_edges, PSVertex root, PSVertex dest, std::vector<PSVSet>& vset)
{
   PSVSet set;
   set.v = root;
   set.level = 0;
   vset.push_back(set);
   int distance = 0;
   bool flag = false;
   bool found = false;
   while(!found)
   {
      for(auto i : spt_edges)
      {
         for(auto j : vset)
         {
            if(j.v == i.first)
            {
               set.level = j.level + 1;
               set.v = i.second;
               if(set.v == dest)
               {
                  distance = set.level + 1;
                  found = true;
               }
               flag = true;
               break;
            }
            else if(j.v == i.second)
            {
               set.level = j.level + 1;
               set.v = i.first;
               if(set.v == dest)
               {
                  distance = set.level + 1;
                  found = true;
               }
               flag = true;
               break;
            }
         }
         for(auto lvl : vset)
            if(lvl.v == set.v)
               flag = false;
         if(flag)
            vset.push_back(set);
         flag = false;
      }
   }
   THROW_ASSERT(distance, "");
   THROW_ASSERT(found, "");
   return distance;
}

port_swapping::PSVertex port_swapping::find_max_degree(CustomOrderedSet<std::pair<PSVertex, unsigned int>>& dSet)
{
   long unsigned int max = 0;
   PSVertex v = 0;
   for(auto e : dSet)
      if(e.second > max)
      {
         max = e.second;
         v = e.first;
      }
   return v;
}

void port_swapping::update_degree(PSGraph g2, CustomOrderedSet<std::pair<PSVertex, unsigned int>>& dSet)
{
   auto g2_vertices = boost::vertices(g2);
   for(auto iterator = g2_vertices.first; iterator != g2_vertices.second; ++iterator)
      dSet.insert(std::make_pair(*iterator, boost::out_degree(*iterator, g2)));
}

port_swapping::PSVertex port_swapping::get_co_tree_vertex(PSVertex v, std::vector<PSE>& e)
{
   for(auto ed : e)
      if(v == ed.second)
         return ed.first;
      else if(v == ed.first)
         return ed.second;
   return v;
}

void port_swapping::port_swapping_algorithm(PSGraph g, std::vector<PSMultiStart>& vector_sets, size_t num_vertices_g, PSVertex root)
{
   //
   // Generating the Spanning Tree starting from Graph g
   //
   std::random_device rd;
   std::mt19937 generator(rd());
   generator.seed(parameters->getOption<long unsigned int>(OPT_seed));
   std::vector<PSVertex> p(num_vertices(g));

   std::vector<PSVertex> component(boost::num_vertices(g));
   size_t num_components = boost::connected_components(g, &component[0]);
   PSVertex connection_ver_1 = *vertices(g).first;
   for(size_t count = 1; count < num_components; count++)
   {
      for(size_t i = 0; i < num_vertices_g; i++)
         if(component[i] == count)
         {
            auto connection_ver_2 = PSVertex(i);
            add_edge(connection_ver_1, connection_ver_2, g);
            connection_ver_1 = connection_ver_2;
            break;
         }
   }

   boost::random_spanning_tree(g, generator, boost::root_vertex(root).predecessor_map(boost::make_iterator_property_map(p.begin(), boost::get(boost::vertex_index, g))));

   //
   // Generating a list of edges following the predecessor map created
   // with the random_spanning_tree algorithm
   //
   auto vertices_g = boost::vertices(g);
   std::vector<PSE> spt_edges;

   for(auto iterator = vertices_g.first; iterator != vertices_g.second; ++iterator)
   {
      PSVertex u = *iterator;
      PSVertex v = p[*iterator];
      if(v > num_vertices_g)
      {
         root = u;
         continue;
      }
      spt_edges.push_back(PSE(v, u));
   }

   auto edge = boost::edges(g);
   std::vector<PSE> g_edges;
   for(auto iterator = edge.first; iterator != edge.second; ++iterator)
      g_edges.push_back(PSE(source(*iterator, g), target(*iterator, g)));

   //
   // Calculating the levels of the various vertices in the
   // spanning tree
   // vset: vector of VSet
   //
   std::vector<PSVSet> vset;
   vertex_levels(spt_edges, root, num_vertices_g, vset);

   std::vector<PSE> co_tree_edges;
   for(auto e : g_edges)
   {
      if(find(spt_edges.begin(), spt_edges.end(), PSE(source(e, g), target(e, g))) != spt_edges.end() || find(spt_edges.begin(), spt_edges.end(), PSE(target(e, g), source(e, g))) != spt_edges.end())
         continue;
      else
         co_tree_edges.push_back(PSE(source(e, g), target(e, g)));
   }

   //
   // Calculating which co_tree_edges are odd. Those who are odd are added to a vector of edges
   //
   std::vector<PSVSet> loop_size;
   std::vector<PSE> odd_co_tree_edges;
   for(auto e : co_tree_edges)
   {
      loop_size.clear();
      if(vertex_distance(spt_edges, e.first, e.second, loop_size) % 2 != 0)
         odd_co_tree_edges.push_back(e);
   }

   PSGraph g2(num_vertices_g);
   for(auto e : odd_co_tree_edges)
      add_edge(e.first, e.second, g2);

   CustomOrderedSet<std::pair<PSVertex, unsigned int>> degree_set;
   CustomOrderedSet<PSVertex> cover_set;
   auto g2_vertices = boost::vertices(g2);
   update_degree(g2, degree_set);

   // while(num_edges(g2) != 0)
   for(auto iterator = g2_vertices.first; iterator != g2_vertices.second; ++iterator)
   {
      if(out_degree(*iterator, g2) == 1)
      {
         auto u1 = get_co_tree_vertex(*iterator, odd_co_tree_edges);
         cover_set.insert(u1);
         clear_vertex(u1, g2);
      }
      else if(out_degree(*iterator, g2) > 1)
      {
         auto u2 = find_max_degree(degree_set);
         cover_set.insert(u2);
         clear_vertex(u2, g2);
      }
      degree_set.clear();
      update_degree(g2, degree_set);
   }

   long unsigned int abCardinality = 0;
   std::vector<PSVSet> temp_set;
   PSVSet v;
   for(auto vs : vset)
      temp_set.push_back(vs);

   vset.clear();

   for(auto set : temp_set)
   {
      if(find(cover_set.begin(), cover_set.end(), set.v) != cover_set.end())
      {
         v.v = set.v;
         v.belongs = SET_AB;
         v.level = set.level;
         vset.push_back(v);
         abCardinality++;
      }
      else
         vset.push_back(set);
   }

   PSMultiStart run;
   run.vset = vset;
   run.cardinality = abCardinality;
   run.spt_vector_edges = spt_edges;
   run.co_tree_vector_edges = co_tree_edges;
   run.odd_co_tree_vector_edges = odd_co_tree_edges;
   vector_sets.push_back(run);

   return;
}

std::vector<std::pair<port_swapping::PSVertex, unsigned int>> port_swapping::p_swap(PSGraph g)
{
   auto g_vertices = vertices(g);
   std::vector<PSMultiStart> vector_sets;
   int count = 0;
   for(auto iterator = g_vertices.first; iterator != g_vertices.second; ++iterator)
   {
      port_swapping_algorithm(g, vector_sets, boost::num_vertices(g), *iterator);

      count++;
   }
   long unsigned int max = boost::num_vertices(g);
   PSMultiStart best_candidate;
   for(auto vset : vector_sets)
      if(vset.cardinality < max)
      {
         max = vset.cardinality;
         best_candidate = vset;
      }

   std::vector<std::pair<PSVertex, unsigned int>> return_values;
   for(auto vertex_set : best_candidate.vset)
      return_values.push_back(std::make_pair(vertex_set.v, vertex_set.belongs));
   return return_values;
}

bool port_swapping::is_commutative_op(const std::string& operation)
{
   return operation == STOK(TOK_PLUS_EXPR) || operation == STOK(TOK_POINTER_PLUS_EXPR) || operation == STOK(TOK_MULT_EXPR) || operation == STOK(TOK_BIT_IOR_EXPR) || operation == STOK(TOK_BIT_XOR_EXPR) || operation == STOK(TOK_BIT_AND_EXPR) ||
          operation == STOK(TOK_EQ_EXPR) || operation == STOK(TOK_NE_EXPR) || operation == STOK(TOK_WIDEN_SUM_EXPR) || operation == STOK(TOK_WIDEN_MULT_EXPR);
}

unsigned int port_swapping::get_results(PSVertex operand, std::vector<std::pair<PSVertex, unsigned int>> results)
{
   for(auto res : results)
   {
      if(res.first == operand)
         return res.second;
   }
   return 0;
}

DesignFlowStep_Status port_swapping::InternalExec()
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const OpGraphConstRef data = FB->CGetOpGraph(FunctionBehavior::FDFG);
   const BehavioralHelperConstRef behavioral_helper = FB->CGetBehavioralHelper();
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();

   typedef struct
   {
      vertex op;
      PSVertex first_op;
      PSVertex second_op;
   } Operands;

   std::map<std::pair<unsigned int, unsigned int>, std::vector<vertex>> fu_map;
   std::map<std::tuple<unsigned int, unsigned int, unsigned int>, PSVertex> op_vertex_map;
   std::tuple<unsigned int, unsigned int, unsigned int> key_value;
   std::vector<PSVertex> vertices_in_op;
   std::vector<Operands> operands;
   std::vector<std::pair<PSVertex, unsigned int>> results;
   bool changed = false;
   size_t n_swaps = 0;
   vertices_in_op.resize(2);

   PSVertex v_input;
   PSGraph g;

   VertexIterator op, opend;
   for(boost::tie(op, opend) = boost::vertices(*data); op != opend; op++)
   {
      std::string operation = data->CGetOpNodeInfo(*op)->GetOperation();
      if(is_commutative_op(operation))
      {
         unsigned int fu = HLS->Rfu->get_assign(*op);
         unsigned int idx = HLS->Rfu->get_index(*op);
         auto fu_key = std::make_pair(fu, idx);
         fu_map[fu_key].push_back(*op);
      }
   }

   for(const auto& fu : fu_map)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Functional Unit ID: " + HLS->allocation_information->get_string_name(fu.first.first));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Functional Unit INDEX: " + STR(fu.first.second));
      for(const auto& fu_operation : fu.second)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Operation: " + GET_NAME(data, fu_operation) + " (" + data->CGetOpNodeInfo(fu_operation)->GetOperation() + ")");
         std::vector<HLS_manager::io_binding_type> var_read = HLSMgr->get_required_values(HLS->functionId, fu_operation);
         THROW_ASSERT(var_read.size() == 2, STR(var_read.size()) + " Vertices in op has wrong size!");
         for(unsigned int var_num = 0; var_num < var_read.size(); var_num++)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "READING OPERANDS");
            unsigned int tree_var = std::get<0>(var_read[var_num]);
            if(tree_var == 0)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Constant: " + STR(std::get<1>(var_read[var_num])));
               key_value = std::make_tuple(0, 0, std::get<1>(var_read[var_num]));
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Read: " + behavioral_helper->PrintVariable(tree_var));
               const CustomOrderedSet<vertex>& running_states = HLS->Rliv->get_state_where_run(fu_operation);
               const CustomOrderedSet<vertex>::const_iterator rs_it_end = running_states.end();
               for(auto rs_it = running_states.begin(); rs_it != rs_it_end; ++rs_it)
               {
                  vertex state = *rs_it;
                  if(tree_helper::is_parameter(TreeM, tree_var) || !HLS->Rliv->has_op_where_defined(tree_var))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Param operand");
                     key_value = std::make_tuple(1, 0, tree_var);
                  }
                  else
                  {
                     vertex def_op = HLS->Rliv->get_op_where_defined(tree_var);
                     const CustomOrderedSet<vertex>& def_op_ending_states = HLS->Rliv->get_state_where_end(def_op);
                     if((GET_TYPE(data, def_op) & TYPE_PHI) == 0)
                     {
                        if(def_op_ending_states.find(state) != def_op_ending_states.end())
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Chained operand");
                           if(HLS->Rfu->get_index(def_op) != INFINITE_UINT)
                              key_value = std::make_tuple(2, HLS->Rfu->get_assign(def_op), HLS->Rfu->get_index(def_op));
                           else
                              key_value = std::make_tuple(2, 0, tree_var);
                        }
                        else if(HLS->storage_value_information->is_a_storage_value(state, tree_var))
                        {
                           unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(state, tree_var);
                           unsigned int r_index = HLS->Rreg->get_register(storage_value);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Register: " + STR(r_index));
                           key_value = std::make_tuple(3, 0, r_index);
                        }
                        else
                           THROW_UNREACHABLE("unexpected");
                     }
                     else
                     {
                        unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(state, tree_var);
                        unsigned int r_index = HLS->Rreg->get_register(storage_value);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Register: " + STR(r_index));
                        key_value = std::make_tuple(3, 0, r_index);
                     }
                  }

                  /// no more than one running state could be considered
                  break; /// TOBEFIXED
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            }
            if(op_vertex_map.find(key_value) == op_vertex_map.end())
            {
               v_input = boost::add_vertex(g);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Vertex: " + STR(v_input));
               op_vertex_map[key_value] = v_input;
            }
            vertices_in_op[var_num] = op_vertex_map.at(key_value);
         }

         if(vertices_in_op[0] != vertices_in_op[1])
         {
            Operands tmp_op;
            add_edge(vertices_in_op[0], vertices_in_op[1], g);
            tmp_op.op = fu_operation;
            tmp_op.first_op = vertices_in_op[0];
            tmp_op.second_op = vertices_in_op[1];
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "PUSH OPERANDS");
            operands.push_back(tmp_op);
         }
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "PORT_SWAPPING STARTING");
      if(DEBUG_LEVEL_VERY_PEDANTIC <= debug_level)
      {
         std::ofstream file("starting-graph_" + HLS->allocation_information->get_string_name(fu.first.first) + "_" + STR(fu.first.second) + ".dot");
         boost::write_graphviz(file, g);
         file.close();
      }
      results = p_swap(g);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "PORT_SWAPPING ENDED");

#ifndef NDEBUG
      for(auto res : results)
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Vertex: " + STR(res.first) + " Belongs To: " + STR(res.second));
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Printing Results");

      for(auto opt : operands)
      {
         unsigned int op1, op2;
         op1 = get_results(opt.first_op, results);
         op2 = get_results(opt.second_op, results);
         if((op1 == op2) && (op1 == SET_B || op1 == SET_A))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- Error in the algorithm");
         }
         else if((op1 == SET_A && op2 == SET_B) || (op1 == SET_AB && op2 == SET_B) || (op1 == SET_A && op2 == SET_AB) || (op1 == SET_AB && op2 == SET_AB))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- No swap to do");
            HLS->Rfu->set_ports_are_swapped(opt.op, false);
         }
         else if((op1 == SET_AB && op2 == SET_A) || (op1 == SET_B && op2 == SET_AB))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--> Swap ports in operation: " + GET_NAME(data, opt.op));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- Vertices swapped: " + STR(opt.first_op) + " and " + STR(opt.second_op));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            HLS->Rfu->set_ports_are_swapped(opt.op, true);
            changed = true;
            ++n_swaps;
         }
      }

      operands.clear();
      op_vertex_map.clear();
      g.clear();
   }

   if(n_swaps)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Port swapping: number of operations swapped= " + STR(n_swaps));
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
   }
   if(changed)
      return DesignFlowStep_Status::SUCCESS;
   else
      return DesignFlowStep_Status::UNCHANGED;
}
