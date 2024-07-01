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
 * @file hls_manager.cpp
 * @brief Data structure containing all the information for HLS.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "hls_manager.hpp"

#include "BackendFlow.hpp"
#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "memory.hpp"
#include "op_graph.hpp"
#include "polixml.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "utility.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#if HAVE_TASTE
#include "aadl_information.hpp"
#include "functions.hpp"
#endif
#define MAX_BITWIDTH_SIZE 4096

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wswitch-enum"
#else
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#endif

#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

HLS_manager::HLS_manager(const ParameterConstRef _Param, const HLS_deviceRef _HLS_D)
    : application_manager(FunctionExpanderConstRef(new FunctionExpander()), false, _Param),
      HLS_D(_HLS_D),
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
   {
      ;
   }
   else
   {
      Rfuns = functionsRef(new functions());
   }
#endif
}

HLS_manager::~HLS_manager() = default;

hlsRef HLS_manager::get_HLS(unsigned int funId) const
{
   if(!funId)
   {
      return hlsRef();
   }
   if(hlsMap.find(funId) == hlsMap.end())
   {
      return hlsRef();
   }
   return hlsMap.find(funId)->second;
}

HLS_deviceRef HLS_manager::get_HLS_device() const
{
   return HLS_D;
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
      for(const auto& globalRC : HLSMgr->global_resource_constraints)
      {
         if(HLS_C->get_number_fu(globalRC.first.first, globalRC.first.second) == INFINITE_UINT)
         {
            HLS_C->set_number_fu(globalRC.first.first, globalRC.first.second, globalRC.second.first);
         }
      }
      HLSMgr->hlsMap[functionId] =
          hlsRef(new hls(HLSMgr->get_parameter(), functionId, Operations, HLSMgr->get_HLS_device(), HLS_C));
   }
   else
   {
      HLSMgr->hlsMap[functionId]->operations = Operations;
   }
   return HLSMgr->hlsMap[functionId];
}

std::string HLS_manager::get_constant_string(unsigned int node_id, unsigned long long precision)
{
   std::string trimmed_value;
   const auto node = TM->GetTreeNode(node_id);
   const auto node_type = tree_helper::CGetType(node);
   if(tree_helper::IsRealType(node_type))
   {
      THROW_ASSERT(tree_helper::Size(node_type) == precision, "real precision mismatch");
      const auto rc = GetPointerS<const real_cst>(node);
      std::string C_value = rc->valr;
      if(C_value == "Inf")
      {
         C_value = rc->valx;
      }
      if(C_value == "Nan" && rc->valx[0] == '-')
      {
         C_value = "-__Nan";
      }
      trimmed_value = convert_fp_to_string(C_value, precision);
   }
   else if(tree_helper::IsVectorType(node_type))
   {
      const auto vc = GetPointerS<const vector_cst>(node);
      auto n_elm = static_cast<unsigned int>(vc->list_of_valu.size());
      auto elm_prec = precision / n_elm;
      trimmed_value = "";
      for(unsigned int i = 0; i < n_elm; ++i)
      {
         trimmed_value = get_constant_string(vc->list_of_valu[i]->index, elm_prec) + trimmed_value;
      }
   }
   else if(tree_helper::IsComplexType(node_type))
   {
      const auto cc = GetPointerS<const complex_cst>(node);
      std::string trimmed_value_r;
      if(cc->real->get_kind() == real_cst_K)
      {
         const auto rcc = GetPointerS<const real_cst>(cc->real);
         std::string C_value_r = rcc->valr;
         if(C_value_r == "Inf")
         {
            C_value_r = rcc->valx;
         }
         trimmed_value_r = convert_fp_to_string(C_value_r, precision / 2);
      }
      else
      {
         trimmed_value_r = convert_to_binary(tree_helper::GetConstValue(cc->real), precision / 2);
      }
      std::string trimmed_value_i;
      if(cc->imag->get_kind() == real_cst_K)
      {
         const auto icc = GetPointerS<const real_cst>(cc->imag);
         std::string C_value_i = icc->valr;
         if(C_value_i == "Inf")
         {
            C_value_i = icc->valx;
         }
         trimmed_value_i = convert_fp_to_string(C_value_i, precision / 2);
      }
      else
      {
         trimmed_value_i = convert_to_binary(tree_helper::GetConstValue(cc->imag), precision / 2);
      }
      trimmed_value = trimmed_value_i + trimmed_value_r;
   }
   else
   {
      trimmed_value = convert_to_binary(tree_helper::GetConstValue(node), precision);
   }
   return trimmed_value;
}

const BackendFlowRef HLS_manager::get_backend_flow()
{
   if(!back_flow)
   {
      back_flow = BackendFlow::CreateFlow(Param, "Synthesis", HLS_D);
   }
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
   const auto& node = cfg->CGetOpNodeInfo(v)->node;
   std::vector<io_binding_type> required;
   if(node)
   {
      tree_helper::get_required_values(required, node);
   }
   return required;
}

