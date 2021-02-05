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

struct FloatFormat
{
 public:
   uint8_t exp_bits;
   uint8_t frac_bits;
   int32_t exp_bias;
   bool has_rounding;
   bool has_nan;
   bool has_one;
   bool has_subnorm;
   bit_lattice sign;

   FloatFormat(uint8_t _exp_bits, uint8_t _frac_bits, int32_t _exp_bias, bool _has_rounding = true, bool _has_nan = true, bool _has_one = true, bool _has_subnorm = false, bit_lattice _sign = bit_lattice::U)
       : exp_bits(_exp_bits), frac_bits(_frac_bits), exp_bias(_exp_bias), has_rounding(_has_rounding), has_nan(_has_nan), has_one(_has_one), has_subnorm(_has_subnorm), sign(_sign)
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
      return "e" + STR(+exp_bits) + "m" + STR(+frac_bits) + "b" + ((exp_bias < 0) ? ("_" + STR(-exp_bias)) : STR(exp_bias)) + (has_rounding ? "r" : "") + (has_nan ? "n" : "") + (has_one ? "h" : "") + (has_subnorm ? "s" : "") +
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
   const refcount<FloatFormat> userRequired;

   // True if all function callers share this function float format
   bool internal;

   // std::set<FloatFormat*, FloatFormatRefCompare> callersFF;

   FunctionVersion() : function_vertex(nullptr), userRequired(nullptr), internal(true)
   {
   }

   FunctionVersion(CallGraph::vertex_descriptor func_v, refcount<FloatFormat> userFormat = nullptr) : function_vertex(func_v), userRequired(userFormat), internal(true)
   {
   }

   FunctionVersion(const FunctionVersion& other) : function_vertex(other.function_vertex), userRequired(other.std_format() ? nullptr : new FloatFormat(*other.userRequired)), internal(other.internal)
   {
   }

   ~FunctionVersion()
   {
   }

   bool operator==(const FunctionVersion& other) const
   {
      return function_vertex == other.function_vertex && internal == other.internal && ((userRequired == nullptr && other.userRequired == nullptr) || (userRequired != nullptr && other.userRequired != nullptr && *userRequired == *other.userRequired));
   }

   bool operator!=(const FunctionVersion& other) const
   {
      return !(this->operator==(other));
   }

   bool std_format() const
   {
      return userRequired == nullptr;
   }

   std::string ToString() const
   {
      return STR(function_vertex) + (internal ? "_internal_" : "") + (userRequired ? userRequired->mngl() : "");
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
   tree_nodeRef int_type;
   tree_nodeRef int_ptr_type;

   // Real type variables to be aliased as integer type variables
   CustomSet<ssa_name*> viewConvert;

   // Real to integer view convert statements to be converted into nop statements
   std::vector<tree_nodeRef> nopConvert;

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

   refcount<FunctionVersion> _version;

   static CustomMap<CallGraph::vertex_descriptor, refcount<FunctionVersion>> funcFF;

   static tree_nodeRef float32_type;
   static tree_nodeRef float32_ptr_type;
   static tree_nodeRef float64_type;
   static tree_nodeRef float64_ptr_type;

   tree_nodeRef parm_int_type_for(const tree_nodeRef& type) const;
   tree_nodeRef int_type_for(const tree_nodeRef& type) const;

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
    * @param castRename
    */
   void RecursiveExaminate(const tree_nodeRef current_statement, const tree_nodeRef current_tree_node, InterfaceType castRename);

   /**
    * Generate necessary statements to convert ssa variable from inFF to outFF and insert them after stmt in bb
    * @param bb Generated operations will be inserted in this basic block
    * @param stmt Generated statements will be inserted after this statement, if nullptr they will be inserted at the beginning of the BB
    * @param ssa Real type ssa_name tree reindex to be converted from inFF to outFF
    * @param inFF Input float format, if nullptr will be deduced as standard IEEE 754 type from ssa bitwidth
    * @param outFF Output float format, if nullptr will be deduced as standard IEEE 754 type from ssa bitwidth
    * @return tree_nodeRef New ssa_name tree reindex reference representing converted input ssa
    */
   tree_nodeRef inPlaceCastRename(blocRef bb, tree_nodeRef stmt, tree_nodeRef ssa, refcount<FloatFormat> inFF, refcount<FloatFormat> outFF) const;

   /**
    * Cast real type constant from inFF to outFF format
    * @param in Real type constant bits represented as inFF
    * @param inFF Input floating point format
    * @param outFF Output floating point format
    * @return tree_nodeRef Unsigned integer constant bits representation of input bits using outFF format
    */
   tree_nodeRef cstCast(uint64_t in, refcount<FloatFormat> inFF, refcount<FloatFormat> outFF) const;

   /**
    * Generate float negate operation based on given floating-point format
    * @param op Negate operand
    * @param ff Floating-point format
    * @return tree_nodeRef Floating-point format related negate expression to replace negate_expr with
    */
   tree_nodeRef floatNegate(tree_nodeRef op, refcount<FloatFormat> ff) const;

   /**
    * Generate float absolute value operation based on given floating-point format
    * @param op Negate operand
    * @param ff Floating-point format
    * @return tree_nodeRef Floating-point format related absolute value expression to replace abs_expr with
    */
   tree_nodeRef floatAbs(tree_nodeRef op, refcount<FloatFormat> ff) const;

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
   soft_float_cg_ext(const ParameterConstRef _parameters, const application_managerRef AppM, unsigned int _function_id, const DesignFlowManagerConstRef design_flow_manager);

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
