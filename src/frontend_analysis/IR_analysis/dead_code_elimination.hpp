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
 *              Copyright (c) 2004-2018 Politecnico di Milano
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
 * @file dead_code_elimination.hpp
 * @brief Eliminates unuseful definitions
 *
 * @author Andrea Cuoccio <andrea.cuoccio@gmail.com>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
*/
#ifndef DEAD_CODE_ELIMINATION_HPP
#define DEAD_CODE_ELIMINATION_HPP
///Super class include
#include "function_frontend_flow_step.hpp"

///Utility include
#include "refcount.hpp"

///behaviour include
#include "call_graph.hpp"

///Frontend include
#include "Parameter.hpp"

///Tree include
#include "tree_manager.hpp"

class dead_code_elimination : public FunctionFrontendFlowStep
{
   private:

      /**
       * Return the set of analyses in relationship with this design step
       * @param relationship_type is the type of relationship to be considered
       */
      const std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship> > ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const;

   public:

      /**
       * Constructor
       * @param Param is the set of the parameters
       * @param AppM is the application manager
       * @param function_id is the index of the function
       * @param design_flow_manager is the design flow manager
       */
      dead_code_elimination(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

      /**
       * Destructor
       */
      ~dead_code_elimination();

      /**
       * Performes dead code elimination.
       * @return the exit status of this step
       */
      DesignFlowStep_Status InternalExec();
};

#endif

