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
 *              Copyright (C) 2024-2026 Politecnico di Milano
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
 * @file mdpi_wrapper_c_writer.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "mdpi_wrapper_c_writer.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "indented_output_stream.hpp"
#include "instruction_writer.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "testbench_generation.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

#include <regex>
#include <string>
#include <vector>

REF_FORWARD_DECL(application_manager);

MdpiWrapperCWriter::MdpiWrapperCWriter(const HLS_managerConstRef _HLSMgr,
                                       const InstructionWriterRef _instruction_writer,
                                       const IndentedOutputStreamRef _indented_output_stream)
    : CWriter(_HLSMgr, _instruction_writer, _indented_output_stream)
{
}

void MdpiWrapperCWriter::WriteFunctionImplementation(unsigned int)
{
}

void MdpiWrapperCWriter::WriteBuiltinWaitCall()
{
}

void MdpiWrapperCWriter::InternalInitialize()
{
}

void MdpiWrapperCWriter::InternalWriteHeader()
{
   indented_output_stream->Append(R"(
#if !defined(__cplusplus) || __cplusplus < 201103L
#error This file must be compiled with C++ 11 standard
#endif

#define _FILE_OFFSET_BITS 64

#undef printf

#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>

#ifndef _Bool
#define _Bool bool
#endif

#ifdef __AC_NAMESPACE
using namespace __AC_NAMESPACE;
#endif

#include <mdpi/mdpi_wrapper.h>
)");

   // get the root function to be tested by the testbench
   const auto top_symbols = Param->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = TM->GetFunction(top_symbols.front());
   const auto top_fname = ir_helper::GetFunctionName(top_fnode);
   const auto& parms = HLSMgr->module_arch->GetArchitecture(top_fname)->parms;

   CustomOrderedSet<std::filesystem::path> includes;
   for(const auto& [parm, attrs] : parms)
   {
      const auto attr_it = attrs.find(FunctionArchitecture::parm_includes);
      if(attr_it != attrs.end())
      {
         string_to_container(std::inserter(includes, includes.end()), attr_it->second, ";");
      }
   }
   if(includes.size())
   {
      indented_output_stream->Append("#define " + top_fname + " __keep_your_declaration_out_of_my_code\n");
      indented_output_stream->Append("#define main __keep_your_main_out_of_my_code\n");

      const auto output_hls_directory =
          Param->getOption<std::filesystem::path>(OPT_output_hls_directory) / "simulation";
      for(const auto& inc : includes)
      {
         if(inc.is_absolute())
         {
            indented_output_stream->Append("#include \"" + inc.string() + "\"\n");
         }
         else
         {
            indented_output_stream->Append(
                "#include \"" +
                (std::filesystem::current_path() / inc).lexically_proximate(output_hls_directory).string() + "\"\n");
         }
      }
      indented_output_stream->Append("#undef " + top_fname + "\n");
      indented_output_stream->Append("#undef main\n\n");
   }
   indented_output_stream->Append(R"(
#ifndef CDECL
#define CDECL extern "C"
#endif

#ifndef EXTERN_CDECL
#define EXTERN_CDECL extern "C"
#endif

)");
}

void MdpiWrapperCWriter::InternalWriteGlobalDeclarations()
{
   instrWriter->write_declarations();
}

