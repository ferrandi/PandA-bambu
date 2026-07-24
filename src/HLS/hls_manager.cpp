/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file hls_manager.cpp
 * @brief Data structure containing all the information for HLS.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "hls_manager.hpp"

#include "BackendWrapper.hpp"
#include "Parameter.hpp"
#include "SimulationInformation.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "functions.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "memory.hpp"
#include "op_graph.hpp"
#include "polixml.hpp"
#include "storage_value_information.hpp"
#include "utility.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#define MAX_BITWIDTH_SIZE 4096

#include <pugixml.hpp>

HLS_manager::HLS_manager(const ParameterConstRef _Param, const HLS_deviceRef _HLS_D)
    : application_manager(false, _Param),
      HLS_D(_HLS_D),
      back_flow(nullptr),
      memory_version(1),
      base_address(0),
      HLS_execution_time(0),
      Rfuns(nullptr),
      Rmem(nullptr),
      RSim(nullptr),
      module_arch(nullptr)
{
}

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

std::vector<hlsRef> HLS_manager::GetAllImplementations() const
{
   std::vector<hlsRef> implementations;
   implementations.reserve(hlsMap.size());
   for(const auto& [_, implementation] : hlsMap)
   {
      if(implementation)
      {
         implementations.push_back(implementation);
      }
   }
   return implementations;
}

void HLS_manager::clear_ding_dong_buffer_candidates()
{
   ding_dong_buffer_candidates_.clear();
}

void HLS_manager::add_ding_dong_buffer_candidate(const unsigned int top_id, const unsigned int var_id,
                                                 const ding_dong_buffer_info& info)
{
   ding_dong_buffer_candidates_[top_id][var_id] = info;
}

const std::map<unsigned int, std::map<unsigned int, HLS_manager::ding_dong_buffer_info>>&
HLS_manager::get_ding_dong_buffer_candidates() const
{
   return ding_dong_buffer_candidates_;
}

const std::map<unsigned int, HLS_manager::ding_dong_buffer_info>&
HLS_manager::get_ding_dong_buffer_candidates(const unsigned int top_id) const
{
   static const std::map<unsigned int, ding_dong_buffer_info> empty_candidates;
   const auto candidate_it = ding_dong_buffer_candidates_.find(top_id);
   return candidate_it == ding_dong_buffer_candidates_.end() ? empty_candidates : candidate_it->second;
}

const HLS_manager::ding_dong_buffer_info* HLS_manager::get_ding_dong_buffer_candidate(const unsigned int top_id,
                                                                                      const unsigned int var_id) const
{
   const auto top_it = ding_dong_buffer_candidates_.find(top_id);
   if(top_it == ding_dong_buffer_candidates_.end())
   {
      return nullptr;
   }
   const auto var_it = top_it->second.find(var_id);
   return var_it == top_it->second.end() ? nullptr : &var_it->second;
}

