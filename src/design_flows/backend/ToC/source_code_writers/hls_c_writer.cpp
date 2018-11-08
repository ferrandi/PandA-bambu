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
 * @file hls_c_writer.cpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Pietro Fezzardi <pietro.fezzardi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * $Revision: $
 * $Date: $
 * Last modified by $Author: $
 *
 */

/// Header include
#include "hls_c_writer.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "var_pp_functor.hpp"

/// circuit include
#include "structural_objects.hpp"

/// design_flows/backend/ToC includes
#include "hls_c_backend_information.hpp"

/// design_flows/backend/ToC/source_code_writer
#include "instruction_writer.hpp"

/// HLS include
#include "hls_manager.hpp"

/// HLS/memory include
#include "memory.hpp"

/// HLS/simulation include
#include "SimulationInformation.hpp"
#include "testbench_generation_base_step.hpp"

/// technology/physical_library include
#include "technology_node.hpp"

/// tree include
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// utility include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_NONE
#include "indented_output_stream.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

REF_FORWARD_DECL(memory_symbol);

HLSCWriter::HLSCWriter(const HLSCBackendInformationConstRef _hls_c_backend_information, const application_managerConstRef _AppM, const InstructionWriterRef _instruction_writer, const IndentedOutputStreamRef _indented_output_stream,
                       const ParameterConstRef _parameters, bool _verbose)
    : CWriter(_AppM, _instruction_writer, _indented_output_stream, _parameters, _verbose), hls_c_backend_information(_hls_c_backend_information)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

HLSCWriter::~HLSCWriter() = default;

void HLSCWriter::WriteHeader()
{
   indented_output_stream->Append("#define _FILE_OFFSET_BITS 64\n\n");
   indented_output_stream->Append("#define __Inf (1.0/0.0)\n");
   indented_output_stream->Append("#define __Nan (0.0/0.0)\n\n");
   indented_output_stream->Append("#ifdef __cplusplus\n");
   indented_output_stream->Append("#include <cstdio>\n\n");
   indented_output_stream->Append("#include <cstdlib>\n\n");
   indented_output_stream->Append("typedef bool _Bool;\n\n");
   indented_output_stream->Append("#else\n");
   indented_output_stream->Append("#include <stdio.h>\n\n");
   indented_output_stream->Append("extern void exit(int status);\n");
   indented_output_stream->Append("#endif\n\n");

   /// include from cpp
   bool flag_cpp = TM->is_CPP() && !Param->isOption(OPT_pretty_print) && (!Param->isOption(OPT_discrepancy) || !Param->getOption<bool>(OPT_discrepancy));
   if(flag_cpp)
   {
      // get the root function to be tested by the testbench
      const auto top_function_ids = AppM->CGetCallGraphManager()->GetRootFunctions();
      THROW_ASSERT(top_function_ids.size() == 1, "Multiple top function");
      const auto function_id = *(top_function_ids.begin());
      auto fnode = TM->get_tree_node_const(function_id);
      auto fd = GetPointer<function_decl>(fnode);
      std::string fname;
      tree_helper::get_mangled_fname(fd, fname);
      auto& DesignInterfaceInclude = hls_c_backend_information->HLSMgr->design_interface_typenameinclude;
      if(DesignInterfaceInclude.find(fname) != DesignInterfaceInclude.end())
      {
         std::set<std::string> includes;
         const auto& DesignInterfaceArgsInclude = DesignInterfaceInclude.find(fname)->second;
         for(auto argInclude: DesignInterfaceArgsInclude)
            includes.insert(argInclude.second);
         for(auto inc : includes)
            if(inc != "")
               indented_output_stream->Append("#include \"" + inc + "\"\n");
      }
      indented_output_stream->Append("\n");
   }

}

void HLSCWriter::WriteGlobalDeclarations()
{
   instrWriter->write_declarations();
   WriteTestbenchGlobalVars();
   WriteTestbenchHelperFunctions();
}

void HLSCWriter::WriteTestbenchGlobalVars()
{
   // global variables for tesbench
   indented_output_stream->Append("//global variable used to store the output file\n");
   indented_output_stream->Append("FILE * __bambu_testbench_fp;\n\n");
}

