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
 * @file storage_value_information.cpp
 * @brief Storage value information: variable are described by a pair: the variable id and the stage in which the
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "storage_value_information.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "math_function.hpp"
#include "utility.hpp"

#include "config_HAVE_ASSERTS.hpp"

StorageValueInformation::StorageValueInformation(const HLS_managerConstRef _HLS_mgr, const unsigned int _function_id)
    : number_of_storage_values(0),
      HLS_mgr(_HLS_mgr),
      function_id(_function_id),
      data(HLS_mgr->CGetFunctionBehavior(function_id)->GetOpGraph(FunctionBehavior::DFG)),
      fu(nullptr)
{
}

void StorageValueInformation::Initialize()
{
   const auto HLS = HLS_mgr->get_HLS(function_id);
   const auto IRM = HLS_mgr->get_ir_manager();
   fu = HLS->Rfu.get();

   vw2info.clear();
   /// initialize the vw2info relation
   for(auto ki : data.vertices())
   {
      const auto& scalar_defs = getVariablesScalarDef(data, ki);
      if(!scalar_defs.empty())
      {
         for(auto def : scalar_defs)
         {
            if(vw2info.count(def) == 0)
            {
               const auto varNode = IRM->GetIRNode(def);
               if(ir_helper::IsSsaName(varNode) && !ir_helper::IsVirtual(varNode))
               {
                  const auto isParam = ir_helper::IsParameter(varNode);
                  const auto& op_info = data.CGetNodeInfo(ki);
                  const auto isPhi = op_info.node_type & TYPE_PHI;
                  const auto isInt = ir_helper::IsSignedIntegerType(varNode);
                  const auto isReal = ir_helper::IsRealType(varNode);
                  const auto fu_unit = fu ? fu->get_assign(ki) : 0;
                  const auto fu_unit_index = fu ? fu->get_index(ki) : 0;

                  vw2info.emplace(def, VarInfo(ir_helper::Size(varNode), isParam, isPhi, isInt, isReal,
                                               isParam ? OpGraph::null_vertex() : ki, fu_unit, fu_unit_index));
               }
            }
         }
      }
   }
}

unsigned int StorageValueInformation::get_number_of_storage_values() const
{
   return number_of_storage_values;
}

std::pair<unsigned int, unsigned int> StorageValueInformation::get_variable(unsigned int storage_value_index) const
{
   THROW_ASSERT(sv2variable.find(storage_value_index) != sv2variable.end(), "the storage value is missing");
   return sv2variable.at(storage_value_index);
}

