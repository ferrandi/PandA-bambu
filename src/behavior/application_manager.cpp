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
 * @file application_manager.cpp
 * @brief Implementation of some methods to manage a generic C application
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "application_manager.hpp"

#include "Parameter.hpp"
#include "UnfoldedFunctionInfo.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "loops.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "tree_common.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include "config_HAVE_FROM_DISCREPANCY_BUILT.hpp"
#include "config_HAVE_PRAGMA_BUILT.hpp"

#if HAVE_FROM_DISCREPANCY_BUILT
#include "Discrepancy.hpp"
#endif

#if HAVE_PRAGMA_BUILT
#include "pragma_manager.hpp"
#endif

#include <limits>

application_manager::application_manager(const FunctionExpanderConstRef function_expander,
                                         const bool _allow_recursive_functions, const ParameterConstRef _Param)
    : cfg_transformations(0),
      cfg_max_transformations(_Param->getOption<size_t>(OPT_max_transformations)),
      TM(new tree_manager(_Param)),
      call_graph_manager(new CallGraphManager(function_expander, _allow_recursive_functions, TM, _Param)),
      Param(_Param),
      address_bitsize(_Param->isOption(OPT_addr_bus_bitsize) ?
                          _Param->getOption<unsigned int>(OPT_addr_bus_bitsize) :
                          (_Param->getOption<std::string>(OPT_gcc_m_env).find("-m64") != std::string::npos ? 64 : 32)),
#if HAVE_PRAGMA_BUILT
      PM(new pragma_manager(application_managerRef(this, null_deleter()), _Param)),
#endif
      debug_level(_Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE))
#if HAVE_FROM_DISCREPANCY_BUILT
      ,
      RDiscr(((_Param->isOption(OPT_discrepancy) and _Param->getOption<bool>(OPT_discrepancy)) or
              (_Param->isOption(OPT_discrepancy_hw) and _Param->getOption<bool>(OPT_discrepancy_hw))) ?
                 new Discrepancy() :
                 nullptr)
#endif
{
   const auto in_files = _Param->getOption<std::list<std::string>>(OPT_input_file);
   input_files.reserve(in_files.size());
   std::copy(in_files.begin(), in_files.end(), std::back_inserter(input_files));
}

application_manager::~application_manager() = default;

const ParameterConstRef application_manager::get_parameter() const
{
   return Param;
}

const tree_managerRef application_manager::get_tree_manager() const
{
   return TM;
}

CallGraphManagerRef application_manager::GetCallGraphManager()
{
   return call_graph_manager;
}

const CallGraphManagerConstRef application_manager::CGetCallGraphManager() const
{
   return call_graph_manager;
}

bool application_manager::hasToBeInterfaced(unsigned int funId) const
{
   // all the root functions and the reached addressed functions must be interfaced
   return CGetCallGraphManager()->GetRootFunctions().count(funId) ||
          CGetCallGraphManager()->GetAddressedFunctions().count(funId);
}

FunctionBehaviorRef application_manager::GetFunctionBehavior(unsigned int index)
{
   const auto& behaviors = call_graph_manager->CGetCallGraph()->CGetCallGraphInfo()->behaviors;
   THROW_ASSERT(behaviors.find(index) != behaviors.end(), "There is no function with index " + STR(index));
   return behaviors.at(index);
}

const FunctionBehaviorConstRef application_manager::CGetFunctionBehavior(unsigned int index) const
{
   const auto& behaviors = call_graph_manager->CGetCallGraph()->CGetCallGraphInfo()->behaviors;
   THROW_ASSERT(behaviors.count(index), "There is no function with index " + STR(index));
   return behaviors.at(index);
}

const TreeNodeConstSet& application_manager::GetGlobalVariables() const
{
   return global_variables;
}

void application_manager::AddGlobalVariable(const tree_nodeConstRef& var)
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

#if HAVE_PRAGMA_BUILT
const pragma_managerRef application_manager::get_pragma_manager() const
{
   return PM;
}
#endif

