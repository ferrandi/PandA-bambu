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
 * @file storage_value_information.cpp
 * @brief This package is used to define the storage value scheme adopted by the register allocation algorithms.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
/// Header include
#include "storage_value_information.hpp"

/// Autoheader include
#include "config_HAVE_ASSERTS.hpp"

/// behavior include
#include "function_behavior.hpp"

/// HLS include
#include "hls.hpp"
#include "hls_manager.hpp"

/// HLS/binding/module_binding includes
#include "fu_binding.hpp"

/// tree includes
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "math_function.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"

StorageValueInformation::StorageValueInformation(const HLS_managerConstRef _HLS_mgr, const unsigned int _function_id) : number_of_storage_values(0), HLS_mgr(_HLS_mgr), function_id(_function_id)
{
}

StorageValueInformation::~StorageValueInformation() = default;

void StorageValueInformation::Initialize()
{
   const hlsRef HLS = HLS_mgr->get_HLS(function_id);
   const FunctionBehaviorConstRef FB = HLS_mgr->CGetFunctionBehavior(function_id);
   data = FB->CGetOpGraph(FunctionBehavior::DFG);
   fu = HLS->Rfu;
   const tree_managerRef TreeM = HLS_mgr->get_tree_manager();

   /// initialize the vw2vertex relation
   VertexIterator ki, ki_end;
   for(boost::tie(ki, ki_end) = boost::vertices(*data); ki != ki_end; ++ki)
   {
      const CustomSet<unsigned int>& scalar_defs = data->CGetOpNodeInfo(*ki)->GetVariables(FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::DEFINITION);
      if(not scalar_defs.empty())
      {
         auto it_end = scalar_defs.end();
#if HAVE_ASSERTS
         size_t counter = 0;
#endif
         for(auto it = scalar_defs.begin(); it != it_end; ++it)
         {
            if(tree_helper::is_ssa_name(TreeM, *it) && !tree_helper::is_virtual(TreeM, *it) && !tree_helper::is_parameter(TreeM, *it))
            {
               HLS->storage_value_information->vw2vertex[*it] = *ki;
#if HAVE_ASSERTS
               ++counter;
#endif
            }
         }
#if HAVE_ASSERTS
         if(counter > 1 and not(GET_TYPE(data, *ki) & TYPE_ENTRY))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_NONE, 0, GET_NAME(data, *ki) + " defines:");
            for(const auto scalar_def : scalar_defs)
            {
               if(tree_helper::is_ssa_name(TreeM, scalar_def) and not tree_helper::is_virtual(TreeM, scalar_def) and not tree_helper::is_parameter(TreeM, scalar_def))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_NONE, 0, STR(TreeM->CGetTreeNode(scalar_def)));
               }
            }
            THROW_UNREACHABLE("More than one definition");
         }
#endif
      }
   }
}

unsigned int StorageValueInformation::get_number_of_storage_values() const
{
   return number_of_storage_values;
}

bool StorageValueInformation::is_a_storage_value(vertex, unsigned int var_index) const
{
   return storage_index_map.find(var_index) != storage_index_map.end();
}

unsigned int StorageValueInformation::get_storage_value_index(vertex, unsigned int var_index) const
{
   THROW_ASSERT(storage_index_map.find(var_index) != storage_index_map.end(), "the storage value is missing");
   return storage_index_map.find(var_index)->second;
}

unsigned int StorageValueInformation::get_variable_index(unsigned int storage_value_index) const
{
   THROW_ASSERT(variable_index_vect.size() > storage_value_index, "the storage value is missing");
   return variable_index_vect[storage_value_index];
}