int StorageValueInformation::get_compatibility_weight(unsigned int storage_value_index1,
                                                      unsigned int storage_value_index2) const
{
   auto var1 = get_variable(storage_value_index1).first;
   auto var2 = get_variable(storage_value_index2).first;
   auto var1_it = vw2info.find(var1);
   if(var1_it == vw2info.end() || var1_it->second.is_parameter)
   {
      return 1;
   }
   auto var2_it = vw2info.find(var2);
   if(vw2info.find(var2) == vw2info.end() || var2_it->second.is_parameter)
   {
      return 1;
   }
   const auto& varInfo1 = var1_it->second;
   const auto& varInfo2 = var2_it->second;

   auto v1 = varInfo1.op_vertex;
   auto is_a_phi1 = varInfo1.is_a_phi;
   auto v2 = varInfo2.op_vertex;
   auto is_a_phi2 = varInfo2.is_a_phi;

   /// disabled input register sharing
   if(HLS_mgr->get_parameter()->getOption<bool>(OPT_shared_input_registers))
   {
      // compute the successors of v1 e v2
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0,
                     "-->Evaluation storage values (vars): [" + STR(HLS_mgr->get_ir_manager()->GetIRNode(var1)) +
                         "]"
                         " and [" +
                         STR(HLS_mgr->get_ir_manager()->GetIRNode(var2)) + "]");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0,
                     "---vertex names: [" + data.CGetNodeInfo(v1).vertex_name +
                         "]"
                         " and [" +
                         data.CGetNodeInfo(v2).vertex_name + "]");

      static const std::vector<std::string> labels = {"mul_node",        "widen_mul_node",  "ternary_add_node",
                                                      "ternary_ss_node", "ternary_as_node", "ternary_sa_node"};
      const auto it_succ_v1 = boost::adjacent_vertices(v1, data);
      const auto it_succ_v2 = boost::adjacent_vertices(v2, data);
      for(const auto& label : labels)
      {
         // check if v1 or v2 drive complex operations
         // variable coming from the Entry vertex have to be neglected in this analysis
         CustomOrderedSet<unsigned int> op_succ_of_v1_port0, op_succ_of_v1_port1, op_succ_of_v1_port2;
         if(!(data.CGetNodeInfo(v1).node_type & TYPE_ENTRY))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "-->Statement with USE first variable");
            std::for_each(it_succ_v1.first, it_succ_v1.second,
                          [this, &op_succ_of_v1_port0, &op_succ_of_v1_port1, &op_succ_of_v1_port2, &var1,
                           &label](const OpGraph::vertex_descriptor succ) {
                             const std::string op_label = data.CGetNodeInfo(succ).GetOperation();
                             const unsigned int succ_id = data.CGetNodeInfo(succ).GetNodeId();
                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0,
                                            "---[" + STR(succ_id) + "] type: " + STR(op_label));
                             if((op_label == label))
                             {
                                std::vector<HLS_manager::io_binding_type> var_read =
                                    HLS_mgr->get_required_values(function_id, succ);
                                if(std::get<0>(var_read[0]) == var1)
                                {
                                   op_succ_of_v1_port0.insert(succ_id);
                                }
                                else if(std::get<0>(var_read[1]) == var1)
                                {
                                   op_succ_of_v1_port1.insert(succ_id);
                                }
                                else if(var_read.size() == 3 && std::get<0>(var_read[2]) == var1)
                                {
                                   op_succ_of_v1_port2.insert(succ_id);
                                }
                                else
                                {
                                   THROW_ERROR("unexpected case:" + STR(succ_id) + "|" + STR(std::get<0>(var_read[0])) +
                                               ":" + STR(std::get<0>(var_read[1])));
                                }
                             }
                          });
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "<--");
         }

         CustomOrderedSet<unsigned int> op_succ_of_v2_port0, op_succ_of_v2_port1, op_succ_of_v2_port2;
         if(!(data.CGetNodeInfo(v2).node_type & TYPE_ENTRY))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "-->Statement with USE second variable");
            std::for_each(it_succ_v2.first, it_succ_v2.second,
                          [this, &op_succ_of_v2_port0, &op_succ_of_v2_port1, &op_succ_of_v2_port2, &var2,
                           &label](const OpGraph::vertex_descriptor succ) {
                             const std::string op_label = data.CGetNodeInfo(succ).GetOperation();
                             const unsigned int succ_id = data.CGetNodeInfo(succ).GetNodeId();
                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0,
                                            "---[" + STR(succ_id) + "] type: " + STR(op_label));
                             if(op_label == label)
                             {
                                std::vector<HLS_manager::io_binding_type> var_read =
                                    HLS_mgr->get_required_values(function_id, succ);
                                if(std::get<0>(var_read[0]) == var2)
                                {
                                   op_succ_of_v2_port0.insert(succ_id);
                                }
                                else if(std::get<0>(var_read[1]) == var2)
                                {
                                   op_succ_of_v2_port1.insert(succ_id);
                                }
                                else if(var_read.size() == 3 && std::get<0>(var_read[2]) == var2)
                                {
                                   op_succ_of_v2_port2.insert(succ_id);
                                }
                                else
                                {
                                   THROW_ERROR("unexpected case");
                                }
                             }
                          });
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "<--");
         }

         // Check if both pilot complex operations
         auto P0cond = !op_succ_of_v1_port0.empty() && !op_succ_of_v2_port0.empty();
         auto P1cond = (!op_succ_of_v1_port1.empty() && !op_succ_of_v2_port1.empty());
         auto P2cond = (!op_succ_of_v1_port2.empty() && !op_succ_of_v2_port2.empty());
         const auto both_pilot_complex_ops = P0cond || P1cond || P2cond;

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "Both pilot a complex operation: " + STR(both_pilot_complex_ops));
         if(both_pilot_complex_ops)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "<--");
            if(P0cond)
            {
               return 6;
            }
            else if(P1cond)
            {
               return 7;
            }
            else if(P2cond)
            {
               return 8;
            }
            else
            {
               THROW_ERROR("unexpected condition");
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 0, "<--");
   }

   const auto& ssa_read1 = getVariablesScalarUse(data, v1);
   if(is_a_phi1)
   {
      if(ssa_read1.find(var2) != ssa_read1.end())
      {
         return 5;
      }
   }
   const auto& ssa_read2 = getVariablesScalarUse(data, v2);
   if(is_a_phi2)
   {
      if(ssa_read2.find(var1) != ssa_read2.end())
      {
         return 5;
      }
   }
   if(fu)
   {
      auto fu_unit1 = varInfo1.fu_unit;
      auto fu_unit2 = varInfo2.fu_unit;
      auto fu_unit1_index = varInfo1.fu_unit_index;
      auto fu_unit2_index = varInfo2.fu_unit_index;
      if(fu_unit1 == fu_unit2)
      {
         if(fu_unit1_index != INFINITE_UINT)
         {
            if(fu_unit1_index == fu_unit2_index)
            {
               return 5;
            }
            else
            {
               return 1;
            }
         }
         auto they_have_common_inputs = false;
         auto it1_end = ssa_read1.end();
         for(auto it1 = ssa_read1.begin(); it1 != it1_end; ++it1)
         {
            if(ssa_read2.find(*it1) != ssa_read2.end())
            {
               they_have_common_inputs = true;
               break;
            }
            else if(vw2info.find(*it1) != vw2info.end())
            {
               auto from_v1 = vw2info.at(*it1);
               auto it2_end = ssa_read2.end();
               for(auto it2 = ssa_read2.begin(); it2 != it2_end; ++it2)
               {
                  if(vw2info.find(*it2) != vw2info.end())
                  {
                     auto from_v2 = vw2info.at(*it2);
                     if(from_v1.fu_unit == from_v2.fu_unit && from_v1.fu_unit_index != INFINITE_UINT &&
                        from_v1.fu_unit_index == from_v2.fu_unit_index)
                     {
                        they_have_common_inputs = true;
                        break;
                     }
                  }
               }
               if(they_have_common_inputs)
               {
                  break;
               }
            }
         }
         if(they_have_common_inputs)
         {
            return 4;
         }
         if(ssa_read1.find(var2) != ssa_read1.end())
         {
            return 3;
         }
         if(ssa_read2.find(var1) != ssa_read2.end())
         {
            return 3;
         }
         return 2;
      }
   }
   return 1;
}

