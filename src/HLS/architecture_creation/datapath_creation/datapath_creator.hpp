/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file datapath_creator.hpp
 * @brief Base class for all datapath creation algorithms.
 *
 * This class is a pure virtual one, that has to be specilized in order to implement a particular algorithm to create
 * datapath.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
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
    * @param Param is the set of input parameters
    * @param HLSMgr is the HLS manager
    * @param funId is the identifier of the function being processed
    * @param design_flow_manager is the design flow manager
    * @param _hls_flow_step_type is the type of algorithm used to create a datapath
    */
   datapath_creator(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                    const DesignFlowManager& design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type);
};
/// refcount definition of the class
using datapath_creatorRef = refcount<datapath_creator>;

#endif
