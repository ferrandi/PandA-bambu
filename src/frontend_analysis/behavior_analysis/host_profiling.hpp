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
 * @file host_profiling.hpp
 * @brief Abstract class for passes performing a dynamic profiling of loops, paths or both by means of predependence
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 */
#ifndef HOST_PROFILING_HPP
#define HOST_PROFILING_HPP

/// Superclass include
#include "application_frontend_flow_step.hpp"

#include "custom_map.hpp"

/// Utility include
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(loop);
//@}

/// Different profiling method
enum class HostProfiling_Method
{
   PM_NONE = 0,                /**< None profiling method selected */
   PM_BBP = 1,                 /**< Basic blocks profiling  */
   PM_HPP = 2,                 /**< Hierarchical Path Profiling */
   PM_TP = 4,                  /**< Tree based Path Profiling */
   PM_MAX_LOOP_ITERATIONS = 8, /**< Maximum number of iteration profiling */
   PM_PATH_PROBABILITY = 16,   /**< Probability based path */
   PM_XML_FILE = 32            /**< Data read from XML file */
};

HostProfiling_Method operator&(const HostProfiling_Method first, const HostProfiling_Method second);

/**
 * Class to perform profiling
 */
class HostProfiling : public ApplicationFrontendFlowStep
{
 private:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of the parameters
    */
   HostProfiling(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~HostProfiling() override;

   /**
    * Do nothing
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Normalize path frequency according to execution times of whole function_id
    * @param AppM is the application manger
    * @param loop_instances is how many times each loop is executed
    * @param Parameters is the set of input parameters
    */
   static void normalize(const application_managerRef app_man, const CustomUnorderedMap<unsigned int, CustomUnorderedMapStable<unsigned int, long long unsigned int>>& loop_instances, const ParameterConstRef parameters);
};

#endif