void HLSCWriter::WriteTestbenchHelperFunctions()
{
   // exit function
   indented_output_stream->Append("//variable used to detect a standard end of the main (exit has not been called)\n");
   indented_output_stream->Append("unsigned int __standard_exit;\n");
   indented_output_stream->Append("//definition of __bambu_testbench_exit function\n");
   indented_output_stream->Append("void __bambu_testbench_exit(void) __attribute__ ((destructor));\n");
   indented_output_stream->Append("void __bambu_testbench_exit(void)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("if (!__standard_exit)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for return value\\n\");\n");
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o00000000000000000000000000000000\\n\");\n");
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"e\\n\");\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("}\n\n");
   // decimal to binary conversion function
   indented_output_stream->Append("void _Dec2Bin_(FILE * __bambu_testbench_fp, long long int num, unsigned int precision)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("unsigned int i;\n");
   indented_output_stream->Append("unsigned long long int ull_value = (unsigned long long int) num;\n");
   indented_output_stream->Append("for (i = 0; i < precision; ++i)\n");
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"%c\", (((1LLU << (precision - i -1)) & ull_value) ? '1' : '0'));\n");
   indented_output_stream->Append("}\n\n");
   // pointer to binary conversion function
   indented_output_stream->Append("void _Ptd2Bin_(FILE * __bambu_testbench_fp, unsigned char * num, unsigned int precision)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("unsigned int i, j;\n");
   indented_output_stream->Append("char value;\n");
   indented_output_stream->Append("if (precision%8)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("value = *(num+precision/8);\n");
   indented_output_stream->Append("for (j = 8-precision%8; j < 8; ++j)\n");
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"%c\", (((1LLU << (8 - j - 1)) & value) ? '1' : '0'));\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("for (i = 0; i < 8*(precision/8); i = i + 8)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("value = *(num + (precision / 8) - (i / 8) - 1);\n");
   indented_output_stream->Append("for (j = 0; j < 8; ++j)\n");
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"%c\", (((1LLU << (8 - j - 1)) & value) ? '1' : '0'));\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("}\n\n");
   if(Param->isOption(OPT_discrepancy) and Param->getOption<bool>(OPT_discrepancy))
   {
      // Builtin floating point checkers
      indented_output_stream->Append("unsigned char _FPs32Mismatch_(float c, float e, float max_ulp)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("unsigned int binary_c = *((unsigned int*)&c);\n");
      indented_output_stream->Append("unsigned int binary_e = *((unsigned int*)&e);\n");
      indented_output_stream->Append("unsigned int binary_abs_c = binary_c&(~(1U<<31));\n");
      indented_output_stream->Append("unsigned int binary_abs_e = binary_e&(~(1U<<31));\n");
      indented_output_stream->Append("unsigned int denom_0 = 0x34000000;\n");
      indented_output_stream->Append("unsigned int denom_e = ((binary_abs_e>> 23)-23)<<23;\n");
      indented_output_stream->Append("float cme=c - e;\n");
      indented_output_stream->Append("unsigned int binary_cme = *((unsigned int*)&cme);\n");
      indented_output_stream->Append("unsigned int binary_abs_cme = binary_cme&(~(1U<<31));\n");
      indented_output_stream->Append("float abs_cme=*((float*)&binary_abs_cme);\n");
      indented_output_stream->Append("float ulp = 0.0;\n");
      indented_output_stream->Append("if (binary_abs_c>0X7F800000 && binary_abs_c>0X7F800000) return 0;\n");
      indented_output_stream->Append("else if (binary_abs_c==0X7F800000 && binary_abs_e==0X7F800000)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if ((binary_c>>31) != (binary_e>>31))\n");
      indented_output_stream->Append("return 1;\n");
      indented_output_stream->Append("else\n");
      indented_output_stream->Append("return 0;\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("else if (binary_abs_c==0X7F800000 || binary_abs_e==0X7F800000 || binary_abs_c>0X7F800000 || binary_abs_e==0X7F800000) return 0;\n");
      indented_output_stream->Append("else\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if (binary_abs_e == 0) ulp = abs_cme / (*((float *)&denom_0));\n");
      indented_output_stream->Append("else ulp = abs_cme / (*((float *)&denom_e));\n");
      indented_output_stream->Append("return ulp > max_ulp;\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("}\n\n");
      indented_output_stream->Append("unsigned char _FPs64Mismatch_(double c, double e, double max_ulp)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("unsigned long long int binary_c = *((unsigned long long int*)&c);\n");
      indented_output_stream->Append("unsigned long long int binary_e = *((unsigned long long int*)&e);\n");
      indented_output_stream->Append("unsigned long long int binary_abs_c = binary_c&(~(1ULL<<63));\n");
      indented_output_stream->Append("unsigned long long int binary_abs_e = binary_e&(~(1ULL<<63));\n");
      indented_output_stream->Append("unsigned long long int denom_0 = 0x3CB0000000000000;\n");
      indented_output_stream->Append("unsigned long long int denom_e = ((binary_abs_e>> 52)-52)<<52;\n");
      indented_output_stream->Append("double cme=c - e;\n");
      indented_output_stream->Append("unsigned long long int binary_cme = *((unsigned int*)&cme);\n");
      indented_output_stream->Append("unsigned long long int binary_abs_cme = binary_cme&(~(1U<<31));\n");
      indented_output_stream->Append("double abs_cme=*((double*)&binary_abs_cme);\n");
      indented_output_stream->Append("double ulp = 0.0;\n");
      indented_output_stream->Append("if (binary_abs_c>0X7FF0000000000000 && binary_abs_c>0X7FF0000000000000) return 0;\n");
      indented_output_stream->Append("else if (binary_abs_c==0X7FF0000000000000 && binary_abs_e==0X7FF0000000000000)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if ((binary_c>>63) != (binary_e>>63))\n");
      indented_output_stream->Append("return 1;\n");
      indented_output_stream->Append("else\n");
      indented_output_stream->Append("return 0;\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("else if (binary_abs_c==0X7FF0000000000000 || binary_abs_e==0X7FF0000000000000 || binary_abs_c>0X7FF0000000000000 || binary_abs_e==0X7FF0000000000000) return 0;\n");
      indented_output_stream->Append("else\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if (binary_abs_e == 0) ulp = abs_cme / (*((double *)&denom_0));\n");
      indented_output_stream->Append("else ulp = abs_cme / (*((double *)&denom_e));\n");
      indented_output_stream->Append("return ulp > max_ulp;\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("}\n\n");
      indented_output_stream->Append("void _CheckBuiltinFPs32_(char * chk_str, unsigned char neq, float par_expected, float par_res, float par_a, float par_b)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if(neq)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append(
          "printf(\"\\n\\n***********************************************************\\nERROR ON A BASIC FLOATING POINT OPERATION : %s : expected=%a res=%a a=%a b=%a\\n***********************************************************\\n\\n\", chk_str, "
          "par_expected, par_res, par_a, par_b);\n");
      indented_output_stream->Append("exit(1);\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("}\n\n");
      indented_output_stream->Append("void _CheckBuiltinFPs64_(char * chk_str, unsigned char neq, double par_expected, double par_res, double par_a, double par_b)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if(neq)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append(
          "printf(\"\\n\\n***********************************************************\\nERROR ON A BASIC FLOATING POINT OPERATION : %s : expected=%a res=%a a=%a b=%a\\n***********************************************************\\n\\n\", chk_str, "
          "par_expected, par_res, par_a, par_b);\n");
      indented_output_stream->Append("exit(1);\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("}\n\n");
   }
}

void HLSCWriter::WriteParamDecl(const BehavioralHelperConstRef behavioral_helper)
{
   std::string type;
   std::string param;
   bool flag_cpp = TM->is_CPP() && !Param->isOption(OPT_pretty_print) && (!Param->isOption(OPT_discrepancy) || !Param->getOption<bool>(OPT_discrepancy));

   bool doStandardWay = true;
   hls_c_backend_information->HLSMgr->RSim->simulationArgSignature.clear();
   if(flag_cpp)
   {
      auto fnode = TM->get_tree_node_const(behavioral_helper->get_function_index());
      auto fd = GetPointer<function_decl>(fnode);
      std::string fname;
      tree_helper::get_mangled_fname(fd, fname);
      auto& DesignInterfaceTypename = hls_c_backend_information->HLSMgr->design_interface_typename;
      if(DesignInterfaceTypename.find(fname) != DesignInterfaceTypename.end())
      {
         const auto& DesignInterfaceArgsTypename = DesignInterfaceTypename.find(fname)->second;
         unsigned int pIndex=0;
         for(const auto& p : behavioral_helper->get_parameters())
         {
            unsigned int type_id = behavioral_helper->get_type(p);
            type = behavioral_helper->print_type(type_id);
            param = behavioral_helper->PrintVariable(p);
            hls_c_backend_information->HLSMgr->RSim->simulationArgSignature.push_back(param);

            if(tree_helper::is_a_vector(TM, type_id))
            {
               THROW_ERROR("parameter " + param + " of function under test " + behavioral_helper->get_function_name() + " has type " + type +
                           "\n"
                           "co-simulation does not support vectorized parameters at top level");
            }
            THROW_ASSERT(DesignInterfaceArgsTypename.find(param) != DesignInterfaceArgsTypename.end(), "unexpected condition");
            auto argTypename = DesignInterfaceArgsTypename.find(param)->second;
            if(argTypename.find("const ")==0)
               argTypename=argTypename.substr(std::string("const ").size());
            if(argTypename.at(argTypename.size()-1) == '&')
               argTypename=argTypename.substr(0,argTypename.size()-1);
            indented_output_stream->Append(argTypename + " " + param + ";\n");
            ++pIndex;
         }
         doStandardWay = false;
      }
   }
   if(doStandardWay)
   {
      indented_output_stream->Append("// parameters declaration\n");
      for(const auto& p : behavioral_helper->get_parameters())
      {
         unsigned int type_id = behavioral_helper->get_type(p);
         type = behavioral_helper->print_type(type_id);
         param = behavioral_helper->PrintVariable(p);
         hls_c_backend_information->HLSMgr->RSim->simulationArgSignature.push_back(param);

         if(tree_helper::is_a_vector(TM, type_id))
         {
            THROW_ERROR("parameter " + param + " of function under test " + behavioral_helper->get_function_name() + " has type " + type +
                        "\n"
                        "co-simulation does not support vectorized parameters at top level");
         }
         if(behavioral_helper->is_a_pointer(p))
         {
            var_pp_functorRef var_functor = var_pp_functorRef(new std_var_pp_functor(behavioral_helper));
            std::string type_declaration = tree_helper::print_type(TM, type_id, false, false, false, p, var_functor);
            if(flag_cpp)
            {
               bool reference_type_p = false;
               tree_nodeRef pt_node = TM->get_tree_node_const(type_id);
               if(pt_node->get_kind() == pointer_type_K) { reference_type_p = false; }
               else if(pt_node->get_kind() == reference_type_K)
               {
                  reference_type_p = true;
               }
               else
                  THROW_ERROR("A pointer type is expected");
               if(reference_type_p) boost::replace_all(type_declaration, "/*&*/*", "");
            }
            indented_output_stream->Append(type_declaration + ";\n");
         }
         else
         {
            indented_output_stream->Append(type + " " + param + ";\n");
         }
      }
   }
}

void HLSCWriter::WriteParamInitialization(const BehavioralHelperConstRef behavioral_helper, const std::map<std::string, std::string>& curr_test_vector, const unsigned int v_idx)
{
   bool flag_cpp = TM->is_CPP() && !Param->isOption(OPT_pretty_print) && (!Param->isOption(OPT_discrepancy) || !Param->getOption<bool>(OPT_discrepancy));

   for(const auto& p : behavioral_helper->get_parameters())
   {
      unsigned int type_id = behavioral_helper->get_type(p);
      std::string type = behavioral_helper->print_type(type_id);
      std::string param = behavioral_helper->PrintVariable(p);
      if(behavioral_helper->is_a_pointer(p))
      {
         bool reference_type_p = false;
         unsigned int base_type = tree_helper::get_type_index(TM, p);
         tree_nodeRef pt_node = TM->get_tree_node_const(base_type);
         if(pt_node->get_kind() == pointer_type_K)
         {
            reference_type_p = false;
            base_type = GET_INDEX_NODE(GetPointer<pointer_type>(pt_node)->ptd);
         }
         else if(pt_node->get_kind() == reference_type_K)
         {
            reference_type_p = true;
            base_type = GET_INDEX_NODE(GetPointer<reference_type>(pt_node)->refd);
         }
         else
            THROW_ERROR("A pointer type is expected");
         bool interfaceBaseAlloc=false;
         if(flag_cpp)
         {
            auto fnode = TM->get_tree_node_const(behavioral_helper->get_function_index());
            auto fd = GetPointer<function_decl>(fnode);
            std::string fname;
            tree_helper::get_mangled_fname(fd, fname);
            auto& DesignInterfaceTypename = hls_c_backend_information->HLSMgr->design_interface_typename;
            if(DesignInterfaceTypename.find(fname) != DesignInterfaceTypename.end())
            {
               const auto& DesignInterfaceArgsTypename = DesignInterfaceTypename.find(fname)->second;
               auto argTypename = DesignInterfaceArgsTypename.find(param)->second;
               if((*argTypename.rbegin())!='*')
                  reference_type_p=true;
               if((*argTypename.rbegin())=='*')
               {
                  interfaceBaseAlloc=true;
                  indented_output_stream->Append(param + " = (" + argTypename + ")malloc(sizeof(" + argTypename.substr(0,argTypename.size()-1) + "));\n");
               }
            }
         }
         std::string test_v = "0";
         if(curr_test_vector.find(param) != curr_test_vector.end()) test_v = curr_test_vector.find(param)->second;

         std::vector<std::string> splitted;
         boost::algorithm::split(splitted, test_v, boost::algorithm::is_any_of(","));

         unsigned int base_type_bytesize = tree_helper::size(TM, base_type) / 8;
         if(base_type_bytesize == 0) // must be at least a byte
            base_type_bytesize = 1;

         if(!interfaceBaseAlloc && (splitted.size() != 1 || !reference_type_p || !flag_cpp))
         {
            var_pp_functorRef var_functor = var_pp_functorRef(new std_var_pp_functor(behavioral_helper));
            indented_output_stream->Append((*var_functor)(p));

            indented_output_stream->Append(" = (" + type + ")malloc(" + STR(base_type_bytesize * splitted.size()) + ");\n");
         }

         // check for regularity
         bool all_equal = splitted.size() > 1;
         for(unsigned int i = 1; i < splitted.size(); i++)
            if(splitted[i] != splitted[0]) all_equal = false;

         for(unsigned int i = 0; i < splitted.size(); i++)
         {
            if(!reference_type_p && !interfaceBaseAlloc && (behavioral_helper->is_a_struct(base_type) || behavioral_helper->is_an_union(base_type)))
            {
               std::vector<std::string> splitted_fields;
               std::string field_value = splitted[i];
               boost::algorithm::split(splitted_fields, field_value, boost::algorithm::is_any_of("|"));
               const std::list<tree_nodeConstRef> fields = tree_helper::CGetFieldTypes(TM->CGetTreeNode(base_type));
               size_t n_values = splitted_fields.size();
               unsigned int index = 0;
               for(auto it = fields.begin(); it != fields.end(); ++it, ++index)
               {
                  if(index < n_values) { indented_output_stream->Append(param + "[" + STR(i) + "]." + behavioral_helper->PrintVariable(tree_helper::get_field_idx(TM, base_type, index)) + " = " + splitted_fields[index] + ";\n"); }
                  else
                  {
                     indented_output_stream->Append(param + "[" + STR(i) + "]." + behavioral_helper->PrintVariable(tree_helper::get_field_idx(TM, base_type, index)) + " = 0;\n");
                  }
               }
            }
            else if(!reference_type_p && !interfaceBaseAlloc && (behavioral_helper->is_an_array(base_type)))
            {
               unsigned int num_elements = tree_helper::get_array_num_elements(TM, base_type);
               if(splitted.size() == 1)
               {
                  for(unsigned int l = 0; l < num_elements; l++) { indented_output_stream->Append("(*" + param + ")" + "[" + STR(l) + "] = " + splitted[i] + ";\n"); }
               }
               else
               {
                  unsigned int elmts_type = behavioral_helper->GetElements(base_type);
                  while(behavioral_helper->is_an_array(elmts_type)) { elmts_type = behavioral_helper->GetElements(elmts_type); }
                  indented_output_stream->Append("(*(((" + behavioral_helper->print_type(elmts_type) + "*)" + param + ") + " + STR(i) + ")) = " + splitted[i] + ";\n");
               }
            }
            else
            {
               if(all_equal)
               {
                  indented_output_stream->Append("for (__testbench_index2 = 0; __testbench_index2 < " + STR(splitted.size()) + "; ++__testbench_index2)\n");
                  indented_output_stream->Append(param + "[__testbench_index2] = " + splitted[0] + ";\n");
                  break;
               }
               else
               {
                  if(flag_cpp && splitted.size() == 1 && reference_type_p)
                     indented_output_stream->Append(param + " = " + splitted[i] + ";\n");
                  else
                     indented_output_stream->Append(param + "[" + STR(i) + "] = " + splitted[i] + ";\n");
               }
            }
         }
         std::string memory_addr;
         THROW_ASSERT(hls_c_backend_information->HLSMgr->RSim->param_address.find(v_idx)->second.find(p) != hls_c_backend_information->HLSMgr->RSim->param_address.find(v_idx)->second.end(), "parameter does not have an address");
         memory_addr = STR(hls_c_backend_information->HLSMgr->RSim->param_address.find(v_idx)->second.find(p)->second);

         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//parameter: " + param + " value: " + memory_addr + "\\n\");\n");

         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"p" + convert_in_binary(behavioral_helper, 0, memory_addr, 32) + "\\n\");\n");
      }
      else
      {
         if(curr_test_vector.find(param) == curr_test_vector.end()) THROW_ERROR("Value of " + param + " is missing in test vector");

         if(type_id && behavioral_helper->is_real(type_id) && curr_test_vector.find(param)->second == "-0")
         {
            if(tree_helper::size(TM, type_id) == 32)
               indented_output_stream->Append(param + " = copysignf(0.0, -1.0);\n");
            else
               indented_output_stream->Append(param + " = copysign(0.0, -1.0);\n");
         }
         else
            indented_output_stream->Append(param + " = " + curr_test_vector.find(param)->second + ";\n");

         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//parameter: " + param + " value: " + curr_test_vector.find(param)->second + "\\n\");\n");

         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"p" + convert_in_binary(behavioral_helper, type_id, curr_test_vector.find(param)->second, tree_helper::size(TM, type_id)) + "\\n\");\n");
      }
   }

}

void HLSCWriter::WriteTestbenchFunctionCall(const BehavioralHelperConstRef behavioral_helper)
{
   const unsigned int function_index = behavioral_helper->get_function_index();
   const unsigned int return_type_index = behavioral_helper->GetFunctionReturnType(function_index);
   bool flag_cpp = TM->is_CPP() && !Param->isOption(OPT_pretty_print) && (!Param->isOption(OPT_discrepancy) || !Param->getOption<bool>(OPT_discrepancy));

   std::string function_name;

   if(flag_cpp)
   {
      tree_nodeRef fd_node = TM->get_tree_node_const(function_index);
      auto* fd = GetPointer<function_decl>(fd_node);
      std::string simple_name;
      tree_nodeRef id_name = GET_NODE(fd->name);
      if(id_name->get_kind() == identifier_node_K)
      {
         auto* in = GetPointer<identifier_node>(id_name);
         if(!in->operator_flag) simple_name = in->strg;
      }
      if(simple_name != "")
         function_name = simple_name;
      else
         function_name = behavioral_helper->get_function_name();
   }
   else
      function_name = behavioral_helper->get_function_name();
   // avoid collision with the main
   if(function_name == "main")
   {
      if(not Param->isOption(OPT_discrepancy) or not Param->getOption<bool>(OPT_discrepancy)) { function_name = "system"; }
      else
      {
         function_name = "_main";
      }
   }

   bool is_system;
   std::string decl = std::get<0>(behavioral_helper->get_definition(behavioral_helper->get_function_index(), is_system));
   if((is_system || decl == "<built-in>") && behavioral_helper->is_real(return_type_index))
   {
      indented_output_stream->Append("extern " + behavioral_helper->print_type(return_type_index) + " " + function_name + "(");
      bool is_first_parameter = true;
      for(const auto& p : behavioral_helper->get_parameters())
      {
         if(is_first_parameter)
            is_first_parameter = false;
         else
            indented_output_stream->Append(", ");

         unsigned int type_id = behavioral_helper->get_type(p);
         std::string type = behavioral_helper->print_type(type_id);
         std::string param = behavioral_helper->PrintVariable(p);

         if(behavioral_helper->is_a_pointer(p))
         {
            var_pp_functorRef var_functor = var_pp_functorRef(new std_var_pp_functor(behavioral_helper));
            indented_output_stream->Append(tree_helper::print_type(TM, type_id, false, false, false, p, var_functor));
         }
         else
         {
            indented_output_stream->Append(type + " " + param + "");
         }
      }
      indented_output_stream->Append(");\n");
   }

   if(return_type_index) indented_output_stream->Append(std::string(RETURN_PORT_NAME) + " = ");

   indented_output_stream->Append(function_name + "(");
   // function arguments
   if(function_name != "system")
   {
      bool is_first_argument = true;
      for(const auto& p : behavioral_helper->get_parameters())
      {
         if(!is_first_argument)
            indented_output_stream->Append(", ");
         else
            is_first_argument = false;
         std::string param = behavioral_helper->PrintVariable(p);
         indented_output_stream->Append(param);
      }
   }
   else
   {
      indented_output_stream->Append("\"" + Param->getOption<std::string>(OPT_output_directory) + "/simulation/main_exec\"");
   }
   indented_output_stream->Append(");\n");
   if(function_name == "system" and return_type_index and not Param->getOption<bool>(OPT_no_return_zero)) { indented_output_stream->Append("if(" RETURN_PORT_NAME " != 0) exit(1);\n"); }
}

void HLSCWriter::WriteExpectedResults(const BehavioralHelperConstRef behavioral_helper, const std::map<std::string, std::string>& curr_test_vector)
{
   bool flag_cpp = TM->is_CPP() && !Param->isOption(OPT_pretty_print) && (!Param->isOption(OPT_discrepancy) || !Param->getOption<bool>(OPT_discrepancy));
   auto fnode = TM->get_tree_node_const(behavioral_helper->get_function_index());
   auto fd = GetPointer<function_decl>(fnode);
   std::string fname;
   tree_helper::get_mangled_fname(fd, fname);
   auto& DesignInterfaceTypename = hls_c_backend_information->HLSMgr->design_interface_typename;
   bool hasInterface = flag_cpp && DesignInterfaceTypename.find(fname) != DesignInterfaceTypename.end();

   const unsigned int return_type_index = behavioral_helper->GetFunctionReturnType(behavioral_helper->get_function_index());

   for(const auto& p : behavioral_helper->get_parameters())
   {
      std::string param = behavioral_helper->PrintVariable(p);
      if(behavioral_helper->is_a_pointer(p))
      {
         bool reference_type_p = false;
         std::string test_v = "0";
         if(curr_test_vector.find(param) != curr_test_vector.end()) test_v = curr_test_vector.find(param)->second;

         std::vector<std::string> splitted;
         boost::algorithm::split(splitted, test_v, boost::algorithm::is_any_of(","));

         unsigned int base_type = tree_helper::get_type_index(TM, p);
         tree_nodeRef pt_node = TM->get_tree_node_const(base_type);
         if(pt_node->get_kind() == pointer_type_K)
         {
            reference_type_p = false;
            base_type = GET_INDEX_NODE(GetPointer<pointer_type>(pt_node)->ptd);
         }
         else if(pt_node->get_kind() == reference_type_K)
         {
            reference_type_p = true;
            base_type = GET_INDEX_NODE(GetPointer<reference_type>(pt_node)->refd);
         }
         else
            THROW_ERROR("A pointer type is expected");
         unsigned int base_type_bitsize = tree_helper::size(TM, base_type);
         if(hasInterface)
         {
            auto argTypename = DesignInterfaceTypename.find(fname)->second.find(param)->second;
            if((*argTypename.rbegin())!='*')
               reference_type_p=true;
            auto acTypeBw=ac_type_bitwidth(argTypename);
            if(acTypeBw)
               base_type_bitsize=acTypeBw;
         }

         if((behavioral_helper->is_real(base_type) || behavioral_helper->is_a_struct(base_type) || behavioral_helper->is_an_union(base_type)))
         {
            if(splitted.size() == 1 && flag_cpp && reference_type_p)
            {

               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output " + param + ": %" + (behavioral_helper->is_real(base_type) ? std::string("g") : std::string("d")) + "\\n\", " + (behavioral_helper->is_real(base_type) ? std::string("") : std::string("(int)")) + param +
                                              ");\n");
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
               indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, (unsigned char*)&" + param + ", " + STR(base_type_bitsize) + ");\n");
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
            }
            else
            {
               indented_output_stream->Append("for (__testbench_index2 = 0; __testbench_index2 < " + STR(splitted.size()) + "; ++__testbench_index2)\n{\n");
               if(output_level > OUTPUT_LEVEL_MINIMUM)
               {
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output " + param + "[__testbench_index2]: %" + (behavioral_helper->is_real(base_type) ? std::string("g") : std::string("d")) + "\\n\", " + (behavioral_helper->is_real(base_type) ? std::string("") : std::string("(int)")) + param +
                                                 "[__testbench_index2]);\n");
               }
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
               indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, (unsigned char*)&" + param + "[__testbench_index2], " + STR(base_type_bitsize) + ");\n");
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
               indented_output_stream->Append("}\n");
            }
         }
         else if(!reference_type_p && behavioral_helper->is_an_array(base_type))
         {
            indented_output_stream->Append("for (__testbench_index2 = 0; __testbench_index2 < " + STR(splitted.size()) + "; ++__testbench_index2)\n{\n");
            unsigned int data_bitsize = tree_helper::get_array_data_bitsize(TM, base_type);
            unsigned int num_elements = tree_helper::get_array_num_elements(TM, base_type);
            unsigned int elmts_type = behavioral_helper->GetElements(base_type);
            while(behavioral_helper->is_an_array(elmts_type)) elmts_type = behavioral_helper->GetElements(elmts_type);
            if(output_level > OUTPUT_LEVEL_MINIMUM)
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output (*(((" + behavioral_helper->print_type(elmts_type) + "*)" + param + ")+ __testbench_index2)): %" +
                                              (behavioral_helper->is_real(elmts_type) ? std::string("g") : std::string("d")) + "\\n\", (*(((" + behavioral_helper->print_type(elmts_type) + "*)" + param + ")+ __testbench_index2)));\n");
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
            if(splitted.size() == 1)
            {
               for(unsigned int l = 0; l < num_elements; l++)
               {
                  if(behavioral_helper->is_real(elmts_type))
                  {
                     indented_output_stream->Append(
                         "_Ptd2Bin_(__bambu_testbench_fp, "
                         "(unsigned char*)&(*" +
                         param + ")[" + STR(l) + "], " + STR(data_bitsize) + ");\n");
                  }
                  else
                  {
                     indented_output_stream->Append(
                         "_Dec2Bin_(__bambu_testbench_fp, "
                         "(*" +
                         param + ")[" + STR(l) + "], " + STR(data_bitsize) + ");\n");
                  }
               }
            }
            else
            {
               if(behavioral_helper->is_real(elmts_type))
               {
                  indented_output_stream->Append(
                      "_Ptd2Bin_(__bambu_testbench_fp, "
                      "(unsigned char*)&(*(((" +
                      behavioral_helper->print_type(elmts_type) + "*)" + param + ")+ __testbench_index2)), " + STR(data_bitsize) + ");\n");
               }
               else
               {
                  indented_output_stream->Append(
                      "_Dec2Bin_(__bambu_testbench_fp, "
                      "(*(((" +
                      behavioral_helper->print_type(elmts_type) + "*)" + param + ") + __testbench_index2)), " + STR(data_bitsize) + ");\n");
               }
            }
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
            indented_output_stream->Append("}\n");
         }
         else
         {
            if(splitted.size() == 1 && flag_cpp && reference_type_p)
            {
               if(output_level > OUTPUT_LEVEL_MINIMUM)
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output " + param + ": %" + (behavioral_helper->is_real(base_type) ? std::string("g") : std::string("d")) + "\\n\", " + (behavioral_helper->is_real(base_type) ? std::string("") : std::string("(int)")) + param + ");\n");
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
               indented_output_stream->Append("_Dec2Bin_(__bambu_testbench_fp, " + param + ", " + STR(base_type_bitsize) + ");\n");
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
            }
            else
            {
               indented_output_stream->Append("for (__testbench_index2 = 0; __testbench_index2 < " + STR(splitted.size()) + "; ++__testbench_index2)\n{\n");
               if(output_level > OUTPUT_LEVEL_MINIMUM)
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output " + param + "[__testbench_index2]: %" + (behavioral_helper->is_real(base_type) ? std::string("g") : std::string("d")) + "\\n\", " + (behavioral_helper->is_real(base_type) ? std::string("") : std::string("(int)")) + param +
                                                 "[__testbench_index2]);\n");
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
               indented_output_stream->Append("_Dec2Bin_(__bambu_testbench_fp, " + param + "[__testbench_index2], " + STR(base_type_bitsize) + ");\n");
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
               indented_output_stream->Append("}\n");
            }
         }
         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"e\\n\");\n");
      }
   }

   if(return_type_index)
   {
      if(output_level > OUTPUT_LEVEL_MINIMUM) indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for return value\\n\");\n");
      indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
      unsigned int base_type_bitsize = tree_helper::size(TM, return_type_index);
      if(behavioral_helper->is_real(return_type_index) || behavioral_helper->is_a_struct(return_type_index) || behavioral_helper->is_an_union(return_type_index))
      { indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, (unsigned char*)&" + std::string(RETURN_PORT_NAME) + ", " + STR(base_type_bitsize) + ");\n"); } else
      {
         indented_output_stream->Append("_Dec2Bin_(__bambu_testbench_fp, " + std::string(RETURN_PORT_NAME) + ", " + STR(base_type_bitsize) + ");\n");
      }
      indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
      indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"e\\n\");\n");
   }
}

