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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file soft_float_cg_ext.hpp
 * @brief Step that extends the call graph with the soft-float calls where appropriate.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef SOFT_FLOAT_CG_EXT_HPP
#define SOFT_FLOAT_CG_EXT_HPP
#include "function_frontend_flow_step.hpp"

#include "bit_lattice.hpp"
#include "call_graph.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"

#include <array>
#include <tuple>
#include <vector>

struct function_val_node;
struct ssa_node;
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(FloatFormat);
REF_FORWARD_DECL(FunctionVersion);
REF_FORWARD_DECL(soft_float_cg_ext);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_manipulation);
REF_FORWARD_DECL(ir_node);

/**
 * Add to the call graph the function calls associated with the floating point primitive operations
 */
class soft_float_cg_ext : public FunctionFrontendFlowStep
{
 private:
   /// Floating-point function version map
   static CustomMap<CallGraph::vertex_descriptor, FunctionVersionRef> funcFF;

   /// Arguments list to feed specialization parameters of versioned functions
   static CustomMap<std::string, std::array<ir_nodeRef, 8>> spec_parms_map;

   static bool inline_math;
   static bool inline_conversion;
   static ir_nodeRef float32_type;
   static ir_nodeRef float32_ptr_type;
   static ir_nodeRef float64_type;
   static ir_nodeRef float64_ptr_type;

   static bool lowering_needed(const ssa_node* ssa);

   enum InterfaceType
   {
      INTERFACE_TYPE_NONE = 0,   // Cast rename not needed
      INTERFACE_TYPE_INPUT = 1,  // Cast rename after definition may be required
      INTERFACE_TYPE_OUTPUT = 2, // Cast rename before usage may be required
      INTERFACE_TYPE_REAL = 4    // Floating-point type must be persisted
   };

   /// Already visited IR node (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited;

   /// IR manager
   const ir_managerRef IRM;

   /// IR manipulation
   const ir_manipulationRef ir_man;

   function_val_node* fd;
   bool isTopFunction;
   std::vector<ir_nodeRef> topReturn;
   bool bindingCompleted;
   std::vector<ir_nodeRef> paramBinding;

   FunctionVersionRef _version;

   ir_nodeRef int_type;
   ir_nodeRef int_ptr_type;

   // Real type variables to be aliased as integer type variables {ssa_node, internal_type}
   CustomMap<ssa_node*, bool> bitcastCandidates;

   // Real to integer view convert statements to be converted into nop statements
   std::vector<ir_nodeRef> nopConvertibleBitcasts;

   /// SSA variable which requires cast renaming from standard to user-defined float format in all but given statements
   CustomMap<ssa_node*, std::tuple<FloatFormatRef, std::vector<unsigned int>>> inputInterface;

   /// SSA variable which requires cast renaming from user-defined to standard float format in given statements only
   CustomMap<ssa_node*, std::tuple<FloatFormatRef, std::vector<ir_nodeRef>>> outputInterface;

   /// Hardware implemented functions need parameters specified as real_ty_node, thus it is necessary to add a bitcast
   CustomMap<ssa_node*, std::set<unsigned int>> hwParam;

   /// Hardware implemented functions return values as real_ty_node, thus a bitcast is necessary
   std::vector<ssa_node*> hwReturn;

   ir_nodeRef int_type_for(const ir_nodeRef& type, bool use_internal) const;

   bool signature_lowering(function_val_node* f_decl) const;

   void ssa_lowering(ssa_node* ssa, bool internal_type) const;

   /**
    * Replace current_ir_node with a call_node to fu_name function specialized with specFF fp format in
    * current_statement
    *
    * @param specFF FP format for fu_name function specialization
    * @param fu_name Function name
    * @param args Function arguments
    * @param current_statement
    * @param current_ir_node
    * @param current_scrp
    * @return ir_nodeRef IR node of specialized function
    */
   ir_nodeRef replaceWithCall(const FloatFormatRef& specFF, const std::string& fu_name, std::vector<ir_nodeRef> args,
                              const ir_nodeRef& current_statement, const ir_nodeRef& current_ir_node,
                              const std::string& current_scrp);

