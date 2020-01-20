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
 * @file scalar_ssa_data_dependence_computation.hpp
 * @brief Analysis step performing data flow analysis based on ssa variables
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef SCALAR_SSA_DATA_DEPENDENCE_COMPUTATION_HPP
#define SCALAR_SSA_DATA_DEPENDENCE_COMPUTATION_HPP

/// Superclass include
#include "data_dependence_computation.hpp"

#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(tree_manager);
//@}

/**
 * ssa data flow analysis step
 */
class ScalarSsaDataDependenceComputation : public DataDependenceComputation
{
 private:
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
    * @param function_id is the node id of the function analyzed.
    */
   ScalarSsaDataDependenceComputation(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~ScalarSsaDataDependenceComputation() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
