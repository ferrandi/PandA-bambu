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
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */
#ifndef CTESTBENCHEXECUTION_HPP
#define CTESTBENCHEXECUTION_HPP

// include superclass header
#include "hls_step.hpp"

/// STD include
#include <string>

/// STL includes
#include "custom_set.hpp"
#include <tuple>

CONSTREF_FORWARD_DECL(DesignFlowManager);
CONSTREF_FORWARD_DECL(Parameter);

class CTestbenchExecution : public HLS_step
{
 protected:
   const std::string output_directory;

   const std::string testbench_basename;

   virtual const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const;

 public:
   /**
    * constructor
    */
   CTestbenchExecution(const ParameterConstRef Param, const HLS_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager, const std::string& testbench_basename = "values");

   void ComputeRelationships(DesignFlowStepSet& design_flow_step_set, const DesignFlowStep::RelationshipType relationship_type);

   virtual DesignFlowStep_Status Exec();

   virtual bool HasToBeExecuted() const;
};
#endif
