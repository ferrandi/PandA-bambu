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
 *              Copyright (C) 2018-2020 Politecnico di Milano
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
 * @file function_parm_mask.hpp
 * @brief Restructure the top level function to adhere the specified interface.
 *
 * @author Michele Fiorito <michele2.fiorito@mail.polimi.it>
 *
 */

#include "function_parm_mask.hpp"

///. include
#include "Parameter.hpp"
#include "refcount.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "pragma_manager.hpp"

/// tree includes
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// XML includes used for writing and reading the configuration file
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include "Range.hpp"
#include "bit_lattice.hpp"
#include "custom_map.hpp"
#include "var_pp_functor.hpp" // for std_var_pp_functor

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> function_parm_mask::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         if(parameters->isOption(OPT_soft_float) and parameters->getOption<bool>(OPT_soft_float))
         {
            relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, ALL_FUNCTIONS));
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool function_parm_mask::executed = false;
bit_lattice function_parm_mask::dc = bit_lattice::ZERO;

function_parm_mask::function_parm_mask(const application_managerRef AM, const DesignFlowManagerConstRef dfm, const ParameterConstRef par) : ApplicationFrontendFlowStep(AM, FUNCTION_PARM_MASK, dfm, par)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

function_parm_mask::~function_parm_mask() = default;

void function_parm_mask::Initialize()
{
   ApplicationFrontendFlowStep::Initialize();
}

bool function_parm_mask::HasToBeExecuted() const
{
   return !executed;
}