void HLSCWriter::WriteSimulatorInitMemory(const unsigned int function_id)
{
   const BehavioralHelperConstRef behavioral_helper = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   // print base address
   unsigned int base_address = hls_c_backend_information->HLSMgr->base_address;
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//base address " + STR(base_address) + "\\n\");\n");
   std::string trimmed_value;
   for(unsigned int ind = 0; ind < 32; ind++) trimmed_value = trimmed_value + (((1LLU << (31 - ind)) & base_address) ? '1' : '0');
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"b" + trimmed_value + "\\n\");\n");

   const std::map<unsigned int, memory_symbolRef>& mem_vars = hls_c_backend_information->HLSMgr->Rmem->get_ext_memory_variables();
   // get the mapping between variables in external memory and their external
   // base address
   std::map<unsigned int, unsigned int> address;
   for(const auto& m : mem_vars) address[hls_c_backend_information->HLSMgr->Rmem->get_external_base_address(m.first)] = m.first;

   std::list<unsigned int> mem;
   for(const auto& ma : address) mem.push_back(ma.second);

   const std::list<unsigned int>& parameters = behavioral_helper->get_parameters();
   for(const auto& p : parameters)
   {
      // if the function has some pointer parameters some memory needs to be
      // reserved for the place where they point to
      if(behavioral_helper->is_a_pointer(p) && mem_vars.find(p) == mem_vars.end()) mem.push_back(p);
   }

   unsigned int v_idx = 0;
   // loop on the test vectors
   for(const auto& curr_test_vector : hls_c_backend_information->HLSMgr->RSim->test_vectors)
   {
      // loop on the variables in memory
      for(const auto& l : mem)
      {
         std::string param = behavioral_helper->PrintVariable(l);
         if(param[0] == '"') param = "@" + STR(l);

         bool is_memory = false;
         std::string test_v = "0";
         if(mem_vars.find(l) != mem_vars.end() && std::find(parameters.begin(), parameters.end(), l) == parameters.end())
         {
            is_memory = true;
            test_v = TestbenchGenerationBaseStep::print_var_init(TM, l, hls_c_backend_information->HLSMgr->Rmem);
            if(output_level > OUTPUT_LEVEL_MINIMUM) indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//memory initialization for variable " + param + "\\n\");\n");
         }
         else if(curr_test_vector.find(param) != curr_test_vector.end())
         {
            test_v = curr_test_vector.find(param)->second;
         }

         if(v_idx > 0 && is_memory) continue; // memory has been already initialized

         size_t reserved_mem_bytes = tree_helper::size(TM, l) / 8;
         if(reserved_mem_bytes == 0) // must be at least a byte
            reserved_mem_bytes = 1;

         unsigned int base_type = tree_helper::get_type_index(TM, l);
         unsigned int base_type_bytesize = 1;
         std::vector<std::string> splitted;
         boost::algorithm::split(splitted, test_v, boost::algorithm::is_any_of(","));
         if(tree_helper::is_a_pointer(TM, l) && !is_memory)
         {
            tree_nodeRef pt_node = TM->get_tree_node_const(base_type);
            unsigned int ptd_base_type = 0;
            if(pt_node->get_kind() == pointer_type_K)
               ptd_base_type = GET_INDEX_NODE(GetPointer<pointer_type>(pt_node)->ptd);
            else if(pt_node->get_kind() == reference_type_K)
               ptd_base_type = GET_INDEX_NODE(GetPointer<reference_type>(pt_node)->refd);
            else
               THROW_ERROR("A pointer type is expected");
            if(behavioral_helper->is_an_array(ptd_base_type))
               base_type_bytesize = tree_helper::get_array_data_bitsize(TM, ptd_base_type) / 8;
            else if(tree_helper::size(TM, ptd_base_type) == 1)
               base_type_bytesize = 1;
            else
               base_type_bytesize = tree_helper::size(TM, ptd_base_type) / 8;

            reserved_mem_bytes = hls_c_backend_information->HLSMgr->RSim->param_mem_size.find(v_idx)->second.find(l)->second;
         }
         else if(!is_memory)
         {
            THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "not yet supported case");
         }
         else
         {
            base_type_bytesize = static_cast<unsigned int>(splitted[0].size() / 8);
         }
         if(base_type_bytesize == 0) // must be at least a byte
            base_type_bytesize = 1;

         /// check for regularity
         bool all_equal = splitted.size() > 1;
         for(unsigned int i = 1; i < splitted.size(); i++)
            if(splitted[i] != splitted[0]) all_equal = false;

         size_t printed_bytes = 0;
         std::string bits_offset = "";
         if(all_equal)
         {
            indented_output_stream->Append("for (__testbench_index1 = 0; __testbench_index1 < " + STR(splitted.size()) + "; ++__testbench_index1)\n{\n");
            printed_bytes += (base_type_bytesize) * (splitted.size() - 1);
         }

         for(unsigned int i = 0; i < splitted.size() && (!all_equal || i == 0); i++)
         {
            THROW_ASSERT(splitted[i] != "", "Not well formed test vector: " + test_v);

            std::string initial_string = splitted[i];
            std::string binary_string;
            if(is_memory) { printed_bytes += WriteBinaryMemoryInit(initial_string, static_cast<unsigned int>(initial_string.size()), bits_offset); }
            else
            {
               std::string init_value_copy = initial_string;
               boost::replace_all(init_value_copy, "\\", "\\\\");
               boost::replace_all(init_value_copy, "\n", "");
               boost::replace_all(init_value_copy, "\"", "");
               if(output_level > OUTPUT_LEVEL_MINIMUM) indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//memory initialization for variable " + param /*+ " value: " + init_value_copy */+ "\\n\");\n");
               tree_nodeRef pt_node = TM->get_tree_node_const(base_type);
               unsigned int ptd_base_type = 0;
               if(pt_node->get_kind() == pointer_type_K)
                  ptd_base_type = GET_INDEX_NODE(GetPointer<pointer_type>(pt_node)->ptd);
               else if(pt_node->get_kind() == reference_type_K)
                  ptd_base_type = GET_INDEX_NODE(GetPointer<reference_type>(pt_node)->refd);
               else
                  THROW_ERROR("A pointer type is expected");
               tree_nodeRef ptd_base_type_node;
               if(pt_node->get_kind() == pointer_type_K)
                  ptd_base_type_node = GET_NODE(GetPointer<pointer_type>(pt_node)->ptd);
               else if(pt_node->get_kind() == reference_type_K)
                  ptd_base_type_node = GET_NODE(GetPointer<reference_type>(pt_node)->refd);
               else
                  THROW_ERROR("A pointer type is expected");

               unsigned int data_bitsize = tree_helper::size(TM, ptd_base_type);
               if(behavioral_helper->is_a_struct(ptd_base_type))
               {
                  std::vector<std::string> splitted_fields;
                  boost::algorithm::split(splitted_fields, initial_string, boost::algorithm::is_any_of("|"));
                  const std::list<tree_nodeConstRef> fields = tree_helper::CGetFieldTypes(TM->CGetTreeNode(ptd_base_type));
                  size_t n_values = splitted_fields.size();
                  unsigned int index = 0;
                  for(auto it = fields.begin(); it != fields.end(); ++it, ++index)
                  {
                     const tree_nodeConstRef field_type = *it;
                     unsigned int field_size = tree_helper::Size(field_type);
                     if(index < n_values) { binary_string = convert_in_binary(behavioral_helper, field_type->index, splitted_fields[index], field_size); }
                     else
                     {
                        binary_string = convert_in_binary(behavioral_helper, field_type->index, "0", field_size);
                     }

                     printed_bytes += WriteBinaryMemoryInit(binary_string, field_size, bits_offset);
                  }
               }
               else if(behavioral_helper->is_an_union(ptd_base_type))
               {
                  unsigned int max_bitsize_field = 0;
                  tree_helper::accessed_greatest_bitsize(TM, ptd_base_type_node, ptd_base_type, max_bitsize_field);
                  binary_string = convert_in_binary(behavioral_helper, 0, "0", max_bitsize_field);
                  printed_bytes += WriteBinaryMemoryInit(binary_string, max_bitsize_field, bits_offset);
               }
               else if(behavioral_helper->is_an_array(ptd_base_type))
               {
                  unsigned int elmts_type = behavioral_helper->GetElements(ptd_base_type);

                  while(behavioral_helper->is_an_array(elmts_type)) elmts_type = behavioral_helper->GetElements(elmts_type);

                  data_bitsize = tree_helper::get_array_data_bitsize(TM, ptd_base_type);

                  unsigned int num_elements = 1;
                  if(splitted.size() == 1) num_elements = tree_helper::get_array_num_elements(TM, ptd_base_type);

                  indented_output_stream->Append("for (__testbench_index0 = 0; __testbench_index0 < " + STR(num_elements) + "; ++__testbench_index0)\n{\n");

                  binary_string = convert_in_binary(behavioral_helper, elmts_type, initial_string, data_bitsize);
                  printed_bytes += WriteBinaryMemoryInit(binary_string, data_bitsize, bits_offset);
                  indented_output_stream->Append("}\n");
               }
               else
               {
                  binary_string = convert_in_binary(behavioral_helper, ptd_base_type, initial_string, data_bitsize);

                  if(data_bitsize == 1)
                  {
                     data_bitsize = 8;
                     binary_string = "0000000" + binary_string;
                  }
                  else
                  {
                     THROW_ASSERT(data_bitsize % 8 == 0, "unexpected case");
                  }

                  printed_bytes += WriteBinaryMemoryInit(binary_string, data_bitsize, bits_offset);
               }
            }
         }

         if(bits_offset.size())
         {
            std::string tail_padding;
            for(auto tail_padding_ind = bits_offset.size(); tail_padding_ind < 8; ++tail_padding_ind) tail_padding += "0";
            tail_padding = tail_padding + bits_offset;
            bits_offset = "";
            ++printed_bytes;
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"m" + tail_padding + "\\n\");\n");
         }
         if(all_equal) indented_output_stream->Append("}\n");

         if(printed_bytes > reserved_mem_bytes)
         {
            THROW_ERROR(
                "mismatch between bytes reserved in external memory and printed object size:"
                " Printed(" +
                STR(printed_bytes) + "), ObjSize(" + STR(reserved_mem_bytes) + ") for " + STR(l));
         }

         if(reserved_mem_bytes > printed_bytes)
         {
            indented_output_stream->Append("// reserved_mem_bytes > printed_bytes\n");
            WriteZeroedBytes(reserved_mem_bytes - printed_bytes);
         }

         size_t next_object_offset = hls_c_backend_information->HLSMgr->RSim->param_next_off.find(v_idx)->second.find(l)->second;

         if(next_object_offset > reserved_mem_bytes)
         {
            indented_output_stream->Append("// next_object_offset > reserved_mem_bytes\n");
            WriteZeroedBytes(next_object_offset - reserved_mem_bytes);
         }
      }
      ++v_idx;
   }
}

