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
 * @file add_library.hpp
 * @brief This step adds the current module to the technology library
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef ADD_LIBRARY_HPP
#define ADD_LIBRARY_HPP

#include "design_flow_step.hpp"  // for DesignFlowManagerConstRef, DesignFl...
#include "hls_function_step.hpp" // for HLSFunctionStep
#include "hls_step.hpp"          // for HLSFlowStepSpecialization, HLSFlowS...
#include "refcount.hpp"          // for REF_FORWARD_DECL
#include <string>                // for string

REF_FORWARD_DECL(add_library);
CONSTREF_FORWARD_DECL(Parameter);

/**
 * Information about speciaization of add_library
 */
class AddLibrarySpecialization : public HLSFlowStepSpecialization
{
 public:
   /// True if we are adding module with interface
   const bool interfaced;

   /**
    * Constructor
    * @param interfaced is true if we are adding module with interface
    */
   explicit AddLibrarySpecialization(const bool interfaced);

   /**
    * Return the string representation of this
    */
   const std::string GetKindText() const override;

   /**
    * Return the contribution to the signature of a step given by the specialization
    */
   const std::string GetSignature() const override;
};

class add_library : public HLSFunctionStep
{
 private:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    */
   add_library(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);

   /**
    * Destructor
    */
   ~add_library() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif // ADD_LIBRARY_HPP