std::pair<std::string, RangeRef> function_parm_mask::tagDecode(const attribute_sequence::attribute_list& attributes, Range::bw_t bw) const
{
   std::string bitmask;
   std::string sign;
   std::string exp_range;
   std::string sig_bitwidth;
   for(auto attrArg : attributes)
   {
      const auto key = attrArg->get_name();
      const auto value = attrArg->get_value();
      if(key == "bitmask")
      {
         bitmask = value;
      }
      if(key == "sign")
      {
         sign = value;
      }
      if(key == "exp_range")
      {
         exp_range = value;
      }
      if(key == "sig_bitwidth")
      {
         sig_bitwidth = value;
      }
   }
   std::string bit_values;
   RangeRef range;
   if(attributes.size() > 1)
   {
      if(bitmask.size())
      {
         auto bit_mask = create_bitstring_from_constant(std::strtoll(bitmask.data(), nullptr, 0), bw, false);
         for(auto& b : bit_mask)
         {
            if(b == bit_lattice::ONE)
            {
               b = bit_lattice::U;
            }
            else if(b == bit_lattice::ZERO)
            {
               b = dc;
            }
         }
         for(auto i = bit_mask.size(); i < bw; ++i)
         {
            bit_mask.push_front(dc);
         }
         bit_values = bitstring_to_string(bit_mask);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Bitmask " + bit_values);
         // TODO: set range to a valid range based on bitmask and bw
      }
      else
      {
         refcount<RealRange> rr(new RealRange(RangeRef(new Range(Regular, bw))));
         auto bv_mask = create_u_bitstring(bw);
         if(sig_bitwidth.size())
         {
            const auto sig_bw = std::strtol(sig_bitwidth.data(), nullptr, 0);
            const auto max_sig_bw = bw == 64 ? 52U : 23U;
            THROW_ASSERT(sig_bw < max_sig_bw, "Restricted significand bitwidth should be less than standard bitwidth");
            const auto sig_mask = static_cast<int64_t>(UINT64_MAX << (max_sig_bw - sig_bw));
            rr->setSignificand(RangeRef(new Range(Anti, static_cast<Range::bw_t>(max_sig_bw), sig_mask + 1, -1)));
            bv_mask = create_bitstring_from_constant(sig_mask, bw, false);
            for(auto& b : bv_mask)
            {
               if(b == bit_lattice::ONE)
               {
                  b = bit_lattice::U;
               }
               else if(b == bit_lattice::ZERO)
               {
                  b = dc;
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Floating-point significand bitwidth set to " + sig_bitwidth);
         }
         if(sign.size())
         {
            const auto sign_val = std::strtol(sign.data(), nullptr, 0);
            rr->setSign(RangeRef(new Range(Regular, 1, static_cast<bool>(sign_val), static_cast<bool>(sign_val))));
            bv_mask.pop_front();
            bv_mask.push_front(sign_val == 0 ? bit_lattice::ZERO : bit_lattice::ONE);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, std::string("---Floating-point sign fixed as ") + (sign_val == 0 ? "positive" : "negative"));
         }
         if(exp_range.size())
         {
            const auto separator_idx = exp_range.find(',');
            const auto exp_l = std::strtol(exp_range.substr(0, separator_idx).data(), nullptr, 0);
            const auto exp_u = std::strtol(exp_range.substr(separator_idx + 1, exp_range.size() - separator_idx - 1).data(), nullptr, 0);
            RangeRef e_range(new Range(Regular, bw == 64 ? 11 : 8, exp_l, exp_u));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Floating-point exponent restricted range " + e_range->sub(RangeRef(bw == 64 ? new Range(Regular, 11, 1023, 1023) : new Range(Regular, 8, 127, 127)))->ToString());
            rr->setExponent(e_range);
         }
         THROW_ASSERT(bv_mask.size() == rr->getBitWidth(), "Floating-point bit_values must be exact");
         bit_values = bitstring_to_string(bv_mask);
         range = rr;
      }
   }
   return {bit_values, range};
}

bool function_parm_mask::fullFunctionMask(function_decl* fd, const function_parm_mask::funcMask& fm) const
{
   const auto TM = AppM->get_tree_manager();

   Range::bw_t typeBW = 0;
   // Gather valid function parameters to mask
   std::vector<tree_nodeRef> maskParms;
   const auto f_args = fd->list_of_args;
   for(const auto& parmNode : f_args)
   {
      if(tree_helper::is_real(TM, GET_INDEX_CONST_NODE(parmNode)))
      {
         maskParms.push_back(parmNode);
         const auto parmBW = static_cast<Range::bw_t>(tree_helper::Size(GET_NODE(parmNode)));
         if(typeBW)
         {
            THROW_ASSERT(typeBW == parmBW, "All real parameters should have same size when using full function mask");
         }
         else
         {
            typeBW = parmBW;
         }
      }
   }
   // Check if return value may be masked
   bool retMask = false;
   const auto retType = tree_helper::GetFunctionReturnType(TM->get_tree_node_const(fd->index));
   if(retType != nullptr && tree_helper::is_real(TM, retType->index))
   {
      retMask = true;
      const auto retBW = static_cast<Range::bw_t>(tree_helper::Size(retType));
      if(typeBW)
      {
         THROW_ASSERT(typeBW == retBW, "Return type should have same size of parameters when using full function mask");
      }
      else
      {
         typeBW = retBW;
      }
   }

   // Abort if no real type value is present
   if(!typeBW)
   {
      return false;
   }
   THROW_ASSERT(typeBW == 32 || typeBW == 64, "");

   // Decode function mask
   refcount<RealRange> rr(new RealRange(RangeRef(new Range(Regular, typeBW))));
   std::deque<bit_lattice> bv;
   if(fm.sign != bit_lattice::U)
   {
      const auto sign = fm.sign == bit_lattice::ONE ? 1 : 0;
      rr->setSign(RangeRef(new Range(Regular, 1, sign, sign)));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, std::string("---Floating-point sign fixed as ") + (sign == 0 ? "positive" : "negative"));
   }
   if((fm.exp_u - fm.exp_l) < (typeBW == 64 ? 2047 : 255))
   {
      const auto range_fix = typeBW == 64 ? 1023 : 127;
      RangeRef e_range(new Range(Regular, typeBW == 64 ? 11 : 8, fm.exp_l + range_fix, fm.exp_u + range_fix));
      rr->setExponent(e_range);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Floating-point exponent restricted range " + e_range->sub(RangeRef(typeBW == 64 ? new Range(Regular, 11, 1023, 1023) : new Range(Regular, 8, 127, 127)))->ToString());
   }
   if(fm.m_bits < (typeBW == 64 ? 52 : 23))
   {
      const auto sig_mask = static_cast<int64_t>(UINT64_MAX << ((typeBW == 64 ? 52 : 23) - fm.m_bits));
      rr->setSignificand(RangeRef(new Range(Anti, static_cast<Range::bw_t>((typeBW == 64 ? 52 : 23)), sig_mask + 1, -1)));
      bv = create_bitstring_from_constant(sig_mask, typeBW, false);
      for(auto& b : bv)
      {
         if(b == bit_lattice::ONE)
         {
            b = bit_lattice::U;
         }
         else if(b == bit_lattice::ZERO)
         {
            b = dc;
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Floating-point significand bitwidth set to " + STR(+fm.m_bits));
   }

   // Skip if function mask is useless
   if(rr->isFullSet() && bv.empty())
   {
      return false;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "Full mask for function " + tree_helper::print_type(TM, fd->index, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(AppM->CGetFunctionBehavior(fd->index)->CGetBehavioralHelper()))));
   const auto bv_str = bitstring_to_string(bv);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Floating-point bounds set to " + rr->ToString() + "<" + bv_str + "> on the following: ");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   for(const auto& parmNode : maskParms)
   {
      auto* parm = GetPointer<parm_decl>(GET_NODE(parmNode));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter " + GET_CONST_NODE(parm->type)->get_kind_text() + "<" + STR(typeBW) + "> " + GetPointer<const identifier_node>(GET_CONST_NODE(parm->name))->strg);
      parm->bit_values = bv_str;
      parm->range = RangeRef(rr->clone());
   }
   if(retMask)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Return value " + retType->get_kind_text() + "<" + STR(typeBW) + ">");
      fd->bit_values = bv_str;
      fd->range = RangeRef(rr->clone());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return true;
}

DesignFlowStep_Status function_parm_mask::Exec()
{
   const auto TM = AppM->get_tree_manager();
   CustomMap<std::string, funcMask> funcMasks;
   auto opts = SplitString(parameters->getOption<std::string>(OPT_mask), ",");

   if(opts.size() && opts.front().front() == 'X')
   {
      dc = bit_lattice::X;
      opts[0] = std::string();
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Bit value mask don't care symbol: " + std::string(dc == bit_lattice::X ? "X" : "0"));

   for(const auto& fMask : opts)
   {
      if(fMask.empty())
      {
         continue;
      }
      const auto mask = SplitString(fMask, "*");
      if(mask.size() < 5 || mask[0].empty())
      {
         THROW_ERROR("Incorrect mask parameter format: <func_name>*<sign>*<exp_l>*<exp_u>*<m_bits>");
      }

      funcMask m;
      m.sign = mask[1] == "0" ? bit_lattice::ZERO : (mask[1] == "1" ? bit_lattice::ONE : bit_lattice::U);
      m.exp_l = static_cast<int16_t>(strtol(mask[2].data(), nullptr, 10));
      m.exp_u = static_cast<int16_t>(strtol(mask[3].data(), nullptr, 10));
      m.m_bits = static_cast<uint8_t>(strtoul(mask[4].data(), nullptr, 10));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Full mask required for function " + mask[0] + ": sign<" + bitstring_to_string({m.sign}) + ">, exp[" + STR(m.exp_l) + "," + STR(m.exp_u) + "], significand<" + STR(+m.m_bits) + ">");

      funcMasks.insert(std::make_pair<>(mask[0], std::move(m)));
   }

   CustomMap<std::string, function_decl*> nameToFunc;
   for(const auto f : AppM->CGetCallGraphManager()->GetReachedBodyFunctions())
   {
      auto* fd = GetPointer<function_decl>(TM->GetTreeNode(f));
      std::string fname;
      tree_helper::get_mangled_fname(fd, fname);
      nameToFunc.insert(std::make_pair(fname, fd));
   }

   bool modified = false;
   const std::string output_temporary_directory = parameters->getOption<std::string>(OPT_output_temporary_directory);
   for(const auto source_file : AppM->input_files)
   {
      std::string leaf_name = source_file.second == "-" ? "stdin-" : GetLeafFileName(source_file.second);
      auto XMLfilename = output_temporary_directory + leaf_name + ".mask.xml";
      if(boost::filesystem::exists(boost::filesystem::path(XMLfilename)))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "parsing " + leaf_name);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         XMLDomParser parser(XMLfilename);
         parser.Exec();
         if(parser)
         {
            // Walk the tree:
            const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
            for(const auto& iter : node->get_children())
            {
               const auto* Enode = GetPointer<const xml_element>(iter);
               if(!Enode)
               {
                  continue;
               }
               if(Enode->get_name() == "function")
               {
                  std::string fname;
                  for(auto attr : Enode->get_attributes())
                  {
                     std::string key = attr->get_name();
                     std::string value = attr->get_value();
                     if(key == "id")
                        fname = value;
                  }

                  if(fname == "")
                  {
                     THROW_ERROR("malformed mask file: unnamed function");
                  }
                  else if(!nameToFunc.contains(fname))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Function " + fname + " has no body, skipping...");
                     continue;
                  }
                  auto* fd = nameToFunc.at(fname);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "Parameter mask for function " + tree_helper::print_type(TM, fd->index, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(AppM->CGetFunctionBehavior(fd->index)->CGetBehavioralHelper()))));
                  const auto f_args = fd->list_of_args;
                  CustomMap<std::string, parm_decl*> f_parms;
                  std::transform(f_args.begin(), f_args.end(), std::inserter(f_parms, f_parms.end()), [](const tree_nodeRef& tn) {
                     auto* parm_dec = GetPointer<parm_decl>(GET_NODE(tn));
                     return std::make_pair(GetPointer<const identifier_node>(GET_CONST_NODE(parm_dec->name))->strg, parm_dec);
                  });

                  for(const auto& iterArg : Enode->get_children())
                  {
                     const auto* EnodeArg = GetPointer<const xml_element>(iterArg);
                     if(!EnodeArg)
                     {
                        continue;
                     }
                     if(EnodeArg->get_name() == "arg")
                     {
                        const auto id = EnodeArg->get_attribute("id");
                        if(id == nullptr || !f_parms.count(id->get_value()))
                        {
                           THROW_ERROR("malformed mask file: argument not valid");
                        }
                        const auto argName = id->get_value();
                        auto* parm = f_parms.at(argName);
                        const auto bw = tree_helper::Size(TM->get_tree_node_const(parm->index));
                        THROW_ASSERT(parm->bit_values.empty(), "Parameter bitmask should be empty (" + parm->bit_values + ")");
                        THROW_ASSERT(parm->range == nullptr, "Parameter range should be unset (" + parm->range->ToString() + ")");

                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->" + GET_CONST_NODE(parm->type)->get_kind_text() + "<" + STR(bw) + "> " + argName);
                        const auto bit_values_range = tagDecode(EnodeArg->get_attributes(), static_cast<Range::bw_t>(bw));
                        auto bit_values = bit_values_range.first;
                        auto range = bit_values_range.second;
                        if(range == nullptr)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                           continue;
                        }
                        parm->bit_values = bit_values;
                        parm->range = range;
                        modified = true;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Floating-point bounds set to " + parm->range->ToString() + "<" + parm->bit_values + ">");
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                     }
                  }
                  const auto retType = tree_helper::GetFunctionReturnType(TM->get_tree_node_const(fd->index));
                  const auto retBW = tree_helper::Size(retType);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->" + retType->get_kind_text() + "<" + STR(retBW) + "> return value");
                  auto bit_values_range = tagDecode(Enode->get_attributes(), static_cast<Range::bw_t>(retBW));
                  auto bit_values = bit_values_range.first;
                  auto range = bit_values_range.second;
                  if(range != nullptr)
                  {
                     THROW_ASSERT(fd->bit_values.empty(), "Return value bitmask should be empty (" + fd->bit_values + ")");
                     THROW_ASSERT(fd->range == nullptr, "Return value range should be unset (" + fd->range->ToString() + ")");
                     fd->bit_values = bit_values;
                     fd->range = range;
                     modified = true;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Floating-point bounds set to " + fd->range->ToString() + "<" + fd->bit_values + ">");
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "parsed file " + leaf_name);
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function parameter mask for " + leaf_name + " not present");
      }
   }

   const auto maskAll = funcMasks.find("@");
   if(maskAll != funcMasks.end())
   {
      const auto mask = maskAll->second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Global full function mask required");
      for(const auto& nameFunc : nameToFunc)
      {
         THROW_ASSERT(nameFunc.second, "");
         modified |= fullFunctionMask(nameFunc.second, mask);
      }
      funcMasks.erase(maskAll);
   }

   for(const auto& fm : funcMasks)
   {
      const auto f = nameToFunc.find(fm.first);
      if(f == nameToFunc.end())
      {
         THROW_ERROR("Required function not found: " + fm.first);
      }
      modified |= fullFunctionMask(f->second, fm.second);
   }

   executed = true;
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