void MdpiWrapperCWriter::InternalWriteFile()
{
   const auto top_symbols = Param->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = TM->GetFunction(top_symbols.front());
   const auto top_fb = HLSMgr->CGetFunctionBehavior(top_fnode->index);
   const auto top_bh = top_fb->CGetBehavioralHelper();
   const auto top_fname = top_bh->GetFunctionName();
   const auto top_fname_mngl = top_bh->GetFunctionName();
   const auto interface_type = Param->getOption<HLSFlowStep_Type>(OPT_interface_type);
   const auto is_interface_inferred = interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION;
   auto func_arch = HLSMgr->module_arch->GetArchitecture(top_fname_mngl);
   THROW_ASSERT(func_arch, "Expected interface architecture for function " + top_fname_mngl);

   // if the function is partitioned this variable is different from nullptr and contains the architecture of the csroa
   // wrapper, which is used to get the information about the interface partitioning
   const auto orig_func_arch = HLSMgr->module_arch->GetArchitecture("original_" + top_fname_mngl);
   if(orig_func_arch)
   {
      THROW_ASSERT(orig_func_arch->attrs.find(FunctionArchitecture::func_original) != orig_func_arch->attrs.end(),
                   "Expected original function architecture for function " + top_fname_mngl);
   }
   bool is_interface_partitioned = func_arch->attrs.find(FunctionArchitecture::func_csroa) != func_arch->attrs.end();

   const auto return_type = ir_helper::GetFunctionReturnType(top_fnode);
   const auto top_params = top_bh->GetParameters();
   auto args_decl_size = 0U;
   auto banked_args_decl_size = 0U;
   const auto has_subnormals = Param->isOption(OPT_fp_subnormal) && Param->getOption<bool>(OPT_fp_subnormal);
   const auto cmp_type = [&](ir_nodeConstRef t, const std::string& tname) -> std::string {
      if(std::regex_search(tname, std::regex("^a[pc]_u?(int|fixed)<")))
      {
         return "val";
      }
      else if(std::regex_search(tname, std::regex(R"((\bfloat\b|\bdouble\b))")))
      {
         return has_subnormals ? "flts" : "flt";
      }
      else if(t)
      {
         bool isPtd = false;
         if(ir_helper::IsPointerType(t))
         {
            t = ir_helper::CGetPointedType(t);
            isPtd = true;
         }
         while(ir_helper::IsArrayType(t))
         {
            t = ir_helper::CGetElements(t);
         }
         if(ir_helper::IsRealType(t))
         {
            return has_subnormals ? "flts" : "flt";
         }
         else if(ir_helper::IsStructType(t) || ir_helper::IsVoidType(t) || starts_with(tname, "void") ||
                 starts_with(tname, "const void") ||
                 (isPtd && t->get_kind() == integer_ty_node_K &&
                  GetPointerS<const integer_ty_node>(t)->bitsizealloc == 8))
         {
            return "mem";
         }
      }
      return "val";
   };
   const auto param_size_default = [&]() {
      CustomMap<size_t, std::string> idx_size;
      if(Param->isOption(OPT_testbench_param_size))
      {
         const auto param_size_str = Param->getOption<std::string>(OPT_testbench_param_size);
         size_t param_idx = 0;
         for(const auto& param : top_bh->GetParameters())
         {
            const auto parm_name = top_bh->PrintVariable(param->index);
            std::cmatch what;
            if(std::regex_search(param_size_str.c_str(), what, std::regex("\\b" + parm_name + ":(\\d+)")))
            {
               idx_size[param_idx] = what[1u].str();
            }
            ++param_idx;
         }
      }
      return idx_size;
   }();
   bool global_use_banked = false;
   if(func_arch->ifaces.find("bus") != func_arch->ifaces.end())
   {
      const auto& bus_if = func_arch->ifaces.at("bus");
      if(bus_if.find(FunctionArchitecture::iface_bank_number) != bus_if.end())
      {
         global_use_banked = std::stoul((bus_if.find(FunctionArchitecture::iface_bank_number)->second)) > 0U;
      }
   }
   const auto tb_memmap_mode = Param->getOption<std::string>(OPT_testbench_map_mode);

   std::string top_decl;
   std::string gold_decl = "EXTERN_CDECL ";
   std::string pp_decl = "EXTERN_CDECL ";
   std::string gold_call;
   std::string pp_call;
   std::string args_init;
   std::string args_decl = "__m_argmap_t args[] = {\n";
   std::string banked_args_decl = "__m_argmap_t banked_args[] = {\n";
   std::string args_map = " = {";
   std::string banked_args_map = " = {";
   std::string args_set;
   std::string gold_cmp;
   size_t param_idx = 0;
   if(return_type)
   {
      const auto return_type_str = ir_helper::PrintType(return_type);
      top_decl += top_fname == "main" ? "int" : return_type_str;
      gold_decl += return_type_str;
      pp_decl += return_type_str;
      gold_call += "retval_gold = ";
      pp_call += "retval_pp = ";
      args_decl = return_type_str + " retval, retval_gold, retval_pp;\n" + args_decl;
   }
   else
   {
      top_decl += top_fname == "main" ? "int" : "void";
      gold_decl += "void";
      pp_decl += "void";
   }
   top_decl += " " + top_fname_mngl + "(";
   gold_decl += " " + cxa_prefix_mangled(top_fname_mngl, "__m_") + "(";
   pp_decl += " __m_pp_" + top_fname + "(";
   gold_call += cxa_prefix_mangled(top_fname_mngl, "__m_") + "(";
   pp_call += "__m_pp_" + top_fname + "(";
   if(top_params.size())
   {
      for(size_t i = 0; i < top_params.size();)
      {
         const auto& arg = top_params[i];
         const auto parm_name = top_bh->PrintVariable(arg->index);
         // A parameter is considered partitioned only if the interface is partitioned and the original architecture
         // is available; partitioned parameters are not found in the original architecture since their name changes
         // into {name}_{idx_partition}.
         const bool is_var_partitioned = is_interface_partitioned && orig_func_arch &&
                                         orig_func_arch->parms.find(parm_name) == orig_func_arch->parms.end();
         // A_0 => A
         std::string orig_name = is_var_partitioned ? parm_name.substr(0, parm_name.rfind('_')) : parm_name;
         size_t to_add = 1;

         THROW_ASSERT(func_arch->parms.find(parm_name) != func_arch->parms.end(),
                      "Attributes missing for parameter " + parm_name + " in function " + top_fname);
         const auto& parm_attrs = func_arch->parms.at(parm_name);
         const auto* orig_parm_attrs = is_var_partitioned ? &orig_func_arch->parms.at(orig_name) : nullptr;
         const auto& iface_attrs = func_arch->ifaces.at(parm_attrs.at(FunctionArchitecture::parm_bundle));
         const auto arg_type = ir_helper::CGetType(arg);
         const auto& arg_interface = iface_attrs.at(FunctionArchitecture::iface_mode);
         const auto& arg_bitsize = iface_attrs.at(FunctionArchitecture::iface_bitwidth);
         const auto arg_align = [&]() {
            if(iface_attrs.find(FunctionArchitecture::iface_cache_bus_size) != iface_attrs.end())
            {
               const auto bus_size = std::stoull(iface_attrs.at(FunctionArchitecture::iface_cache_bus_size));
               const auto line_size = std::stoull(iface_attrs.at(FunctionArchitecture::iface_cache_line_size));
               return std::to_string(line_size * bus_size / 8ULL);
            }
            return iface_attrs.at(FunctionArchitecture::iface_alignment);
         }();

         const auto& arg_array_dims =
             is_var_partitioned &&
                     orig_parm_attrs->find(FunctionArchitecture::parm_array_dims) != orig_parm_attrs->end() ?
                 orig_parm_attrs->at(FunctionArchitecture::parm_array_dims) :
                 "";
         const auto& arg_array_partition_types =
             is_var_partitioned &&
                     orig_parm_attrs->find(FunctionArchitecture::parm_array_partition_types) != orig_parm_attrs->end() ?
                 orig_parm_attrs->at(FunctionArchitecture::parm_array_partition_types) :
                 "";
         const auto& arg_array_partition_factors =
             is_var_partitioned && orig_parm_attrs->find(FunctionArchitecture::parm_array_partition_factors) !=
                                       orig_parm_attrs->end() ?
                 orig_parm_attrs->at(FunctionArchitecture::parm_array_partition_factors) :
                 "";
         std::string iface_type, arg_size;

         const auto raw_typename = ir_helper::PrintType(arg, true);
         auto arg_typename = parm_attrs.at(FunctionArchitecture::parm_original_typename);
         if(arg_typename.find("(*)") != std::string::npos)
         {
            arg_typename = arg_typename.substr(0, arg_typename.find("(*)")) + "*";
         }
         const auto arg_name = "P" + std::to_string(param_idx);
         const auto is_pointer_type = arg_typename.back() == '*';
         const auto is_reference_type = arg_typename.back() == '&';
         const auto is_banked =
             arg_interface == "default" && global_use_banked && (is_reference_type || is_pointer_type);

         top_decl += arg_typename + " " + arg_name + ", ";
         gold_decl += arg_typename + ", ";
         pp_decl += raw_typename + ", ";
         std::cmatch what;
         const auto arg_is_channel =
             std::regex_search(arg_typename.data(), what, std::regex("(ac_channel|stream|hls::stream)<(.*)>"));
         if(arg_is_channel)
         {
            THROW_ASSERT(is_pointer_type || is_reference_type, "Channel parameters must be pointers or references.");
            const std::string channel_type(what[1].first, what[1].second);
            arg_typename.pop_back();
            gold_call += arg_name + "_gold, ";
            gold_cmp += "m_channelcmp(" + STR(param_idx) + ", " + cmp_type(arg_type, channel_type) + ");\n";
            iface_type = "channel";
            THROW_ASSERT(iface_attrs.find(FunctionArchitecture::iface_depth) != iface_attrs.end(),
                         "Expected channel depth information.");
            arg_size = iface_attrs.at(FunctionArchitecture::iface_depth);
         }
         else if(is_pointer_type)
         {
            gold_call += "(" + arg_typename + ")" + arg_name + "_gold, ";
            pp_call += "(" + raw_typename + ")" + arg_name + "_pp, ";
            gold_cmp += "m_argcmp(" + STR(param_idx) + ", " + cmp_type(arg_type, arg_typename) + ");\n";
            iface_type = arg_interface == "default" ? "ptr" : arg_interface;
            if(param_size_default.find(param_idx) != param_size_default.end())
            {
               arg_size = param_size_default.at(param_idx);
            }
            else
            {
               const auto parm_size_in_bytes = [&]() -> const std::string* {
                  if(is_var_partitioned)
                  {
                     THROW_ASSERT(orig_parm_attrs, "Missing original parameter attributes for " + parm_name);
                     const auto orig_size_in_bytes = orig_parm_attrs->find(FunctionArchitecture::parm_size_in_bytes);
                     if(orig_size_in_bytes != orig_parm_attrs->end())
                     {
                        return &orig_size_in_bytes->second;
                     }
                  }
                  const auto size_in_bytes = parm_attrs.find(FunctionArchitecture::parm_size_in_bytes);
                  return size_in_bytes != parm_attrs.end() ? &size_in_bytes->second : nullptr;
               }();
               if(parm_size_in_bytes)
               {
                  arg_size = *parm_size_in_bytes;
               }
               else
               {
                  const auto ptd_type = ir_helper::CGetPointedType(arg_type);
                  if(is_interface_inferred)
                  {
                     const auto is_void_pointer = ir_helper::IsVoidType(ptd_type) ||
                                                  (ptd_type->get_kind() == integer_ty_node_K &&
                                                   GetPointerS<const integer_ty_node>(ptd_type)->bitsizealloc == 8);
                     if(is_void_pointer)
                     {
                        THROW_ERROR_USAGE(
                            "Unable to infer the byte size of parameter '" + parm_name + "' in function '" + top_fname +
                            "': a top-level void* parameter has no element-size information for co-simulation. "
                            "Use a typed pointer (e.g., uint8_t*) or use a C/C++ testbench and call "
                            "m_param_alloc(<parameter_position>, <size_in_bytes>) before invoking the top "
                            "function.");
                     }
                     THROW_ERROR_USAGE(
                         "Unable to infer the size in bytes of parameter '" + parm_name + "' in function '" +
                         top_fname +
                         "'. For co-simulation you must provide the pointed-data size with a C/C++ testbench by "
                         "calling m_param_alloc(<parameter_position>, <size_in_bytes>) before the top-function "
                         "call, also when the parameter type is not void*.");
                  }
                  const auto array_size = [&]() {
                     return ir_helper::IsArrayType(ptd_type) ? ir_helper::GetArrayTotalSize(ptd_type) : 1ULL;
                  }();
                  if(ir_helper::IsVoidType(ptd_type) ||
                     (ptd_type->get_kind() == integer_ty_node_K &&
                      GetPointerS<const integer_ty_node>(ptd_type)->bitsizealloc == 8))
                  {
                     arg_size = std::to_string(array_size);
                  }
                  else
                  {
                     arg_size = "sizeof(*" + arg_name + ") * " + std::to_string(array_size);
                  }
               }
            }
         }
         else if(is_reference_type)
         {
            arg_typename.pop_back();
            gold_call += "*(" + arg_typename + "*)" + arg_name + "_gold, ";
            pp_call += "(" + raw_typename + "*)" + arg_name + "_pp, ";
            gold_cmp += "m_argcmp(" + STR(param_idx) + ", " + cmp_type(arg_type, arg_typename) + ");\n";
            iface_type = arg_interface == "default" ? "ptr" : arg_interface;
            arg_size = "sizeof(" + arg_typename + ")";
         }
         else
         {
            gold_call += arg_name + ", ";
            pp_call += arg_name + ", ";
            iface_type = arg_interface;
            arg_size = "sizeof(" + arg_typename + ")";
         }
         const auto arg_ptr = (is_pointer_type ? "(void*)" : "(void*)&") + arg_name;
         args_init += "__m_param_alloc(" + std::to_string(param_idx) + ", " + arg_size + ");\n";
         if(is_banked)
         {
            banked_args_decl += "{" + arg_ptr + ", " + arg_align + ", m_map_" + iface_type + "(" + arg_ptr + ")},\n";
            const auto param_number = param_idx - args_decl_size;
            banked_args_map += std::to_string(param_idx) + ",";
            const auto inter_type = is_pointer_type ? "banked" : iface_type;
            args_set += "m_interface_" + inter_type + "(" + std::to_string(param_idx) + ", banked_args[" +
                        std::to_string(param_number) + "].addr, " + arg_bitsize + ", " + arg_align + ");\n";
            banked_args_decl_size += 1;
         }
         else
         {
            args_decl += "{" + arg_ptr + ", " + arg_align + ", ";
            args_decl += tb_memmap_mode == "SHARED" ? "NULL" : ("m_map_" + iface_type + "(" + arg_ptr + ")");
            args_decl += "},\n";
            const auto param_number = param_idx - banked_args_decl_size;
            args_map += std::to_string(param_idx) + ",";
            if(is_var_partitioned)
            {
               const auto getToks = [](const std::string& str) {
                  std::vector<std::string> dims;
                  size_t start = 0;
                  size_t end;
                  do
                  {
                     end = str.find(',', start);
                     std::string tok = str.substr(start, end - start);
                     dims.push_back(tok);
                     start = end + 1;
                  } while(end != std::string::npos);
                  return dims;
               };
               const auto arg_array_dims_vec = getToks(arg_array_dims);
               const auto arg_array_partition_types_vec = getToks(arg_array_partition_types);
               const auto arg_array_partition_factors_vec = getToks(arg_array_partition_factors);
               THROW_ASSERT(arg_array_dims_vec.size() == arg_array_partition_types_vec.size() &&
                                arg_array_dims_vec.size() == arg_array_partition_factors_vec.size(),
                            "Mismatching number of dimensions, partition types and partition factors for parameter " +
                                parm_name);
               size_t dim_count = arg_array_dims_vec.size();
               args_set += "partition_desc_t part_desc_" + arg_name + "[] = {";
               for(size_t dim_idx = 0; dim_idx < arg_array_dims_vec.size(); ++dim_idx)
               {
                  if(dim_idx > 0)
                  {
                     args_set += ",";
                  }
                  args_set += "{partition_kind_t::" + arg_array_partition_types_vec.at(dim_idx) + ", " +
                              arg_array_partition_factors_vec.at(dim_idx) + "}";

                  if(arg_array_partition_types_vec.at(dim_idx) != "none")
                  {
                     to_add *= std::stoul(arg_array_partition_factors_vec.at(dim_idx));
                  }
               }
               args_set += "};\n";

               args_set += "uint64_t dim_sizes_" + arg_name + "[] = {";
               for(size_t dim_idx = 0; dim_idx < arg_array_dims_vec.size(); ++dim_idx)
               {
                  if(dim_idx > 0)
                  {
                     args_set += ",";
                  }
                  args_set += arg_array_dims_vec.at(dim_idx);
               }
               args_set += "};\n";
               args_set += "m_interface_array_csroa(" + std::to_string(param_idx) + ", args[" +
                           std::to_string(param_number) + "].map_addr, " + arg_bitsize + ", " + arg_align +
                           ", part_desc_" + arg_name + ", dim_sizes_" + arg_name + ", " + std::to_string(dim_count) +
                           ");\n";
            }
            else
            {
               args_set += "m_interface_" + iface_type + "(" + std::to_string(param_idx) + ", args[" +
                           std::to_string(param_number) + "].map_addr, " + arg_bitsize + ", " + arg_align + ");\n";
            }
            args_decl_size += 1;
         }
         ++param_idx;
         i += to_add;
      }
      top_decl.erase(top_decl.size() - 2);
      gold_decl.erase(gold_decl.size() - 2);
      pp_decl.erase(pp_decl.size() - 2);
      gold_call.erase(gold_call.size() - 2);
      pp_call.erase(pp_call.size() - 2);
      if(!return_type && args_decl_size > 0)
      {
         args_decl.erase(args_decl.size() - 2);
         args_map.erase(args_map.size() - 1);
      }
      if(banked_args_decl_size > 0)
      {
         banked_args_decl.erase(banked_args_decl.size() - 2);
         banked_args_map.erase(banked_args_map.size() - 1);
      }
   }
   if(return_type)
   {
      args_init += "__m_param_alloc(" + std::to_string(param_idx) + ", sizeof(retval));\n";
      args_decl += "{&retval, 1, m_map_default(&retval)}";
      args_decl_size += 1;
      const auto param_number = param_idx - banked_args_decl_size;
      args_map += std::to_string(param_idx);
      args_set += "m_interface_default(" + std::to_string(param_idx) + ", args[" + std::to_string(param_number) +
                  "].map_addr, " + std::to_string(ir_helper::SizeAlloc(return_type)) + ", sizeof(retval));\n";
      ++param_idx;
   }
   args_map = "char args_map[" + std::to_string(args_decl_size) + "]" + args_map + "};\n";
   banked_args_map = "char banked_args_map[" + std::to_string(banked_args_decl_size) + "]" + banked_args_map + "};\n";
   args_set += "__m_interface_mem();\n";
   top_decl += ")\n";
   gold_decl += ");\n";
   pp_decl += ");\n";
   gold_call += ");\n";
   pp_call += ");\n";
   args_decl += "};\n";
   banked_args_decl += "};\n";

   if(top_fname != "main")
   {
      indented_output_stream->AppendIndented("CDECL " + top_decl.substr(0, top_decl.size() - 1) + ";\n");
   }
   indented_output_stream->Append("#ifndef MDPI_MEMMAP_MODE\n");
   indented_output_stream->Append("#define MDPI_MEMMAP_MODE MDPI_MEMMAP_" + tb_memmap_mode + "\n");
   indented_output_stream->Append("#endif\n");
   indented_output_stream->AppendIndented(R"(
#ifdef __cplusplus
#include <cstring>
#else
#include <string.h>
#endif
)");

   indented_output_stream->Append("#ifndef BAMBU_SKIP_VERIFICATION\n");
   indented_output_stream->Append(gold_decl);
   indented_output_stream->Append("#endif\n");
   indented_output_stream->Append("#ifdef PP_VERIFICATION\n");
   indented_output_stream->Append(pp_decl);
   indented_output_stream->Append("#endif\n");

   // write C code used to print initialization values for the HDL simulator's memory
   WriteSimulatorInitMemory(top_fnode->index, global_use_banked);
   if(global_use_banked)
   {
      WriteSimulatorBankedMemory(top_fnode->index, banked_args_decl_size);
      WriteBankedMemoryWritebackValue(top_fnode->index);
   }

   indented_output_stream->Append(top_decl);
   indented_output_stream->Append("{\n");
   const auto max_ulp = [&]() -> std::string {
      const auto par = Param->getOption<std::string>(OPT_max_ulp);
      if(par.find(".") != std::string::npos)
      {
         return par + ".0L";
      }
      return par;
   }();
   indented_output_stream->Append("const long double max_ulp = " + max_ulp + ";\n");
   indented_output_stream->Append("size_t i;\n");
   indented_output_stream->Append(args_decl);
   if(global_use_banked)
   {
      indented_output_stream->Append(banked_args_decl);
      if(banked_args_decl_size > 0)
      {
         indented_output_stream->Append(banked_args_map);
      }
      if(args_decl_size > 0)
      {
         indented_output_stream->Append(args_map);
      }
   }
   indented_output_stream->Append(args_init);
   if(global_use_banked)
   {
      if(args_decl_size > 0)
      {
         indented_output_stream->Append("ptr_t banked_base_address = __m_memsetup(args, " + STR(args_decl_size) +
                                        ", args_map);\n\n");
      }
      else
      {
         indented_output_stream->Append("ptr_t banked_base_address = " + STR(HLSMgr->base_address) + ";\n");
      }
      indented_output_stream->Append("char* bank_pointers[WR_BANK_NUMBER * WR_BUNDLE_NUMBER];\n");
      indented_output_stream->Append("unsigned int bank_alignment = WR_CHUNK_SIZE * WR_BANK_NUMBER;\n");
      indented_output_stream->Append("banked_base_address = banked_base_address + (bank_alignment - 1) - "
                                     "((banked_base_address - 1) % bank_alignment);\n");
      indented_output_stream->Append("__m_memsetup_banked(banked_args, WR_BANK_ARG_NUMBER, bank_pointers, "
                                     "banked_args_map, banked_base_address);\n");
   }
   else
   {
      indented_output_stream->Append("__m_memsetup(args, " + STR(args_decl_size) + ");\n\n");
   }

   indented_output_stream->Append(args_set);
   if(global_use_banked)
   {
      // bank_offset = non banked space / bank number * (bank number - 1)
      // example: 8 banks with 1000 of non banked space
      // internal addr from 0 to 1000 mapped on non banked space (for example axi)
      // internal addr 1000 on bank 0 -> bank addr become 125 + 1000 / 8 * 7 = 1000
      // keeps unique internal addres for all variables.
      indented_output_stream->Append("ptr_t bank_offset = ((banked_base_address - (ptr_t)" + STR(HLSMgr->base_address) +
                                     ") / WR_BANK_NUMBER) * (WR_BANK_NUMBER - 1);\n");
      indented_output_stream->Append("m_interface_bank_offset(&bank_offset);\n");
   }

   indented_output_stream->Append("\n__m_sim_start();\n\n");
   indented_output_stream->Append("#ifndef BAMBU_SKIP_VERIFICATION\n");
   indented_output_stream->Append(gold_call);
   indented_output_stream->Append("#endif\n\n");
   indented_output_stream->Append("#ifdef PP_VERIFICATION\n");
   indented_output_stream->Append(pp_call);
   indented_output_stream->Append("#endif\n\n");
   indented_output_stream->Append("__m_sim_end();\n");
   indented_output_stream->Append("__m_interface_fini();\n\n");

   if(global_use_banked)
   {
      indented_output_stream->Append("__membankwb(bank_pointers, banked_args, banked_base_address);\n\n");
   }

   if(gold_cmp.size() || return_type)
   {
      indented_output_stream->Append(R"(
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpointer-type-mismatch"
#endif

)");

      indented_output_stream->Append("__m_argmap_fini(args, " + STR(args_decl_size) + ");\n\n");
      indented_output_stream->Append("size_t mismatch_count = 0;\n");
      indented_output_stream->Append(gold_cmp + "\n");
      if(return_type)
      {
         indented_output_stream->Append("// Return value compare\n");
         indented_output_stream->Append("m_retvalcmp(" + cmp_type(return_type, "") + ")\n\n");
      }
      indented_output_stream->Append(R"(
if(mismatch_count)
{
error("Memory parameter mismatch has been found.\n");
__m_abort();
}

m_call_next();

#ifdef __clang__
#pragma clang diagnostic pop
#endif
)");
   }

   if(return_type)
   {
      indented_output_stream->Append("return retval;\n");
   }
   else if(top_fname == "main")
   {
      indented_output_stream->Append("return 0;\n");
   }
   indented_output_stream->Append("}\n\n");
}

