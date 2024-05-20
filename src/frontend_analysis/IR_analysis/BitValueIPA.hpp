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
 * @file BitValueIPA.hpp
 *
 * Created on: June 27, 2016
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef BIT_VALUE_IPA_HPP
#define BIT_VALUE_IPA_HPP

// include superclass header
#include "BitLatticeManipulator.hpp"
#include "application_frontend_flow_step.hpp"

/**
 * @brief Inter-Procedural extension of the Bit Value analysis as described in
 * BitValue Inference: Detecting and Exploiting
 * Narrow Bitwidth Computations
 * Mihai Budiu Seth Copen Goldstein
 * http://www.cs.cmu.edu/~seth/papers/budiu-tr00.pdf
 */
class BitValueIPA : public ApplicationFrontendFlowStep, public BitLatticeManipulator
{
   /**
    * stores the function ids of the functions whose Bit_Value intra procedural
    * steps have to be invalidated by this step
    */
   CustomOrderedSet<unsigned int> fun_id_to_restart;
   CustomOrderedSet<unsigned int> fun_id_to_restart_caller;

   /* Sum of reached body functions' bb+bitvalue versions after last Exec call */
   unsigned int last_ver_sum;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& relationships,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   BitValueIPA(const application_managerRef AM, const DesignFlowManagerConstRef dfm,
               const ParameterConstRef parameters);

   ~BitValueIPA() override;

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
