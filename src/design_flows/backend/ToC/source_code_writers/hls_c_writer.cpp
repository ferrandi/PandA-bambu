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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file hls_c_writer.cpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Pietro Fezzardi <pietro.fezzardi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "hls_c_writer.hpp"

#include "Parameter.hpp"
#include "SimulationInformation.hpp"
#include "behavioral_helper.hpp"
#include "c_initialization_parser.hpp"
#include "call_graph_manager.hpp"
#include "constants.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_NONE
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "indented_output_stream.hpp"
#include "instruction_writer.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "memory_initialization_c_writer.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "structural_objects.hpp"
#include "technology_node.hpp"
#include "testbench_generation.hpp"
#include "testbench_generation_constants.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"
#include "var_pp_functor.hpp"

#include <boost/algorithm/string/trim_all.hpp>
#include <regex>

#include <list>
#include <string>
#include <vector>

REF_FORWARD_DECL(memory_symbol);

static const std::regex wrapper_def("(ac_channel|stream|hls::stream)<(.*)>");
#define WRAPPER_GROUP_WTYPE 2

HLSCWriter::HLSCWriter(const CBackendInformationConstRef _c_backend_info, const HLS_managerConstRef _HLSMgr,
                       const InstructionWriterRef _instruction_writer,
                       const IndentedOutputStreamRef _indented_output_stream, const ParameterConstRef _parameters,
                       bool _verbose)
    : CWriter(_HLSMgr, _instruction_writer, _indented_output_stream, _parameters, _verbose),
      c_backend_info(_c_backend_info)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

HLSCWriter::~HLSCWriter() = default;

void HLSCWriter::WriteHeader()
{
   indented_output_stream->Append(R"(
#define _FILE_OFFSET_BITS 64

#define __Inf (1.0 / 0.0)
#define __Nan (0.0 / 0.0)

#ifdef __cplusplus
#undef printf

#include <cstdio>
#include <cstdlib>

typedef bool _Bool;
#else
#include <stdio.h>
#include <stdlib.h>

extern void exit(int status);
#endif

#include <sys/types.h>

#ifdef __AC_NAMESPACE
using namespace __AC_NAMESPACE;
#endif

#ifdef __clang__
#define GCC_VERSION 0
#else
#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)
#endif

)");

   // get the root function to be tested by the testbench
   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   CustomOrderedSet<std::string> includes;
   CustomSet<std::string> top_fnames;
   for(auto function_id : top_function_ids)
   {
      const auto fnode = TM->CGetTreeNode(function_id);
      const auto fd = GetPointerS<const function_decl>(fnode);
      const auto fname = tree_helper::GetMangledFunctionName(fd);
      auto& DesignInterfaceInclude = HLSMgr->design_interface_typenameinclude;
      if(DesignInterfaceInclude.find(fname) != DesignInterfaceInclude.end())
      {
         const auto& DesignInterfaceArgsInclude = DesignInterfaceInclude.find(fname)->second;
         for(const auto& argInclude : DesignInterfaceArgsInclude)
         {
            const auto incls = convert_string_to_vector<std::string>(argInclude.second, ";");
            includes.insert(incls.begin(), incls.end());
         }
         top_fnames.insert(fname);
      }
   }
   if(includes.size())
   {
      for(const auto& fname : top_fnames)
      {
         indented_output_stream->Append("#define " + fname + " __keep_your_declaration_out_of_my_code\n");
      }
      for(const auto& inc : includes)
      {
         indented_output_stream->Append("#include \"" + inc + "\"\n");
      }
      for(const auto& fname : top_fnames)
      {
         indented_output_stream->Append("#undef " + fname + "\n");
      }
   }
}

void HLSCWriter::WriteGlobalDeclarations()
{
   instrWriter->write_declarations();
}

void HLSCWriter::WriteParamDecl(const BehavioralHelperConstRef BH)
{
   indented_output_stream->Append("// parameters declaration\n");

   for(const auto& par : BH->GetParameters())
   {
      const auto parm_type = tree_helper::CGetType(par);
      const auto type = tree_helper::IsPointerType(par) ? "void*" : tree_helper::PrintType(TM, parm_type);
      const auto param = BH->PrintVariable(par->index);

      if(tree_helper::IsVectorType(parm_type))
      {
         THROW_ERROR("parameter " + param + " of function under test " + BH->get_function_name() + " has type " + type +
                     "\nco-simulation does not support vectorized parameters at top level");
      }
      indented_output_stream->Append(type + " " + param + ";\n");
   }
}

