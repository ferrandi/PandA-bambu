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
 * @file virtual_aggregate_data_flow_analysis.hpp
 * @brief Analysis step performing aggregate variable computation on the basis of gcc virtual operands
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS_HPP
#define VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS_HPP

/// Superclass include
#include "data_dependence_computation.hpp"

#include "refcount.hpp"

class VirtualAggregateDataFlowAnalysis : public DataDependenceComputation
{
 private:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param function_index is the index of the function
    * @param parameters is the set of the parameters
    */
   VirtualAggregateDataFlowAnalysis(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager, const unsigned int function_id, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~VirtualAggregateDataFlowAnalysis() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