int StorageValueInformation::get_compatibility_weight(unsigned int storage_value_index1, unsigned int storage_value_index2) const
{
   unsigned int var1 = get_variable_index(storage_value_index1);
   unsigned int var2 = get_variable_index(storage_value_index2);
#if 0
   if(vw2vertex.find(var1) == vw2vertex.end())
   {
      //std::cerr << var1 << std::endl;
      return 1;
   }
#endif
   THROW_ASSERT(vw2vertex.find(var1) != vw2vertex.end(), "variable not in the map " + STR(var1));
   THROW_ASSERT(vw2vertex.find(var2) != vw2vertex.end(), "variable " + STR(HLS_mgr->get_tree_manager()->CGetTreeNode(var2)) + " not in the map");
   vertex v1 = vw2vertex.find(var1)->second;
   bool is_a_phi1 = (GET_TYPE(data, v1) & TYPE_PHI) != 0;
   vertex v2 = vw2vertex.find(var2)->second;
   bool is_a_phi2 = (GET_TYPE(data, v2) & TYPE_PHI) != 0;

   // compute the successors of v1 e v2
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0,
                  "-->Evaluation storage values (vars): [" + STR(HLS_mgr->get_tree_manager()->CGetTreeNode(var1)) +
                      "]"
                      " and [" +
                      STR(HLS_mgr->get_tree_manager()->CGetTreeNode(var2)) + "]");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0,
                  "---vertex names: [" + GET_NAME(data, v1) +
                      "]"
                      " and [" +
                      GET_NAME(data, v2) + "]");

   const auto it_succ_v1 = boost::adjacent_vertices(v1, *data);
   const auto it_succ_v2 = boost::adjacent_vertices(v2, *data);

   static const std::vector<std::string> labels = {"mult_expr", "widen_mult_expr", "ternary_plus_expr", "ternary_mm_expr", "ternary_pm_expr", "ternary_mp_expr"};
   for(const auto& label : labels)
   {
      // check if v1 or v2 drive complex operations
      // variable coming from the Entry vertex have to be neglected in this analysis
      CustomOrderedSet<unsigned int> op_succ_of_v1_port0, op_succ_of_v1_port1, op_succ_of_v1_port2;
      if(!(GET_TYPE(data, v1) & TYPE_ENTRY))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "-->Statement with USE first variable");
         std::for_each(it_succ_v1.first, it_succ_v1.second, [this, &op_succ_of_v1_port0, &op_succ_of_v1_port1, &op_succ_of_v1_port2, &var1, &label](const vertex succ) {
            const std::string op_label = data->CGetOpNodeInfo(succ)->GetOperation();
            const unsigned int succ_id = data->CGetOpNodeInfo(succ)->GetNodeId();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "---[" + STR(succ_id) + "] type: " + STR(op_label));
            if((op_label == label))
            {
               std::vector<HLS_manager::io_binding_type> var_read = HLS_mgr->get_required_values(function_id, succ);
               if(std::get<0>(var_read[0]) == var1)
                  op_succ_of_v1_port0.insert(succ_id);
               else if(std::get<0>(var_read[1]) == var1)
                  op_succ_of_v1_port1.insert(succ_id);
               else if(var_read.size() == 3 && std::get<0>(var_read[2]) == var1)
                  op_succ_of_v1_port2.insert(succ_id);
               else
                  THROW_ERROR("unexpected case:" + STR(succ_id) + "|" + STR(std::get<0>(var_read[0])) + ":" + STR(std::get<0>(var_read[1])));
            }
         });
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "<--");
      }

      CustomOrderedSet<unsigned int> op_succ_of_v2_port0, op_succ_of_v2_port1, op_succ_of_v2_port2;
      if(!(GET_TYPE(data, v2) & TYPE_ENTRY))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "-->Statement with USE second variable");
         std::for_each(it_succ_v2.first, it_succ_v2.second, [this, &op_succ_of_v2_port0, &op_succ_of_v2_port1, &op_succ_of_v2_port2, &var2, &label](const vertex succ) {
            const std::string op_label = data->CGetOpNodeInfo(succ)->GetOperation();
            const unsigned int succ_id = data->CGetOpNodeInfo(succ)->GetNodeId();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "---[" + STR(succ_id) + "] type: " + STR(op_label));
            if(op_label == label)
            {
               std::vector<HLS_manager::io_binding_type> var_read = HLS_mgr->get_required_values(function_id, succ);
               if(std::get<0>(var_read[0]) == var2)
                  op_succ_of_v2_port0.insert(succ_id);
               else if(std::get<0>(var_read[1]) == var2)
                  op_succ_of_v2_port1.insert(succ_id);
               else if(var_read.size() == 3 && std::get<0>(var_read[2]) == var2)
                  op_succ_of_v2_port2.insert(succ_id);
               else
                  THROW_ERROR("unexpected case");
            }
         });
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "<--");
      }

      // Check if both pilot complex operations
      auto P0cond = !op_succ_of_v1_port0.empty() && !op_succ_of_v2_port0.empty();
      auto P1cond = (!op_succ_of_v1_port1.empty() && !op_succ_of_v2_port1.empty());
      auto P2cond = (!op_succ_of_v1_port2.empty() && !op_succ_of_v2_port2.empty());
      const bool both_pilot_complex_ops = P0cond || P1cond || P2cond;

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "Both pilot a complex operation: " + STR(both_pilot_complex_ops));
      if(both_pilot_complex_ops)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "<--");
         if(P0cond)
            return 6;
         else if(P1cond)
            return 7;
         else if(P2cond)
            return 8;
         else
         {
            THROW_ERROR("unexpected condition");
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "<--");
   // ------------

   const CustomSet<unsigned int>& ssa_read1 = data->CGetOpNodeInfo(v1)->GetVariables(FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::USE);
   if(is_a_phi1)
   {
      if(ssa_read1.find(var2) != ssa_read1.end())
         return 5;
   }
   const CustomSet<unsigned int>& ssa_read2 = data->CGetOpNodeInfo(v2)->GetVariables(FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::USE);
   if(is_a_phi2)
   {
      if(ssa_read2.find(var1) != ssa_read2.end())
         return 5;
   }
   if(fu.lock())
   {
      unsigned int fu_unit1 = fu.lock()->get_assign(v1);
      unsigned int fu_unit2 = fu.lock()->get_assign(v2);
      if(fu_unit1 == fu_unit2)
      {
         if(fu.lock()->get_index(v1) != INFINITE_UINT)
         {
            /*unsigned int base_index1= tree_helper::get_base_index(HLSMgr->get_tree_manager(), var1);
            unsigned int base_index2= tree_helper::get_base_index(HLSMgr->get_tree_manager(), var2);*/
            if(fu.lock()->get_index(v1) == fu.lock()->get_index(v2) /* || base_index1 == base_index2*/)
               return 5;
            else
               return 1;
         }
         bool they_have_common_inputs = false;
         auto it1_end = ssa_read1.end();
         for(auto it1 = ssa_read1.begin(); it1 != it1_end; ++it1)
         {
            if(ssa_read2.find(*it1) != ssa_read2.end())
            {
               they_have_common_inputs = true;
               break;
            }
            else if(vw2vertex.find(*it1) != vw2vertex.end())
            {
               vertex from_v1 = vw2vertex.find(*it1)->second;
               auto it2_end = ssa_read2.end();
               for(auto it2 = ssa_read2.begin(); it2 != it2_end; ++it2)
               {
                  if(vw2vertex.find(*it2) != vw2vertex.end())
                  {
                     vertex from_v2 = vw2vertex.find(*it2)->second;
                     if(fu.lock()->get_assign(from_v1) == fu.lock()->get_assign(from_v2) && fu.lock()->get_index(from_v1) != INFINITE_UINT && fu.lock()->get_index(from_v1) == fu.lock()->get_index(from_v2))
                     {
                        they_have_common_inputs = true;
                        break;
                     }
                  }
               }
               if(they_have_common_inputs)
                  break;
            }
         }
         if(they_have_common_inputs)
            return 4;
         if(ssa_read1.find(var2) != ssa_read1.end())
            return 3;
         if(ssa_read2.find(var1) != ssa_read2.end())
            return 3;
         return 2;
      }
   }
   return 1;
}

bool StorageValueInformation::are_value_bitsize_compatible(unsigned int storage_value_index1, unsigned int storage_value_index2) const
{
   auto var1 = get_variable_index(storage_value_index1);
   auto var2 = get_variable_index(storage_value_index2);
   auto TM = HLS_mgr->get_tree_manager();
   auto isInt1 = tree_helper::is_int(TM, var1);
   auto isInt2 = tree_helper::is_int(TM, var2);
   auto size1 = tree_helper::size(TM, var1);
   auto size2 = tree_helper::size(TM, var2);
   return isInt1 == isInt2 && (isInt1 && isInt2 ? size1 == size2 : resize_to_1_8_16_32_64_128_256_512(size1) == resize_to_1_8_16_32_64_128_256_512(size2));
}
