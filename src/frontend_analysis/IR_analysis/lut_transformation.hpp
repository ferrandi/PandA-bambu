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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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
 * @file lut_transformation.hpp
 * @brief recognize lut expressions.
 * @author Di Simone Jacopo
 * @author  Cappello Paolo
 * @author Inajjar Ilyas
 * @author Angelo Gallarello
 * @author  Stefano Longari
 * Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef LUT_TRANSFORMATION_HPP
#define LUT_TRANSFORMATION_HPP

///Super class include
#include "function_frontend_flow_step.hpp"

///STD include
#include <string>
#include <set>
#include <list>
#include <math.h> 
#include <map>
#include <vector>
#include <algorithm>
///Utility include
#include "refcount.hpp"

//@{
REF_FORWARD_DECL(bloc);
//class integer_cst;
//class target_mem_ref461;
//REF_FORWARD_DECL(lut_transformation);
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_node);
//@}


class lut_transformation : public FunctionFrontendFlowStep
{
   private:
      ///The tree manager
      tree_managerRef TM;

      ///The lut manipulation
      tree_manipulationRef tree_man;

      ///The maximum number of inputs of a lut
      size_t max_lut_size;

      /**
       * Create gimple assignment
       * @param type is the type the assignment
       * @param op is the right part
       * @param bb_index is the index of the basic block index
       * @param srcp_default is the srcp to be assigned
       */
      tree_nodeRef CreateGimpleAssign(const tree_nodeRef type, const tree_nodeRef op, const unsigned int bb_index, const std::string& srcp_default);
      std::vector<tree_nodeRef> GetInputs(const tree_nodeRef node);
      std::string DecToBin(unsigned long long int number);
      unsigned long long int BinToDec(const std::string& number);
      unsigned long long int GenerateIndexOfLutValue(const std::string& binString,const std::vector<std::size_t>& indexesSet);
      std::string AddZeroes(const std::string& bitString,const double setSize);
      std::vector<std::size_t> CreateLutIndexSet( std::vector<tree_nodeRef> nodeSet, std::vector<tree_nodeRef> values);
      void MergeLut( std::list<tree_nodeRef> gimpleLutList,std::pair<const unsigned int, boost::shared_ptr<bloc>> bb);
      std::vector<tree_nodeRef> GetLutList( std::vector<tree_nodeRef> list_of_stmt);
      tree_nodeRef CreateConcat(const tree_nodeRef op0, const tree_nodeRef op1, std::pair<const unsigned int, boost::shared_ptr<bloc>> bb, tree_nodeRef stm_to_append);
      tree_nodeRef CreateMultiConcat( std::vector<tree_nodeRef> set_of_nodes,std::pair<const unsigned int, boost::shared_ptr<bloc>> bb,tree_nodeRef stm_to_append);
      std::vector<tree_nodeRef> CreateSetFromVector( std::vector<tree_nodeRef> firstSet, std::vector<tree_nodeRef> secondSet);
      std::string CreateFinalString(std::string binaryString, std::vector<tree_nodeRef> unmergedSet,std::vector<tree_nodeRef> mergedSet, std::string mergingValue );
      std::vector<std::size_t> FindIndex( std::vector<tree_nodeRef> mergedSet, tree_nodeRef node);
      /**
       * Return the set of analyses in relationship with this design step
       * @param relationship_type is the type of relationship to be considered
       */
      const std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship> > ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const;

   public:
      /**
       * Constructor.
       * @param Param is the set of the parameters
       * @param AppM is the application manager
       * @param function_id is the identifier of the function
       * @param DesignFlowManagerConstRef is the design flow manager
       */
      lut_transformation(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

      /**
       *  Destructor
       */
      ~lut_transformation();

      /**
       * Computes the operations CFG graph data structure.
       * @return the exit status of this step
       */
      DesignFlowStep_Status InternalExec();

      /**
       * Initialize the step (i.e., like a constructor, but executed just before exec
       */
      virtual void Initialize();

      /**
       * Compute the relationships of a step with other steps
       * @param dependencies is where relationships will be stored
       * @param relationship_type is the type of relationship to be computed
       */
      virtual void ComputeRelationships(DesignFlowStepSet & relationship, const DesignFlowStep::RelationshipType relationship_type);
};

#endif