unsigned int application_manager::get_produced_value(unsigned int fun_id, const vertex& v) const
{
   const auto node = GetProducedValue(fun_id, v);
   return node ? node->index : 0;
}

tree_nodeConstRef application_manager::GetProducedValue(unsigned int fun_id, const vertex& v) const
{
   const auto cfg = CGetFunctionBehavior(fun_id)->CGetOpGraph(FunctionBehavior::CFG);
   const auto node = cfg->CGetOpNodeInfo(v)->node;
   return node ? GetProducedValue(node) : nullptr;
}

unsigned int application_manager::get_produced_value(const tree_nodeRef& tn) const
{
   const auto node = GetProducedValue(tn);
   return node ? node->index : 0;
}

tree_nodeConstRef application_manager::GetProducedValue(const tree_nodeConstRef& tn) const
{
   switch(tn->get_kind())
   {
      case gimple_while_K:
      {
         const auto we = GetPointerS<const gimple_while>(tn);
         return we->op0;
      }
      case gimple_cond_K:
      {
         const auto gc = GetPointerS<const gimple_cond>(tn);
         return gc->op0;
      }
      case gimple_label_K:
      case gimple_return_K:
      case gimple_call_K:
      case gimple_goto_K:
      case CASE_PRAGMA_NODES:
      case gimple_multi_way_if_K:
      case gimple_nop_K:
      {
         break;
      }
      case gimple_phi_K:
      {
         const auto gp = GetPointerS<const gimple_phi>(tn);
         return gp->res;
      }
      case gimple_switch_K:
      {
         const auto se = GetPointerS<const gimple_switch>(tn);
         return se->op0;
      }
      case gimple_assign_K:
      {
         const auto gm = GetPointerS<const gimple_assign>(tn);
         const auto op0 = gm->op0;
         if(gm->init_assignment || gm->clobber)
         {
            break;
         }
         else if(op0->get_kind() == array_ref_K)
         {
            break;
         }
         else if(op0->get_kind() == indirect_ref_K)
         {
            break;
         }
         else if(op0->get_kind() == misaligned_indirect_ref_K)
         {
            break;
         }
         else if(op0->get_kind() == mem_ref_K)
         {
            break;
         }
         else if(op0->get_kind() == target_mem_ref_K)
         {
            break;
         }
         else if(op0->get_kind() == target_mem_ref461_K)
         {
            break;
         }
         else
         {
            return gm->op0;
         }
         break;
      }
      case gimple_asm_K:
      {
         const auto ga = GetPointerS<const gimple_asm>(tn);
         if(ga->out)
         {
            const auto tl = GetPointerS<const tree_list>(ga->out);
            /// only the first output and so only single output gimple_asm are supported
            if(tl->valu)
            {
               return tl->valu;
            }
            else
            {
               THROW_ERROR("unexpected condition");
            }
         }
         else
         {
            return nullptr;
         }
         break;
      }
      case binfo_K:
      case block_K:
      case call_expr_K:
      case aggr_init_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case gimple_bind_K:
      case gimple_for_K:
      case gimple_pragma_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case error_mark_K:
      case lut_expr_K:
      case CASE_BINARY_EXPRESSION:
      case CASE_CPP_NODES:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_TYPE_NODES:
      case CASE_UNARY_EXPRESSION:
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
void application_manager::RegisterTransformation(const std::string& step, const tree_nodeConstRef& tn)
{
   std::string tn_str = "";
   if(tn)
   {
      tn_str = tn->get_kind() == function_decl_K ?
                   ("@" + STR(tn->index) +
                    tree_helper::print_function_name(get_tree_manager(), GetPointerS<const function_decl>(tn))) :
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
   THROW_ASSERT(parm_index, "unexpected null parm_decl index");
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
   THROW_ASSERT(parm_index, "unexpected null parm_decl index " + STR(parm_index));
   THROW_ASSERT(ssa_index, "unexpected null ssa_name index " + STR(ssa_index));
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