void MdpiWrapperCWriter::WriteSimulatorInitMemory(const unsigned int function_id, bool global_use_banked)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing simulator init memory");
   THROW_ASSERT(HLSMgr->Rmem, "Expected memory allocation to be already computed at this point.");
   const auto mem_vars = HLSMgr->Rmem->get_ext_memory_variables();
   const auto BH = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto parameters = BH->get_parameters();
   const auto funcSymbol = BH->GetFunctionName();
   const auto& ifaces = HLSMgr->module_arch->GetArchitecture(funcSymbol)->ifaces;

   const auto align_bus = [&]() {
      unsigned long long alignment = std::max(8ULL, HLSMgr->Rmem->get_bus_data_bitsize() / 8ULL);
      if(ifaces.find("bus") != ifaces.end())
      {
         const auto bus_iface = ifaces.at("bus");
         if(!global_use_banked && bus_iface.find(FunctionArchitecture::iface_cache_bus_size) != bus_iface.end())
         {
            const auto bus_size = std::stoull(bus_iface.at(FunctionArchitecture::iface_cache_bus_size));
            const auto line_size = std::stoull(bus_iface.at(FunctionArchitecture::iface_cache_line_size));
            alignment = std::max(alignment, (line_size * bus_size / 8ULL));
         }
      }
      return alignment;
   }();
   const auto align_infer = [&]() {
      unsigned long long max = 0ULL;
      for(const auto& [name, attrs] : ifaces)
      {
         if(name != "bus")
         {
            THROW_ASSERT(attrs.find(FunctionArchitecture::iface_alignment) != attrs.end(),
                         "iface alignment not present for interface " + name);
            max = std::max(max, std::stoull(attrs.at(FunctionArchitecture::iface_alignment)));
         }
      }
      return max;
   }();
   const auto align = std::max(align_bus, align_infer);

   indented_output_stream->Append(R"(
typedef struct
{
const char* filename;
size_t size;
const ptr_t addrmap;
void* addr;
} __m_memmap_t;

typedef struct
{
void* addr;
size_t align;
void* map_addr;
} __m_argmap_t;

)");
   if(global_use_banked)
   {
      indented_output_stream->Append(
          "static unsigned long long __m_memsetup(__m_argmap_t args[], size_t args_count, char* args_map)");
   }
   else
   {
      indented_output_stream->Append("static void __m_memsetup(__m_argmap_t args[], size_t args_count)");
   }
   indented_output_stream->Append(R"(
{
int error = 0;
size_t i;
)");
   indented_output_stream->Append("const ptr_t align = " + STR(align) + ";\n");
   auto base_addr = HLSMgr->base_address;
   if(!global_use_banked)
   {
      indented_output_stream->Append("static __m_memmap_t memmap_init[] = {\n");
      if(mem_vars.size())
      {
         const auto output_hls_directory =
             Param->getOption<std::filesystem::path>(OPT_output_hls_directory) / "simulation";
         for(const auto& mem_var : mem_vars)
         {
            const auto var_id = mem_var.first;
            const auto is_top_param = std::find(parameters.begin(), parameters.end(), var_id) != parameters.end();
            if(!is_top_param)
            {
               const auto var_name = BH->PrintVariable(var_id);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization for " + var_name);
               const auto var_addr = HLSMgr->Rmem->get_external_base_address(var_id);
               const auto var_init_dat = output_hls_directory / ("mem_" + STR(var_id) + "." + var_name + ".dat");
               const auto byte_count = TestbenchGeneration::generate_init_file(var_init_dat, TM, var_id, HLSMgr->Rmem);
               indented_output_stream->Append("  {\"" + boost::replace_all_copy(var_init_dat.string(), "\"", "\\\"") +
                                              "\", " + STR(byte_count) + ", " + STR(var_addr) + ", NULL},\n");
               base_addr = std::max(base_addr, var_addr + byte_count);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Init file   : '" + var_init_dat.string() + "'");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Memory usage: " + STR(byte_count) + " bytes");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Base address: " + STR(var_addr));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            }
         }
      }
      indented_output_stream->Append("};\n");
      indented_output_stream->Append("ptr_t base_addr = " + STR(base_addr) + ";\n\n");
      indented_output_stream->Append("__m_memmap_init(MDPI_MEMMAP_MODE);\n");
      indented_output_stream->Append(R"(

// Memory-mapped internal variables initialization
for(i = 0; i < sizeof(memmap_init) / sizeof(*memmap_init); ++i)
{
FILE* fp = fopen(memmap_init[i].filename, "rb");
if(!fp)
{
error("Unable to open file: %s\n", memmap_init[i].filename);
perror("Unable to open memory variable initialization file");
error |= 2;
continue;
}
if(memmap_init[i].addr == NULL)
{
memmap_init[i].addr = malloc(memmap_init[i].size);
}
size_t nbytes = fread(memmap_init[i].addr, 1, memmap_init[i].size, fp);
if(nbytes != memmap_init[i].size)
{
error("Only %zu/%zu bytes were read from file: %s\n", nbytes, memmap_init[i].size, memmap_init[i].filename);
if(ferror(fp))
{
perror("Unable to read from memory variable initialization file");
}
error |= 4;
fclose(fp);
continue;
}
fclose(fp);
error |= __m_memmap(memmap_init[i].addrmap, memmap_init[i].addr, memmap_init[i].size);
}
)");
   }
   else
   {
      indented_output_stream->Append("ptr_t base_addr = " + STR(base_addr) + ";\n\n");
      indented_output_stream->Append("__m_memmap_init(MDPI_MEMMAP_MODE);\n");
   }

   indented_output_stream->Append(R"(
for(i = 0; i < args_count; ++i)
{
if(args[i].map_addr == NULL)
{
args[i].map_addr = args[i].addr;
continue;
}
)");
   if(global_use_banked)
   {
      indented_output_stream->Append("const size_t arg_size = __m_param_size(args_map[i]);\n");
   }
   else
   {
      indented_output_stream->Append("const size_t arg_size = __m_param_size(i);\n");
   }
   indented_output_stream->Append(R"(size_t map_size = arg_size;
base_addr += (align - 1) - ((base_addr - 1) % align);
args[i].map_addr = args[i].addr;
if(arg_size % args[i].align)
{
map_size = arg_size + (args[i].align - 1) - ((arg_size - 1) % args[i].align);
info("Parameter %zu map size extended: %zu bytes -> %zu bytes\n", i, arg_size, map_size);
args[i].map_addr = malloc(map_size);
memcpy(args[i].map_addr, args[i].addr, arg_size);
}
error |= __m_memmap(base_addr, args[i].map_addr, map_size);
base_addr += map_size;
}
if(error)
{
__m_abort();
}
)");
   if(global_use_banked)
   {
      indented_output_stream->Append("return base_addr;\n");
   }
   indented_output_stream->Append(R"(
}

static void __m_argmap_fini(__m_argmap_t args[], size_t args_count)
{
size_t i = 0;
for(i = 0; i < args_count; i++)
{
if(args[i].map_addr != args[i].addr)
{
memcpy(args[i].addr, args[i].map_addr, __m_param_size(i));
free(args[i].map_addr);
args[i].map_addr = args[i].addr;
}
}
}

)");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written simulator init memory");
}

