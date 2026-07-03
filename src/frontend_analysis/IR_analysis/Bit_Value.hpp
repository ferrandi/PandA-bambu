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
 * @file Bit_Value.hpp
 * @brief Full implementation of Bit Value analysis as described in
 * BitValue Inference: Detecting and Exploiting Narrow Bitwidth Computations
 * Mihai Budiu Seth Copen Goldstein
 * http://www.cs.cmu.edu/~seth/papers/budiu-tr00.pdf
 *
 * @author Giulio Stramondo
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef BIT_VALUE_HPP
#define BIT_VALUE_HPP
#include "BitLatticeManipulator.hpp"
#include "function_frontend_flow_step.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(Bit_Value);
REF_FORWARD_DECL(bloc);
class binary_node;
enum class bit_lattice;
class assign_stmt;
class ssa_node;
class statement_list_node;
class addr_node;
REF_FORWARD_DECL(ir_node);
CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(ir_manager);

/**
 * @brief Full implementation of Bit Value analysis as described in
 * BitValue Inference: Detecting and Exploiting
 * Narrow Bitwidth Computations
 * Mihai Budiu Seth Copen Goldstein
 * http://www.cs.cmu.edu/~seth/papers/budiu-tr00.pdf
 */
class Bit_Value : public FunctionFrontendFlowStep, public BitLatticeManipulator
{
 private:
   /// True if this step is not executed in the frontend
   bool not_frontend;

   /**
    * Topologically ordered basic blocks
    */
   std::vector<blocRef> bb_topological;

   /**
    * Maps the id of a statement to the id of the function called in
    * that statement. This relationship is created only for direct calls,
    * because for indirect calls there is not a one-to-one relationship
    */
   CustomUnorderedMapUnstable<unsigned int, unsigned int> direct_call_id_to_called_id;

   /**
    * Contains the input parameters of the function that's being analyzed.
    */
   CustomUnorderedSet<unsigned int> arguments;

   /**
    * Debugging function used to print the contents of the current and best maps.
    * @param map map to be printed
    */
   void print_bitstring_map(const CustomMap<unsigned int, std::deque<bit_lattice>>& map) const;

   unsigned long long pointer_resizing(const ir_nodeRef& tn) const;

   unsigned int lsb_to_zero(const addr_node* ae, bool safe, bool is_private) const;

   /**
    * Initializes best with C type as bitstring, signed_var and arguments using the information taken from the syntax
    * IR given by the application manager.
    *
    *
    * Scan all the assign statements in each block
    * if the left hand side is signed this is inserted into the signed_var set (this information is used by the
    * signextension)
    *
    * for each lhs variable in the assign_stmt instruction an entry is created in the best map
    * the entry maps the lhs node index to a bit string made of unknown values with the variable length
    * for each assign_stmt the used variables are checked against the values in the parm set in order to identify the
    * parameters of the function when those are found their ssa index node is added in the best map and in the arguments
    * map ( used by the clear() function )
    *
    *
    * Scans each phi in the bloc (not virtual)
    * res of the phi is added to the signed_var set if it's signed, and to the best map
    *
    * The edge of each phi are scanned,
    * if the edge is carrying a constant of type constant_int_val_node the index of the constant_int_val_node node is
    * used to identify it, one entry is created for the best table mapping the constant_int_val_node index to its
    * bitstring representation if the value of the constant is negative it's added to the signed_var set
    *
    * if the edge is carrying a ssa variable it's checked if its a parameter,
    * if it is, it is added to the arguments set and, it is added to the best map
    */
   void initialize();

   /**
    * Clears all the entry in the current map, except for the input arguments
    */
   void clear_current();

   /**
    * Applies the forward algorithm, as described in the paper, analyzing each assignment statement following the
    * program order, and each phi. Uses the forward_transfer() function to compute the output's bitstring, that stores
    * in current. The algorithm loops until current is modified.
    * @see forward_transfer()
    */
   void forward();

   /**
    * Applies the backward algorithm, as described in the paper, analyzing each assignment statement starting from the
    * output, going up to the inputs, and each phi. Uses the backward_transfer() function to compute the output's
    * bitstring, that stores in current. The algorithm loops until current is modified.
    * @see backward_transfer()
    */
   void backward();

   /**
    * Takes a assign_stmt, analyzes the operation performed from the rhs and its input bitstring, and generate a
    * bitstring from the output.
    * @param ga assignment to analyze
    * @return output bitstring
    */
   std::deque<bit_lattice> forward_transfer(const assign_stmt* ga) const;

   /**
    * Compute the inputs back propagation values, given a assign_stmt and the uid of the output variable.
    * @param ga assign_stmt that is being analyzed
    * @param res_tn ir_node of the output of the given assign_stmt.
    * @return computed backpropagation bitstring
    */
   std::deque<bit_lattice> backward_transfer(const assign_stmt* ga, const ir_nodeRef& res_tn) const;

   std::deque<bit_lattice> backward_chain(const ir_nodeRef& ssa) const;

   /**
    * Updates the bitvalues of the intermediate representation with the values taken from the input map.
    */
   bool update_IR();

   /**
    * Given an operand, returns its current bitvalue
    * @param tn Operand node
    * @return std::deque<bit_lattice> Current bitvalue for given operand
    */
   std::deque<bit_lattice> get_current(const ir_nodeRef& tn) const;

   /**
    * Given an operand, returns its current bitvalue, or its best if current is not available
    * @param tn Operand node
    * @return std::deque<bit_lattice> Current or best bitvalue for given operand
    */
   std::deque<bit_lattice> get_current_or_best(const ir_nodeRef& tn) const;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   Bit_Value(const ParameterConstRef Param, const application_managerRef AM, unsigned int f_id,
             const DesignFlowManager& dfm);

   void Initialize() override;

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status InternalExec() override;
};

#endif /* BIT_VALUE_HPP */
