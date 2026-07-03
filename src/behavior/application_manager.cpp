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
 * @file application_manager.cpp
 * @brief Implementation of some methods to manage a generic C application
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "application_manager.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "ir_common.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "loops.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"

#include "config_HAVE_FROM_DISCREPANCY_BUILT.hpp"

#if HAVE_FROM_DISCREPANCY_BUILT
#include "Discrepancy.hpp"
#endif

#include <limits>

application_manager::application_manager(const bool _allow_recursive_functions, const ParameterConstRef _Param)
    : cfg_transformations(0),
#ifndef NDEBUG
      cfg_max_transformations(_Param->getOption<size_t>(OPT_max_transformations)),
#endif
      TM(new ir_manager(_Param)),
      call_graph_manager(std::make_unique<CallGraphManager>(_allow_recursive_functions, TM, _Param)),
      Param(_Param),
      address_bitsize(_Param->isOption(OPT_addr_bus_bitsize) ?
                          _Param->getOption<unsigned int>(OPT_addr_bus_bitsize) :
                          (_Param->getOption<std::string>(OPT_cc_m_env).find("-m64") != std::string::npos ? 64 : 32)),
      debug_level(_Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE))
#if HAVE_FROM_DISCREPANCY_BUILT
      ,
      RDiscr(((_Param->isOption(OPT_discrepancy) and _Param->getOption<bool>(OPT_discrepancy))) ? new Discrepancy() :
                                                                                                  nullptr)
#endif
{
   const auto in_files = _Param->getOption<std::list<std::string>>(OPT_input_file);
   input_files.reserve(in_files.size());
   std::copy(in_files.begin(), in_files.end(), std::back_inserter(input_files));
}

ParameterConstRef application_manager::get_parameter() const
{
   return Param;
}

ir_managerRef application_manager::get_ir_manager() const
{
   return TM;
}

CallGraphManager& application_manager::GetCallGraphManager()
{
   return *call_graph_manager;
}

const CallGraphManager& application_manager::CGetCallGraphManager() const
{
   return *call_graph_manager;
}

bool application_manager::hasToBeInterfaced(unsigned int funId) const
{
   // all the root functions and the reached addressed functions must be interfaced
   return call_graph_manager->GetRootFunctions().count(funId) ||
          call_graph_manager->GetAddressedFunctions().count(funId);
}

bool application_manager::isOmpLambdaFunction(unsigned int funId) const
{
   return call_graph_manager->IsOMPLambdaFunction(funId);
}

unsigned application_manager::GetOMPThreadsCount(unsigned int funId) const
{
   return call_graph_manager->GetOMPThreadsCount(funId);
}

FunctionBehaviorRef application_manager::GetFunctionBehavior(unsigned int index)
{
   const auto& behaviors = call_graph_manager->GetCallGraph().CGetGraphInfo().behaviors;
   THROW_ASSERT(behaviors.count(index), "There is no function with index " + STR(index));
   return behaviors.at(index);
}

FunctionBehaviorConstRef application_manager::CGetFunctionBehavior(unsigned int index) const
{
   const auto& behaviors = call_graph_manager->GetCallGraph().CGetGraphInfo().behaviors;
   THROW_ASSERT(behaviors.count(index), "There is no function with index " + STR(index));
   return behaviors.at(index);
}

const IRNodeConstSet& application_manager::GetGlobalVariables() const
{
   return global_variables;
}

void application_manager::AddGlobalVariable(const ir_nodeConstRef& var)
{
   global_variables.insert(var);
}

CustomOrderedSet<unsigned int> application_manager::get_functions_with_body() const
{
   return call_graph_manager->GetReachedBodyFunctions();
}

CustomOrderedSet<unsigned int> application_manager::get_functions_without_body() const
{
   return call_graph_manager->GetReachedLibraryFunctions();
}

unsigned int application_manager::get_produced_value(unsigned int fun_id, OpGraph::vertex_descriptor v) const
{
   const auto node = GetProducedValue(fun_id, v);
   return node ? node->index : 0;
}

ir_nodeConstRef application_manager::GetProducedValue(unsigned int fun_id, OpGraph::vertex_descriptor v) const
{
   const auto node = CGetFunctionBehavior(fun_id)->GetOpGraphsCollection().CGetNodeInfo(v).node;
   return node ? GetProducedValue(node) : nullptr;
}

