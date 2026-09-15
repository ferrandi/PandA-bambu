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
 * @file controller_creator_base_step.hpp
 * @brief Base class for all the controller creation algorithms.
 *
 * This class is a pure virtual one, that has to be specilized in order to implement a particular algorithm to create
 * the controller.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */

#ifndef CONTROLLER_CREATOR_BASE_STEP_HPP
#define CONTROLLER_CREATOR_BASE_STEP_HPP
#include "hls_function_step.hpp"

#include "FSMInfo.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "graph.hpp"
#include "refcount.hpp"

#include <fstream>
#include <tuple>

REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(generic_obj);
REF_FORWARD_DECL(structural_manager);
class xml_element;

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
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   ControllerCreatorBaseStep(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                             const DesignFlowManager& design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);

 protected:
   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * This member function adds the standard ports (clock, reset, done and command ones) to a circuit.
    * \param circuit it is the data-structure of the component where to add these ports
    * \param SM is the structural manager owning the circuit
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
   std::map<gc_vertex_descriptor, unsigned int> cond_ports;
   /// This map put into relation fsm states and alldone multi_unbounded ports
   std::map<FSMInfo::state_descriptor, unsigned int> mu_ports;

   /// Initialized after add_common_ports is called. It represents the current number of output ports
   unsigned int out_num;

   /// Initialized after add_common_ports is called. It represents the current number of input ports
   unsigned int in_num;

 private:
   /**
    * Adds the clock and reset ports to a circuit. Called by add_common_ports
    * \param circuit the circuit where to add the clock and reset ports
    * \param SM is the structural manager owning the circuit
    */
   void add_clock_reset(structural_objectRef circuit, structural_managerRef SM);

   /**
    * Adds the done port to a circuit. Called by add_common_ports
    * The done port appears to go high once all the calculation of a function are completed
    * \param circuit the circuit where to add the done port
    * \param SM is the structural manager owning the circuit
    */
   void add_done_port(structural_objectRef circuit, structural_managerRef SM);

   /**
    * Adds the start port to a circuit. Called by add_common_ports
    * \param circuit the circuit where to add the start port
    * \param SM is the structural manager owning the circuit
    */
   void add_start_port(structural_objectRef circuit, structural_managerRef SM);

   /**
    * Adds the idle port to a circuit. Called by add_common_ports, only for top functions.
    * The idle port is high when the top is not operating (no computation in progress).
    * \param circuit the circuit where to add the idle port
    * \param SM is the structural manager owning the circuit
    */
   void add_idle_port(structural_objectRef circuit, structural_managerRef SM);

   /**
    * Adds the command ports to a circuit. Called by add_common_ports
    * Command ports are of three types:
    * - Selectors to enable functional units in the datapath, these go from the controller to the datapath
    * - Selectors of mux in the datapath, these go from the controller to the datapath
    * - Conditions, used to modify instruction flow in constructs such as for, if, while,
    *   these go in the opposite direction, from the datapath to the controller
    * \param circuit the circuit where to add the command ports
    * \param SM is the structural manager owning the circuit
    */
   void add_command_ports(structural_objectRef circuit, structural_managerRef SM);
};
/// refcount definition for the class
using ControllerCreatorBaseStepRef = refcount<ControllerCreatorBaseStep>;

#endif
