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
 *              Copyright (C) 2019-2024 Politecnico di Milano
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
 * @file Range_Analysis.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef RANGE_ANALYSIS_HPP
#define RANGE_ANALYSIS_HPP
#include "application_frontend_flow_step.hpp"

#include "custom_set.hpp"
#include "refcount.hpp"

#include <cstdint>
#include <iostream>
#include <map>

REF_FORWARD_DECL(ConstraintGraph);

enum SolverType
{
   st_Cousot,
   st_Crop
};

class RangeAnalysis : public ApplicationFrontendFlowStep
{
#ifndef NDEBUG
   int graph_debug;
   uint64_t iteration;
   uint64_t stop_iteration;
   uint64_t stop_transformation;
#endif

   SolverType solverType;
   int execution_mode;

   bool finalize(ConstraintGraphRef);

 protected:
   /// stores the function ids of the functions whose Dead Code need to be restarted
   CustomOrderedSet<unsigned int> fun_id_to_restart;
   std::map<unsigned int, unsigned int> last_bitvalue_ver;
   std::map<unsigned int, unsigned int> last_bb_ver;

   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;
   void ComputeRelationships(DesignFlowStepSet& relationships,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor.
    * @param _Param is the set of the parameters
    * @param _AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   RangeAnalysis(const application_managerRef AM, const DesignFlowManagerConstRef dfm,
                 const ParameterConstRef parameters);

   ~RangeAnalysis() override;

   DesignFlowStep_Status Exec() override;

   void Initialize() override;

   bool HasToBeExecuted() const override;
};

#endif // !RANGE_ANALYSIS_HPP