void HLSCWriter::WriteParamInitialization(const BehavioralHelperConstRef BH,
                                          const std::map<std::string, std::string>& curr_test_vector)
{
   const auto fnode = TM->CGetTreeReindex(BH->get_function_index());
   const auto fname = tree_helper::GetMangledFunctionName(GetPointerS<const function_decl>(GET_CONST_NODE(fnode)));
   const auto& DesignAttributes = HLSMgr->design_attributes;
   const auto function_if = [&]() -> const std::map<std::string, std::map<interface_attributes, std::string>>* {
      const auto it = DesignAttributes.find(fname);
      if(it != DesignAttributes.end())
      {
         return &it->second;
      }
      return nullptr;
   }();
   const auto arg_signature_typename = HLSMgr->design_interface_typename_orig_signature.find(fname);
   const auto params = BH->GetParameters();
   for(auto par_idx = 0U; par_idx < params.size(); ++par_idx)
   {
      const auto& par = params.at(par_idx);
      const auto parm_type = tree_helper::CGetType(par);
      const auto param = BH->PrintVariable(GET_INDEX_CONST_NODE(par));
      const auto has_file_init = [&]() {
         if(function_if)
         {
            const auto& type_name = function_if->at(param).at(attr_typename);
            return std::regex_search(type_name.c_str(), wrapper_def);
         }
         return false;
      }();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization of " + param);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Type: " + GET_CONST_NODE(parm_type)->get_kind_text() + " - " + STR(parm_type));
      const auto init_it = curr_test_vector.find(param);
      if(init_it == curr_test_vector.end())
      {
         THROW_ERROR("Value of " + param + " is missing in test vector");
      }
      const auto test_v =
          (has_file_init && ends_with(init_it->second, ".dat")) ? ("\"" + init_it->second + "\"") : init_it->second;
      if(tree_helper::IsPointerType(parm_type))
      {
         const auto is_binary_init = ends_with(test_v, ".dat");

         std::string var_ptdtype;
         std::string temp_var_decl;
         bool is_a_true_pointer = true;
         if(!is_binary_init && arg_signature_typename != HLSMgr->design_interface_typename_orig_signature.end())
         {
            var_ptdtype = arg_signature_typename->second.at(par_idx);
            static const std::regex voidP = std::regex(R"(\bvoid\b)");
            while(std::regex_search(var_ptdtype, voidP))
            {
               var_ptdtype = std::regex_replace(var_ptdtype, voidP, "char");
            }
            is_a_true_pointer = var_ptdtype.back() == '*';
            if(is_a_true_pointer || var_ptdtype.back() == '&')
            {
               var_ptdtype.pop_back();
            }
            if(var_ptdtype.find("(*)") != std::string::npos)
            {
               temp_var_decl = var_ptdtype;
               temp_var_decl.replace(var_ptdtype.find("(*)"), 3, param + "_temp" + "[]");
            }
            else
            {
               temp_var_decl = var_ptdtype + " " + param + "_temp" + (is_a_true_pointer ? "[]" : "");
            }
         }
         if(temp_var_decl == "")
         {
            const auto var_functor = var_pp_functorRef(new std_var_pp_functor(BH));
            const auto ptd = tree_helper::CGetPointedType(parm_type);
            temp_var_decl = tree_helper::PrintType(TM, ptd, false, false, false, par, var_functor);
            var_ptdtype = temp_var_decl.substr(0, temp_var_decl.find_last_of((*var_functor)(par->index)));
            if(tree_helper::IsVoidType(ptd))
            {
               boost::replace_all(temp_var_decl, "void ", "char ");
               boost::replace_all(var_ptdtype, "void ", "char ");
            }
            const auto first_square = temp_var_decl.find('[');
            if(first_square == std::string::npos)
            {
               temp_var_decl = temp_var_decl + "_temp[]";
            }
            else
            {
               temp_var_decl.insert(first_square, "_temp[]");
            }
         }
         THROW_ASSERT(temp_var_decl.size() && var_ptdtype.size(),
                      "var_decl: " + temp_var_decl + ", ptd_type: " + var_ptdtype);
         const auto arg_channel = std::regex_search(var_ptdtype, std::regex("(ac_channel|stream|hls::stream)<(.*)>"));
         const auto ptd_type = tree_helper::GetRealType(tree_helper::CGetPointedType(parm_type));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Pointed type: " + GET_CONST_NODE(ptd_type)->get_kind_text() + " - " + STR(ptd_type));

         std::string param_size;
         if(is_binary_init)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Initialized from binary file: " + test_v);
            const auto fp = param + "_fp";
            indented_output_stream->Append("FILE* " + fp + " = fopen(\"" + test_v + "\", \"rb\");\n");
            indented_output_stream->Append("fseek(" + fp + ", 0, SEEK_END);\n");
            indented_output_stream->Append("size_t " + param + "_size = ftell(" + fp + ");\n");
            indented_output_stream->Append("fseek(" + fp + ", 0, SEEK_SET);\n");
            indented_output_stream->Append("void* " + param + "_buf = malloc(" + param + "_size);\n");
            indented_output_stream->Append("if(fread((unsigned char*)" + param + "_buf, 1, " + param + "_size, " + fp +
                                           ") != " + param + "_size)\n");
            indented_output_stream->Append("{\n");
            indented_output_stream->Append("fclose(" + fp + ");\n");
            indented_output_stream->Append("printf(\"Unable to read " + test_v + " to initialize parameter " + param +
                                           "\");\n");
            indented_output_stream->Append("exit(-1);\n");
            indented_output_stream->Append("}\n");
            indented_output_stream->Append("fclose(" + fp + ");\n");
            indented_output_stream->Append(param + " = " + param + "_buf;\n");
            indented_output_stream->Append("m_param_alloc(" + STR(par_idx) + ", " + param + "_size);\n");
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inline pointer initialization");
            const auto temp_initialization =
                temp_var_decl + " = " +
                ((test_v.front() != '{' && test_v.back() != '}' && is_a_true_pointer) ? "{" + test_v + "}" : test_v) +
                ";\n";
            indented_output_stream->Append(temp_initialization);
            indented_output_stream->Append(param + " = (void*)" + (is_a_true_pointer ? "" : "&") + param + "_temp;\n");
            if(!arg_channel)
            {
               indented_output_stream->Append("m_param_alloc(" + STR(par_idx) + ", sizeof(" + param + "_temp));\n");
            }
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inline initialization");
         if(tree_helper::IsRealType(parm_type) && test_v == "-0")
         {
            indented_output_stream->Append(param + " = -0.0;\n");
         }
         else
         {
            indented_output_stream->Append(param + " = " + test_v + ";\n");
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written initialization of " + param);
   }
}

void HLSCWriter::WriteTestbenchFunctionCall(const BehavioralHelperConstRef BH)
{
   const auto function_index = BH->get_function_index();
   const auto return_type_index = BH->GetFunctionReturnType(function_index);

   auto function_name = BH->get_function_name();
   // avoid collision with the main
   if(function_name == "main")
   {
      const auto is_discrepancy = (Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy)) ||
                                  (Param->isOption(OPT_discrepancy_hw) && Param->getOption<bool>(OPT_discrepancy_hw));
      if(is_discrepancy)
      {
         function_name = "_main";
      }
   }
   if(return_type_index)
   {
      indented_output_stream->Append(RETURN_PORT_NAME " = ");
   }

   indented_output_stream->Append(function_name + "(");
   bool is_first_argument = true;
   unsigned par_index = 0;
   const auto top_fname_mngl = BH->GetMangledFunctionName();
   const auto& DesignInterfaceTypenameOrig = HLSMgr->design_interface_typename_orig_signature;
   for(const auto& par : BH->GetParameters())
   {
      if(!is_first_argument)
      {
         indented_output_stream->Append(", ");
      }
      else
      {
         is_first_argument = false;
      }
      if(tree_helper::IsPointerType(par))
      {
         if(DesignInterfaceTypenameOrig.find(top_fname_mngl) != DesignInterfaceTypenameOrig.end())
         {
            auto arg_typename = DesignInterfaceTypenameOrig.find(top_fname_mngl)->second.at(par_index);
            if(arg_typename.find("(*)") != std::string::npos)
            {
               arg_typename = arg_typename.substr(0, arg_typename.find("(*)")) + "*";
            }
            if(arg_typename.back() != '*')
            {
               indented_output_stream->Append("*(");
               indented_output_stream->Append(
                   arg_typename.substr(0, arg_typename.size() - (arg_typename.back() == '&')) + "*");
               indented_output_stream->Append(") ");
            }
            else
            {
               indented_output_stream->Append("(");
               indented_output_stream->Append(arg_typename);
               indented_output_stream->Append(") ");
            }
         }
         else
         {
            const auto parm_type = tree_helper::CGetType(par);
            const auto type_name = tree_helper::PrintType(TM, parm_type);
            indented_output_stream->Append("(" + type_name + ")");
         }
      }
      const auto param = BH->PrintVariable(GET_INDEX_CONST_NODE(par));
      indented_output_stream->Append(param);
      ++par_index;
   }
   indented_output_stream->Append(");\n");
}

