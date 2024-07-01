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
 * @file pragma_analysis.hpp
 * @brief Analysis step that recognizes the pragma calls in the specification
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef PRAGMA_ANALYSIS_HPP
#define PRAGMA_ANALYSIS_HPP

#include "application_frontend_flow_step.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(tree_node);

/**
 * Restructure the tree control flow graph
 */
class PragmaAnalysis : public ApplicationFrontendFlowStep
{
 private:
   /**
    * Given the index of a function replacing a pragma, returns a parameter
    * @param tn is the tree node of the call
    * @param param is the index of the parameter (starting from 1)
    */
   std::string get_call_parameter(const tree_nodeRef& tn, const unsigned int idx) const;

   /**
    * Create a map pragma
    * @param tn is the tree node of the gimple containing the call which will be directly replaced by the
    * pragma
    */
   tree_nodeRef create_map_pragma(const tree_nodeRef& tn) const;

   /**
    * Create an omp pragma
    * @param tn is the tree node of the gimple containing the call which will be directly replaced by the
    * pragma
    */
   tree_nodeRef create_omp_pragma(const tree_nodeRef& tn) const;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   PragmaAnalysis(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager,
                  const ParameterConstRef parameters);

   DesignFlowStep_Status Exec() override;
};
#endif
