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
 * @file datapath.hpp
 * @brief Base class for all datapath creation algorithms.
 *
 * This class is a pure virtual one, that has to be specilized in order to implement a particular algorithm to create
 * datapath.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef _DATAPATH_CREATOR_HPP_
#define _DATAPATH_CREATOR_HPP_

#include "hls_function_step.hpp"
REF_FORWARD_DECL(datapath_creator);

/**
 * @class datapath_creator
 * Generic class managing datapath creation algorithms.
 */
class datapath_creator : public HLSFunctionStep
{
 protected:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the type of algorithm used to create a datapath
    */
   datapath_creator(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                    const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type);

   /**
    * Destructor.
    */
   ~datapath_creator() override;
};
/// refcount definition of the class
using datapath_creatorRef = refcount<datapath_creator>;

#endif
