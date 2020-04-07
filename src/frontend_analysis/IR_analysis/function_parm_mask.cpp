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
#include "function_behavior.hpp"
#include "pragma_manager.hpp"

/// tree includes
#include "tree_node.hpp"
#include "tree_helper.hpp"
#include "tree_reindex.hpp"
#include "tree_manager.hpp"
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

/// XML includes used for writing and reading the configuration file
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include "bit_lattice.hpp"
#include "custom_map.hpp"
#include "Range.hpp"
#include "var_pp_functor.hpp"  // for std_var_pp_functor

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> function_parm_mask::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
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

DesignFlowStep_Status function_parm_mask::Exec()
{
   const auto TM = AppM->get_tree_manager();
   CustomMap<std::string, const function_decl*> nameToFunc;
   for(const auto f : AppM->get_functions_with_body())
   {
      const auto* fd = GetPointer<const function_decl>(TM->get_tree_node_const(f));
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->parsing " + leaf_name);
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
                  const auto fd = nameToFunc.at(fname);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter mask for function " + tree_helper::print_type(TM, fd->index, false, true, false, 0U, var_pp_functorConstRef(new std_var_pp_functor(AppM->CGetFunctionBehavior(fd->index)->CGetBehavioralHelper()))));
                  const auto f_args = fd->list_of_args;
                  CustomMap<std::string, parm_decl*> f_parms;
                  std::transform(f_args.begin(), f_args.end(), std::inserter(f_parms, f_parms.end()), 
                     [](const tree_nodeRef& tn){
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
                        std::string argName;
                        std::string bitmask;
                        std::string sign;
                        std::string exp_range;
                        std::string sig_bitwidth;
                        for(auto attrArg : EnodeArg->get_attributes())
                        {
                           const auto key = attrArg->get_name();
                           const auto value = attrArg->get_value();
                           if(key == "id")
                           {
                              argName = value;
                           }
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
                        if(argName == "" || !f_parms.contains(argName))
                        {
                           THROW_ERROR("malformed mask file: argument " + argName + " not valid");
                        }
                        auto* parm = f_parms.at(argName);
                        const auto bw = tree_helper::Size(TM->get_tree_node_const(parm->index));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->" + GET_CONST_NODE(parm->type)->get_kind_text() + "<" + STR(bw) + "> " + argName);
                        THROW_ASSERT(parm->bit_values.empty(), "Parameter bitmask should be empty (" + parm->bit_values + ")");
                        THROW_ASSERT(parm->range == nullptr, "Parameter range should be unset (" + parm->range->ToString() + ")");
                        if(EnodeArg->get_attributes().size() > 1)
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
                                    b = bit_lattice::X;
                                 }
                              }
                              for(auto i = bit_mask.size(); i < bw; ++i)
                              {
                                 bit_mask.push_front(bit_lattice::X);
                              }
                              parm->bit_values = bitstring_to_string(bit_mask);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Bitmask " + parm->bit_values);
                              // TODO: set parm->range to a valid range based on bitmask and bw
                           }
                           else
                           {
                              refcount<RealRange> range(new RealRange(RangeRef(new Range(Regular, static_cast<Range::bw_t>(bw)))));
                              auto bv_mask = create_u_bitstring(bw);
                              if(sig_bitwidth.size())
                              {
                                 const auto sig_bw = std::strtol(sig_bitwidth.data(), nullptr, 0);
                                 const auto max_sig_bw = bw == 64 ? 52U : 23U;
                                 THROW_ASSERT(sig_bw < max_sig_bw, "Restricted significand bitwidth should be less than standard bitwidth");
                                 const auto sig_mask = static_cast<int64_t>(UINT64_MAX << (max_sig_bw - sig_bw));
                                 range->setSignificand(RangeRef(new Range(Anti, static_cast<Range::bw_t>(max_sig_bw), sig_mask + 1, -1)));
                                 bv_mask = create_bitstring_from_constant(sig_mask, bw, false);
                                 for(auto& b : bv_mask)
                                 {
                                    if(b == bit_lattice::ONE)  
                                    {
                                       b = bit_lattice::U;
                                    }
                                    else if(b == bit_lattice::ZERO)
                                    {
                                       b = bit_lattice::X;
                                    }
                                 }
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Floating point significand bitwidth set to " + sig_bitwidth);
                              }
                              if(sign.size())
                              {
                                 const auto sign_val = std::strtol(sign.data(), nullptr, 0);
                                 range->setSign(RangeRef(new Range(Regular, 1, static_cast<bool>(sign_val), static_cast<bool>(sign_val))));
                                 bv_mask.pop_front();
                                 bv_mask.push_front(sign_val == 0 ? bit_lattice::ZERO : bit_lattice::ONE);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, std::string("---Floating point sign fixed as ") + (sign_val == 0 ? "positive" : "negative"));
                              }
                              if(exp_range.size())
                              {
                                 const auto separator_idx = exp_range.find(',');
                                 const auto exp_l = std::strtol(exp_range.substr(0, separator_idx).data(), nullptr, 0);
                                 const auto exp_u = std::strtol(exp_range.substr(separator_idx + 1, exp_range.size() - separator_idx - 1).data(), nullptr, 0);
                                 RangeRef e_range(new Range(Regular, bw == 64 ? 11 : 8, exp_l, exp_u));
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Floating point exponent restricted range " + e_range->sub(RangeRef(bw == 64 ? new Range(Regular, 11, 1023, 1023) : new Range(Regular, 8, 127, 127)))->ToString());
                                 range->setExponent(e_range);
                              }
                              THROW_ASSERT(bv_mask.size() == range->getBitWidth(), "Floating-point bit_values must be exact (" + parm->ToString() + ")");
                              parm->ra_bit_values = bitstring_to_string(bv_mask);
                              parm->range = range;
                              modified = true;
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Floating point bounds set to " + parm->range->ToString() + "<" + parm->ra_bit_values + ">");
                           }
                        }
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                     }
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parsed file " + leaf_name);
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function parameter mask for " + leaf_name + " not present");
      }
   }

   executed = true;
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}