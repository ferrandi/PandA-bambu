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
 * @file hls_manager.cpp
 * @brief Data structure containing all the information for HLS.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "hls_manager.hpp"

///. include
#include "Parameter.hpp"

#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_target.hpp"
#include "memory.hpp"

#include "BackendFlow.hpp"

#include "ext_tree_node.hpp"

#include "call_graph_manager.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"

#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

/// behavior include
#include "op_graph.hpp"

#if HAVE_TASTE
/// intermediate_representation/aadl include
#include "aadl_information.hpp"

/// HLS/function_allocation include
#include "functions.hpp"
#endif

HLS_manager::HLS_manager(const ParameterConstRef _Param, const HLS_targetRef _HLS_T)
    : application_manager(FunctionExpanderConstRef(new FunctionExpander()), true, false, _Param),
      HLS_T(_HLS_T),
      memory_version(1),
      base_address(0),
      HLS_execution_time(0)
#if HAVE_TASTE
      ,
      aadl_information(new AadlInformation())
#endif
{
#if HAVE_TASTE
   if(Param->isOption(OPT_context_switch))
      ;
   else
      Rfuns = functionsRef(new functions());
#endif
}

HLS_manager::~HLS_manager() = default;

hlsRef HLS_manager::get_HLS(unsigned int funId) const
{
   if(!funId)
      return hlsRef();
   if(hlsMap.find(funId) == hlsMap.end())
      return hlsRef();
   return hlsMap.find(funId)->second;
}

HLS_targetRef HLS_manager::get_HLS_target() const
{
   return HLS_T;
}

hlsRef HLS_manager::create_HLS(const HLS_managerRef HLSMgr, unsigned int functionId)
{
   THROW_ASSERT(functionId, "No function");
   const std::deque<vertex>& OperationsList = HLSMgr->CGetFunctionBehavior(functionId)->get_levels();
   OpVertexSet Operations(HLSMgr->CGetFunctionBehavior(functionId)->CGetOpGraph(FunctionBehavior::CFG));
   Operations.insert(OperationsList.begin(), OperationsList.end());
   if(HLSMgr->hlsMap.find(functionId) == HLSMgr->hlsMap.end())
   {
      /// creates the new HLS data structure associated with the function
      const std::string function_name = tree_helper::name_function(HLSMgr->get_tree_manager(), functionId);
      HLS_constraintsRef HLS_C = HLS_constraintsRef(new HLS_constraints(HLSMgr->get_parameter(), function_name));
      for(auto globalRC : HLSMgr->global_resource_constraints)
         if(HLS_C->get_number_fu(globalRC.first.first, globalRC.first.second) == INFINITE_UINT)
            HLS_C->set_number_fu(globalRC.first.first, globalRC.first.second, globalRC.second);
      HLSMgr->hlsMap[functionId] = hlsRef(new hls(HLSMgr->get_parameter(), functionId, Operations, HLSMgr->get_HLS_target(), HLS_C));
      if(HLSMgr->design_interface_constraints.find(functionId) != HLSMgr->design_interface_constraints.end())
      {
         const auto& function_design_interface_constraints = HLSMgr->design_interface_constraints.find(functionId)->second;
         for(const auto& lib_resmap : function_design_interface_constraints)
            for(const auto& res_num : lib_resmap.second)
               HLS_C->set_number_fu(res_num.first, lib_resmap.first, res_num.second);
      }
   }
   else
   {
      HLSMgr->hlsMap[functionId]->operations = Operations;
   }
   return HLSMgr->hlsMap[functionId];
}

std::string HLS_manager::get_constant_string(unsigned int node, unsigned int precision)
{
   std::string trimmed_value;
   if(tree_helper::is_real(TM, tree_helper::get_type_index(TM, node)))
   {
      THROW_ASSERT(tree_helper::size(TM, tree_helper::get_type_index(TM, node)) == precision, "real precision mismatch");
      tree_nodeRef rc_node = TM->get_tree_node_const(node);
      auto* rc = GetPointer<real_cst>(rc_node);
      std::string C_value = rc->valr;
      if(C_value == "Inf")
         C_value = rc->valx;
      if(C_value == "Nan" && rc->valx[0] == '-')
         C_value = "-__Nan";
      trimmed_value = convert_fp_to_string(C_value, precision);
   }
   else if(tree_helper::is_a_vector(TM, tree_helper::get_type_index(TM, node)))
   {
      tree_nodeRef vc_node = TM->get_tree_node_const(node);
      auto* vc = GetPointer<vector_cst>(vc_node);
      auto n_elm = static_cast<unsigned int>(vc->list_of_valu.size());
      unsigned int elm_prec = precision / n_elm;
      trimmed_value = "";
      for(unsigned int i = 0; i < n_elm; ++i)
         trimmed_value = get_constant_string(GET_INDEX_NODE(vc->list_of_valu[i]), elm_prec) + trimmed_value;
   }
   else if(tree_helper::is_a_complex(TM, tree_helper::get_type_index(TM, node)))
   {
      tree_nodeRef cc_node = TM->get_tree_node_const(node);
      auto* cc = GetPointer<complex_cst>(cc_node);
      auto* rcc = GetPointer<real_cst>(GET_NODE(cc->real));
      std::string trimmed_value_r;
      if(rcc)
      {
         std::string C_value_r = rcc->valr;
         if(C_value_r == "Inf")
            C_value_r = rcc->valx;
         trimmed_value_r = convert_fp_to_string(C_value_r, precision / 2);
      }
      else
      {
         auto* ic = GetPointer<integer_cst>(GET_NODE(cc->real));
         THROW_ASSERT(ic, "expected an integer_cst");
         auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
         trimmed_value_r = convert_to_binary(ull_value, precision / 2);
      }
      auto* icc = GetPointer<real_cst>(GET_NODE(cc->imag));
      std::string trimmed_value_i;
      if(icc)
      {
         std::string C_value_i = icc->valr;
         if(C_value_i == "Inf")
            C_value_i = icc->valx;
         trimmed_value_i = convert_fp_to_string(C_value_i, precision / 2);
      }
      else
      {
         auto* ic = GetPointer<integer_cst>(GET_NODE(cc->imag));
         THROW_ASSERT(ic, "expected an integer_cst");
         auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
         trimmed_value_i = convert_to_binary(ull_value, precision / 2);
      }
      trimmed_value = trimmed_value_i + trimmed_value_r;
   }
   else
   {
      tree_nodeRef ic_node = TM->get_tree_node_const(node);
      auto* ic = GetPointer<integer_cst>(ic_node);
      auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
      trimmed_value = convert_to_binary(ull_value, precision);
   }
   return trimmed_value;
}

const BackendFlowRef HLS_manager::get_backend_flow()
{
   if(!back_flow)
      back_flow = BackendFlow::CreateFlow(Param, "Synthesis", HLS_T);
   return back_flow;
}

void HLS_manager::xwrite(const std::string& filename)
{
   try
   {
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("HLS");
      Rmem->xwrite(nodeRoot);
      for(const auto top_function : call_graph_manager->GetRootFunctions())
      {
         hlsRef HLS = hlsMap[top_function];
         HLS->xwrite(nodeRoot, CGetFunctionBehavior(top_function)->CGetOpGraph(FunctionBehavior::FDFG));
      }
      document.write_to_file_formatted(filename);
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
}

std::vector<HLS_manager::io_binding_type> HLS_manager::get_required_values(unsigned int fun_id, const vertex& v) const
{
   const OpGraphConstRef cfg = CGetFunctionBehavior(fun_id)->CGetOpGraph(FunctionBehavior::CFG);
   const unsigned int node_id = cfg->CGetOpNodeInfo(v)->GetNodeId();
   std::vector<io_binding_type> required;
   if(node_id != ENTRY_ID and node_id != EXIT_ID)
      tree_helper::get_required_values(TM, required, TM->get_tree_node_const(node_id), node_id);
   return required;
}

bool HLS_manager::is_register_compatible(unsigned int var) const
{
   return tree_helper::is_ssa_name(TM, var) and not tree_helper::is_virtual(TM, var) and // virtual ssa_name is not considered
          not tree_helper::is_parameter(TM, var) and                                     // parameters have been already stored in a register by the calling function
          not Rmem->has_base_address(var);                                               // ssa_name allocated in memory
}

bool HLS_manager::is_reading_writing_function(unsigned funID) const
{
   auto fun_node = TM->get_tree_node_const(funID);
   auto fd = GetPointer<function_decl>(fun_node);
   THROW_ASSERT(fd, "unexpected condition");
   return fd->reading_memory || fd->writing_memory;
}

bool HLS_manager::IsSingleWriteMemory() const
{
   if(Param->getOption<bool>(OPT_gcc_serialize_memory_accesses))
      return true;
   return get_HLS_target() and get_HLS_target()->get_target_device()->has_parameter("is_single_write_memory") and get_HLS_target()->get_target_device()->get_parameter<std::string>("is_single_write_memory") == "1";
}

unsigned int HLS_manager::GetMemVersion() const
{
   return memory_version;
}

unsigned int HLS_manager::UpdateMemVersion()
{
   memory_version++;
   return memory_version;
}
