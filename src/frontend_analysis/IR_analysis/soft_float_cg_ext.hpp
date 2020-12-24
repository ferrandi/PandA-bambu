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
 *              Copyright (C) 2004-2021 Politecnico di Milano
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
 * @file soft_float_cg_ext.hpp
 * @brief Step that extends the call graph with the soft-float calls where appropriate.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef SOFT_FLOAT_CG_EXT_HPP
#define SOFT_FLOAT_CG_EXT_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

#include "bit_lattice.hpp"
#include "call_graph.hpp"
#include "function_behavior.hpp"
#include "tree_node.hpp"

/// Utility include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"
#include <tuple>
#include <vector>

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(soft_float_cg_ext);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_node);
//@}

class FloatFormat
{
 public:
   uint8_t frac_bits;
   uint8_t exp_bits;
   int32_t exp_bias;
   bool has_rounding;
   bool has_nan;
   bool has_one;
   bool has_subnorm;
   bit_lattice sign;

   FloatFormat(uint8_t frac_bits, uint8_t exp_bits, int32_t exp_bias, bool round = true, bool has_nan = true, bool has_one = true, bool has_subnorm = false, bit_lattice sign = bit_lattice::U)
       : frac_bits(frac_bits), exp_bits(exp_bits), exp_bias(exp_bias), has_rounding(round), has_nan(has_nan), has_one(has_one), has_subnorm(has_subnorm), sign(sign)
   {
   }

   bool operator==(const FloatFormat& other) const
   {
      return std::tie(exp_bits, frac_bits, exp_bias, has_rounding, has_nan, has_one, has_subnorm, sign) == std::tie(other.exp_bits, other.frac_bits, other.exp_bias, other.has_rounding, other.has_nan, other.has_one, other.has_subnorm, other.sign);
   }

   bool operator!=(const FloatFormat& other) const
   {
      return std::tie(exp_bits, frac_bits, exp_bias, has_rounding, has_nan, has_one, has_subnorm, sign) != std::tie(other.exp_bits, other.frac_bits, other.exp_bias, other.has_rounding, other.has_nan, other.has_one, other.has_subnorm, other.sign);
   }

   std::string mngl() const
   {
      return "e" + STR(exp_bits) + "m" + STR(frac_bits) + "b" + ((exp_bias < 0) ? ("_" + STR(-exp_bias)) : STR(exp_bias)) + (has_rounding ? "r" : "") + (has_nan ? "n" : "") + (has_one ? "h" : "") + (has_subnorm ? "s" : "") +
             (sign != bit_lattice::U ? bitstring_to_string({sign}) : "");
   }
};

struct FloatFormatRefCompare
{
   bool operator()(const FloatFormat* lhs, const FloatFormat* rhs)
   {
      return std::tie(lhs->exp_bits, lhs->frac_bits, lhs->exp_bias, lhs->has_rounding, lhs->has_nan, lhs->has_one, lhs->has_subnorm, lhs->sign) <
             std::tie(rhs->exp_bits, rhs->frac_bits, rhs->exp_bias, rhs->has_rounding, rhs->has_nan, rhs->has_one, rhs->has_subnorm, rhs->sign);
   }
};

class FunctionVersion
{
 public:
   // Id of reference function
   const CallGraph::vertex_descriptor function_vertex;

   // Float format required from the user
   FloatFormat* const userRequired;

   // True if all function callers share this function float format
   bool internal;

   // std::set<FloatFormat*, FloatFormatRefCompare> callersFF;

   FunctionVersion() : function_vertex(nullptr), userRequired(nullptr), internal(true)
   {
   }

   FunctionVersion(CallGraph::vertex_descriptor func_v, FloatFormat* userFormat = nullptr) : function_vertex(func_v), userRequired(userFormat), internal(true)
   {
   }

   ~FunctionVersion()
   {
      if(userRequired)
      {
         delete userRequired;
      }
   }

   bool std_format() const
   {
      return userRequired == nullptr;
   }
};

/**
 * Add to the call graph the function calls associated with the floating point primitive operations
 */
class soft_float_cg_ext : public FunctionFrontendFlowStep
{
 protected:
   enum InterfaceType
   {
      INTERFACE_TYPE_NONE,  // Cast rename not needed
      INTERFACE_TYPE_INPUT, // Cast rename after definition may be required
      INTERFACE_TYPE_OUTPUT // Cast rename before usage may be required
   };

   function_decl* fd;
   FunctionBehaviorConstRef FB;
   std::vector<tree_nodeRef> paramBinding;
   bool bindingCompleted;

   /// SSA variable which requires cast renaming from standard to user-defined float format in all but given statements
   CustomMap<ssa_name*, std::vector<unsigned int>> inputCastRename;

   /// SSA variable which requires cast renaming from user-defined to standard float format in given statements only
   CustomMap<ssa_name*, std::vector<tree_nodeRef>> outputCastRename;

   /// Tree manager
   const tree_managerRef TreeM;

   /// tree manipulation
   const tree_manipulationRef tree_man;

   /// when true IR has been modified
   bool modified;

   FunctionVersion* _version;

   static unsigned int unique_id;
   static CustomMap<CallGraph::vertex_descriptor, FunctionVersion> funcFF;

   /**
    *
    * @param fu_name
    * @param current_statement
    * @param current_tree_node
    * @param current_scrp
    */
   void replaceWithCall(const std::string& fu_name, const std::vector<tree_nodeRef>& args, tree_nodeRef current_statement, tree_nodeRef current_tree_node, const std::string& current_scrp);

   /**
    * Recursive examine tree node
    * @param current_statement is the current analyzed statement
    * @param current_tree_node is the current tree node
    */
   void RecursiveExaminate(const tree_nodeRef current_statement, const tree_nodeRef current_tree_node, InterfaceType castRename);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param fun_id is the function index
    * @param design_flow_manager is the design flow manager
    */
   soft_float_cg_ext(const ParameterConstRef Param, const application_managerRef AppM, unsigned int fun_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~soft_float_cg_ext() override;

   /**
    * Fixes the var_decl duplication.
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif
