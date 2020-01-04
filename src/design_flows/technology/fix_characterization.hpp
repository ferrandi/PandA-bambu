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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file fix_characterization.hpp
 * @brief Step to fix components characterization
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef FIX_CHARACTERIZATION_HPP
#define FIX_CHARACTERIZATION_HPP

/// Superclass include
#include "technology_flow_step.hpp"

/// utility include
#include "refcount.hpp"
#include "utility.hpp"

REF_FORWARD_DECL(target_device);
REF_FORWARD_DECL(technology_manager);

/**
 * Step which fixes characterization
 */
class FixCharacterization : public TechnologyFlowStep
{
 protected:
   /// The execution time of the assignment
   double assignment_execution_time;

   /// The estimated execution time of connection
   double connection_time;

   /// The timestamp of the assignment
   TimeStamp assignment_characterization_timestamp;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<TechnologyFlowStep_Type> ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param TM is the technology manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   FixCharacterization(const technology_managerRef TM, const target_deviceRef target, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~FixCharacterization() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