int StorageValueInformation::get_max_weight() const
{
   return 8;
}

void StorageValueInformation::set_storage_value_index(FSMInfo::state_descriptor, unsigned int variable,
                                                      unsigned int stage, unsigned int sv)
{
   variable2sv[std::make_pair(variable, stage)] = sv;
   sv2variable[sv] = std::make_pair(variable, stage);
   number_of_storage_values = std::max(number_of_storage_values, sv + 1);
}

bool StorageValueInformation::is_a_storage_value(FSMInfo::state_descriptor, unsigned int var_index, unsigned int stage)
{
   return variable2sv.find(std::make_pair(var_index, stage)) != variable2sv.end();
}

unsigned int StorageValueInformation::get_storage_value_index(FSMInfo::state_descriptor, unsigned int var_index,
                                                              unsigned int stage)
{
   THROW_ASSERT(variable2sv.find(std::make_pair(var_index, stage)) != variable2sv.end(),
                "the storage value is missing");
   return variable2sv.find(std::make_pair(var_index, stage))->second;
}

bool StorageValueInformation::are_storage_value_compatible(unsigned int storage_value_index1,
                                                           unsigned int storage_value_index2) const
{
   THROW_ASSERT(storage_value_index1 != storage_value_index2, "unexpected condition");
   const auto var1_nid = get_variable(storage_value_index1);
   const auto var2_nid = get_variable(storage_value_index2);
   const auto varInfo1 = vw2info.at(var1_nid.first);
   const auto varInfo2 = vw2info.at(var2_nid.first);
   const auto isInt1 = varInfo1.isInt;
   const auto isInt2 = varInfo2.isInt;
   const auto isReal1 = varInfo1.isReal;
   ;
   const auto isReal2 = varInfo2.isReal;
   const auto size1 = varInfo1.size;
   const auto size2 = varInfo2.size;
   const auto are_value_bitsize_compatible =
       isInt1 == isInt2 && isReal1 == isReal2 &&
       (((isInt1 && isInt2) || (isReal1 && isReal2)) ? size1 == size2 : ceil_pow2(size1) == ceil_pow2(size2));

   if(!are_value_bitsize_compatible)
   {
      return false;
   }
   const auto is_par1 = varInfo1.is_parameter;
   const auto is_par2 = varInfo2.is_parameter;
   if((is_par1 && var1_nid.second == 0) || (is_par2 && var2_nid.second == 0))
   {
      return false;
   }
   else
   {
      return true;
   }
}
