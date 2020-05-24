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
 * @file translator.hpp
 * @brief Dump information read already read from profiling file to other files
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_RTL_BUILT.hpp"

/// Header include
#include "translator.hpp"

/// Constant include
#include "experimental_setup_xml.hpp"
#include "latex_table_constants.hpp"
#include "latex_table_xml.hpp"
#include "weights_xml.hpp"

/// HLS/evaluation include
#include "evaluation.hpp"

/// Parameter include
#include "Parameter.hpp"

#if HAVE_RTL_BUILT
/// RTL include
#include "rtl_common.hpp"
#include "rtl_node.hpp"
#endif

/// STD include
#include <cmath>
#include <fstream>
#include <iomanip>
#include <string>

/// STL include
#include <string>

/// Utility include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "utility.hpp"
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/operations.hpp>

/// XML include
#include "polixml.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#define SKIPPED_COLUMN ("Loop_number")("bit_expr")("comp_expr")("const_readings")("div_expr")("mult_expr")("plusminus_expr")("memory_writings")("register_accesses")("memory_readings")("register_writing")("Backward_branches")

#define SKIPPING_MACRO(r, data, elem) skipping.insert(elem);

#define TF_NAME(r, data, elem)            \
   name = "CA_" #elem;                    \
   name = name.substr(22);                \
   name = name.substr(0, name.find(')')); \
   string_to_TF[name] = BOOST_PP_CAT(TF_, elem);

#define CO_NAME(r, data, elem)            \
   name = "CO_" #elem;                    \
   name = name.substr(22);                \
   name = name.substr(0, name.find(')')); \
   string_to_CO[name] = BOOST_PP_CAT(CO_, elem);

#define TOF_NAME(r, data, elem)           \
   name = "TOF_" #elem;                   \
   name = name.substr(23);                \
   name = name.substr(0, name.find(')')); \
   string_to_TOF[name] = BOOST_PP_CAT(TOF_, elem);

const char* default_latex_format_stat = {
#include "latex_format_stat.data"
};
#if HAVE_EXPERIMENTAL
const char* latex_format_af_edges = {
#include "latex_format_af_edges.data"
};
const char* latex_format_edges_reduction = {
#include "latex_format_edges_reduction.data"
};
#endif
Translator::LatexColumnFormat::LatexColumnFormat()
    : column_alignment("c|"), text_format(LatexColumnFormat::TF_number), precision(NUM_CST_latex_table_number_precision), comparison_operator(LatexColumnFormat::CO_abs_le), total_format(LatexColumnFormat::TOF_none)
{
}

CustomUnorderedMap<std::string, Translator::LatexColumnFormat::TextFormat> Translator::LatexColumnFormat::string_to_TF;

CustomUnorderedMap<std::string, Translator::LatexColumnFormat::ComparisonOperator> Translator::LatexColumnFormat::string_to_CO;

CustomUnorderedMap<std::string, Translator::LatexColumnFormat::TotalFormat> Translator::LatexColumnFormat::string_to_TOF;

Translator::LatexColumnFormat::TextFormat Translator::LatexColumnFormat::LatexColumnFormat::get_TF(const std::string& string)
{
   if(string_to_TF.empty())
   {
      // cppcheck-suppress unusedVariable
      std::string name;
      BOOST_PP_SEQ_FOR_EACH(TF_NAME, BOOST_PP_EMPTY, TEXT_FORMAT);
   }
   THROW_ASSERT(string_to_TF.find(string) != string_to_TF.end(), "String " + string + " is not a valid Text Format");
   return string_to_TF.find(string)->second;
}

Translator::LatexColumnFormat::ComparisonOperator Translator::LatexColumnFormat::LatexColumnFormat::get_CO(const std::string& string)
{
   if(string_to_CO.empty())
   {
      // cppcheck-suppress unusedVariable
      std::string name;
      BOOST_PP_SEQ_FOR_EACH(CO_NAME, BOOST_PP_EMPTY, COMPARISON_OPERATOR);
   }
   THROW_ASSERT(string_to_CO.find(string) != string_to_CO.end(), "String " + string + " is not a valid Comparison Operator");
   return string_to_CO.find(string)->second;
}

Translator::LatexColumnFormat::TotalFormat Translator::LatexColumnFormat::LatexColumnFormat::GetTotalFormat(const std::string& string)
{
   if(string_to_TOF.empty())
   {
      // cppcheck-suppress unusedVariable
      std::string name;
      BOOST_PP_SEQ_FOR_EACH(TOF_NAME, BOOST_PP_EMPTY, TOTAL_FORMAT);
   }
   THROW_ASSERT(string_to_TOF.find(string) != string_to_TOF.end(), "String " + string + " is not a valid Total Format");
   return string_to_TOF.find(string)->second;
}

bool Translator::LatexColumnFormat::Compare(const long double A, const ComparisonOperator comparator, const long double B)
{
   switch(comparator)
   {
      case(CO_abs_le):
      {
         return fabsl(A) <= fabsl(B);
      }
      default:
      {
         THROW_UNREACHABLE("Operator " + STR(comparator) + " not supported");
      }
   }
   return false;
}

Translator::Translator(const ParameterConstRef _Param) : Param(_Param), debug_level(_Param->get_class_debug_level(GET_CLASS(*this)))
{
}