void MdpiWrapperCWriter::InitilizedBMDataStructure(const unsigned int bundle_number, const unsigned int bank_number,
                                                   const bool use_space_required)
{
   indented_output_stream->Append("unsigned int last_bank_used_on_bundles[WR_BUNDLE_NUMBER] = {");
   for(unsigned int i = 0; i < bundle_number; i++)
   {
      if(i > 0)
      {
         indented_output_stream->Append(",");
      }
      indented_output_stream->Append("0");
   }
   indented_output_stream->Append("};\n");

   indented_output_stream->Append("unsigned int last_chunk_space_used_on_bundles[WR_BUNDLE_NUMBER] = {");
   for(unsigned int i = 0; i < bundle_number; i++)
   {
      if(i > 0)
      {
         indented_output_stream->Append(",");
      }
      indented_output_stream->Append("0");
   }
   indented_output_stream->Append("};\n");

   if(use_space_required)
   {
      indented_output_stream->Append(
          "unsigned int space_used_on_banks_by_bundles[WR_BANK_NUMBER * WR_BUNDLE_NUMBER] = {");
      for(unsigned int i = 0; i < bundle_number * bank_number; i++)
      {
         if(i > 0)
         {
            indented_output_stream->Append(",");
         }
         indented_output_stream->Append("0");
      }
      indented_output_stream->Append("};\n\n");
   }
}

