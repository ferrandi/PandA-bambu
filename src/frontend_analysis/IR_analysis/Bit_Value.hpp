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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file Bit_Value.hpp
 * @brief Full implementation of Bit Value analysis as described in
 * BitValue Inference: Detecting and Exploiting Narrow Bitwidth Computations
 * Mihai Budiu Seth Copen Goldstein
 * http://www.cs.cmu.edu/~seth/papers/budiu-tr00.pdf
 *
 * @author Giulio Stramondo
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef BIT_VALUE_HPP
#define BIT_VALUE_HPP
#include "BitLatticeManipulator.hpp"
#include "function_frontend_flow_step.hpp"
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(Bit_Value);
REF_FORWARD_DECL(bloc);
class binary_expr;
enum class bit_lattice;
class gimple_assign;
class ssa_name;
class statement_list;
class addr_expr;
REF_FORWARD_DECL(tree_node);
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(tree_manager);
//@}

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
   /**
    * @brief Map storing the implementation of the forward_transfer's plus_expr.
    */
   static const std::map<bit_lattice, std::map<bit_lattice, std::map<bit_lattice, std::deque<bit_lattice>>>>
       plus_expr_map;

   /**
    * @brief Map storing the implementation of the forward_transfer's minus_expr.
    */
   static const std::map<bit_lattice, std::map<bit_lattice, std::map<bit_lattice, std::deque<bit_lattice>>>>
       minus_expr_map;

   /**
    * @brief Map storing the implementation of the forward_transfer's bit_ior_expr_map.
    */
   static const std::map<bit_lattice, std::map<bit_lattice, bit_lattice>> bit_ior_expr_map;

   /**
    * Map storing the implementation of the forward_transfer's bit_xor_expr_map.
    */
   static const std::map<bit_lattice, std::map<bit_lattice, bit_lattice>> bit_xor_expr_map;

   /**
    * Map storing the implementation of the forward_transfer's bit_and_expr_map.
    */
   static const std::map<bit_lattice, std::map<bit_lattice, bit_lattice>> bit_and_expr_map;

   /// True if this step is not executed in the frontend
   bool not_frontend;

   /**
    * Topologically ordered basic blocks
    */
   std::vector<blocRef> bb_topological;

   /**
    * Maps the id of a gimple statement to the id of the function called in
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

   unsigned long long pointer_resizing(unsigned int output_id) const;

   unsigned int lsb_to_zero(const addr_expr* ae, bool safe) const;

   /**
    * Initializes best with C type as bitstring, signed_var and arguments using the information taken from the syntax
    * tree given by the application manager.
    *
    *
    * Scan all the gimple assign statements in each bloc
    * if the left hand side is signed this is inserted into the signed_var set (this information is used by the
    * signextension)
    *
    * for each lhs variable in the gimple_assign instruction and entry is created in the best map
    * the entry in the map is of the type <lhs->index, bit_string of <U>s of the variable lenght>
    * for each gimple_assign the used variables are checked against the values in the parm set in order to identify the
    * parameters of the function when those are found their ssa index node is added in the best map and in the arguments
    * map ( used by the clear() function )
    *
    *
    * Scans each phi in the bloc (not virtual)
    * res of the phi is added to the signed_var set if it's signed, and to the best map
    *
    * The edge of each phi are scanned,
    * if the edge is carrying a constant of type integer_cst the index of the integer_cst node is used to identify it,
    * one entry is created for the best table mapping the integer_cst index to its bitstring representation
    * if the value of the constant is negative it's added to the signed_var set
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
    * Takes a gimple assignment, analyzes the operation performed from the rhs and its input bitstring, and generate a
    * bitstring from the output.
    * @param ga assignment to analyze
    * @return output bitstring
    */
   std::deque<bit_lattice> forward_transfer(const gimple_assign* ga) const;

   /**
    * Compute the inputs back propagation values, given a gimple assignment and the uid of the output variable.
    * @param ga gimple assignment that is being analyzed
    * @param output_id uid of the output of the given gimple assignment.
    * @return computed backpropagation bitstring
    */
   std::deque<bit_lattice> backward_transfer(const gimple_assign* ga, unsigned int output_id) const;

   std::deque<bit_lattice> backward_chain(const tree_nodeConstRef& ssa) const;

   /**
    * Updates the bitvalues of the intermediate representation with the values taken from the input map.
    */
   bool update_IR();

   /**
    * Given an operand, returns its current bitvalue
    * @param tn Operand node
    * @return std::deque<bit_lattice> Current bitvalue for given operand
    */
   std::deque<bit_lattice> get_current(const tree_nodeConstRef& tn) const;

   /**
    * Given an operand, returns its current bitvalue, or its best if current is not available
    * @param tn Operand node
    * @return std::deque<bit_lattice> Current or best bitvalue for given operand
    */
   std::deque<bit_lattice> get_current_or_best(const tree_nodeConstRef& tn) const;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param _Param is the set of the parameters
    * @param _AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   Bit_Value(const ParameterConstRef Param, const application_managerRef AM, unsigned int f_id,
             const DesignFlowManagerConstRef dfm);

   /**
    *  Destructor
    */
   ~Bit_Value() override;
   /**
    * perform the bit value analysis
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};

#endif /* BIT_VALUE_HPP */