#if HAVE_RTL_BUILT
void Translator::Translate(const CustomUnorderedMap<std::string, long double> input, std::map<enum rtl_kind, std::map<enum mode_kind, long double>>& output) const
{
   CustomOrderedSet<mode_kind> int_type, float_type;

   rtl_node::get_int_modes(int_type);
   rtl_node::get_float_modes(float_type);

   CustomOrderedSet<mode_kind>::const_iterator it2, it2_end;

   CustomUnorderedMap<std::string, long double> normalization;
   if(Param->isOption("normalize_file"))
   {
      get_normalization(normalization);
   }

   CustomUnorderedMap<std::string, long double>::const_iterator it, it_end = input.end();

   long double base = 0.0;
   for(it = input.begin(); it != it_end; ++it)
   {
      if(it->first == rtl_node::GetString(base_R))
         base = it->second;
   }

   for(it = input.begin(); it != it_end; ++it)
   {
      long double normalization_value = 1.0;
      if(normalization.find(it->first) != normalization.end())
      {
         normalization_value = normalization[it->first];
      }
      std::string tag = it->first;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Translating tag " + tag);
      if(tag == "Dynamic_operations_is_main")
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "traslator: translate: dynamic_operation_is_main: getting constant distribution");
         output[is_main_R][none_R] = it->second * normalization_value + base * normalization_value;
         PRINT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "traslator: translate: dynamic_operation_is_main: constant distribution r updated, saved");
      }
      else if(tag == "Dynamic_operations_assignment")
      {
         output[insn_R][none_R] = it->second * normalization_value + base * normalization_value;
         output[call_insn_R][none_R] = it->second * normalization_value + base * normalization_value;
      }
      else if(tag == "Dynamic_operations_comp_int_expr")
      {
         output[compare_R][cc_R] = it->second * normalization_value + base * normalization_value;
         output[compare_R][ccz_R] = it->second * normalization_value + base * normalization_value;
      }
      else if(tag == "Dynamic_operations_comp_float_expr")
      {
         output[compare_R][ccfp_R] = it->second * normalization_value + base * normalization_value;
         output[compare_R][ccfpe_R] = it->second * normalization_value + base * normalization_value;
      }
      else if(tag == "Dynamic_operations_Backward_branches")
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "traslator: translate: dynamic_operation_is_Backward_braches: getting constant distribution");
         output[backward_branches_R][none_R] = it->second * normalization_value + base * normalization_value;
         PRINT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "traslator: translate: dynamic_operation_Backward_braches: constant distribution r updated, saved");
         // OLD VERSION
         // rtl_node::backward_branches[Param->getOption<std::string>(processing_element")] = it->second*normalization_value + base*normalization_value;
      }
      else if(tag == "Dynamic_operations_branch_expr")
      {
         output[jump_insn_R][none_R] = it->second * normalization_value + base * normalization_value;
         output[if_then_else_R][none_R] = it->second * normalization_value + base * normalization_value;
      }
      else if(tag == "Dynamic_operations_call_expr")
      {
         output[call_R][none_R] = it->second * normalization_value + base * normalization_value;
      }
      else if(tag == "Dynamic_operations_convert_expr")
      {
         output[fix_R][none_R] = it->second * normalization_value + base * normalization_value;
         output[float_R][none_R] = it->second * normalization_value + base * normalization_value;
         output[fract_convert_R][none_R] = it->second * normalization_value + base * normalization_value;
         output[sat_fract_R][none_R] = it->second * normalization_value + base * normalization_value;
         output[unsigned_fix_R][none_R] = it->second * normalization_value + base * normalization_value;
         output[unsigned_float_R][none_R] = it->second * normalization_value + base * normalization_value;
         output[unsigned_fract_convert_R][none_R] = it->second * normalization_value + base * normalization_value;
         output[unsigned_sat_fract_R][none_R] = it->second * normalization_value + base * normalization_value;
      }
      else if(tag == "Dynamic_operations_plusminus_int_expr")
      {
         it2_end = int_type.end();
         for(it2 = int_type.begin(); it2 != it2_end; ++it2)
         {
            output[abs_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[minus_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[neg_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[plus_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[lo_sum_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_plusminus_float_expr")
      {
         it2_end = float_type.end();
         for(it2 = float_type.begin(); it2 != it2_end; ++it2)
         {
            output[abs_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[minus_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[neg_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[plus_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_mult_int_expr")
      {
         it2_end = int_type.end();
         for(it2 = int_type.begin(); it2 != it2_end; ++it2)
         {
            output[mult_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_mult_float_expr")
      {
         it2_end = float_type.end();
         for(it2 = float_type.begin(); it2 != it2_end; ++it2)
         {
            output[mult_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_bit_int_expr")
      {
         it2_end = int_type.end();
         for(it2 = int_type.begin(); it2 != it2_end; ++it2)
         {
            output[and_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[ashift_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[ashiftrt_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[bswap_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[clz_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[ctz_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[ffs_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[float_extend_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[float_truncate_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[ior_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[lshiftrt_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[not_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[concat_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[parity_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[popcount_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[rotate_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[rotatert_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[sign_extend_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[truncate_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[xor_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[sign_extend_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[zero_extend_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_bit_float_expr")
      {
         it2_end = float_type.end();
         for(it2 = float_type.begin(); it2 != it2_end; ++it2)
         {
            output[and_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[ashiftrt_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[bswap_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[clz_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[ctz_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[ffs_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[float_extend_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[float_truncate_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[ior_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[lshiftrt_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[not_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[concat_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[parity_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[popcount_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[rotate_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[rotatert_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[sign_extend_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[truncate_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[xor_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[sign_extend_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[zero_extend_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_div_int_expr")
      {
         it2_end = int_type.end();
         for(it2 = int_type.begin(); it2 != it2_end; ++it2)
         {
            output[div_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[udiv_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_div_float_expr")
      {
         it2_end = float_type.end();
         for(it2 = float_type.begin(); it2 != it2_end; ++it2)
         {
            output[div_R][*it2] = it->second * normalization_value + base * normalization_value;
            output[udiv_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_integer_register_writing")
      {
         it2_end = int_type.end();
         for(it2 = int_type.begin(); it2 != it2_end; ++it2)
         {
            output[set_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_float_register_writing")
      {
         it2_end = float_type.end();
         for(it2 = float_type.begin(); it2 != it2_end; ++it2)
         {
            output[set_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_integer_register_accesses")
      {
         it2_end = int_type.end();
         for(it2 = int_type.begin(); it2 != it2_end; ++it2)
         {
            output[reg_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_float_register_accesses")
      {
         it2_end = float_type.end();
         for(it2 = float_type.begin(); it2 != it2_end; ++it2)
         {
            output[reg_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_integer_memory_readings")
      {
         it2_end = int_type.end();
         for(it2 = int_type.begin(); it2 != it2_end; ++it2)
         {
            output[read_mem_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_float_memory_readings")
      {
         it2_end = float_type.end();
         for(it2 = float_type.begin(); it2 != it2_end; ++it2)
         {
            output[read_mem_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_integer_memory_writings")
      {
         it2_end = int_type.end();
         for(it2 = int_type.begin(); it2 != it2_end; ++it2)
         {
            output[write_mem_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_float_memory_writings")
      {
         it2_end = float_type.end();
         for(it2 = float_type.begin(); it2 != it2_end; ++it2)
         {
            output[write_mem_R][*it2] = it->second * normalization_value + base * normalization_value;
         }
      }
      else if(tag == "Dynamic_operations_const_int_readings")
      {
         output[const_int_R][none_R] = it->second * normalization_value + base * normalization_value;
         output[high_R][none_R] = it->second * normalization_value + base * normalization_value;
      }
      else if(tag == "Dynamic_operations_const_double_readings")
      {
         output[const_double_R][none_R] = it->second * normalization_value + base * normalization_value;
      }
      else if(tag == rtl_node::GetString(base_R))
      {
         output[base_R][none_R] = it->second * normalization_value + base * normalization_value;

         PRINT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Translator: translate: BASE: constant distribution saved");
         // OLD VERSION
         // rtl_node::base[Param->getOption<std::string>("processing_element")] = it->second*normalization_value + base*normalization_value;
      }
      else
         THROW_ERROR("Found tag " + tag);
   }
}

void Translator::write_to_xml(const std::map<enum rtl_kind, std::map<enum mode_kind, long double>>& data, std::string file_name) const
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Staring writing to xml");
   xml_document document;

   xml_element* root = document.create_root_node(STR_XML_weights_root);
   xml_element* rtl_weights = root->add_child_element(STR_XML_weights_rtl);
   WRITE_XNVM2("processing_element_type", Param->getOption<std::string>(OPT_processing_element_type), rtl_weights);

   if(data.find(is_main_R) != data.end() and data.find(is_main_R)->second.find(none_R) != data.find(is_main_R)->second.end())
   {
      long double is_main_value = data.find(is_main_R)->second.find(none_R)->second;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "is_main is " + STR(is_main_value));
      xml_element* is_main = rtl_weights->add_child_element("is_main");
      WRITE_XNVM2(STR_XML_weights_cycles, STR(is_main_value), is_main);
   }
   if(data.find(backward_branches_R) != data.end() and data.find(backward_branches_R)->second.find(none_R) != data.find(backward_branches_R)->second.end())
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "Translator: write_to_xml: br is not zero");
      xml_element* backward_branches = rtl_weights->add_child_element("backward_branches");
      WRITE_XNVM2(STR_XML_weights_cycles, STR(data.find(backward_branches_R)->second.find(none_R)->second), backward_branches);
   }
   std::map<enum rtl_kind, std::map<enum mode_kind, long double>>::const_iterator it, it_end = data.end();
   for(it = data.begin(); it != it_end; ++it)
   {
      xml_element* node = rtl_weights->add_child_element(STR_XML_weights_rtx);
      WRITE_XNVM2("name", rtl_node::GetString(it->first), node);
      const std::map<enum mode_kind, long double>& internal_weights = it->second;
      std::map<enum mode_kind, long double>::const_iterator it2, it2_end = internal_weights.end();
      for(it2 = internal_weights.begin(); it2 != it2_end; ++it2)
      {
         if(it2->first == none_R)
         {
            WRITE_XNVM2(STR_XML_weights_cycles, STR(it2->second), node);
         }
         else
         {
            xml_element* mode = node->add_child_element(STR_XML_weights_mode);
            WRITE_XNVM2("name", rtl_node::GetString(it2->first), mode);
            WRITE_XNVM2(STR_XML_weights_cycles, STR(it2->second), mode);
         }
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Ended building xml structure");
   document.write_to_file_formatted(file_name);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Ended writing to xml");
}

void Translator::write_to_csv(const std::map<std::string, CustomOrderedSet<std::string>>& tags, const CustomUnorderedMap<std::string, CustomUnorderedMapStable<std::string, CustomUnorderedMapStable<std::string, long double>>>& results,
                              const std::string& file_name) const
{
   CustomUnorderedSet<std::string> skipping;
   BOOST_PP_SEQ_FOR_EACH(SKIPPING_MACRO, BOOST_PP_EMPTY, SKIPPED_COLUMN);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Translating " + STR(results.size()));
   std::ofstream out(file_name.c_str());
   THROW_ASSERT(out, "Error in opening output file " + file_name);
   out << "Benchmark ";

   CustomUnorderedMap<std::string, long double> normalization;
   if(Param->isOption(OPT_normalization_file))
   {
      get_normalization(normalization);
   }

   std::map<std::string, CustomOrderedSet<std::string>>::const_iterator it2, it2_end = tags.end();
   for(it2 = tags.begin(); it2 != it2_end; ++it2)
   {
      const CustomOrderedSet<std::string>& cat_tags = it2->second;
      CustomOrderedSet<std::string>::const_iterator it4, it4_end = cat_tags.end();
      for(it4 = cat_tags.begin(); it4 != it4_end; ++it4)
      {
         if(skipping.find(*it4) == skipping.end())
            out << ", " << it2->first << "_" << *it4;
      }
      /// writing is_main
      out << ", " << it2->first << "_is_main";
   }
   out << std::endl;
   CustomUnorderedMap<std::string, CustomUnorderedMapStable<std::string, CustomUnorderedMapStable<std::string, long double>>>::const_iterator it, it_end = results.end();
   for(it = results.begin(); it != it_end; ++it)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   Analyzing line " + it->first);
      out << it->first;
      std::map<std::string, CustomOrderedSet<std::string>>::const_iterator tag, tag_end = tags.end();
      for(tag = tags.begin(); tag != tag_end; ++tag)
      {
         long double operations = 0.0;
         const auto& bench_counters = it->second;
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "      Category " + tag->first);
         if(bench_counters.find(tag->first) != bench_counters.end())
         {
            const CustomOrderedSet<std::string>& cat_tags = tag->second;
            const CustomUnorderedMap<std::string, long double>& cat_counters = bench_counters.find(tag->first)->second;
            CustomOrderedSet<std::string>::const_iterator it4, it4_end = cat_tags.end();
            for(it4 = cat_tags.begin(); it4 != it4_end; ++it4)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "         Tag " + *it4);
               if(skipping.find(*it4) == skipping.end() && *it4 != "Average_cycles")
               {
                  std::string complete_tag = tag->first + "_" + *it4;
                  long double normalization_value = 1.0;
                  if(normalization.find(complete_tag) != normalization.end())
                  {
                     normalization_value = normalization[complete_tag];
                  }
                  if(cat_counters.find(*it4) != cat_counters.end())
                  {
                     operations += cat_counters.find(*it4)->second * normalization_value;
                  }
               }
            }
            if(normalization.find("Dynamic_operations_is_main") != normalization.end())
               operations += normalization["Dynamic_operations_is_main"];
            else
               operations += 1;
            if(tag->first == "Dynamic_operations")
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "      Operations " + STR(operations));
               out << " - Operations " << operations << " ";
            }
            for(it4 = cat_tags.begin(); it4 != it4_end; ++it4)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "         Tag " + *it4);
               if(skipping.find(*it4) == skipping.end())
               {
                  std::string complete_tag = tag->first + "_" + *it4;
                  long double normalization_value = 1.0;
                  if(normalization.find(complete_tag) != normalization.end())
                     normalization_value = normalization[complete_tag];
                  if(cat_counters.find(*it4) != cat_counters.end())
                     out << ", " << ((cat_counters.find(*it4)->second) * normalization_value) / operations;
                  else
                     out << ", 0.0";
               }
            }
         }
         /// writing is_main
         if(normalization.find("Dynamic_operations_is_main") != normalization.end())
            out << ", " << normalization["Dynamic_operations_is_main"] / operations;
         else
            out << ", " << 1 / operations;
      }
      out << std::endl;
   }
}
#endif

void Translator::write_to_csv(const std::map<std::string, CustomMap<std::string, std::string>>& results, const std::string& file_name) const
{
   std::ofstream out(file_name.c_str());
   THROW_ASSERT(out, "Error in opening output file " + file_name);
   CustomOrderedSet<std::string> column_labels;
   for(const auto row : results)
   {
      for(const auto column : row.second)
      {
         column_labels.insert(column.first);
      }
   }
   out << "Benchmark, ";
   for(const auto column_label : column_labels)
   {
      out << column_label << ", ";
   }
   out << std::endl;
   for(const auto row : results)
   {
      THROW_ASSERT(static_cast<decltype(row.second.size())>(column_labels.size()) == row.second.size(), "Lines with different number of fields " + STR(row.second.size()) + " vs. " + STR(column_labels.size()));
      out << row.first << ", ";
      for(const auto column_label : column_labels)
      {
         out << row.second.at(column_label) << ",";
      }
      out << std::endl;
   }
}

void Translator::write_to_pa(const std::map<std::string, CustomOrderedSet<std::string>>& tags, const CustomUnorderedMap<std::string, CustomUnorderedMapStable<std::string, CustomUnorderedMapStable<std::string, long double>>>& results,
                             const std::string& file_name) const
{
   std::ofstream out(file_name.c_str());
   THROW_ASSERT(out, "Error in opening output file " + file_name);
   CustomUnorderedMap<std::string, CustomUnorderedMapStable<std::string, CustomUnorderedMapStable<std::string, long double>>>::const_iterator it, it_end = results.end();
   for(it = results.begin(); it != it_end; ++it)
   {
      out << "#Benchmark " << it->first << "# ";
      std::map<std::string, CustomOrderedSet<std::string>>::const_iterator it2, it2_end = tags.end();
      for(it2 = tags.begin(); it2 != it2_end; ++it2)
      {
         out << "#" << it2->first << "# ";
         const auto& bench_counters = it->second;
         if(bench_counters.find(it2->first) != bench_counters.end())
         {
            const auto& cat_tags = it2->second;
            const auto& cat_counters = bench_counters.find(it2->first)->second;
            CustomOrderedSet<std::string>::const_iterator it4, it4_end = cat_tags.end();
            for(it4 = cat_tags.begin(); it4 != it4_end; ++it4)
            {
               out << "#" << *it4 << "# ";
               if(cat_counters.find(*it4) != cat_counters.end())
                  out << cat_counters.find(*it4)->second;
               else
                  out << "0.0";
               out << ", ";
            }
         }
      }
      out << std::endl;
   }
}

void Translator::write_to_latex(std::map<std::string, CustomMap<std::string, std::string>>& results,
                                const Parameters_FileFormat
#if HAVE_SOURCE_CODE_STATISTICS_XML
                                    ASSERT_PARAMETER(input_format)
#endif
                                        ,
                                const std::string& file_name) const
{
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
#ifndef NDEBUG
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Raw data");
      for(const auto& row : results)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->" + row.first);
         for(auto const& column : row.second)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + column.first + ":" + column.second);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
#endif
   }

   /// The maximum width of a column
   size_t max_column_width = NUM_CST_latex_table_max_column_width;

   /// The bold cells
   std::map<std::string, CustomUnorderedMapStable<std::string, bool>> bold_cells;

   /// The bambu version
   std::string bambu_version;

   /// The timestamp
   std::string timestamp;

   /// The bambu arguments
   std::string bambu_arguments;

   /// The list of benchmarks
   std::list<std::string> benchmarks;

   if(Param->isOption(OPT_experimental_setup_file))
   {
      try
      {
         XMLDomParser parser(Param->getOption<std::string>(OPT_experimental_setup_file));
         parser.Exec();

         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.

         for(const auto& root_child : node->get_children())
         {
            const auto* child = GetPointer<const xml_element>(root_child);
            if(not child)
               continue;
            if(child->get_name() == STR_XML_experimental_setup_bambu_version)
            {
               bambu_version = child->get_attribute(STR_XML_experimental_setup_value)->get_value();
            }
            else if(child->get_name() == STR_XML_experimental_setup_timestamp)
            {
               timestamp = child->get_attribute(STR_XML_experimental_setup_value)->get_value();
            }
            else if(child->get_name() == STR_XML_experimental_setup_bambu_arguments)
            {
               bambu_arguments = child->get_attribute(STR_XML_experimental_setup_value)->get_value();
            }
            else if(child->get_name() == STR_XML_experimental_setup_benchmarks)
            {
               for(const auto& benchmark : child->get_children())
               {
                  const auto* benchmark_xml = GetPointer<const xml_element>(benchmark);
                  if(not benchmark_xml)
                     continue;
                  benchmarks.push_back(benchmark_xml->get_attribute(STR_XML_experimental_setup_value)->get_value());
               }
            }
         }
      }
      catch(const char* msg)
      {
         std::cerr << msg << std::endl;
         THROW_ERROR("Error during parsing of experimental setup");
      }
      catch(const std::string& msg)
      {
         std::cerr << msg << std::endl;
         THROW_ERROR("Error during parsing of experimental setup");
      }
      catch(const std::exception& ex)
      {
         std::cout << "Exception caught: " << ex.what() << std::endl;
         THROW_ERROR("Error during parsing of experimental setup");
      }
      catch(...)
      {
         std::cerr << "unknown exception" << std::endl;
         THROW_ERROR("Error during parsing of experimental setup");
      }
   }

   /// The stream
   XMLDomParserRef parser;

#if HAVE_EXPERIMENTAL
   if(Param->isOption(OPT_evaluation) and (Param->getOption<Evaluation_Mode>(OPT_evaluation_mode) == Evaluation_Mode::EXACT and Param->isOption(OPT_evaluation_objectives)))
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Case: Estimation=EXACT");
      std::string objective_string = Param->getOption<std::string>(OPT_evaluation_objectives);
      std::vector<std::string> objective_vector = convert_string_to_vector<std::string>(objective_string, ",");
      unsigned int objectives_hits = 0;
      for(unsigned int v = 0; v < objective_vector.size(); v++)
      {
         if(objective_vector[v] == "NUM_AF_EDGES")
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Case: Objective=NUM_AF_EDGES");
            parser = XMLDomParserRef(new XMLDomParser("latex_format_af_edges.data", latex_format_af_edges));
            objectives_hits++;
         }
         else if(objective_vector[v] == "EDGES_REDUCTION")
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Case: Objective=EDGES_REDUCTION");
            parser = XMLDomParserRef(new XMLDomParser("latex_format_edges_reduction.data", latex_format_edges_reduction));
            objectives_hits++;
         }
         else if(objectives_hits == 0 and v == objective_vector.size() - 1)
            THROW_ERROR("None of the specified Estimation Objectives is compatible with the EXACT estimation method!");
      }
   }
   else
#endif
       if(Param->isOption(OPT_latex_format_file))
   {
      parser = XMLDomParserRef(new XMLDomParser(Param->getOption<std::string>(OPT_latex_format_file)));
   }
   else
   {
#if HAVE_SOURCE_CODE_STATISTICS_XML
      THROW_ASSERT(input_format == Parameters_FileFormat::FF_XML_STAT, "Input format " + STR(static_cast<int>(input_format)) + " not supported for conversion to latex table");
#endif
      parser = XMLDomParserRef(new XMLDomParser("default_latex_format_stat.data", default_latex_format_stat));
   }

   std::list<LatexColumnFormat> latex_column_formats;
   read_column_formats(parser, latex_column_formats, max_column_width);

   // The width of the column in the data section
   CustomUnorderedMap<std::string, size_t> data_width;

   std::ofstream out(file_name.c_str());

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing totals");
   /// Computing total when necessary
   CustomMap<std::string, std::string> totals;
   for(auto latex_column_format : latex_column_formats)
   {
      switch(latex_column_format.total_format)
      {
         case(LatexColumnFormat::TotalFormat::TOF_none):
            break;
         case(LatexColumnFormat::TotalFormat::TOF_average):
         {
            bool found = false;
            long double total = 0;
            for(auto line : results)
            {
               if(line.second.find(latex_column_format.source_name) != line.second.end())
               {
                  total += boost::lexical_cast<long double>(line.second[latex_column_format.source_name]);
                  found = true;
               }
            }
            if(found)
            {
               total /= results.size();
               totals[latex_column_format.source_name] = STR(total);
            }
            break;
         }
         case(LatexColumnFormat::TotalFormat::TOF_overall):
         {
            bool found = false;
            long double total = 0;
            for(auto line : results)
            {
               if(line.second.find(latex_column_format.source_name) != line.second.end())
               {
                  total += boost::lexical_cast<long double>(line.second[latex_column_format.source_name]);
                  found = true;
               }
            }
            if(found)
            {
               totals[latex_column_format.source_name] = STR(total);
            }
            break;
         }
         default:
            THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed totals");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adjusting precision");
   /// Adjusting precision
   for(const auto& column : latex_column_formats)
   {
      if(column.text_format == LatexColumnFormat::TF_number and column.precision != 0)
      {
         for(auto& line : results)
         {
            if(line.second.find(column.source_name) != line.second.end())
            {
               std::stringstream modified_string_stream;
               modified_string_stream.setf(std::ios::fixed, std::ios::floatfield);
               modified_string_stream.precision(column.precision);
               modified_string_stream << boost::lexical_cast<long double>((line.second)[column.source_name]);
               (line.second)[column.source_name] = modified_string_stream.str();
            }
         }
         if(totals.find(column.source_name) != totals.end())
         {
            std::stringstream modified_string_stream;
            modified_string_stream.setf(std::ios::fixed, std::ios::floatfield);
            modified_string_stream.precision(column.precision);
            modified_string_stream << boost::lexical_cast<long double>(totals[column.source_name]);
            totals[column.source_name] = modified_string_stream.str();
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Adjusted precision");

   // Checking for bold column
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking bold cells");
   for(auto const column : latex_column_formats)
   {
      if(column.compared_columns.size())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking column " + column.source_name);
         for(auto line : results)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking cell " + line.first);
            bool bold = true;
            const CustomUnorderedSet<std::string> columns_to_be_compared = column.compared_columns;
            CustomUnorderedSet<std::string>::const_iterator column_to_be_compared, column_to_be_compared_end = columns_to_be_compared.end();
            for(column_to_be_compared = columns_to_be_compared.begin(); column_to_be_compared != column_to_be_compared_end; ++column_to_be_compared)
            {
               if(not LatexColumnFormat::Compare(boost::lexical_cast<long double>(line.second[column.source_name]), column.comparison_operator, boost::lexical_cast<long double>(line.second[*column_to_be_compared])))
               {
                  bold = false;
                  break;
               }
            }
            if(bold)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Bold");
               bold_cells[line.first][column.source_name] = true;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked cell " + line.first);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked column " + column.source_name);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked bold cells");

   // Computing data_width
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing column width");
   for(const auto& line : results)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Reading benchmark " + line.first);
      std::string escaped_index = line.first;
      add_escape(escaped_index, "_");
      if(escaped_index.size() > data_width[(latex_column_formats.begin())->source_name])
      {
         data_width[(latex_column_formats.begin())->source_name] = escaped_index.size();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Set data width for column " + std::string((latex_column_formats.begin())->source_name) + " to " + STR(escaped_index.size()));
      }

      const auto& current_line = line.second;
      for(const auto& current_tag : current_line)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Reading column " + current_tag.first);
         if(current_tag.second.size() > data_width[current_tag.first])
         {
            data_width[current_tag.first] = current_tag.second.size();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New column width " + STR(data_width[current_tag.first]));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Read column " + current_tag.first);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Read benchmark " + line.first);
   }
   for(const auto& total : totals)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering width total for column " + total.first + ": " + STR(total.second.size()) + " vs " + STR(data_width[total.first]));
      if(total.second.size() > data_width[total.first])
      {
         data_width[total.first] = total.second.size();
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed column width");

   /// Checking if we have to use exponetial notation
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking exponential notation column width");
   for(auto& column : latex_column_formats)
   {
      if(column.text_format == LatexColumnFormat::TF_number)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + column.column_name + " " + STR(data_width[column.source_name]) + " vs " + STR(max_column_width));
      }
      if(data_width[column.source_name] > max_column_width and column.text_format == LatexColumnFormat::TF_number)
      {
         column.text_format = LatexColumnFormat::TF_exponential;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked exponential notation column width");

   // Transforming into exponential_notation
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking exponential notation");
   for(auto const column : latex_column_formats)
   {
      if(column.text_format == LatexColumnFormat::TF_exponential)
      {
         for(auto& line : results)
         {
            auto& current_line = line.second;
            if(current_line.find(column.source_name) != current_line.end())
            {
               const auto value = current_line.find(column.source_name)->second;
               current_line[column.source_name] = get_exponential_notation(value);
            }
         }
         if(totals.find(column.source_name) != totals.end())
         {
            const auto value = totals.find(column.source_name)->second;
            totals[column.source_name] = get_exponential_notation(value);
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked exponential notation");

   for(auto const& bold_line : bold_cells)
   {
      const auto& current_bold_line = bold_line.second;
      CustomUnorderedMapStable<std::string, bool>::const_iterator bold_cell, bold_cell_end = current_bold_line.end();
      for(bold_cell = current_bold_line.begin(); bold_cell != bold_cell_end; ++bold_cell)
      {
         std::string before_bold = results[bold_line.first][bold_cell->first];
         results[bold_line.first][bold_cell->first] = "\\textbf{" + before_bold + "}";
      }
   }

   /// Recomputing column width
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Recomputing column width");
   for(auto const& column : latex_column_formats)
   {
      if(column.text_format == LatexColumnFormat::TF_exponential or column.compared_columns.size())
      {
         data_width[column.source_name] = 0;
         for(auto const line : results)
         {
            if(line.second.find(column.source_name) != line.second.end() and line.second.find(column.source_name)->second.size() > data_width[column.source_name])
               data_width[column.source_name] = line.second.find(column.source_name)->second.size();
         }
         data_width[column.source_name] += 4;
      }
      else if(column.text_format == LatexColumnFormat::TF_number)
      {
         data_width[column.source_name] += 4;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New size of " + std::string(column.source_name) + ": " + STR(data_width[column.source_name]));
      }
   }
   for(auto const& column : latex_column_formats)
   {
      if(column.column_name.size() > data_width[column.source_name])
         data_width[column.source_name] = column.column_name.size();
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Recomputed column width");

   // Writing file
   // Opening tabular
   // cppcheck-suppress duplicateExpression
   if(bambu_version != "" or timestamp != "" or bambu_arguments != "")
   {
      out << "%Generated";
      if(timestamp != "")
      {
         out << " at " << timestamp;
      }
      if(bambu_version != "")
      {
         out << " with bambu " << bambu_version;
      }
      if(bambu_arguments != "")
      {
         out << " with arguments " << bambu_arguments;
      }
      out << std::endl;
   }
   out << "\\begin{tabular}{|";
   for(auto const& column : latex_column_formats)
   {
      out << column.column_alignment;
   }
   out << "}\n";
   out << "\\hline\n";

   // Writing the header
   bool first_column = true;
   for(auto column : latex_column_formats)
   {
      if(not first_column)
         out << " & ";
      first_column = false;
      std::string& column_name = column.column_name;
      if(column_name.size() < data_width[column.source_name])
         column_name.resize(data_width[column.source_name], ' ');
      out << column_name;
   }
   out << " \\\\" << std::endl;
   out << "\\hline" << std::endl;
   for(auto const line : results)
   {
      first_column = true;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Printing line for benchmark " + line.first);
      for(auto const column : latex_column_formats)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Printing column " + column.column_name + " (" + column.source_name + ")");
         if(not first_column)
            out << " & ";
         if(column.text_format == LatexColumnFormat::TF_number or column.text_format == LatexColumnFormat::TF_exponential)
         {
            out << "$ ";
         }
         std::string value;
         if(first_column)
         {
            value = line.first;
         }
         else if(line.second.find(column.source_name) != line.second.end())
         {
            value = line.second.find(column.source_name)->second;
         }
         add_escape(value, "_");
         size_t numerical_delimiters_size = (column.text_format == LatexColumnFormat::TF_number or column.text_format == LatexColumnFormat::TF_exponential) ? 4 : 0;
         if(value.size() + numerical_delimiters_size < data_width[column.source_name])
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Resize to " + STR(data_width[column.source_name]));
            value.resize(data_width[column.source_name] - numerical_delimiters_size, ' ');
         }
         out << value;
         if(column.text_format == LatexColumnFormat::TF_number or column.text_format == LatexColumnFormat::TF_exponential)
         {
            out << " $";
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Printed column " + column.column_name);
         first_column = false;
      }
      out << " \\\\" << std::endl;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Printed line for benchmark " + line.first);
   }

   CustomOrderedSet<LatexColumnFormat::TotalFormat> totals_to_be_written;

   /// Checking if we have to print average line
   for(const auto& latex_column_format : latex_column_formats)
   {
      switch(latex_column_format.total_format)
      {
         case(LatexColumnFormat::TotalFormat::TOF_average):
         case(LatexColumnFormat::TotalFormat::TOF_overall):
         {
            totals_to_be_written.insert(latex_column_format.total_format);
            break;
         }
         case(LatexColumnFormat::TotalFormat::TOF_none):
         {
            break;
         }
         default:
         {
            THROW_UNREACHABLE("");
         }
      }
   }
   for(const auto& line_to_be_written : totals_to_be_written)
   {
      out << "\\hline" << std::endl;
      first_column = true;
      for(const auto& column : latex_column_formats)
      {
         if(not first_column)
            out << " & ";
         if(column.text_format == LatexColumnFormat::TF_number or column.text_format == LatexColumnFormat::TF_exponential)
         {
            out << "$ ";
         }
         std::string value;
         if(first_column)
         {
            switch(line_to_be_written)
            {
               case(LatexColumnFormat::TotalFormat::TOF_average):
               {
                  value = "Average";
                  break;
               }
               case(LatexColumnFormat::TotalFormat::TOF_overall):
               {
                  value = "Overall";
                  break;
               }
               case(LatexColumnFormat::TotalFormat::TOF_none):
               {
                  THROW_UNREACHABLE("");
                  break;
               }
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
         }
         else if(column.total_format == line_to_be_written and totals.find(column.source_name) != totals.end())
         {
            value = totals.find(column.source_name)->second;
         }
         add_escape(value, "_");
         size_t numerical_delimiters_size = (column.text_format == LatexColumnFormat::TF_number or column.text_format == LatexColumnFormat::TF_exponential) ? 4 : 0;
         if(value.size() + numerical_delimiters_size < data_width[column.source_name])
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Resize to " + STR(data_width[column.source_name]));
            value.resize(data_width[column.source_name] - numerical_delimiters_size, ' ');
         }
         out << value;
         if(column.text_format == LatexColumnFormat::TF_number or column.text_format == LatexColumnFormat::TF_exponential)
         {
            out << " $";
         }
         first_column = false;
      }
      out << " \\\\" << std::endl;
   }

   // Closing tabular
   out << "\\hline" << std::endl;
   out << "\\end{tabular}\n";
   if(not benchmarks.empty())
   {
      out << "%Benchmarks:" << std::endl;
      for(const auto& benchmark : benchmarks)
      {
         out << "%" << benchmark << std::endl;
      }
   }
}

void Translator::merge_pa(const std::map<std::string, CustomOrderedSet<std::string>>& tags, const CustomUnorderedMap<std::string, CustomOrderedSet<std::string>>& keys,
                          const CustomUnorderedMap<std::string, CustomUnorderedMapStable<std::string, CustomUnorderedMapStable<std::string, long double>>>& input_data,
                          const CustomUnorderedMap<std::string, CustomUnorderedMapStable<std::string, CustomUnorderedMapStable<std::string, long double>>>& merge_data,
                          CustomUnorderedMap<std::string, CustomUnorderedMapStable<std::string, CustomUnorderedMapStable<std::string, long double>>>& output_data) const
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting merging");
   output_data = input_data;
   CustomUnorderedMap<std::string, CustomUnorderedMapStable<std::string, CustomUnorderedMapStable<std::string, long double>>>::const_iterator it, it_end = input_data.end();
   for(it = input_data.begin(); it != it_end; ++it)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Considering input " + it->first);
      const auto& cat = it->second;
      CustomUnorderedMapStable<std::string, CustomUnorderedMapStable<std::string, long double>>::const_iterator it2, it2_end = cat.end();
      for(it2 = cat.begin(); it2 != it2_end; ++it2)
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Considering category " + it2->first);
         if(tags.find(it2->first) != tags.end())
         {
            CustomOrderedSet<std::string>::const_iterator it3, it3_end = tags.find(it2->first)->second.end();
            for(it3 = tags.find(it2->first)->second.begin(); it3 != it3_end; ++it3)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Considering tag " + *it3);
               if(keys.find(it2->first) != keys.end())
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Considering key category " + it2->first);
                  if(merge_data.find(it->first) != merge_data.end())
                  {
                     const auto& benchmark_merge_data = merge_data.find(it->first)->second;
                     if(benchmark_merge_data.find(it2->first) != benchmark_merge_data.end())
                     {
                        const auto& cat_merge_data = benchmark_merge_data.find(it2->first)->second;
                        if(cat_merge_data.find(*it3) != cat_merge_data.end())
                        {
                           output_data[it->first][it2->first][*it3] = cat_merge_data.find(*it3)->second;
                           PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Replacing " + it->first + "." + it2->first);
                        }
                     }
                  }
               }
            }
         }
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Ended merging");
}

void Translator::get_normalization(CustomUnorderedMap<std::string, long double>& normalization) const
{
   try
   {
      XMLDomParser parser(Param->getOption<std::string>(OPT_normalization_file));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "File exists");
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.

         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Got Root");
         // Recurse through child nodes:
         const xml_node::node_list list = node->get_children();
         for(const auto& iter : list)
         {
            const auto* feature = GetPointer<const xml_element>(iter);
            if(!feature)
               continue;
            if(feature->get_name() == "feature")
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Read a new feature");
               long double value;
               std::string name;
               LOAD_XVM(name, feature);
               LOAD_XVM(value, feature);
               normalization[name] = value;
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Read feature normalization" + name + " " + STR(value));
            }
         }
      }
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
      THROW_ERROR("Error during parsing of normalization file");
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
      THROW_ERROR("Error during parsing of normalization file");
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
      THROW_ERROR("Error during parsing of normalization file");
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
      THROW_ERROR("Error during parsing of normalization file");
   }
}

void Translator::replace_underscore(std::string& ioString)
{
   boost::algorithm::replace_all(ioString, "_", " ");
}

std::string Translator::get_exponential_notation(const std::string& input) const
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   Get exponential notation of " + input);
   std::ostringstream output;
   output << std::setiosflags(std::ios::scientific);
   output << std::setprecision(Param->getOption<int>(OPT_precision));
   output << boost::lexical_cast<long double>(input);
   std::string result = output.str();
   std::string mantissa = result.substr(0, result.find('e'));
   std::string exponent = result.substr(result.find('e') + 1);
   std::string::size_type pos = 1;
   while(exponent[pos] == '0')
      pos++;
   if(pos != exponent.size())
   {
      if(exponent[0] == '-')
         exponent = exponent[0] + exponent.substr(pos);
      else
         exponent = exponent.substr(pos);
   }
   else
      exponent = "0";
   return mantissa + " \\cdot 10^{" + exponent + "}";
}

void Translator::read_column_formats(const XMLDomParserRef parser, std::list<LatexColumnFormat>& latex_column_formats, size_t& max_column_size) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Read column formats");
   parser->Exec();
   xml_element* root = parser->get_document()->get_root_node();
   THROW_ASSERT(root->get_name() == STR_XML_latex_table_root, "XML root node not correct: " + root->get_name());
   if(CE_XVM(max_column_size, root))
      max_column_size = boost::lexical_cast<size_t>(LOAD_XVM(max_column_size, root));
   const xml_node::node_list list = root->get_children();
   xml_node::node_list::const_iterator child, child_end = list.end();
   for(child = list.begin(); child != child_end; ++child)
   {
      const auto* child_element = GetPointer<const xml_element>(*child);
      if(!child_element)
         continue;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Reading information about new column");
      THROW_ASSERT(child_element->get_name() == STR_XML_latex_table_column, "Child not known: " + child_element->get_name());
      LatexColumnFormat latex_column_format;
      const std::string column_name = child_element->get_attribute(STR_XML_latex_table_column_name)->get_value();
      latex_column_format.column_name = column_name;
      const xml_node::node_list fields = child_element->get_children();
      xml_node::node_list::const_iterator field, field_end = fields.end();
      for(field = fields.begin(); field != field_end; ++field)
      {
         const auto* field_element = GetPointer<const xml_element>(*field);
         if(!field_element)
            continue;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Read field " + field_element->get_name());
         if(field_element->get_name() == STR_XML_latex_table_alignment)
         {
            const std::string alignment = field_element->get_attribute(STR_XML_latex_table_value)->get_value();
            latex_column_format.column_alignment = alignment;
         }
         else if(field_element->get_name() == STR_XML_latex_table_text_format)
         {
            const std::string text_format = field_element->get_attribute(STR_XML_latex_table_value)->get_value();
            latex_column_format.text_format = LatexColumnFormat::get_TF(text_format);
         }
         else if(field_element->get_name() == STR_XML_latex_table_source_name)
         {
            const std::string source_name = field_element->get_attribute(STR_XML_latex_table_value)->get_value();
            latex_column_format.source_name = source_name;
         }
         else if(field_element->get_name() == STR_XML_latex_table_precision)
         {
            const std::string precision = field_element->get_attribute(STR_XML_latex_table_value)->get_value();
            latex_column_format.precision = boost::lexical_cast<std::streamsize>(precision);
         }
         else if(field_element->get_name() == STR_XML_latex_table_comparison)
         {
            const std::string values = field_element->get_attribute(STR_XML_latex_table_value)->get_value();
            std::vector<std::string> splitted = SplitString(values, ",");
            for(const auto& column : splitted)
            {
               latex_column_format.compared_columns.insert(column);
            }
            if(field_element->get_attribute(STR_XML_latex_table_operator))
            {
               const std::string comparison_operator = field_element->get_attribute(STR_XML_latex_table_operator)->get_value();
               latex_column_format.comparison_operator = LatexColumnFormat::get_CO(comparison_operator);
            }
         }
         else if(field_element->get_name() == STR_XML_latex_table_total)
         {
            const std::string total = field_element->get_attribute(STR_XML_latex_table_value)->get_value();
            latex_column_format.total_format = LatexColumnFormat::GetTotalFormat(total);
         }
         else
         {
            THROW_ERROR("Field " + field_element->get_name() + " not supported");
         }
      }
      latex_column_formats.push_back(latex_column_format);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Read information about new column");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Read " + STR(latex_column_formats.size()) + " column format");
}
