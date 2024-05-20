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
 *              Copyright (C) 2024 Politecnico di Milano
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
 * @file mdpi_wrapper_c_writer.hpp
 * @brief
 *
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
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
#include "memory.hpp"
#include "memory_initialization_c_writer.hpp"
#include "testbench_generation.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"

#include <regex>
#include <string>
#include <vector>

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

#include <cstdio>
#include <cstdlib>

typedef bool _Bool;

#include <sys/types.h>

#ifdef __AC_NAMESPACE
using namespace __AC_NAMESPACE;
#endif

)");

   // get the root function to be tested by the testbench
   const auto top_symbols = Param->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = TM->GetFunction(top_symbols.front());
   const auto fd = GetPointerS<const function_decl>(top_fnode);
   const auto top_fname = tree_helper::GetMangledFunctionName(fd);
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

      const auto output_directory = Param->getOption<std::filesystem::path>(OPT_output_directory) / "simulation";
      for(const auto& inc : includes)
      {
         if(inc.is_absolute())
         {
            indented_output_stream->Append("#include \"" + inc.string() + "\"\n");
         }
         else
         {
            indented_output_stream->Append("#include \"" + inc.lexically_proximate(output_directory).string() + "\"\n");
         }
      }
      indented_output_stream->Append("#undef " + top_fname + "\n");
      indented_output_stream->Append("#undef main\n");
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
   const auto top_fname = top_bh->get_function_name();
   const auto top_fname_mngl = top_bh->GetMangledFunctionName();
   const auto interface_type = Param->getOption<HLSFlowStep_Type>(OPT_interface_type);
   const auto is_interface_inferred = interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION;
   const auto& func_arch = HLSMgr->module_arch->GetArchitecture(top_fname_mngl);
   THROW_ASSERT(func_arch, "Expected interface architecture for function " + top_fname_mngl);

   const auto return_type = tree_helper::GetFunctionReturnType(top_fnode);
   const auto top_params = top_bh->GetParameters();
   const auto args_decl_size = top_params.size() + (return_type != nullptr);
   const auto has_subnormals = Param->isOption(OPT_fp_subnormal) && Param->getOption<bool>(OPT_fp_subnormal);
   const auto cmp_type = [&](tree_nodeConstRef t, const std::string& tname) -> std::string {
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
         if(tree_helper::IsPointerType(t))
         {
            t = tree_helper::CGetPointedType(t);
         }
         while(tree_helper::IsArrayType(t))
         {
            t = tree_helper::CGetElements(t);
         }
         if(tree_helper::IsRealType(t))
         {
            return has_subnormals ? "flts" : "flt";
         }
         else if(tree_helper::IsStructType(t) || tree_helper::IsUnionType(t) || tree_helper::IsVoidType(t) ||
                 starts_with(tname, "void"))
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

   std::string top_decl;
   std::string gold_decl = "EXTERN_CDECL ";
   std::string pp_decl = "EXTERN_CDECL ";
   std::string gold_call;
   std::string pp_call;
   std::string args_init;
   std::string args_decl = "__m_argmap_t args[] = {\n";
   std::string args_set;
   std::string gold_cmp;
   size_t param_idx = 0;
   if(return_type)
   {
      const auto return_type_str = tree_helper::PrintType(TM, return_type);
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
      for(const auto& arg : top_params)
      {
         const auto parm_name = top_bh->PrintVariable(arg->index);
         THROW_ASSERT(func_arch->parms.find(parm_name) != func_arch->parms.end(),
                      "Attributes missing for parameter " + parm_name + " in function " + top_fname);
         const auto& parm_attrs = func_arch->parms.at(parm_name);
         const auto& iface_attrs = func_arch->ifaces.at(parm_attrs.at(FunctionArchitecture::parm_bundle));
         const auto arg_type = tree_helper::CGetType(arg);
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
         std::string iface_type, arg_size;

         const auto raw_typename = tree_helper::PrintType(TM, arg, false, true);
         auto arg_typename = parm_attrs.at(FunctionArchitecture::parm_original_typename);
         if(arg_typename.find("(*)") != std::string::npos)
         {
            arg_typename = arg_typename.substr(0, arg_typename.find("(*)")) + "*";
         }
         const auto arg_name = "P" + std::to_string(param_idx);
         const auto is_pointer_type = arg_typename.back() == '*';
         const auto is_reference_type = arg_typename.back() == '&';
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
               if(is_interface_inferred)
               {
                  THROW_ASSERT(parm_attrs.find(FunctionArchitecture::parm_size_in_bytes) != parm_attrs.end(),
                               "Attributes parm_size_in_bytes missing for parameter " + parm_name + " in function " +
                                   top_fname);
                  arg_size = parm_attrs.at(FunctionArchitecture::parm_size_in_bytes);
               }
               else
               {
                  const auto ptd_type = tree_helper::CGetPointedType(arg_type);
                  const auto array_size = [&]() {
                     return tree_helper::IsArrayType(ptd_type) ? tree_helper::GetArrayTotalSize(ptd_type) : 1ULL;
                  }();
                  if(tree_helper::IsVoidType(ptd_type))
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
         args_decl += "{" + arg_ptr + ", " + arg_align + ", m_map_" + iface_type + "(" + arg_ptr + ")},\n";
         args_set += "m_interface_" + iface_type + "(" + std::to_string(param_idx) + ", args[" +
                     std::to_string(param_idx) + "].map_addr, " + arg_bitsize + ", " + arg_align + ");\n";
         ++param_idx;
      }
      top_decl.erase(top_decl.size() - 2);
      gold_decl.erase(gold_decl.size() - 2);
      pp_decl.erase(pp_decl.size() - 2);
      gold_call.erase(gold_call.size() - 2);
      pp_call.erase(pp_call.size() - 2);
      if(!return_type)
      {
         args_decl.erase(args_decl.size() - 2);
      }
   }
   if(return_type)
   {
      args_init += "__m_param_alloc(" + std::to_string(param_idx) + ", sizeof(retval));\n";
      args_decl += "{&retval, 1, m_map_default(&retval)}";
      args_set += "m_interface_default(" + std::to_string(param_idx) + ", args[" + std::to_string(param_idx) +
                  "].map_addr, " + std::to_string(tree_helper::SizeAlloc(return_type)) + ", sizeof(retval));\n";
      ++param_idx;
   }
   args_set += "__m_interface_mem(" + std::to_string(param_idx) + ");\n";
   top_decl += ")\n";
   gold_decl += ");\n";
   pp_decl += ");\n";
   gold_call += ");\n";
   pp_call += ");\n";
   args_decl += "};\n";

   if(top_fname != "main")
   {
      indented_output_stream->AppendIndented("CDECL " + top_decl.substr(0, top_decl.size() - 1) + ";\n");
   }
   indented_output_stream->AppendIndented(R"(
#ifdef __cplusplus
#include <cstring>
#else
#include <string.h>
#endif

#define __LOCAL_ENTITY MDPI_ENTITY_DRIVER
#include <mdpi/mdpi_debug.h>
#include <mdpi/mdpi_driver.h>
#include <mdpi/mdpi_user.h>

#define typeof __typeof__
#ifdef __cplusplus
template <typename T> struct __m_type { typedef T type; };
template <typename T> struct __m_type<T*> { typedef typename __m_type<T>::type type; };
template <> struct __m_type<void*> { typedef typename __m_type<unsigned char>::type type; };
#define m_getptrt(val) __m_type<typeof(val)>::type*
#define m_getvalt(val) __m_type<typeof(val)>::type
template <typename T> T* m_getptr(T& obj) { return &obj; }
template <typename T> T* m_getptr(T* obj) { return obj; }
#define __m_float_distance(a, b) m_float_distance(a, b)
#else
#define m_getptrt(val) typeof(val)
#define m_getvalt(val) typeof(*val)
#define m_getptr(ptr) (ptr)
#define __m_float_distance(a, b) \
   ((typeof(a)(*)(typeof(a), typeof(a)))((sizeof(a) == sizeof(float)) ? m_float_distancef : m_float_distance))(a, b)
#define __m_floats_distance(a, b) \
   ((typeof(a)(*)(typeof(a), typeof(a)))((sizeof(a) == sizeof(float)) ? m_floats_distancef : m_floats_distance))(a, b)
#endif

#define m_cmpval(ptra, ptrb) *(ptra) != *(ptrb)
#define m_cmpmem(ptra, ptrb) memcmp(ptra, ptrb, sizeof(m_getvalt(ptrb)))
#define m_cmpflt(ptra, ptrb) __m_float_distance(*(ptra), *(ptrb)) > max_ulp
#define m_cmpflts(ptra, ptrb) __m_floats_distance(*(ptra), *(ptrb)) > max_ulp

#define _ms_setargptr(suffix, idx, ptr)                       \
   const size_t P##idx##_size_##suffix = __m_param_size(idx); \
   void* P##idx##_##suffix = malloc(P##idx##_size_##suffix);  \
   memcpy(P##idx##_##suffix, ptr, P##idx##_size_##suffix)

#define _ms_argcmp(suffix, idx, cmp)                                                                          \
   const size_t P##idx##_count_##suffix = P##idx##_size_##suffix / sizeof(m_getvalt(P##idx));                 \
   for(i = 0; i < P##idx##_count_##suffix; ++i)                                                               \
   {                                                                                                          \
      if(m_cmp##cmp((m_getptrt(P##idx))P##idx##_##suffix + i, (m_getptrt(P##idx))m_getptr(P##idx) + i))       \
      {                                                                                                       \
         error("Memory parameter %u (%zu/%zu) mismatch with respect to " #suffix " reference.\n", idx, i + 1, \
               P##idx##_count_##suffix);                                                                      \
         ++mismatch_count;                                                                                    \
      }                                                                                                       \
   }                                                                                                          \
   free(P##idx##_##suffix)

#define _ms_setargchannel(suffix, idx) m_getvalt(P##idx) P##idx##_##suffix = *m_getptr(P##idx)

#define _ms_channelcmp(suffix, idx, cmp)                                                                          \
   if(m_getptr(P##idx)->size() != m_getptr(P##idx##_##suffix)->size())                                            \
   {                                                                                                              \
      error("Channel parameter %u size mismatch with respect to " #suffix " reference: %zu != %zu.\n", idx,       \
            m_getptr(P##idx)->size(), m_getptr(P##idx##_##suffix)->size());                                       \
      ++mismatch_count;                                                                                           \
   }                                                                                                              \
   else                                                                                                           \
   {                                                                                                              \
      for(i = 0; i < m_getptr(P##idx)->size(); ++i)                                                               \
      {                                                                                                           \
         if(m_cmp##cmp(&m_getptr(P##idx)->operator[](i), &m_getptr(P##idx##_##suffix)->operator[](i)))            \
         {                                                                                                        \
            error("Channel parameter %u (%zu/%zu) mismatch with respect to " #suffix " reference.\n", idx, i + 1, \
                  m_getptr(P##idx)->size());                                                                      \
            ++mismatch_count;                                                                                     \
         }                                                                                                        \
      }                                                                                                           \
   }

#define _ms_retvalcmp(suffix, cmp)                                             \
   if(m_cmp##cmp(&retval, &retval_##suffix))                                   \
   {                                                                           \
      error("Return value mismatch with respect to " #suffix " reference.\n"); \
      ++mismatch_count;                                                        \
   }

#ifndef CUSTOM_VERIFICATION
#define _m_setargptr(idx, ptr) _ms_setargptr(gold, idx, ptr)
#define _m_argcmp(idx, cmp) _ms_argcmp(gold, idx, cmp)
#define _m_setargchannel(idx) _ms_setargchannel(gold, idx)
#define _m_channelcmp(idx, cmp) _ms_channelcmp(gold, idx, cmp)
#define _m_retvalcmp(cmp) _ms_retvalcmp(gold, cmp)

)" + gold_decl +
                                          R"(#else
#define _m_setargptr(...)
#define _m_argcmp(...)
#define _m_setargchannel(...)
#define _m_channelcmp(...)
#define _m_retvalcmp(...)
#endif

#ifdef PP_VERIFICATION
#define _m_pp_setargptr(idx, ptr) _ms_setargptr(pp, idx, ptr)
#define _m_pp_argcmp(idx, cmp) _ms_argcmp(pp, idx, cmp)
#define _m_pp_retvalcmp(cmp) _ms_retvalcmp(pp, cmp)

)" + pp_decl +
                                          R"(#else
#define _m_pp_setargptr(...)
#define _m_pp_argcmp(...)
#define _m_pp_retvalcmp(...)
#endif

#ifdef DUMP_COSIM_OUTPUT
static size_t __m_call_count = 0;

#ifndef CUSTOM_VERIFICATION
#define _m_golddump(idx)                                                                                            \
   do                                                                                                               \
   {                                                                                                                \
      char filename[32];                                                                                            \
      sprintf(filename, "P" #idx "_gold.%zu.dat", __m_call_count);                                                  \
      FILE* out = fopen(filename, "wb");                                                                            \
      if(out != NULL)                                                                                               \
      {                                                                                                             \
         fwrite(P##idx##_gold, 1, __m_param_size(idx), out);                                                        \
         fclose(out);                                                                                               \
         debug("Parameter " #idx " gold output dump for execution %zu stored in '%s'\n", __m_call_count, filename); \
      }                                                                                                             \
      else                                                                                                          \
      {                                                                                                             \
         error("Unable to open parameter dump file '%s'\n", filename);                                              \
      }                                                                                                             \
   } while(0)
#else
#define _m_golddump(idx)
#endif

#define _m_argdump(idx)                                                                                        \
   do                                                                                                          \
   {                                                                                                           \
      char filename[32];                                                                                       \
      sprintf(filename, "P" #idx ".%zu.dat", __m_call_count);                                                  \
      FILE* out = fopen(filename, "wb");                                                                       \
      if(out != NULL)                                                                                          \
      {                                                                                                        \
         fwrite(P##idx, 1, __m_param_size(idx), out);                                                          \
         fclose(out);                                                                                          \
         debug("Parameter " #idx " output dump for execution %zu stored in '%s'\n", __m_call_count, filename); \
      }                                                                                                        \
      else                                                                                                     \
      {                                                                                                        \
         error("Unable to open parameter dump file '%s'\n", filename);                                         \
      }                                                                                                        \
   } while(0)
#else
#define _m_argdump(idx)
#define _m_golddump(idx)
#endif

#define m_map_default(ptr) NULL
#define m_interface_default(idx, ptr, bitsize, align) \
   __m_interface_port(idx, ptr, bitsize);             \
   _m_pp_setargptr(idx, ptr);                         \
   _m_setargptr(idx, ptr)

#define m_map_ptr(ptr) (void*)ptr
#define m_interface_ptr(idx, ptr, bitsize, align)               \
   bptr_t __ptrval_##idx = (bptr_t)ptr;                         \
   __m_interface_ptr(idx, &__ptrval_##idx, sizeof(bptr_t) * 8); \
   _m_pp_setargptr(idx, ptr);                                   \
   _m_setargptr(idx, ptr)

#define m_map_array(...) m_map_default(__VA_ARGS__)
#define m_interface_array(idx, ptr, bitsize, align)                            \
   __m_interface_array(idx, ptr, bitsize, align, __m_param_size(idx) / align); \
   _m_pp_setargptr(idx, ptr);                                                  \
   _m_setargptr(idx, ptr)

#define m_map_fifo(...) m_map_default(__VA_ARGS__)
#define m_interface_fifo(idx, ptr, bitsize, align)                            \
   __m_interface_fifo(idx, ptr, bitsize, align, __m_param_size(idx) / align); \
   _m_pp_setargptr(idx, ptr);                                                 \
   _m_setargptr(idx, ptr)

#define m_map_channel(ptr) NULL
#define m_interface_channel(idx, ptr, bitsize, align)                  \
   __m_interface_channel(idx, *m_getptr(P##idx), __m_param_size(idx)); \
   _m_setargchannel(idx)

#define m_map_none(...) m_map_default(__VA_ARGS__)
#define m_interface_none(...) m_interface_default(__VA_ARGS__)

#define m_map_valid(...) m_map_default(__VA_ARGS__)
#define m_interface_valid(...) m_interface_default(__VA_ARGS__)

#define m_map_ovalid(...) m_map_default(__VA_ARGS__)
#define m_interface_ovalid(...) m_interface_default(__VA_ARGS__)

#define m_map_acknowledge(...) m_map_default(__VA_ARGS__)
#define m_interface_acknowledge(...) m_interface_default(__VA_ARGS__)

#define m_map_handshake(...) m_map_default(__VA_ARGS__)
#define m_interface_handshake(...) m_interface_default(__VA_ARGS__)

#define m_map_axis(...) m_map_fifo(__VA_ARGS__)
#define m_interface_axis(...) m_interface_fifo(__VA_ARGS__)

#define m_map_m_axi(...) m_map_ptr(__VA_ARGS__)
#define m_interface_m_axi(...) m_interface_ptr(__VA_ARGS__)

#define m_argcmp(idx, cmp) \
   _m_argdump(idx);        \
   _m_golddump(idx);       \
   _m_pp_argcmp(idx, cmp); \
   _m_argcmp(idx, cmp)

#define m_channelcmp(idx, cmp) _m_channelcmp(idx, cmp)

#define m_retvalcmp(cmp) _m_pp_retvalcmp(cmp) _m_retvalcmp(cmp)
)");

   // write C code used to print initialization values for the HDL simulator's memory
   WriteSimulatorInitMemory(top_fnode->index);

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
   indented_output_stream->Append(args_init);
   indented_output_stream->Append("__m_memsetup(args, " + STR(args_decl_size) + ");\n\n");

   indented_output_stream->Append(args_set);

   indented_output_stream->Append("\n__m_sim_start();\n\n");
   indented_output_stream->Append("#ifndef CUSTOM_VERIFICATION\n");
   indented_output_stream->Append(gold_call);
   indented_output_stream->Append("#endif\n\n");
   indented_output_stream->Append("#ifdef PP_VERIFICATION\n");
   indented_output_stream->Append(pp_call);
   indented_output_stream->Append("#endif\n\n");
   indented_output_stream->Append("__m_sim_end();\n");
   indented_output_stream->Append("__m_interface_fini();\n\n");

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

#ifdef DUMP_COSIM_OUTPUT
++__m_call_count;
#endif

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

void MdpiWrapperCWriter::WriteSimulatorInitMemory(const unsigned int function_id)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing simulator init memory");
   THROW_ASSERT(HLSMgr->Rmem, "Expected memory allocation to be already computed at this point.");
   const auto mem_vars = HLSMgr->Rmem->get_ext_memory_variables();
   const auto BH = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto parameters = BH->get_parameters();

   const auto align_bus = std::max(8ULL, HLSMgr->Rmem->get_bus_data_bitsize() / 8ULL);
   const auto align_infer = [&]() {
      const auto funcSymbol = BH->GetMangledFunctionName();
      const auto& ifaces = HLSMgr->module_arch->GetArchitecture(funcSymbol)->ifaces;
      unsigned long long max = 0ULL;
      for(const auto& [name, attrs] : ifaces)
      {
         max = std::max(max, std::stoull(attrs.at(FunctionArchitecture::iface_alignment)));
      }
      return max;
   }();
   const auto align = std::max(align_bus, align_infer);
   const auto tb_map_mode = "MDPI_MEMMAP_" + Param->getOption<std::string>(OPT_testbench_map_mode);

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

static void __m_memsetup(__m_argmap_t args[], size_t args_count)
{
int error = 0;
size_t i;
)");
   auto base_addr = HLSMgr->base_address;
   indented_output_stream->Append("static __m_memmap_t memmap_init[] = {\n");
   if(mem_vars.size())
   {
      const auto output_directory = Param->getOption<std::filesystem::path>(OPT_output_directory) / "simulation";
      for(const auto& mem_var : mem_vars)
      {
         const auto var_id = mem_var.first;
         const auto is_top_param = std::find(parameters.begin(), parameters.end(), var_id) != parameters.end();
         if(!is_top_param)
         {
            const auto var_name = BH->PrintVariable(var_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization for " + var_name);
            const auto var_addr = HLSMgr->Rmem->get_external_base_address(var_id);
            const auto var_init_dat = output_directory / ("mem_" + STR(var_id) + "." + var_name + ".dat");
            const auto byte_count = TestbenchGeneration::generate_init_file(var_init_dat, TM, var_id, HLSMgr->Rmem);
            indented_output_stream->Append("  {\"" + boost::replace_all_copy(var_init_dat.string(), "\"", "\\\"") +
                                           "\", " + STR(byte_count) + ", " + STR(var_addr) + ", NULL},\n");
            base_addr = std::max(base_addr, var_addr + byte_count);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Init file   : '" + var_init_dat.string() + "'");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Memory usage: " + STR(byte_count) + " bytes");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Base address: " + STR(var_addr));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
      }
   }
   indented_output_stream->Append("};\n");
   indented_output_stream->Append("const ptr_t align = " + STR(align) + ";\n");
   indented_output_stream->Append("ptr_t base_addr = " + STR(base_addr) + ";\n\n");

   indented_output_stream->Append("__m_memmap_init(" + tb_map_mode + ");\n");
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

for(i = 0; i < args_count; ++i)
{
if(args[i].map_addr == NULL)
{
args[i].map_addr = args[i].addr;
continue;
}
const size_t arg_size = __m_param_size(i);
size_t map_size = arg_size;
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
