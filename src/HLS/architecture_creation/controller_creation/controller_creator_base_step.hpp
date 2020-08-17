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
 * @file controller_creator_base_step.hpp
 * @brief Base class for all the controller creation algorithms.
 *
 * This class is a pure virtual one, that has to be specilized in order to implement a particular algorithm to create the
 * controller.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */

#ifndef CONTROLLER_CREATOR_BASE_STEP_HPP
#define CONTROLLER_CREATOR_BASE_STEP_HPP

/// Superclass include
#include "hls_function_step.hpp"

/// graph include
#include "graph.hpp"

/// STD include
#include <fstream>

/// STL includes
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <tuple>

/// utility include
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(generic_obj);
REF_FORWARD_DECL(structural_manager);
class xml_element;
//@}

/**
 * Generic class managing controller creation algorithms.
 */
class ControllerCreatorBaseStep : public HLSFunctionStep
{
 protected:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    */
   ControllerCreatorBaseStep(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);

   /**
    * Destructor.
    */
   ~ControllerCreatorBaseStep() override;

 protected:
   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * This member function adds the standard ports (clock, reset, done and command ones) to a circuit.
    * \param circuit it is the datastructure of the component where to add these ports
    */
   virtual void add_common_ports(structural_objectRef circuit, structural_managerRef SM);

   /// This contains all the ports that go from the controller to the datapath, used to enable the registers
   /// and to control muxes in the datapath. The first element in the map
   /// is the port object. Despite its type is generic_objRef it appears that this map contains only objects
   /// of type commandport_obj. This is suggested by the dynamic cast hidden behind a GetPointer call in
   /// controller::add_command_ports(). The second element of the map is the integer identifier of the port
   /// Initialized only after add_common_ports is called
   std::map<generic_objRef, unsigned int> out_ports;

   /// This is the same as in_ports except that the first element is of type vertex. Each element is obtained by
   /// calling GetPointer<commandport_obj>(j->second)->get_vertex() to the elements into in_ports. The second
   /// element is the same number of the generic_objRef into in_ports to which get_vertex() was called
   /// Initialized after add_common_ports is called
   std::map<vertex, unsigned int> cond_ports;
   /// This map put into relation fsm states and alldone multi_unbounded ports
   std::map<vertex, unsigned int> mu_ports;

   /// Initialized after add_common_ports is called. It represents the current number of output ports
   unsigned int out_num;

   /// Initialized after add_common_ports is called. It represents the current number of input ports
   unsigned int in_num;

 private:
   /**
    * Adds the clock and reset ports to a circuit. Called by add_common_ports
    * \param circuit the circuit where to add the clock and reset ports
    */
   void add_clock_reset(structural_objectRef circuit, structural_managerRef SM);

   /**
    * Adds the done port to a circuit. Called by add_common_ports
    * The done port appears to go high once all the calculation of a function are completed
    * \param circuit the circuit where to add the done port
    */
   void add_done_port(structural_objectRef circuit, structural_managerRef SM);

   /**
    * Adds the start port to a circuit. Called by add_common_ports
    * \param circuit the circuit where to add the start port
    */
   void add_start_port(structural_objectRef circuit, structural_managerRef SM);

   /**
    * Adds the command ports to a circuit. Called by add_common_ports
    * Command ports are of three types:
    * - Selectors to enable functional units in the datapath, these go from the controller to the datapath
    * - Selectors of mux in the datapath, these go from the controller to the datapath
    * - Conditions, used to modify instruction flow in constructs such as for, if, while,
    *   these go in the opposite direction, from the datapath to the controller
    * \param circuit the circuit where to add the command ports
    */
   void add_command_ports(structural_objectRef circuit, structural_managerRef SM);
};
/// refcount definition for the class
typedef refcount<ControllerCreatorBaseStep> ControllerCreatorBaseStepRef;

#endif