hlsRef HLS_manager::create_HLS(const HLS_managerRef HLSMgr, unsigned int functionId)
{
   THROW_ASSERT(functionId, "No function");
   const auto& OperationsList = HLSMgr->CGetFunctionBehavior(functionId)->get_levels();
   OpVertexSet Operations(&HLSMgr->CGetFunctionBehavior(functionId)->GetOpGraphsCollection());
   Operations.insert(OperationsList.begin(), OperationsList.end());
   if(HLSMgr->hlsMap.find(functionId) == HLSMgr->hlsMap.end())
   {
      /// creates the new HLS data structure associated with the function
      const auto function_name = ir_helper::GetFunctionName(HLSMgr->get_ir_manager()->GetIRNode(functionId));
      HLS_constraintsRef HLS_C = HLS_constraintsRef(new HLS_constraints(HLSMgr->get_parameter(), function_name));
      for(const auto& [fu_info, constraints] : HLSMgr->global_resource_constraints)
      {
         if(HLS_C->get_number_fu(fu_info.first, fu_info.second) == INFINITE_UINT)
         {
            HLS_C->set_number_fu(fu_info.first, fu_info.second, constraints.first);
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
   const auto node = TM->GetIRNode(node_id);
   const auto node_type = ir_helper::CGetType(node);
   if(ir_helper::IsRealType(node_type))
   {
      THROW_ASSERT(ir_helper::Size(node_type) == precision, "real precision mismatch");
      const auto rc = GetPointerS<const constant_fp_val_node>(node);
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
   else if(ir_helper::IsVectorType(node_type))
   {
      const auto vc = GetPointerS<const constant_vector_val_node>(node);
      auto n_elm = static_cast<unsigned int>(vc->list_of_valu.size());
      auto elm_prec = precision / n_elm;
      trimmed_value = "";
      for(unsigned int i = 0; i < n_elm; ++i)
      {
         trimmed_value = get_constant_string(vc->list_of_valu[i]->index, elm_prec) + trimmed_value;
      }
   }
   else
   {
      trimmed_value = convert_to_binary(ir_helper::GetConstValue(node), precision);
   }
   return trimmed_value;
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
         HLS->xwrite(nodeRoot, CGetFunctionBehavior(top_function)->GetOpGraph(FunctionBehavior::FDFG));
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

std::vector<HLS_manager::io_binding_type> HLS_manager::get_required_values(unsigned int fun_id,
                                                                           OpGraph::vertex_descriptor v) const
{
   const auto cfg = CGetFunctionBehavior(fun_id)->GetOpGraph(FunctionBehavior::CFG);
   const auto& node = cfg.CGetNodeInfo(v).node;
   std::vector<io_binding_type> required;
   if(node)
   {
      ir_helper::get_required_values(required, node);
   }
   return required;
}

bool HLS_manager::is_register_compatible(unsigned int var) const
{
   const auto var_node = TM->GetIRNode(var);
   return ir_helper::IsSsaName(var_node) && !ir_helper::IsVirtual(var_node) && // virtual ssa_node is not considered
          !ir_helper::IsParameter(
              var_node) &&              // parameters have been already stored in a register by the calling function
          !Rmem->has_base_address(var); // ssa_node allocated in memory
}

bool HLS_manager::is_reading_writing_function(unsigned funID) const
{
   auto fun_node = TM->GetIRNode(funID);
   THROW_ASSERT(fun_node->get_kind() == function_val_node_K, "unexpected condition");
   auto fd = GetPointerS<function_val_node>(fun_node);
   return fd->reading_memory || fd->writing_memory;
}

bool HLS_manager::IsSingleWriteMemory() const
{
   if(Param->getOption<bool>(OPT_cc_serialize_memory_accesses))
   {
      return true;
   }
   const auto hls_d = get_HLS_device();
   return GetParameterFromParameterOrDeviceOrDefault<std::string>("is_single_write_memory", hls_d, "0") == "1";
}

bool HLS_manager::UseSinglePortSdsMemory() const
{
   const auto hls_d = get_HLS_device();
   if(hls_d && hls_d->has_parameter("model") && hls_d->get_parameter<std::string>("model") == "asap7")
   {
      return true;
   }
   const auto sds_memory_mode =
       GetParameterFromParameterOrDeviceOrDefault<std::string>("sds_memory_mode", hls_d, "SDS");
   if(sds_memory_mode == "SDS")
   {
      return false;
   }
   if(sds_memory_mode == "SDS1")
   {
      return true;
   }
   THROW_ERROR_USAGE("Unsupported value '" + sds_memory_mode +
                     "' for parameter sds_memory_mode. Allowed values are: SDS, SDS1");
   return false;
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
   THROW_ASSERT(to_enum.find(attr) != to_enum.end(), attr);
   return to_enum.at(attr);
}

enum FunctionArchitecture::parm_attr FunctionArchitecture::to_parm_attr(const std::string& attr)
{
   TO_ENUM(parm_attr, FUNC_ARCH_PARM_ATTR_ENUM);
   THROW_ASSERT(to_enum.find(attr) != to_enum.end(), attr);
   return to_enum.at(attr);
}

enum FunctionArchitecture::iface_attr FunctionArchitecture::to_iface_attr(const std::string& attr)
{
   TO_ENUM(iface_attr, FUNC_ARCH_IFACE_ATTR_ENUM);
   THROW_ASSERT(to_enum.find(attr) != to_enum.end(), attr);
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

std::pair<std::string, bool> HLS_manager::bundle_required_get_nth_element(unsigned int index) const
{
   std::pair<std::string, bool> val;
   if(bundle_required.size() > index)
   {
      auto it = next(bundle_required.begin(), index);
      val.first = *it;
      val.second = true;
   }
   else
   {
      val.second = false;
   }
   return val;
}