bool HLS_manager::is_register_compatible(unsigned int var) const
{
   return tree_helper::is_ssa_name(TM, var) and
          not tree_helper::is_virtual(TM, var) and // virtual ssa_name is not considered
          not tree_helper::is_parameter(
              TM, var) and                 // parameters have been already stored in a register by the calling function
          not Rmem->has_base_address(var); // ssa_name allocated in memory
}

bool HLS_manager::is_reading_writing_function(unsigned funID) const
{
   auto fun_node = TM->GetTreeNode(funID);
   THROW_ASSERT(fun_node->get_kind() == function_decl_K, "unexpected condition");
   auto fd = GetPointerS<function_decl>(fun_node);
   return fd->reading_memory || fd->writing_memory;
}

bool HLS_manager::IsSingleWriteMemory() const
{
   if(Param->getOption<bool>(OPT_gcc_serialize_memory_accesses))
   {
      return true;
   }
   return get_HLS_device() and get_HLS_device()->has_parameter("is_single_write_memory") and
          get_HLS_device()->get_parameter<std::string>("is_single_write_memory") == "1";
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

void HLS_manager::check_bitwidth(unsigned long long prec)
{
   if(prec > MAX_BITWIDTH_SIZE)
   {
      THROW_ERROR("The maximum bit-width size for connection is " + STR(MAX_BITWIDTH_SIZE) +
                  " Requested size: " + STR(prec));
   }
}

#define __TO_ENUM_HELPER(r, data, elem) {BOOST_PP_STRINGIZE(elem), data::elem},
#define TO_ENUM(enum_type, elem_list)                                       \
   static const std::unordered_map<std::string, enum enum_type> to_enum = { \
       BOOST_PP_SEQ_FOR_EACH(__TO_ENUM_HELPER, enum_type, elem_list)}

enum FunctionArchitecture::func_attr FunctionArchitecture::to_func_attr(const std::string& attr)
{
   TO_ENUM(func_attr, FUNC_ARCH_ATTR_ENUM);
   THROW_ASSERT(to_enum.find(attr) != to_enum.end(), "");
   return to_enum.at(attr);
}

enum FunctionArchitecture::parm_attr FunctionArchitecture::to_parm_attr(const std::string& attr)
{
   TO_ENUM(parm_attr, FUNC_ARCH_PARM_ATTR_ENUM);
   THROW_ASSERT(to_enum.find(attr) != to_enum.end(), "");
   return to_enum.at(attr);
}

enum FunctionArchitecture::iface_attr FunctionArchitecture::to_iface_attr(const std::string& attr)
{
   TO_ENUM(iface_attr, FUNC_ARCH_IFACE_ATTR_ENUM);
   THROW_ASSERT(to_enum.find(attr) != to_enum.end(), "");
   return to_enum.at(attr);
}

ModuleArchitecture::ModuleArchitecture(const std::string& filename)
{
   pugi::xml_document doc;
   pugi::xml_node n;
   auto result = doc.load_file(filename.c_str());
   if(result.status == pugi::xml_parse_status::status_file_not_found)
   {
      THROW_WARNING("Missing XML module architecture file.");
   }
   else if(result.status != pugi::xml_parse_status::status_ok)
   {
      THROW_ERROR("Unable to parse XML file: " + filename);
   }
   if((n = doc.child("module")))
   {
      for(auto& f : n)
      {
         THROW_ASSERT(!f.attribute("symbol").empty(), "Function symbol attribute missing from XML.");
         auto& fa = _funcArchs[f.attribute("symbol").value()] =
             refcount<FunctionArchitecture>(new FunctionArchitecture());
         for(auto& a : f.attributes())
         {
            fa->attrs.emplace(FunctionArchitecture::to_func_attr("func_" + std::string(a.name())), a.value());
         }
         for(auto& p : f.child("parameters"))
         {
            THROW_ASSERT(!p.attribute("port").empty(), "Parameter name attribute missing from XML.");
            auto& parm_attrs = fa->parms[p.attribute("port").value()];
            for(auto& a : p.attributes())
            {
               parm_attrs.emplace(FunctionArchitecture::to_parm_attr("parm_" + std::string(a.name())), a.value());
            }
         }
         for(auto& i : f.child("bundles"))
         {
            THROW_ASSERT(!i.attribute("name").empty(), "Interface name attribute missing from XML.");
            auto& iface_attr = fa->ifaces[i.attribute("name").value()];
            for(auto& a : i.attributes())
            {
               iface_attr.emplace(FunctionArchitecture::to_iface_attr("iface_" + std::string(a.name())), a.value());
            }
         }
      }
   }
}

ModuleArchitecture::~ModuleArchitecture() = default;

FunctionArchitectureRef ModuleArchitecture::GetArchitecture(const std::string& funcSymbol) const
{
   auto it = _funcArchs.find(funcSymbol);
   if(it != _funcArchs.end())
   {
      return it->second;
   }
   return nullptr;
}

void ModuleArchitecture::AddArchitecture(const std::string& symbol, FunctionArchitectureRef arch)
{
   _funcArchs[symbol] = arch;
}

void ModuleArchitecture::RemoveArchitecture(const std::string& funcSymbol)
{
   _funcArchs.erase(funcSymbol);
}