void HLSCWriter::WriteSimulatorInitMemory(const unsigned int function_id)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing simulator init memory");
   const auto mem_vars = HLSMgr->Rmem->get_ext_memory_variables();
   const auto BH = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto parameters = BH->get_parameters();
   const auto align = std::max(8ULL, HLSMgr->Rmem->get_bus_data_bitsize() / 8ULL);
   indented_output_stream->Append(R"(
typedef struct
{
const char* filename;
size_t size;
const ptr_t addrmap;
void* addr;
} __m_memmap_t;

static int cmpptr(const ptr_t a, const ptr_t b) { return a < b ? -1 : (a > b); }
static int cmpaddr(const void* a, const void* b) { return cmpptr((ptr_t)((__m_memmap_t*)a)->addr, (ptr_t)((__m_memmap_t*)b)->addr); }

static void __m_memsetup(void* args[], size_t args_count)
{
int error = 0;
size_t i;
)");
   auto base_addr = HLSMgr->base_address;
   indented_output_stream->Append("static __m_memmap_t memmap_init[] = {\n");
   if(mem_vars.size())
   {
      const auto output_directory = Param->getOption<std::string>(OPT_output_directory) + "/simulation/";
      for(const auto& mem_var : mem_vars)
      {
         const auto var_id = mem_var.first;
         const auto is_top_param = std::find(parameters.begin(), parameters.end(), var_id) != parameters.end();
         if(!is_top_param)
         {
            const auto var_name = BH->PrintVariable(var_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization for " + var_name);
            const auto var_addr = HLSMgr->Rmem->get_external_base_address(var_id);
            const auto var_init_dat = output_directory + "mem_" + STR(var_id) + "." + var_name + ".dat";
            const auto byte_count = TestbenchGeneration::generate_init_file(var_init_dat, TM, var_id, HLSMgr->Rmem);
            indented_output_stream->Append("  {\"" + boost::replace_all_copy(var_init_dat, "\"", "\\\"") + "\", " +
                                           STR(byte_count) + ", " + STR(var_addr) + ", NULL},\n");
            base_addr = std::max(base_addr, var_addr + byte_count);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Init file   : '" + var_init_dat + "'");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Memory usage: " + STR(byte_count) + " bytes");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Base address: " + STR(var_addr));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
      }
   }
   indented_output_stream->Append("};\n");
   indented_output_stream->Append("const ptr_t align = " + STR(align) + ";\n");
   indented_output_stream->Append("ptr_t base_addr = " + STR(base_addr) + ";\n");

   indented_output_stream->Append(R"(
__m_memmap_init();

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
const size_t size = __m_param_size(i);
base_addr += (align - 1) - ((base_addr - 1) % align);
error |= __m_memmap(base_addr, args[i], size);
base_addr += size;
}
if(error)
{
abort();
}
}

)");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written simulator init memory");
}