void HLSCWriter::WriteExtraInitCode() {}

void HLSCWriter::WriteExtraCodeBeforeEveryMainCall() {}

void HLSCWriter::WriteMainTestbench()
{
   // get the root function to be tested by the testbench
   const auto top_function_ids = AppM->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top function");
   const auto function_id = *(top_function_ids.begin());
   const BehavioralHelperConstRef behavioral_helper = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const unsigned int return_type_index = behavioral_helper->GetFunctionReturnType(function_id);

   indented_output_stream->Append("int main()\n{\n");
   indented_output_stream->Append("unsigned int __testbench_index, __testbench_index0, __testbench_index1, __testbench_index2;\n");
   indented_output_stream->Append("__standard_exit = 0;\n");
   indented_output_stream->Append("__bambu_testbench_fp = fopen(\"" + hls_c_backend_information->results_filename + "\", \"w\");\n");
   indented_output_stream->Append("if (!__bambu_testbench_fp) {\n");
   indented_output_stream->Append("perror(\"can't open file: " + hls_c_backend_information->results_filename + "\");\n");
   indented_output_stream->Append("exit(1);\n");
   indented_output_stream->Append("}\n\n");
   // write additional initialization code needed by subclasses
   WriteExtraInitCode();
   // write C code used to print initialization values for the HDL simulator's memory
   WriteSimulatorInitMemory(function_id);
   // ---- WRITE VARIABLES DECLARATIONS ----
   // declaration of the return variable of the top function, if not void
   if(return_type_index)
   {
      std::string ret_type = behavioral_helper->print_type(return_type_index);
      if(tree_helper::is_a_vector(TM, return_type_index))
      {
         THROW_ERROR("return type of function under test " + behavioral_helper->get_function_name() + " is " + ret_type +
                     "\n"
                     "co-simulation does not support vectorized return types at top level");
      }

      indented_output_stream->Append("// return variable initialization\n");
      indented_output_stream->Append(ret_type + " " + std::string(RETURN_PORT_NAME) + ";\n");
   }
   // parameters declaration
   WriteParamDecl(behavioral_helper);
   // ---- WRITE PARAMETERS INITIALIZATION, FUNCTION CALLS AND CHECK RESULTS ----
   auto& test_vectors = hls_c_backend_information->HLSMgr->RSim->test_vectors;
   for(unsigned int v_idx = 0; v_idx < test_vectors.size(); v_idx++)
   {
      const auto& curr_test_vector = test_vectors[v_idx];
      // write parameter initialization
      indented_output_stream->Append("// parameter initialization\n");
      WriteParamInitialization(behavioral_helper, curr_test_vector, v_idx);
      WriteExtraCodeBeforeEveryMainCall();
      // write the call to the top function to be tested
      indented_output_stream->Append("// function call\n");
      WriteTestbenchFunctionCall(behavioral_helper);
      // write the expected results
      indented_output_stream->Append("// print expected results\n");
      WriteExpectedResults(behavioral_helper, curr_test_vector);
   }
   // print exit statements
   indented_output_stream->Append("__standard_exit = 1;\n");
   indented_output_stream->Append("exit(0);\n");
   indented_output_stream->Append("}\n");
}