unsigned int application_manager::get_produced_value(const ir_nodeRef& tn) const
{
   const auto node = GetProducedValue(tn);
   return node ? node->index : 0;
}

ir_nodeConstRef application_manager::GetProducedValue(const ir_nodeConstRef& tn) const
{
   switch(tn->get_kind())
   {
      case return_stmt_K:
      case call_stmt_K:
      case multi_way_if_stmt_K:
      case nop_stmt_K:
      {
         break;
      }
      case phi_stmt_K:
      {
         const auto gp = GetPointerS<const phi_stmt>(tn);
         return gp->res;
      }
      case assign_stmt_K:
      {
         const auto gm = GetPointerS<const assign_stmt>(tn);
         const auto op0 = gm->op0;
         if(op0->get_kind() == unaligned_mem_access_node_K)
         {
            break;
         }
         else if(op0->get_kind() == mem_access_node_K)
         {
            break;
         }
         else
         {
            return gm->op0;
         }
         break;
      }
      case call_node_K:
      case constructor_node_K:
      case identifier_node_K:
      case ssa_node_K:
      case statement_list_node_K:
      case lut_node_K:
      case CASE_BINARY_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_TERNARY_NODES:
      case CASE_TYPE_NODES:
      case CASE_UNARY_NODES:
         THROW_ERROR("Operation not yet supported: " + tn->get_kind_text());
         break;
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return nullptr;
}

void application_manager::add_written_object(unsigned int node_id)
{
   // std::cerr << "Written object " << node_id << std::endl;
   written_objects.insert(node_id);
}

const CustomOrderedSet<unsigned int>& application_manager::get_written_objects() const
{
   return written_objects;
}

void application_manager::clean_written_objects()
{
   written_objects.clear();
}

#ifndef NDEBUG
void application_manager::RegisterTransformation(const std::string& step, const ir_nodeConstRef& tn)
{
   std::string tn_str = "";
   if(tn)
   {
      tn_str = tn->get_kind() == function_val_node_K ? ("@" + STR(tn->index) + ir_helper::GetFunctionName(tn)) :
                                                       tn->ToString();
   }
   THROW_ASSERT(cfg_transformations < cfg_max_transformations,
                step + " - " + tn_str + " Transformations " + STR(cfg_transformations));
   cfg_transformations++;
   if(cfg_max_transformations != std::numeric_limits<size_t>::max())
   {
      INDENT_OUT_MEX(0, 0, "---Transformation " + STR(cfg_transformations) + " - " + step + " - " + tn_str);
   }
}
#endif

unsigned application_manager::getSSAFromParm(unsigned int functionID, unsigned parm_index) const
{
   THROW_ASSERT(parm_index, "unexpected null argument_val_node index");
   const auto fun_parms = Parm2SSA_map.find(functionID);
   if(fun_parms != Parm2SSA_map.end())
   {
      const auto parm = fun_parms->second.find(parm_index);
      if(parm != fun_parms->second.end())
      {
         return parm->second;
      }
   }
   return 0U;
}

void application_manager::setSSAFromParm(unsigned int functionID, unsigned int parm_index, unsigned ssa_index)
{
   THROW_ASSERT(functionID, "unexpected null function id: " + STR(functionID));
   THROW_ASSERT(parm_index, "unexpected null argument_val_node index " + STR(parm_index));
   THROW_ASSERT(ssa_index, "unexpected null ssa_node index " + STR(ssa_index));
   if(Parm2SSA_map.find(functionID) == Parm2SSA_map.end())
   {
      Parm2SSA_map[functionID][parm_index] = ssa_index;
   }
   else
   {
      if(Parm2SSA_map.find(functionID)->second.find(parm_index) == Parm2SSA_map.find(functionID)->second.end())
      {
         Parm2SSA_map[functionID][parm_index] = ssa_index;
      }
      else
      {
         THROW_ASSERT(Parm2SSA_map.find(functionID)->second.find(parm_index)->second == ssa_index,
                      "unexpected condition " + STR(functionID) + " " + STR(parm_index) + " " + STR(ssa_index));
      }
   }
}
void application_manager::clearParm2SSA(unsigned int functionID)
{
   Parm2SSA_map[functionID].clear();
}

CustomMap<unsigned, unsigned> application_manager::getACopyParm2SSA(unsigned int functionID)
{
   return Parm2SSA_map[functionID];
}