void MdpiWrapperCWriter::WriteSimulatorBankedMemory(const unsigned int function_id,
                                                    const unsigned int banked_args_decl_size)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing simulator banked memory");
   const auto BH = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto parameters = BH->get_parameters();
   const auto fnode = TM->GetIRNode(function_id);
   const auto fname = ir_helper::GetFunctionName(fnode);

   THROW_ASSERT(HLSMgr->module_arch->GetArchitecture(fname)->ifaces.find("bus") !=
                    HLSMgr->module_arch->GetArchitecture(fname)->ifaces.end(),
                "Bus interface must be present");
   const auto ifaces = HLSMgr->module_arch->GetArchitecture(fname)->ifaces.at("bus");
   const unsigned int bank_number =
       static_cast<unsigned int>(std::stoul((ifaces.at(FunctionArchitecture::iface_bank_number))));
   const unsigned int bundle_number = static_cast<unsigned int>(HLSMgr->bundle_required.size());
   const unsigned int chunk_size =
       static_cast<unsigned int>(std::stoul((ifaces.at(FunctionArchitecture::iface_chunk_size))));
   auto bank_alignment = std::max(8ULL, HLSMgr->Rmem->get_bus_data_bitsize() / 8ULL);
   if(ifaces.find(FunctionArchitecture::iface_cache_bus_size) != ifaces.end())
   {
      const auto bus_size = std::stoull(ifaces.at(FunctionArchitecture::iface_cache_bus_size));
      const auto line_size = std::stoull(ifaces.at(FunctionArchitecture::iface_cache_line_size));
      bank_alignment = std::max(bank_alignment, (line_size * bus_size / 8ULL));
   }
   const auto mem_vars = HLSMgr->Rmem->get_ext_memory_variables();

   const auto top_fb = HLSMgr->CGetFunctionBehavior(function_id);
   const auto top_bh = top_fb->CGetBehavioralHelper();

   const auto SplitString = [&](std::string s, std::string delimiter) -> std::vector<std::string> {
      size_t pos_start = 0, pos_end, delim_len = delimiter.length();
      std::string token;
      std::vector<std::string> res;

      while((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
      {
         token = s.substr(pos_start, pos_end - pos_start);
         pos_start = pos_end + delim_len;
         res.push_back(token);
      }

      res.push_back(s.substr(pos_start));
      return res;
   };
   indented_output_stream->Append("#define WR_BANK_MEM_VAR_NUMBER " + STR(mem_vars.size()) + "u\n");
   indented_output_stream->Append("#define WR_BANK_ARG_NUMBER " + STR(banked_args_decl_size) + "u\n");
   indented_output_stream->Append("#define WR_BANK_OBJ_NUMBER (WR_BANK_MEM_VAR_NUMBER + WR_BANK_ARG_NUMBER)\n");
   indented_output_stream->Append("#define WR_BUNDLE_NUMBER " + STR(bundle_number) + "u\n");
   indented_output_stream->Append("#define WR_BANK_NUMBER " + STR(bank_number) + "u\n");
   indented_output_stream->Append("#define WR_CHUNK_SIZE " + STR(chunk_size) + "u\n");
   indented_output_stream->Append("#define WR_BANK_ALIGN " + STR(bank_alignment) + "u\n");

   indented_output_stream->Append(R"(
typedef struct
{
size_t size;
char* external_addr;
ptr_t internal_addr;
char* bank_addr;
} __bankmap_t;

typedef struct
{
unsigned int* banks;
size_t size;
} bundle_info;

static __bankmap_t bank_map[WR_BANK_OBJ_NUMBER];
)");
   indented_output_stream->Append("static const unsigned int bundle_map[WR_BANK_OBJ_NUMBER] = {");
   for(unsigned i = 0; i < mem_vars.size(); i++)
   {
      if(i > 0)
      {
         indented_output_stream->Append(",");
      }
      indented_output_stream->Append("0");
   }

   unsigned int index = 0U;
   for(const auto& param : top_bh->GetParameters())
   {
      const auto param_name = top_bh->PrintVariable(param->index);
      const auto func_arch = HLSMgr->module_arch->GetArchitecture(fname);
      const auto& parm_attrs = func_arch->parms.at(param_name);
      const auto& iface_attrs = func_arch->ifaces.at(parm_attrs.at(FunctionArchitecture::parm_bundle));
      const auto& if_mode = iface_attrs.at(FunctionArchitecture::iface_mode);

      auto arg_typename = parm_attrs.at(FunctionArchitecture::parm_original_typename);
      if(arg_typename.find("(*)") != std::string::npos)
      {
         arg_typename = arg_typename.substr(0, arg_typename.find("(*)")) + "*";
      }
      const auto is_pointer_type = arg_typename.back() == '*';
      const auto is_reference_type = arg_typename.back() == '&';

      if(if_mode == "default" && (is_reference_type || is_pointer_type))
      {
         unsigned int bundle_map = 0U; // Default bundle id for not assigned parameters
         if((index + mem_vars.size()) > 0)
         {
            indented_output_stream->Append(",");
         }
         auto it = HLSMgr->bundle_map.find(param_name);
         if(it != HLSMgr->bundle_map.end())
         {
            bundle_map = it->second;
         }
         indented_output_stream->Append(STR(bundle_map));
         index++;
      }
   }
   indented_output_stream->Append("};\n");
   indented_output_stream->Append(R"(
static bundle_info bi[WR_BUNDLE_NUMBER];

static void update_space_required(unsigned int memory_size, unsigned int index, unsigned int* last_bank_used_on_bundles, unsigned int * last_chunk_space_used_on_bundles, unsigned int* chunk_required_on_banks_by_bundles)
{
unsigned int* target_bundle = bi[bundle_map[index]].banks;
unsigned int bundle_size = bi[bundle_map[index]].size;
unsigned int first_bank = target_bundle[0];
unsigned int memory_chunk_number = memory_size / WR_CHUNK_SIZE;
unsigned int remaining_memory = memory_size % WR_CHUNK_SIZE;
for(unsigned int temp = 0; temp < bundle_size; temp++)
{
unsigned int current_bank = ((last_bank_used_on_bundles[bundle_map[index]] + 1 + temp) % bundle_size) + first_bank;
chunk_required_on_banks_by_bundles[bundle_map[index] * WR_BANK_NUMBER + current_bank] += ((memory_chunk_number / bundle_size) + (temp < (memory_chunk_number % bundle_size)));
info("Reserving %u chunks for parameter %u on bank %u on bundle %u\n", ((memory_chunk_number / bundle_size) + (temp < (memory_chunk_number % bundle_size))), index, current_bank, bundle_map[index]);
}
last_bank_used_on_bundles[bundle_map[index]] = (last_bank_used_on_bundles[bundle_map[index]] + memory_chunk_number) % bundle_size;
if((last_chunk_space_used_on_bundles[bundle_map[index]] + remaining_memory) < WR_CHUNK_SIZE)
{
last_chunk_space_used_on_bundles[bundle_map[index]] += remaining_memory;
info("Reserving %u bytes for parameter %u on bank %u on bundle %u\n", remaining_memory, index, last_bank_used_on_bundles[bundle_map[index]]+ first_bank, bundle_map[index]);
}
else
{
last_chunk_space_used_on_bundles[bundle_map[index]] = (last_chunk_space_used_on_bundles[bundle_map[index]] + remaining_memory) % WR_CHUNK_SIZE;
last_bank_used_on_bundles[bundle_map[index]] = (last_bank_used_on_bundles[bundle_map[index]] + 1) % bundle_size;
chunk_required_on_banks_by_bundles[bundle_map[index] * WR_BANK_NUMBER + first_bank + last_bank_used_on_bundles[bundle_map[index]]] += 1;
info("Reserving 1 chunks for parameter %u on bank %u on bundle %u and %u bytes on the next bank \n", index, last_bank_used_on_bundles[bundle_map[index]] + first_bank, bundle_map[index], last_chunk_space_used_on_bundles[bundle_map[index]]);
}
return;
}
   
)");
   indented_output_stream->Append(
       "static void compute_space_required(ptr_t base_addr, char** bank_pointers, __m_argmap_t args[], "
       "size_t args_count, char* banked_args_map)\n");
   indented_output_stream->Append("{\n");

   InitilizedBMDataStructure(bundle_number, bank_number, false);

   indented_output_stream->Append(
       "unsigned int chunk_required_on_banks_by_bundles[WR_BANK_NUMBER * WR_BUNDLE_NUMBER] = {");
   for(unsigned int j = 0; j < bundle_number; j++)
   {
      const auto res = HLSMgr->bundle_required_get_nth_element(j);
      if(!res.second)
      {
         THROW_ERROR("Requiring bundle " + STR(j) + " when there are only " + STR(bundle_number) + " bundles.");
      }
      else
      {
         const auto bundle_string = res.first;
         const auto splitted_list = SplitString(bundle_string, ",");
         const auto first_bank = std::stoul(splitted_list.front());
         for(unsigned int i = 0; i < bank_number; i++)
         {
            if(i > 0 || j > 0)
            {
               indented_output_stream->Append(",");
            }
            if(first_bank == i)
            {
               indented_output_stream->Append("1");
            }
            else
            {
               indented_output_stream->Append("0");
            }
         }
      }
   }
   indented_output_stream->Append("};\n");
   indented_output_stream->Append("int error = 0;\n");

   if(mem_vars.size())
   {
      indented_output_stream->Append(R"(for(unsigned int i = 0; i < WR_BANK_MEM_VAR_NUMBER; ++i)
{
size_t memory_size;
if(i == 0)
{
memory_size = bank_map[i].internal_addr - base_addr + bank_map[i].size;
}
else
{
memory_size = bank_map[i].internal_addr - bank_map[i-1].internal_addr + bank_map[i].size - bank_map[i-1].size;
}
update_space_required(memory_size, i, last_bank_used_on_bundles, last_chunk_space_used_on_bundles, chunk_required_on_banks_by_bundles);
info("Considering mem_var: %u, size allocated = %zu\n", i, memory_size);
}
)");
   }
   indented_output_stream->Append(R"(for(unsigned int i = 0; i < args_count; ++i)
{
const size_t size = __m_param_size(banked_args_map[i]);
size_t memory_size = size + (args[i].align - 1) - ((size - 1) % args[i].align);
update_space_required(memory_size, (i + WR_BANK_MEM_VAR_NUMBER), last_bank_used_on_bundles, last_chunk_space_used_on_bundles, chunk_required_on_banks_by_bundles);
bank_map[i + WR_BANK_MEM_VAR_NUMBER].external_addr = (char*)args[i].addr;
bank_map[i + WR_BANK_MEM_VAR_NUMBER].size = size;
info("Space reserved for parameter: %u, size = %zu\n", banked_args_map[i], memory_size);
}
)");
   const auto external_bit = floor_log2(HLSMgr->base_address);
   const auto bank_number_bitsize = ceil_log2(bank_number);
   const auto bank_distance = external_bit - bank_number_bitsize;
   const auto bundle_number_bitsize = std::max(1U, ceil_log2(bundle_number));
   const auto bundle_distance = external_bit - bundle_number_bitsize;
   const auto bundle_distance_on_banks = external_bit - bank_number_bitsize - bundle_number_bitsize;
   indented_output_stream->Append("size_t bank_distance = " + STR(1 << bank_distance) + ";\n");
   indented_output_stream->Append("size_t bundle_distance = " + STR(1 << bundle_distance) + ";\n");
   indented_output_stream->Append("size_t bundle_distance_on_banks = " + STR(1 << bundle_distance_on_banks) + ";\n");
   indented_output_stream->Append(R"(info("bank_distance: " PTR_FORMAT "\n", bank_distance);
info("bundle_distance: " PTR_FORMAT "\n", bundle_distance);
info("bundle_distance_on_banks: " PTR_FORMAT "\n", bundle_distance_on_banks);

for(unsigned int j = 0; j < WR_BUNDLE_NUMBER; j++)
{
for(unsigned int i = 0; i < WR_BANK_NUMBER; i++)
{
if(chunk_required_on_banks_by_bundles[j*WR_BANK_NUMBER + i] > 0)
{
if((chunk_required_on_banks_by_bundles[j*WR_BANK_NUMBER + i] * WR_CHUNK_SIZE) % WR_BANK_ALIGN)
{
unsigned int padding_required = WR_BANK_ALIGN - ((chunk_required_on_banks_by_bundles[j*WR_BANK_NUMBER + i] * WR_CHUNK_SIZE) % WR_BANK_ALIGN);
chunk_required_on_banks_by_bundles[j*WR_BANK_NUMBER + i] += padding_required / WR_BANK_ALIGN + 1;
info("bank %u: added %u chunks on bundle %u to allow correct alignment\n", i, padding_required / WR_BANK_ALIGN + 1, j);
}
bank_pointers[j*WR_BANK_NUMBER + i] = (char*)malloc(chunk_required_on_banks_by_bundles[j*WR_BANK_NUMBER + i] * WR_CHUNK_SIZE);
info("bank %u: allocated %u chunks for bundle %u\n", i, chunk_required_on_banks_by_bundles[j*WR_BANK_NUMBER + i], j);
unsigned int* target_bundle = bi[j].banks;
error |= __m_memmap(base_addr + i * bank_distance + j*bundle_distance_on_banks, (void*) (bank_pointers[j*WR_BANK_NUMBER + i]), chunk_required_on_banks_by_bundles[j*WR_BANK_NUMBER + i] * WR_CHUNK_SIZE);
info("Mapped %u bytes for bank %u at bank address " PTR_FORMAT " to bank pointer " PTR_FORMAT " \n", chunk_required_on_banks_by_bundles[j*WR_BANK_NUMBER + i] * WR_CHUNK_SIZE, i, base_addr + i * bank_distance + j*bundle_distance_on_banks, (ptr_t)(bank_pointers[j*WR_BANK_NUMBER + i]));
}
else
{
bank_pointers[j*WR_BANK_NUMBER + i] = 0;
}
}
}
if(error)
{
abort();
}
}
)");

   indented_output_stream->Append(R"(
static void allocate_on_bank_memory(unsigned int memory_size, unsigned int index, char** bank_pointers, unsigned int* last_bank_used_on_bundles, unsigned int * last_chunk_space_used_on_bundles, unsigned int* space_used_on_banks_by_bundles,char* banked_args_map, ptr_t base_addr)
{
)");
   indented_output_stream->Append("size_t bank_distance = " + STR(1 << bank_distance) + ";\n");
   indented_output_stream->Append("size_t bundle_distance = " + STR(1 << bundle_distance) + ";\n");
   indented_output_stream->Append(R"(
unsigned int* target_bundle = bi[bundle_map[index]].banks;
unsigned int bundle_size = bi[bundle_map[index]].size;
unsigned int first_bank = target_bundle[0];
unsigned int remaining_memory = memory_size;
unsigned int memory_copied = 0;
unsigned int bank_base_addr_position = bundle_map[index] * WR_BANK_NUMBER + last_bank_used_on_bundles[bundle_map[index]]+first_bank;
bank_map[index].bank_addr = bank_pointers[bank_base_addr_position] + space_used_on_banks_by_bundles[bank_base_addr_position];
unsigned int space_used_on_boudle = 0;
for(unsigned int i = 0; i < bundle_size; i++)
{
space_used_on_boudle += space_used_on_banks_by_bundles[bundle_map[index] * WR_BANK_NUMBER + first_bank + i];
}
if(index >= WR_BANK_MEM_VAR_NUMBER)
{
bank_map[index].internal_addr = base_addr + bundle_map[index] * bundle_distance + space_used_on_boudle;
info("Parameter: %u has internal address " PTR_FORMAT "\n", banked_args_map[index - WR_BANK_MEM_VAR_NUMBER], bank_map[index].internal_addr);
}
else
{
info("Mem var: %u has internal address " PTR_FORMAT "\n", index, bank_map[index].internal_addr);
}
if(WR_CHUNK_SIZE - last_chunk_space_used_on_bundles[bundle_map[index]] >  memory_size)
{
last_chunk_space_used_on_bundles[bundle_map[index]] += memory_size;
memcpy(bank_pointers[bank_base_addr_position] + space_used_on_banks_by_bundles[bank_base_addr_position], bank_map[index].external_addr, memory_size);
space_used_on_banks_by_bundles[bank_base_addr_position] += memory_size;
remaining_memory = 0;
}
else
{
memcpy(bank_pointers[bank_base_addr_position] + space_used_on_banks_by_bundles[bank_base_addr_position], bank_map[index].external_addr, WR_CHUNK_SIZE - last_chunk_space_used_on_bundles[bundle_map[index]]);
space_used_on_banks_by_bundles[bank_base_addr_position] += (WR_CHUNK_SIZE - last_chunk_space_used_on_bundles[bundle_map[index]]);
last_bank_used_on_bundles[bundle_map[index]] = (last_bank_used_on_bundles[bundle_map[index]] + 1) % bundle_size;
memory_copied += (WR_CHUNK_SIZE - last_chunk_space_used_on_bundles[bundle_map[index]]);
remaining_memory = remaining_memory - (WR_CHUNK_SIZE - last_chunk_space_used_on_bundles[bundle_map[index]]);
last_chunk_space_used_on_bundles[bundle_map[index]] = 0;
}
if(remaining_memory >= WR_CHUNK_SIZE)
{
unsigned int memory_chunk_number = remaining_memory / WR_CHUNK_SIZE;
for(unsigned int i = 0; i < memory_chunk_number; i++)
{
unsigned int current_bank_base_addr = bundle_map[index] * WR_BANK_NUMBER + last_bank_used_on_bundles[bundle_map[index]] +first_bank;
memcpy(bank_pointers[current_bank_base_addr] + space_used_on_banks_by_bundles[current_bank_base_addr], bank_map[index].external_addr + memory_copied, WR_CHUNK_SIZE);
space_used_on_banks_by_bundles[current_bank_base_addr] += WR_CHUNK_SIZE;
last_bank_used_on_bundles[bundle_map[index]] = (last_bank_used_on_bundles[bundle_map[index]] + 1) % bundle_size;
memory_copied += WR_CHUNK_SIZE;
}
remaining_memory = remaining_memory % WR_CHUNK_SIZE;
}
if(remaining_memory > 0)
{
unsigned int  current_bank_base_addr = bundle_map[index] * WR_BANK_NUMBER + (last_bank_used_on_bundles[bundle_map[index]] % bundle_size) + first_bank;
memcpy(bank_pointers[current_bank_base_addr] + space_used_on_banks_by_bundles[current_bank_base_addr], bank_map[index].external_addr + memory_copied, remaining_memory);
space_used_on_banks_by_bundles[current_bank_base_addr] += remaining_memory;
last_chunk_space_used_on_bundles[bundle_map[index]] = remaining_memory;
}
return;
}
)");
   // mem_diff is used to avoid copyng more data than originally present on the parameter:
   // If param_size is not a multiple of align the orignal memory has to copy only the bytes that where originally
   // allocated
   indented_output_stream->Append(R"(  
static void copy_back_memory_from_banks(unsigned int memory_size, unsigned int index, unsigned int mem_diff, char** bank_pointers, unsigned int* last_bank_used_on_bundles, unsigned int * last_chunk_space_used_on_bundles, unsigned int* space_used_on_banks_by_bundles)
{
unsigned int* target_bundle = bi[bundle_map[index]].banks;
unsigned int bundle_size = bi[bundle_map[index]].size;
unsigned int first_bank = target_bundle[0];
unsigned int remaining_memory = memory_size;
unsigned int memory_copied = 0;
unsigned int bank_base_addr_position = bundle_map[index] * WR_BANK_NUMBER + last_bank_used_on_bundles[bundle_map[index]] + first_bank;
if(WR_CHUNK_SIZE - last_chunk_space_used_on_bundles[bundle_map[index]] >  memory_size)
{
last_chunk_space_used_on_bundles[bundle_map[index]] += memory_size;
memcpy(bank_map[index].external_addr, bank_pointers[bank_base_addr_position] + space_used_on_banks_by_bundles[bank_base_addr_position], (memory_size - mem_diff));  
info("Copied back %u bytes from bank pointer " PTR_FORMAT " to pointer parameter " PTR_FORMAT "\n", (memory_size - mem_diff), (ptr_t)bank_pointers[bank_base_addr_position], (ptr_t)bank_map[index].external_addr);
space_used_on_banks_by_bundles[bank_base_addr_position] += memory_size;
remaining_memory = 0;
}
else
{
unsigned int memory_to_copy = (WR_CHUNK_SIZE - last_chunk_space_used_on_bundles[bundle_map[index]]) < (memory_size - mem_diff) ? (WR_CHUNK_SIZE - last_chunk_space_used_on_bundles[bundle_map[index]]) : (memory_size - mem_diff);
memcpy(bank_map[index].external_addr, bank_pointers[bank_base_addr_position] + space_used_on_banks_by_bundles[bank_base_addr_position], memory_to_copy); 
info("Copied back %u bytes from bank pointer " PTR_FORMAT " to pointer parameter " PTR_FORMAT "\n", memory_to_copy, (ptr_t)bank_pointers[bank_base_addr_position] + space_used_on_banks_by_bundles[bank_base_addr_position], (ptr_t)bank_map[index].external_addr);
space_used_on_banks_by_bundles[bank_base_addr_position] += (WR_CHUNK_SIZE - last_chunk_space_used_on_bundles[bundle_map[index]]);
last_bank_used_on_bundles[bundle_map[index]] = (last_bank_used_on_bundles[bundle_map[index]] + 1) % bundle_size; 
memory_copied += (WR_CHUNK_SIZE - last_chunk_space_used_on_bundles[bundle_map[index]]);
remaining_memory = remaining_memory - (WR_CHUNK_SIZE - last_chunk_space_used_on_bundles[bundle_map[index]]);
last_chunk_space_used_on_bundles[bundle_map[index]] = 0;
}
if(remaining_memory >= WR_CHUNK_SIZE)
{
unsigned int memory_chunk_number = remaining_memory / WR_CHUNK_SIZE;
for(unsigned int i = 0; i < memory_chunk_number; i++)
{
unsigned int memory_to_copy = WR_CHUNK_SIZE > (memory_size - mem_diff - memory_copied) ? ((memory_size - mem_diff - memory_copied) > 0 ? (memory_size - mem_diff - memory_copied) : 0) : WR_CHUNK_SIZE;
unsigned int current_bank_base_addr = bundle_map[index] * WR_BANK_NUMBER + last_bank_used_on_bundles[bundle_map[index]] +first_bank;  
memcpy(bank_map[index].external_addr + memory_copied, bank_pointers[current_bank_base_addr] + space_used_on_banks_by_bundles[current_bank_base_addr],  memory_to_copy);
info("Copied back %u bytes from bank pointer " PTR_FORMAT " to pointer parameter " PTR_FORMAT "\n", memory_to_copy, (ptr_t)bank_pointers[current_bank_base_addr] + space_used_on_banks_by_bundles[current_bank_base_addr], (ptr_t)bank_map[index].external_addr + memory_copied);
space_used_on_banks_by_bundles[current_bank_base_addr] += WR_CHUNK_SIZE;
last_bank_used_on_bundles[bundle_map[index]] = (last_bank_used_on_bundles[bundle_map[index]] + 1) % bundle_size;
memory_copied += WR_CHUNK_SIZE;
}
remaining_memory = remaining_memory % WR_CHUNK_SIZE;
}
if(remaining_memory > 0)
{
unsigned int  current_bank_base_addr = bundle_map[index] * WR_BANK_NUMBER + (last_bank_used_on_bundles[bundle_map[index]] % bundle_size) +first_bank;  
unsigned int memory_to_copy = remaining_memory > (memory_size - mem_diff - memory_copied) ? ((memory_size - mem_diff - memory_copied) > 0 ? (memory_size - mem_diff - memory_copied) : 0) : remaining_memory;   
memcpy(bank_map[index].external_addr + memory_copied, bank_pointers[current_bank_base_addr] + space_used_on_banks_by_bundles[current_bank_base_addr], memory_to_copy);
info("Copied back %u bytes from bank pointer " PTR_FORMAT " to pointer parameter " PTR_FORMAT "\n", memory_to_copy, (ptr_t)bank_pointers[current_bank_base_addr] + space_used_on_banks_by_bundles[current_bank_base_addr], (ptr_t)bank_map[index].external_addr + memory_copied);
space_used_on_banks_by_bundles[current_bank_base_addr] += remaining_memory;
last_chunk_space_used_on_bundles[bundle_map[index]] = remaining_memory;
}
return;
}   
)");
   indented_output_stream->Append(R"(
static void get_bank_map_internal_addr(char* ext_addr, ptr_t* int_addr) 
{
for(int i = 0; i < WR_BANK_OBJ_NUMBER; i++)
{
if(bank_map[i].external_addr == ext_addr)
{
*int_addr = bank_map[i].internal_addr;
return;
}
}
error("Unable to find parameter allocated in bank_map\n");
return;
}

void __m_memsetup_banked(__m_argmap_t args[], size_t args_count, char** bank_pointers, char* banked_args_map, ptr_t base_addr)
{
int error = 0;
size_t i;
info("Creating banked memory\n");
)");

   indented_output_stream->Append("unsigned int internal_address_offset = base_addr - " + STR(HLSMgr->base_address) +
                                  ";\n");
   indented_output_stream->Append("static __m_memmap_t memmap_init[] = {\n");
   if(mem_vars.size())
   {
      std::vector<std::pair<unsigned int, memory_symbolRef>> mem_vars_ordered;
      for(auto& it : mem_vars)
      {
         mem_vars_ordered.push_back(it);
      }

      const auto cmp = [&](std::pair<unsigned int, memory_symbolRef>& a,
                           std::pair<unsigned int, memory_symbolRef>& b) -> bool {
         unsigned long long int var_addr_a, var_addr_b;
         const auto var_id_a = a.first;
         const auto is_top_param_a = std::find(parameters.begin(), parameters.end(), var_id_a) != parameters.end();
         if(!is_top_param_a)
         {
            var_addr_a = HLSMgr->Rmem->get_external_base_address(var_id_a);
         }
         else
         {
            var_addr_a = 0ULL;
         }
         const auto var_id_b = b.first;
         const auto is_top_param_b = std::find(parameters.begin(), parameters.end(), var_id_b) != parameters.end();
         if(!is_top_param_b)
         {
            var_addr_b = HLSMgr->Rmem->get_external_base_address(var_id_b);
         }
         else
         {
            var_addr_b = 0ULL;
         }
         return var_addr_a < var_addr_b;
      };

      sort(mem_vars_ordered.begin(), mem_vars_ordered.end(), cmp);
      const auto output_hls_directory =
          Param->getOption<std::filesystem::path>(OPT_output_hls_directory) / "simulation";
      auto base_addr = HLSMgr->base_address;
      for(const auto& mem_var : mem_vars_ordered)
      {
         const auto var_id = mem_var.first;
         const auto is_top_param = std::find(parameters.begin(), parameters.end(), var_id) != parameters.end();
         if(!is_top_param)
         {
            const auto var_name = BH->PrintVariable(var_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization for " + var_name);
            const auto var_addr = HLSMgr->Rmem->get_external_base_address(var_id);
            const auto var_init_dat = output_hls_directory / ("mem_" + STR(var_id) + "." + var_name + ".dat");
            const auto byte_count = TestbenchGeneration::generate_init_file(var_init_dat, TM, var_id, HLSMgr->Rmem);
            indented_output_stream->Append("  {\"" + boost::replace_all_copy(var_init_dat.string(), "\"", "\\\"") +
                                           "\", " + STR(byte_count) + ", " + STR(var_addr) +
                                           " + internal_address_offset, NULL},\n");
            base_addr = std::max(base_addr, var_addr + byte_count);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Init file   : '" + var_init_dat.string() + "'");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Memory usage: " + STR(byte_count) + " bytes");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Base address: " + STR(var_addr) + " + internal_address_offset");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
      }
   }
   indented_output_stream->Append("};\n");
   if(banked_args_decl_size == top_bh->GetParameters().size())
   {
      // __m_memsetup is not called so it must be initialized here
      indented_output_stream->Append("__m_memmap_init(MDPI_MEMMAP_MODE);\n");
   }
   indented_output_stream->Append(R"(
// Memory-mapped internal variables initialization
for(i = 0; i < sizeof(memmap_init) / sizeof(*memmap_init); ++i)
{
FILE* fp = fopen(memmap_init[i].filename, "rb");
if(!fp)
{
error("Unable to open file: %s\n", memmap_init[i].filename);
perror("Unable to open memory variable initialization file");
error |= 2;
continue;
}
unsigned int memory_offest = 0;
if(memmap_init[i].addr == NULL)
{
if(i == 0)
{
memmap_init[i].addr = malloc(memmap_init[i].addrmap - base_addr + memmap_init[i].size);
memory_offest = memmap_init[i].addrmap - base_addr;
}
else
{
memmap_init[i].addr = malloc(memmap_init[i].addrmap - memmap_init[i-1].addrmap + memmap_init[i].size);  
memory_offest = memmap_init[i].addrmap - memmap_init[i-1].addrmap; 
}
}
bank_map[i].external_addr = (char*)memmap_init[i].addr;
bank_map[i].internal_addr = memmap_init[i].addrmap;
bank_map[i].size = memmap_init[i].size;
size_t nbytes = fread((char*)memmap_init[i].addr + memory_offest, 1, memmap_init[i].size, fp);
info("Allocated memory variable %u at internal addresss " PTR_FORMAT " with size %u\n", i, memmap_init[i].addrmap, memmap_init[i].size);
if(nbytes != memmap_init[i].size)
{
error("Only %zu/%zu bytes were read from file: %s\n", nbytes, memmap_init[i].size, memmap_init[i].filename);
if(ferror(fp))
{
perror("Unable to read from memory variable initialization file");
}
error |= 4;
fclose(fp);
continue;
}
fclose(fp);
}
if(error)
{
   abort();
}
)");
   for(unsigned int i = 0; i < bundle_number; i++)
   {
      const auto res = HLSMgr->bundle_required_get_nth_element(i);
      if(!res.second)
      {
         THROW_ERROR("Requiring bundle " + STR(i) + " when there are only " + STR(bundle_number) + " bundles.");
      }
      else
      {
         const auto bundle_string = res.first;
         const auto splitted_list = SplitString(bundle_string, ",");
         std::vector<unsigned int> bank_identifier;
         std::transform(splitted_list.begin(), splitted_list.end(), std::back_inserter(bank_identifier),
                        [](const std::string& str) { return std::stoul(str); });
         const unsigned int size = static_cast<unsigned int>(bank_identifier.size());
         indented_output_stream->Append("bi[" + STR(i) + "].banks = (unsigned int*)malloc(sizeof(unsigned int)*(" +
                                        STR(size) + "));\n");
         indented_output_stream->Append("bi[" + STR(i) + "].size = " + STR(size) + ";\n");
         for(unsigned int j = 0; j < size; j++)
         {
            indented_output_stream->Append("bi[" + STR(i) + "].banks[" + STR(j) + "] = " + STR(bank_identifier[j]) +
                                           ";\n");
         }
      }
   }
   indented_output_stream->Append(
       "compute_space_required(base_addr, bank_pointers, args, args_count, banked_args_map);\n");
   InitilizedBMDataStructure(bundle_number, bank_number, true);
   indented_output_stream->Append(R"(for(i = 0; i <WR_BANK_OBJ_NUMBER; ++i)
{
size_t memory_size;
if(i == 0 && WR_BANK_MEM_VAR_NUMBER > 0)
{
memory_size = bank_map[i].internal_addr - base_addr + bank_map[i].size;
}
else if(i < WR_BANK_MEM_VAR_NUMBER)
{
memory_size = bank_map[i].internal_addr - bank_map[i-1].internal_addr + bank_map[i].size - bank_map[i-1].size;
}
else
{
memory_size = bank_map[i].size;
memory_size = memory_size + (args[i - WR_BANK_MEM_VAR_NUMBER].align - 1) - ((memory_size - 1) % args[i - WR_BANK_MEM_VAR_NUMBER].align);
}
allocate_on_bank_memory(memory_size, i, bank_pointers, last_bank_used_on_bundles, last_chunk_space_used_on_bundles, space_used_on_banks_by_bundles, banked_args_map, base_addr);
}
}
)");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written simulator banked memory");
}