void HLSCWriter::WriteFile(const std::string& file_name)
{
   const auto top_function_ids = AppM->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top function");
   const auto function_id = *(top_function_ids.begin());
   const BehavioralHelperConstRef behavioral_helper = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->C-based testbench generation for function " + behavioral_helper->get_function_name() + ": " + file_name);

   WriteMainTestbench();
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");

   indented_output_stream->WriteFile(file_name);
}

std::string HLSCWriter::convert_in_binary(const BehavioralHelperConstRef behavioral_helper, unsigned int base_type, const std::string& C_value, unsigned int precision)
{
   std::string trimmed_value;
   THROW_ASSERT(C_value != "", "Empty string for binary conversion");
   if(base_type && behavioral_helper->is_real(base_type)) { trimmed_value = convert_fp_to_string(C_value, precision); }
   else
   {
      long long int ll_value;
      if(C_value[0] == '\"')
      {
         trimmed_value = C_value.substr(1);
         trimmed_value = trimmed_value.substr(0, trimmed_value.find('\"'));
         if(trimmed_value[0]== '0' && trimmed_value[1]== 'b')
            trimmed_value = trimmed_value.substr(2);
         else if(trimmed_value[0]== '0' && (trimmed_value[1]== 'x' || trimmed_value[1]== 'o'))
         {
            bool is_hex = trimmed_value[1]== 'x';
            std::string initial_string = trimmed_value.substr(2);
            trimmed_value="";
            std::string hexTable[16]={"0000",
                                      "0001",
                                      "0010",
                                      "0011",
                                      "0100",
                                      "0101",
                                      "0110",
                                      "0111",
                                      "1000",
                                      "1001",
                                      "1010",
                                      "1011",
                                      "1100",
                                      "1101",
                                      "1110",
                                      "1111"};
            std::string octTable[16]={"000",
                                      "001",
                                      "010",
                                      "011",
                                      "100",
                                      "101",
                                      "110",
                                      "111"
                                     };
            for(char curChar : initial_string)
            {
               int off=0;
               if(is_hex)
               {
                  if(curChar >= '0' && curChar <= '9')
                     off = curChar - '0';
                  else if(curChar >= 'A' && curChar <= 'F')
                     off = curChar - 'A' + 10;
                  else if(curChar >= 'a' && curChar <= 'f')
                     off = curChar - 'a' + 10;
                  else
                     THROW_ERROR("unexpected char in hex string");
               }
               else
               {
                  if(curChar >= '0' && curChar <= '8')
                     off = curChar - '0';
                  else
                     THROW_ERROR("unexpected char in octal string");
               }
               trimmed_value = trimmed_value + (is_hex?hexTable[off]:octTable[off]);
            }
         }
         else
            THROW_ERROR("unsupported format");

         while(trimmed_value.size()<precision)
            trimmed_value = "0"+trimmed_value;
         while(trimmed_value.size()>precision)
            trimmed_value = trimmed_value.substr(1);
         return trimmed_value;
      }
      else if(C_value[0] == '\'')
      {
         trimmed_value = C_value.substr(1);
         THROW_ASSERT(trimmed_value.find('\'') != std::string::npos, "unxpected case");
         trimmed_value = trimmed_value.substr(0, trimmed_value.find('\''));
         if(trimmed_value[0] == '\\')
            ll_value = boost::lexical_cast<long long int>(trimmed_value.substr(1));
         else
            ll_value = boost::lexical_cast<char>(trimmed_value);
      }
      else if(base_type && behavioral_helper->is_unsigned(base_type))
      {
         std::string::size_type sz = 0;
         unsigned long long ull = std::stoull(C_value, &sz, 0);
         ll_value = static_cast<long long int>(ull);
      }
      else
      {
         std::string::size_type sz = 0;
         ll_value = std::stoll(C_value, &sz, 0);
      }
      auto ull_value = static_cast<unsigned long long int>(ll_value);
      trimmed_value = "";
      if(precision<=64)
         for(unsigned int ind = 0; ind < precision; ind++) trimmed_value = trimmed_value + (((1LLU << (precision - ind - 1)) & ull_value) ? '1' : '0');
      else
      {
         for(unsigned int ind = 0; ind < (precision-64); ind++) trimmed_value = trimmed_value + '0';
         for(unsigned int ind = 0; ind < 64; ind++) trimmed_value = trimmed_value + (((1LLU << (64 - ind - 1)) & ull_value) ? '1' : '0');
      }
   }
   return trimmed_value;
}

