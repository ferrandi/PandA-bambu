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
 * @file dead_code_eliminationIPA.hpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef DEAD_CODE_ELIMINATION_IPA_HPP
#define DEAD_CODE_ELIMINATION_IPA_HPP

// include superclass header
#include "application_frontend_flow_step.hpp"

//@{
REF_FORWARD_DECL(tree_manager);
struct function_decl;
//@}

/**
 * @brief Inter-procedural dead code elimination analysis
 */
class dead_code_eliminationIPA : public ApplicationFrontendFlowStep
{
 protected:
   /**
    * stores the function ids of the functions whose Bit_Value intra procedural steps have to be invalidated by this
    * step
    */
   CustomOrderedSet<unsigned int> fun_id_to_restart;
   /**
    * stores the function ids of the functions whose Parm2SSA intra procedural steps have to be invalidated by this step
    */
   CustomOrderedSet<unsigned int> fun_id_to_restartParm;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& relationships,
                             const DesignFlowStep::RelationshipType relationship_type) override;

   bool signature_opt(const tree_managerRef& TM, function_decl* fd, unsigned int function_id,
                      const CustomOrderedSet<unsigned int>& rFunctions);

 public:
   dead_code_eliminationIPA(const application_managerRef AM, const DesignFlowManagerConstRef dfm,
                            const ParameterConstRef parameters);

   ~dead_code_eliminationIPA() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};

#endif