void HLSCWriter::WriteExtraInitCode()
{
}

void HLSCWriter::WriteExtraCodeBeforeEveryMainCall()
{
}

void HLSCWriter::WriteMainTestbench()
{
   THROW_ASSERT(HLSMgr->CGetCallGraphManager()->GetRootFunctions().size() == 1, "Multiple top functions not supported");
   const auto top_id = *HLSMgr->CGetCallGraphManager()->GetRootFunctions().begin();
   const auto top_fb = HLSMgr->CGetFunctionBehavior(top_id);
   const auto top_bh = top_fb->CGetBehavioralHelper();
   const auto top_fnode = TM->CGetTreeReindex(top_id);
   const auto top_fname = top_bh->get_function_name();
   const auto top_fname_mngl = top_bh->GetMangledFunctionName();
   const auto interface_type = Param->getOption<HLSFlowStep_Type>(OPT_interface_type);
   const auto is_interface_inferred = interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION;
   const auto arg_signature_typename = HLSMgr->design_interface_typename_orig_signature.find(top_fname_mngl);
   const auto arg_attributes = HLSMgr->design_attributes.find(top_fname_mngl);
   THROW_ASSERT(!is_interface_inferred ||
                    arg_signature_typename != HLSMgr->design_interface_typename_orig_signature.end(),
                "Original signature not found for function: " + top_fname_mngl + " (" + top_fname + ")");
   THROW_ASSERT(!is_interface_inferred || arg_attributes != HLSMgr->design_attributes.end(),
                "Design attributes not found for function: " + top_fname_mngl + " (" + top_fname + ")");

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
            const auto param_name = top_bh->PrintVariable(GET_INDEX_CONST_NODE(param));
            std::cmatch what;
            if(std::regex_search(param_size_str.c_str(), what, std::regex("\\b" + param_name + ":(\\d+)")))
            {
               idx_size[param_idx] = what[1u].str();
            }
            ++param_idx;
         }
      }
      return idx_size;
   }();
   const auto extern_decl = top_fname == top_fname_mngl ? "EXTERN_C " : "";

   std::string top_decl = extern_decl;
   std::string gold_decl = extern_decl;
   std::string pp_decl = extern_decl +
                         tree_helper::PrintType(TM, TM->CGetTreeReindex(top_id), false, true, false, nullptr,
                                                var_pp_functorConstRef(new std_var_pp_functor(top_bh))) +
                         ";\n";
   THROW_ASSERT(pp_decl.find(top_fname) != std::string::npos, "");
   boost::replace_first(pp_decl, top_fname, "__m_pp_" + top_fname);
   std::string gold_call;
   std::string pp_call;
   std::string args_init;
   std::string args_decl = "void* args[] = {";
   std::string args_set;
   std::string gold_cmp;
   size_t param_idx = 0;
   if(return_type)
   {
      const auto return_type_str = tree_helper::PrintType(TM, return_type);
      top_decl += return_type_str;
      gold_decl += return_type_str;
      gold_call += "retval_gold = ";
      pp_call += "retval_pp = ";
      args_init = "__m_param_alloc(" + STR(top_params.size()) + ", sizeof(" + return_type_str + "));\n";
      args_decl = return_type_str + " retval, retval_gold, retval_pp;\n" + args_decl;
      args_set += "__m_setarg(" + STR(top_params.size()) + ", args[" + STR(top_params.size()) + "], " +
                  STR(tree_helper::Size(return_type)) + ");\n";
   }
   else
   {
      top_decl += "void";
      gold_decl += "void";
   }
   top_decl += " " + top_fname + "(";
   gold_decl += " __m_" + top_fname + "(";
   gold_call += "__m_" + top_fname + "(";
   pp_call += "__m_pp_" + top_fname + "(";
   if(top_params.size())
   {
      for(const auto& arg : top_params)
      {
         std::string arg_typename, arg_interface, arg_size;
         const auto arg_type = tree_helper::CGetType(arg);
         if(arg_signature_typename != HLSMgr->design_interface_typename_orig_signature.end())
         {
            THROW_ASSERT(arg_signature_typename->second.size() > param_idx,
                         "Original signature missing for parameter " + STR(param_idx));
            arg_typename = arg_signature_typename->second.at(param_idx);
         }
         else
         {
            arg_typename = tree_helper::PrintType(TM, arg_type, false, true);
         }
         if(is_interface_inferred)
         {
            const auto param_name = top_bh->PrintVariable(GET_INDEX_CONST_NODE(arg));
            THROW_ASSERT(arg_attributes->second.count(param_name),
                         "Attributes missing for parameter " + param_name + " in function " + top_fname);
            THROW_ASSERT(arg_attributes->second.at(param_name).count(attr_interface_type),
                         "Attribute 'interface type' is missing for parameter " + param_name);
            arg_interface = arg_attributes->second.at(param_name).at(attr_interface_type);
         }
         else
         {
            arg_interface = "default";
         }
         if(arg_typename.find("(*)") != std::string::npos)
         {
            arg_typename = arg_typename.substr(0, arg_typename.find("(*)")) + "*";
         }
         arg_size = STR(tree_helper::Size(arg_type));
         const auto arg_name = "P" + STR(param_idx);
         const auto is_pointer_type = arg_typename.back() == '*';
         const auto is_reference_type = arg_typename.back() == '&';
         top_decl += arg_typename + " " + arg_name + ", ";
         gold_decl += arg_typename + ", ";
         std::cmatch what;
         const auto arg_is_channel =
             std::regex_search(arg_typename.data(), what, std::regex("(ac_channel|stream|hls::stream)<(.*)>"));
         if(arg_is_channel)
         {
            THROW_ASSERT(is_pointer_type || is_reference_type, "Channel parameters must be pointers or references.");
            const std::string channel_type(what[1].first, what[1].second);
            arg_typename.pop_back();
            gold_call += arg_name + ", ";
            gold_cmp += "m_channelcmp(" + STR(param_idx) + ", " + cmp_type(arg_type, channel_type) + ");\n";
            args_init += "m_channel_init(" + STR(param_idx) + ");\n";
            args_decl += arg_name + "_sim, ";
            args_set += "__m_setargptr";
         }
         else if(is_pointer_type)
         {
            gold_call += "(" + arg_typename + ")" + arg_name + "_gold, ";
            pp_call += "(" + tree_helper::PrintType(TM, arg_type, false, true) + ")" + arg_name + "_pp, ";
            gold_cmp += "m_argcmp(" + STR(param_idx) + ", " + cmp_type(arg_type, arg_typename) + ");\n";
            if(param_size_default.find(param_idx) != param_size_default.end())
            {
               args_init += "__m_param_alloc(" + STR(param_idx) + ", " + param_size_default.at(param_idx) + ");\n";
            }
            else
            {
               if(is_interface_inferred)
               {
                  const auto param_name = top_bh->PrintVariable(GET_INDEX_CONST_NODE(arg));
                  THROW_ASSERT(arg_attributes->second.count(param_name),
                               "Attributes missing for parameter " + param_name + " in function " + top_fname);
                  THROW_ASSERT(arg_attributes->second.at(param_name).count(attr_size_in_bytes),
                               "Attributes attr_size_in_bytes missing for parameter " + param_name + " in function " +
                                   top_fname);
                  auto size_in_bytes = arg_attributes->second.at(param_name).at(attr_size_in_bytes);
                  args_init += "__m_param_alloc(" + STR(param_idx) + ", " + STR(size_in_bytes) + ");\n";
               }
               else
               {
                  const auto array_size = [&]() {
                     const auto ptd_type = tree_helper::CGetPointedType(arg_type);
                     return tree_helper::IsArrayType(ptd_type) ? tree_helper::GetArrayTotalSize(ptd_type) : 1ULL;
                  }();
                  args_init +=
                      "__m_param_alloc(" + STR(param_idx) + ", sizeof(*" + arg_name + ") * " + STR(array_size) + ");\n";
               }
            }
            args_decl += "(void*)" + arg_name + ", ";
            args_set += "m_setargptr";
         }
         else if(is_reference_type)
         {
            arg_typename.pop_back();
            gold_call += "*(" + arg_typename + "*)" + arg_name + "_gold, ";
            pp_call += "(" + tree_helper::PrintType(TM, arg_type, false, true) + "*)" + arg_name + "_pp, ";
            gold_cmp += "m_argcmp(" + STR(param_idx) + ", " + cmp_type(arg_type, arg_typename) + ");\n";
            args_init += "__m_param_alloc(" + STR(param_idx) + ", sizeof(" + arg_typename + "));\n";
            args_decl += "(void*)&" + arg_name + ", ";
            args_set += "m_setargptr";
         }
         else
         {
            gold_call += arg_name + ", ";
            pp_call += arg_name + ", ";
            args_init += "__m_param_alloc(" + STR(param_idx) + ", sizeof(" + arg_typename + "));\n";
            args_decl += "(void*)&" + arg_name + ", ";
            args_set += arg_interface == "default" ? "__m_setarg" : "m_setargptr";
         }
         args_set += "(" + STR(param_idx) + ", args[" + STR(param_idx) + "], " + arg_size + ");\n";
         ++param_idx;
      }
      top_decl.erase(top_decl.size() - 2);
      gold_decl.erase(gold_decl.size() - 2);
      gold_call.erase(gold_call.size() - 2);
      pp_call.erase(pp_call.size() - 2);
      if(!return_type)
      {
         args_decl.erase(args_decl.size() - 2);
      }
   }
   if(return_type)
   {
      args_decl += +"(void*)&retval";
   }
   top_decl += ")\n";
   gold_decl += ");\n";
   gold_call += ");\n";
   pp_call += ");\n";
   args_decl += "};\n";

   indented_output_stream->AppendIndented(std::string() + R"(
#ifndef CUSTOM_VERIFICATION
#ifdef __cplusplus
#include <cstring>
#else
#include <string.h>
#endif
#endif

#include <mdpi/mdpi_debug.h>
#include <mdpi/mdpi_wrapper.h>

#define typeof __typeof__
#ifdef __cplusplus
template <typename T> struct __m_type { typedef T type; };
template <typename T> struct __m_type<T*> { typedef typename __m_type<T>::type type; };
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
#define m_cmpmem(ptra, ptrb) memcmp(ptra, ptrb, sizeof(*ptrb))
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
      if(m_cmp##cmp((m_getptrt(P##idx))P##idx##_##suffix + i, &m_getptr(P##idx)[i]))                          \
      {                                                                                                       \
         error("Memory parameter %u (%zu/%zu) mismatch with respect to " #suffix " reference.\n", idx, i + 1, \
               P##idx##_count_##suffix);                                                                      \
         ++mismatch_count;                                                                                    \
      }                                                                                                       \
   }                                                                                                          \
   free(P##idx##_##suffix)

#define _ms_retvalcmp(suffix, cmp)                                             \
   if(m_cmp##cmp(&retval, &retval_##suffix))                                   \
   {                                                                           \
      error("Return value mismatch with respect to " #suffix " reference.\n"); \
      ++mismatch_count;                                                        \
   }

#ifndef CUSTOM_VERIFICATION
#define _m_setargptr(idx, ptr) _ms_setargptr(gold, idx, ptr)
#define _m_argcmp(idx, cmp) _ms_argcmp(gold, idx, cmp)
#define _m_retvalcmp(cmp) _ms_retvalcmp(gold, cmp)

#define m_channelcmp(idx, cmp)                                                                              \
   P##idx##_count = m_getptr(P##idx)->size();                                                               \
   for(i = 0; i < P##idx##_count; ++i)                                                                      \
   {                                                                                                        \
      if(m_cmp##cmp((m_getvalt(m_getptr(P##idx))::element_type*)P##idx##_sim + i, &(*m_getptr(P##idx))[i])) \
      {                                                                                                     \
         error("Channel parameter %u (%zu/%zu) mismatch with respect to golden reference.\n", idx, i + 1,   \
               P##idx##_count);                                                                             \
         ++mismatch_count;                                                                                  \
         break;                                                                                             \
      }                                                                                                     \
   }                                                                                                        \
   free(P##idx##_sim)

)" + gold_decl +
                                          R"(#else
#define _m_setargptr(...)
#define _m_argcmp(...)
#define _m_retvalcmp(...)

#define m_channelcmp(idx, cmp)                                                                                      \
   for(i = 0; i < P##idx##_count; ++i)                                                                              \
   {                                                                                                                \
      memcpy(&(*m_getptr(P##idx))[i], (m_getvalt(m_getptr(P##idx))::element_type*)P##idx##_sim + i, P##idx##_item); \
   }                                                                                                                \
   free(P##idx##_sim)
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

#define m_setargptr(idx, ptr, ptrsize) \
   __m_setargptr(idx, ptr, ptrsize);   \
   _m_pp_setargptr(idx, ptr);          \
   _m_setargptr(idx, ptr)

#define m_argcmp(idx, cmp) \
   _m_argdump(idx);        \
   _m_golddump(idx);       \
   _m_pp_argcmp(idx, cmp); \
   _m_argcmp(idx, cmp)

#define m_retvalcmp(cmp) _m_pp_retvalcmp(cmp) _m_retvalcmp(cmp)

#define m_channel_init(idx)                                                                                         \
   const size_t P##idx##_item = sizeof(m_getvalt(m_getptr(P##idx))::element_type);                                  \
   size_t P##idx##_count = m_getptr(P##idx)->size();                                                                \
   __m_param_alloc(idx, P##idx##_count * P##idx##_item);                                                            \
   void* P##idx##_sim = malloc(P##idx##_count * P##idx##_item);                                                     \
   for(i = 0; i < P##idx##_count; ++i)                                                                              \
   {                                                                                                                \
      memcpy((m_getvalt(m_getptr(P##idx))::element_type*)P##idx##_sim + i, &(*m_getptr(P##idx))[i], P##idx##_item); \
   }
)");

   // write C code used to print initialization values for the HDL simulator's memory
   WriteSimulatorInitMemory(top_id);

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
   indented_output_stream->Append("enum mdpi_state state;\n");
   indented_output_stream->Append(args_init);
   indented_output_stream->Append(args_decl);
   indented_output_stream->Append("__m_memsetup(args, " + STR(args_decl_size) + ");\n\n");

   indented_output_stream->Append("__m_arg_init(" + STR(args_decl_size) + ");\n");
   indented_output_stream->Append(args_set);

   indented_output_stream->Append("\n__m_signal_to(MDPI_ENTITY_SIM, MDPI_SIM_SETUP);\n\n");
   indented_output_stream->Append("#ifndef CUSTOM_VERIFICATION\n");
   indented_output_stream->Append(gold_call);
   indented_output_stream->Append("#endif\n\n");
   indented_output_stream->Append("#ifdef PP_VERIFICATION\n");
   indented_output_stream->Append(pp_call);
   indented_output_stream->Append("#endif\n\n");
   indented_output_stream->Append("state = __m_wait_for(MDPI_ENTITY_COSIM);\n");
   indented_output_stream->Append("__m_arg_fini();\n\n");

   indented_output_stream->Append("if(state != MDPI_COSIM_INIT)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("error(\"Unexpected simulator state : %s\\n\", mdpi_state_str(state));\n");
   indented_output_stream->Append("abort();\n");
   indented_output_stream->Append("}\n");

   if(gold_cmp.size() || return_type)
   {
      indented_output_stream->Append(R"(
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpointer-type-mismatch"
#elif GCC_VERSION >= 40600
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-type-mismatch"
#endif

)");
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
abort();
}

#ifdef DUMP_COSIM_OUTPUT
++__m_call_count;
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#elif GCC_VERSION >= 40600
#pragma GCC diagnostic pop
#endif
)");
   }

   if(return_type)
   {
      indented_output_stream->Append("return retval;\n");
   }
   indented_output_stream->Append("}\n\n");

   const auto& test_vectors = HLSMgr->RSim->test_vectors;
   if(top_fname != "main" && test_vectors.size())
   {
      indented_output_stream->Append("int main()\n{\n");
      // write additional initialization code needed by subclasses
      WriteExtraInitCode();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written extra init code");

      // parameters declaration
      WriteParamDecl(top_bh);

      // ---- WRITE VARIABLES DECLARATIONS ----
      // declaration of the return variable of the top function, if not void
      if(return_type)
      {
         const auto ret_type = tree_helper::PrintType(TM, return_type);
         if(tree_helper::IsVectorType(return_type))
         {
            THROW_ERROR("return type of function under test " + top_fname + " is " + STR(ret_type) +
                        "\nco-simulation does not support vectorized return types at top level");
         }
         indented_output_stream->Append("// return variable initialization\n");
         indented_output_stream->Append("volatile " + ret_type + " " RETURN_PORT_NAME ";\n");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written parameters declaration");
      // ---- WRITE PARAMETERS INITIALIZATION AND FUNCTION CALLS ----
      for(unsigned int v_idx = 0; v_idx < test_vectors.size(); v_idx++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Writing initialization for test vector " + STR(v_idx));
         indented_output_stream->Append("{\n");
         const auto& curr_test_vector = test_vectors.at(v_idx);
         // write parameter initialization
         indented_output_stream->Append("// parameter initialization\n");
         WriteParamInitialization(top_bh, curr_test_vector);
         WriteExtraCodeBeforeEveryMainCall();
         // write the call to the top function to be tested
         indented_output_stream->Append("// function call\n");
         WriteTestbenchFunctionCall(top_bh);

         indented_output_stream->Append("}\n");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Written initialization for test vector " + STR(v_idx));
      }
      indented_output_stream->Append("return 0;\n");
      indented_output_stream->Append("}\n");
   }
}

void HLSCWriter::WriteFile(const std::string& file_name)
{
   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top function");
   const auto function_id = *(top_function_ids.begin());
   const auto BH = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                  "-->C-based testbench generation for function " + BH->get_function_name() + ": " + file_name);

   WriteMainTestbench();
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--Prepared testbench");

   indented_output_stream->WriteFile(file_name);
}

void HLSCWriter::WriteFunctionImplementation(unsigned int)
{
   /// Do nothing
}

void HLSCWriter::WriteBuiltinWaitCall()
{
   /// Do nothing
}