inline void HLSCWriter::WriteZeroedBytes(const size_t n_bytes)
{
   indented_output_stream->Append(
       "for (__testbench_index = 0; "
       "__testbench_index < " +
       STR(n_bytes) + "; " +
       "++__testbench_index)\n"
       "   fprintf(__bambu_testbench_fp, \"m00000000\\n\");\n");
}

size_t HLSCWriter::WriteBinaryMemoryInit(const std::string& binary_string, const size_t data_bitsize, std::string& bits_offset)
{
   size_t printed_bytes = 0;
   if(bits_offset.size() == 0 && is_all_8zeros(binary_string))
   {
      WriteZeroedBytes(binary_string.size() / 8);
      printed_bytes = binary_string.size() / 8;
   }
   else
   {
      std::string local_binary_string;
      size_t local_data_bitsize;
      if(bits_offset.size())
      {
         if(static_cast<int>(data_bitsize) - 8 + static_cast<int>(bits_offset.size()) >= 0)
         {
            local_data_bitsize = data_bitsize - (8 - bits_offset.size());
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"m" + binary_string.substr(data_bitsize - (8 - bits_offset.size()), 8 - bits_offset.size()) + bits_offset + "\\n\");\n");
            local_binary_string = binary_string.substr(0, local_data_bitsize);
            bits_offset = "";
            printed_bytes++;
         }
         else
         {
            local_data_bitsize = 0;
            bits_offset = binary_string + bits_offset;
         }
      }
      else
      {
         local_binary_string = binary_string;
         local_data_bitsize = data_bitsize;
      }
      for(unsigned int base_index = 0; base_index < local_data_bitsize; base_index = base_index + 8)
      {
         if((static_cast<int>(local_data_bitsize) - 8 - static_cast<int>(base_index)) >= 0)
         {
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"m" + local_binary_string.substr(local_data_bitsize - 8 - base_index, 8) + "\\n\");\n");
            printed_bytes++;
         }
         else
         {
            bits_offset = local_binary_string.substr(0, local_data_bitsize - base_index);
         }
      }
   }
   return printed_bytes;
}

bool HLSCWriter::is_all_8zeros(const std::string& str)
{
   size_t size = str.size();
   if(size % 8 != 0 || size == 8) return false;
   for(size_t i = 0; i < size; ++i)
      if(str.at(i) != '0') return false;
   return true;
}

void HLSCWriter::WriteFunctionImplementation(unsigned int)
{
   /// Do nothing
}

void HLSCWriter::WriteBuiltinWaitCall()
{
   /// Do nothing
}
