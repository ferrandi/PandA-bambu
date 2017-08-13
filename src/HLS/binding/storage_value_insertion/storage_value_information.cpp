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
 *              Copyright (c) 2004-2018 Politecnico di Milano
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
///Header include
#include "storage_value_information.hpp"

///Autoheader include
#include "config_HAVE_ASSERTS.hpp"

///behavior include
#include "function_behavior.hpp"

///HLS include
#include "hls.hpp"
#include "hls_manager.hpp"

///HLS/binding/module_binding includes
#include "fu_binding.hpp"

///tree includes
#include "tree_helper.hpp"
#include "tree_manager.hpp"

StorageValueInformation::StorageValueInformation(const HLS_managerConstRef _HLS_mgr, const unsigned int _function_id) :
   number_of_storage_values(0),
   HLS_mgr(_HLS_mgr),
   function_id(_function_id)
{}

StorageValueInformation::~StorageValueInformation()
{}

void StorageValueInformation::Initialize()
{
   const hlsRef HLS = HLS_mgr->get_HLS(function_id);
   const FunctionBehaviorConstRef FB = HLS_mgr->CGetFunctionBehavior(function_id);
   data = FB->CGetOpGraph(FunctionBehavior::DFG);
   fu = HLS->Rfu;
   const tree_managerRef TreeM = HLS_mgr->get_tree_manager();

   /// initialize the vw2vertex relation
   VertexIterator ki, ki_end;
   for (boost::tie(ki, ki_end) = boost::vertices(*data); ki != ki_end; ++ki)
   {
      const CustomSet<unsigned int> & scalar_defs = data->CGetOpNodeInfo(*ki)->GetVariables(FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::DEFINITION);
      if(not scalar_defs.empty())
      {
         CustomSet<unsigned int>::const_iterator it_end = scalar_defs.end();
#if HAVE_ASSERTS
         size_t counter = 0;
#endif
         for(CustomSet<unsigned int>::const_iterator it = scalar_defs.begin(); it != it_end; ++it)
         {
            if(tree_helper::is_ssa_name(TreeM, *it) &&
                  !tree_helper::is_virtual(TreeM, *it) &&
                  !tree_helper::is_parameter(TreeM, *it))
            {
               HLS->storage_value_information->vw2vertex[*it] = *ki;
#if HAVE_ASSERTS
               ++counter;
#endif
            }
         }
#if HAVE_ASSERTS
         if(counter > 1 and not (GET_TYPE(data, *ki) & TYPE_ENTRY))
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
   bool is_a_phi1 = (GET_TYPE(data, v1) & TYPE_PHI)!=0;
   vertex v2 = vw2vertex.find(var2)->second;
   bool is_a_phi2 = (GET_TYPE(data, v2) & TYPE_PHI)!=0;

   // BIAGIO
   // prendo i successori della operazione v1 e v2
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, DEBUG_LEVEL_VERY_PEDANTIC,
                  "-->[D]Evaluation storage values (vars): [" +
                  STR(HLS_mgr->get_tree_manager()->CGetTreeNode(var1)) + "]"
                  " and [" + STR(HLS_mgr->get_tree_manager()->CGetTreeNode(var2)) + "]");
   const auto it_succ_v1 = boost::adjacent_vertices(v1, *data);
   const auto it_succ_v2 = boost::adjacent_vertices(v2, *data);

   // Verifico se v1 e v2 pilotano moltiplicazioni
   std::set<unsigned int> mult_succ_of_v1;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, DEBUG_LEVEL_VERY_PEDANTIC,
                  "-->[D]Statement with USE first variable");
   std::for_each(it_succ_v1.first, it_succ_v1.second,
                 [this, &mult_succ_of_v1] (const vertex succ) {
                   const std::string op_label = data->CGetOpNodeInfo(succ)->GetOperation();
                   const unsigned int succ_id = data->CGetOpNodeInfo(succ)->GetNodeId();
                   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, DEBUG_LEVEL_VERY_PEDANTIC,
                                  "---[D][" + STR(succ_id) + "] type: " + STR(op_label));
                   if (op_label == "mult_expr") {
                     mult_succ_of_v1.insert(succ_id);
                   }
                 });
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, DEBUG_LEVEL_VERY_PEDANTIC, "<--");

   std::set<unsigned int> mult_succ_of_v2;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, DEBUG_LEVEL_VERY_PEDANTIC,
                  "-->[D]Statement with USE second variable");
   std::for_each(it_succ_v2.first, it_succ_v2.second,
                 [this, &mult_succ_of_v2] (const vertex succ) {
                   const std::string op_label = data->CGetOpNodeInfo(succ)->GetOperation();
                   const unsigned int succ_id = data->CGetOpNodeInfo(succ)->GetNodeId();
                   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, DEBUG_LEVEL_VERY_PEDANTIC,
                                  "---[D][" + STR(succ_id) + "] type: " + STR(op_label));
                   if (op_label == "mult_expr") {
                     mult_succ_of_v2.insert(succ_id);
                   }
                 });
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, DEBUG_LEVEL_VERY_PEDANTIC, "<--");

   // Check both pilot mult
   const bool both_pilot_mult = mult_succ_of_v1.empty() == false &&
       mult_succ_of_v2.empty() == false;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, DEBUG_LEVEL_VERY_PEDANTIC,
                  "Both pilot mult_expr: " + STR(both_pilot_mult));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, DEBUG_LEVEL_VERY_PEDANTIC, "<--");
   if (both_pilot_mult) {
     return 5;
   }
   // ------------

   const CustomSet<unsigned int> & ssa_read1 = data->CGetOpNodeInfo(v1)->GetVariables(FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::USE);
   if(is_a_phi1)
   {
      if(ssa_read1.find(var2) != ssa_read1.end())
         return 5;
   }
   const CustomSet<unsigned int> & ssa_read2 = data->CGetOpNodeInfo(v2)->GetVariables(FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::USE);
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
            if(fu.lock()->get_index(v1) == fu.lock()->get_index(v2)/* || base_index1 == base_index2*/)
               return 5;
            else
               return 1;
         }
         bool they_have_common_inputs = false;
         CustomSet<unsigned int>::const_iterator it1_end = ssa_read1.end();
         for(CustomSet<unsigned int>::const_iterator it1 = ssa_read1.begin(); it1 != it1_end; ++it1)
         {
            if(ssa_read2.find(*it1) != ssa_read2.end())
            {
               they_have_common_inputs = true;
               break;
            }
            else if(vw2vertex.find(*it1) != vw2vertex.end())
            {
               vertex from_v1 = vw2vertex.find(*it1)->second;
               CustomSet<unsigned int>::const_iterator it2_end = ssa_read2.end();
               for(CustomSet<unsigned int>::const_iterator it2 = ssa_read2.begin(); it2 != it2_end; ++it2)
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

unsigned int StorageValueInformation::get_storage_value_bitsize(unsigned int storage_value_index) const
{
   return tree_helper::size(HLS_mgr->get_tree_manager(), get_variable_index(storage_value_index));
}