   /**
    * Recursive examine IR node
    * @param current_statement is the current analyzed statement
    * @param current_ir_node is the current IR node
    * @param castRename is the required interface type bitmask reported using InterfaceType enum
    * @return bool True if IR has been modified, else false
    */
   bool RecursiveExaminate(const ir_nodeRef& current_statement, const ir_nodeRef& current_ir_node, int castRename);

   /**
    * Generate necessary statements to convert ssa variable from inFF to outFF and insert them after stmt in bb
    * @param bb Generated operations will be inserted in this basic block
    * @param stmt Generated statements will be inserted after this statement, if nullptr they will be inserted at the
    * beginning of the BB
    * @param ssa Real type ssa_node IR reindex to be converted from inFF to outFF
    * @param inFF Input float format, if nullptr will be deduced as standard IEEE 754 type from ssa bitwidth
    * @param outFF Output float format, if nullptr will be deduced as standard IEEE 754 type from ssa bitwidth
    * @return ir_nodeRef New ssa_node IR reindex reference representing converted input ssa
    */
   ir_nodeRef generate_interface(const blocRef& bb, ir_nodeRef stmt, const ir_nodeRef& ssa, FloatFormatRef inFF,
                                 FloatFormatRef outFF) const;

   /**
    * Cast real type constant from inFF to outFF format
    * @param in Real type constant bits represented as inFF
    * @param inFF Input floating point format
    * @param outFF Output floating point format
    * @return ir_nodeRef Unsigned integer constant bits representation of input bits using outFF format
    */
   ir_nodeRef cstCast(uint64_t in, const FloatFormatRef& inFF, const FloatFormatRef& outFF) const;

   /**
    * Generate float negate operation based on given floating-point format
    * @param op Negate operand
    * @param ff Floating-point format
    * @return ir_nodeRef Floating-point format related negate operation to replace `neg_node` with
    */
   ir_nodeRef floatNegate(const ir_nodeRef& op, const FloatFormatRef& ff) const;

   /**
    * Generate float absolute value operation based on given floating-point format
    * @param op Negate operand
    * @param ff Floating-point format
    * @return ir_nodeRef Floating-point format related absolute value expression to replace abs_node with
    */
   ir_nodeRef floatAbs(const ir_nodeRef& op, const FloatFormatRef& ff) const;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   soft_float_cg_ext(const ParameterConstRef _parameters, const application_managerRef AppM, unsigned int _function_id,
                     const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;
};

class FloatFormat
{
 public:
   enum FPRounding
   {
      FPRounding_Truncate = 0,
      FPRounding_NearestEven = 1
   };

   enum FPException
   {
      FPException_Overflow = 0,
      FPException_IEEE = 1,
      FPException_Saturation = 2,
      FPException_NoNan = 4
   };

   uint8_t exp_bits;
   uint8_t frac_bits;
   int32_t exp_bias;
   FPRounding rounding_mode;
   FPException exception_mode;
   bool has_one;
   bool has_subnorm;
   bit_lattice sign;

   FloatFormat(uint8_t _exp_bits, uint8_t _frac_bits, int32_t _exp_bias,
               FPRounding _rounding_mode = FPRounding_NearestEven, FPException _exception_mode = FPException_IEEE,
               bool _has_one = true, bool _has_subnorm = false, bit_lattice _sign = bit_lattice::U);

   bool operator==(const FloatFormat& other) const;

   bool operator!=(const FloatFormat& other) const;

   bool ieee_format() const;

   std::string ToString() const;

   static FloatFormatRef FromString(std::string ff_str);

   static FloatFormatRef FromArgs(const std::vector<ir_nodeRef>& args);
};

class FunctionVersion
{
 public:
   // Id of reference function
   const CallGraph::vertex_descriptor function_vertex;

   // Float format required from the user
   FloatFormatRef userRequired;

   // Contains callers function versions'
   std::vector<FunctionVersionRef> callers;

   // True if all caller functions share this function float format or if this is a standard ieee format function
   bool internal;

   FunctionVersion();

   FunctionVersion(CallGraph::vertex_descriptor func_v, const FloatFormatRef& userFormat = nullptr);

   FunctionVersion(const FunctionVersion& other);

   int compare(const FunctionVersion& other, bool format_only = false) const;

   bool operator==(const FunctionVersion& other) const;

   bool operator!=(const FunctionVersion& other) const;

   bool ieee_format() const;

   std::string ToString() const;
};

#endif