void MdpiWrapperCWriter::WriteBankedMemoryWritebackValue(const unsigned int function_id)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing simulator write-back banked memory");
   const unsigned int bundle_number = static_cast<unsigned int>(HLSMgr->bundle_required.size());
   const auto fnode = TM->GetIRNode(function_id);
   const auto fname = ir_helper::GetFunctionName(fnode);
   const auto ifaces = HLSMgr->module_arch->GetArchitecture(fname)->ifaces.at("bus");
   const auto bank_number = static_cast<unsigned int>(std::stoul((ifaces.at(FunctionArchitecture::iface_bank_number))));
   indented_output_stream->Append(R"(
static void __membankwb(char** bank_pointers, __m_argmap_t args[], ptr_t base_addr)
{
)");
   InitilizedBMDataStructure(bundle_number, bank_number, true);
   indented_output_stream->Append("info(\"Writing back banked memory\\n\");\n");
   indented_output_stream->Append(R"(for(unsigned int i = 0; i <WR_BANK_OBJ_NUMBER; ++i)
{
size_t memory_size;
size_t aligned_memory;
if(i == 0 && WR_BANK_MEM_VAR_NUMBER > 0)
{
memory_size = bank_map[i].internal_addr - base_addr + bank_map[i].size;
aligned_memory = memory_size;
}
else if(i < WR_BANK_MEM_VAR_NUMBER)
{
memory_size = bank_map[i].internal_addr - bank_map[i-1].internal_addr + bank_map[i].size - bank_map[i-1].size;
aligned_memory = memory_size;
}
else
{
memory_size = bank_map[i].size;
aligned_memory = memory_size + (args[i - WR_BANK_MEM_VAR_NUMBER].align - 1) - ((memory_size - 1) % args[i - WR_BANK_MEM_VAR_NUMBER].align);
}
info("Coping back %u bytes for parameter %u\n", memory_size, i); 
copy_back_memory_from_banks(aligned_memory, i, aligned_memory - memory_size, bank_pointers, last_bank_used_on_bundles, last_chunk_space_used_on_bundles, space_used_on_banks_by_bundles);
}
for(unsigned int j = 0; j < WR_BUNDLE_NUMBER; j++)
{
for(unsigned int i = 0; i < WR_BANK_NUMBER; i++)
{
if(bank_pointers[j*WR_BANK_NUMBER + i])
{
free(bank_pointers[j*WR_BANK_NUMBER + i]);
info("Freed bank %u: for bundle %u\n", i, j);
}
}
}
return;
}
)");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written simulator write-back banked memory");
}
