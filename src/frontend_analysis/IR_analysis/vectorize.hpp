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
 * @file vectorize.hpp
 * @brief This class contains the methods for vectorize loop or whole function
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// utility include
#include "custom_map.hpp"
#include "utility.hpp"

REF_FORWARD_DECL(bloc);
CONSTREF_FORWARD_DECL(Loop);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
CONSTREF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(tree_node);

/// Enum used to classify the statement according to the required transformation
typedef enum
{
   NONE,     /**No transformation is required */
   COND_CON, /**Statement is the condition of the loop; it can remain a scalar since all the destinations are the same, but operand can be a vector */
   COND_DIV, /**Statement is the condition of the loop */
   INC,      /**Statement is the increment of the simd outer loop: it has to be transformed in vector modifying the increment */
   INIT,     /**Statement is the phi initializing the induction variable of the simd outer loop */
   SCALAR,   /**Scalar operation has to be transformed into multiple scalar operation */
   SIMD,     /**Scalar operation has to be transformed into vector */
} Transformation;

class Vectorize : public FunctionFrontendFlowStep
{
 private:
   /// Map between scalar tree node and vector tree node
   CustomUnorderedMapUnstable<unsigned int, unsigned int> scalar_to_vector;

   /// Map between scalar tree node and versioned scalar tree node
   CustomUnorderedMap<unsigned int, CustomUnorderedMapStable<size_t, unsigned int>> scalar_to_scalar;

   /// The tree manager
   const tree_managerRef TM;

   /// The tree_manipulation
   const tree_manipulationRef tree_man;

   /// The increment of induction variables; id is the index of the ssa name defined in the init gimple
   CustomMap<unsigned int, long long int> iv_increment;

   /// Enum used to classify the loop according to the required transformation
   typedef enum
   {
      SIMD_NONE,  /** No transformation is required */
      SIMD_INNER, /** Loop is nested in a outer simd loop */
      SIMD_OUTER  /** Loop is a outer simd */
   } SimdLoop;

   /// Loop classification
   CustomMap<unsigned int, SimdLoop> simd_loop_type;

   /// Loop parallel degree
   CustomMap<unsigned int, size_t> loop_parallel_degree;

   /// Basic block classification: if value is true, the basic block can be executed or not in parallel instances
   CustomMap<unsigned int, bool> basic_block_divergence;

   /// Statement classification
   CustomMap<unsigned int, Transformation> transformations;

   /// The guards for each basic block
   CustomMap<unsigned int, tree_nodeRef> guards;

   /**
    * Classify a loop
    * @param loop is the loop to be analyzed
    * @param parallel_degree specifies the degree of parallelism
    */
   void ClassifyLoop(const LoopConstRef loop, const size_t parallel_degree);

   /**
    * Classify a statement
    * @param loop_id is the id of the looop of the statement
    * @param tree_node is the tree_node to be classified
    */
   void ClassifyTreeNode(const unsigned int loop_id, const tree_nodeConstRef tree_node);

   /**
    * Check recursively if at least an ssa operand is defined an operation outside simd outer loop
    * @param tree_node is the root of the tree to be checked
    * @return true if at least a scalar operand is found
    */
   bool LookForScalar(const tree_nodeConstRef tree_node);

   /**
    * Transform a tree node
    * @param tree_node_index is the tree node to be transformed
    * @param parallel_degree is the degree of parallelism
    * @param scalar_index is the index of the vector when parallel statements are created as multiple statements
    * @param created_statements is the list of statements created during transformation
    * @return the tree node of the transformed node
    */
   unsigned int Transform(const unsigned int tree_node_index, const size_t parallel_degree, const size_t scalar_index, std::list<tree_nodeRef>& created_statements, std::vector<tree_nodeRef>& created_phis);

   /**
    * Duplicate increment statement and update uses of defined variable when necessary
    * @param loop_id is the id of the loop to which the statement belongs
    * @param statement is the statement to be duplicated
    * @return the index of the created statement
    */
   unsigned int DuplicateIncrement(const unsigned int loop_id, const tree_nodeRef statement);

   /**
    * Add the guards for predicated operations
    */
   void AddGuards();

   /**
    * Fix the phis to consider implicitly predicated operations
    */
   void FixPhis();

   /**
    * Set predicate of predicated instructions
    */
   void SetPredication();

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param function_id is the identifier of the function
    * @param DesignFlowManagerConstRef is the design flow manager
    * @param parameters is the set of input parameters
    */
   Vectorize(const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~Vectorize() override;

   /**
    * Restructures the unstructured code
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
