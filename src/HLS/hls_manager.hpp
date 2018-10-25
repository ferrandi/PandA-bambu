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
 *              Copyright (c) 2004-2018 Politecnico di Milano
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
 * @file hls_manager.hpp
 * @brief Data structure representing the entire HLS information
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
*/
#ifndef HLS_MANAGER_HPP
#define HLS_MANAGER_HPP

///Superclass include
#include "application_manager.hpp"

///Autoheader include
#include "config_HAVE_TASTE.hpp"

///utility include
#include "custom_map.hpp"

REF_FORWARD_DECL(AadlInformation);
REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(HLS_target);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(functions);
REF_FORWARD_DECL(memory);
REF_FORWARD_DECL(SimulationInformation);
REF_FORWARD_DECL(BackendFlow);

class HLS_manager : public application_manager
{
   public:
      ///tuple set used to represent the required values or the constant default value associated with the inputs of a node
      typedef std::tuple<unsigned int, unsigned int> io_binding_type;

   private:
      /// information about the target device/technology for the synthesis
      HLS_targetRef HLS_T;

      /// map between the function id and the corresponding HLS datastructure
      std::map<unsigned int, hlsRef> hlsMap;

      /// reference to the datastructure implementing the backend flow
      BackendFlowRef back_flow;

   public:
      /// base address for memory space addressing
      unsigned int base_address;

      /// HLS execution time
      long HLS_execution_time;

      ///information about function allocation
      functionsRef Rfuns;

      ///information about memory allocation
      memoryRef Rmem;

      ///information about the simulation
      SimulationInformationRef RSim;

      /// Evaluations
      CustomMap<std::string, std::vector<double> > evaluations;

      ///The auxiliary files
      std::list<std::string> aux_files;

      ///The HDL files
      std::list<std::string> hdl_files;

      /**
       * A map to store the vcd signals to be dumped. The key is the scope, and
       * the mapped set contains all the signals to be dumped for that scope
       */
      std::map<std::string, std::set<std::string> > selected_vcd_signals;

#if HAVE_TASTE
      ///The information collected from aadl files
      const AadlInformationRef aadl_information;
#endif

      /// store the design interface directives coming from an xml file: function_name->parameter_name->interface_type
      std::map<std::string,std::map<std::string,std::string>> design_interface;

      /// store the constraints on resources added to manage the I/O interfaces: function_id->library_name->resource_function_name->number of resources
      std::map<unsigned,std::map<std::string,std::map<std::string,unsigned int>>> design_interface_constraints;

      /**
       * Constructor.
       */
      HLS_manager(const ParameterConstRef Param, const HLS_targetRef HLS_T);

      /**
       * Destructor.
       */
      ~HLS_manager() override;

      /**
       * Returns the HLS datastructure associated with a specific function
       */
      hlsRef get_HLS(unsigned int funId) const;

      /**
       * Creates the HLS flow starting from the given specification
       */
      static
      hlsRef create_HLS(const HLS_managerRef HLSMgr, unsigned int functionId);

      /**
       * Returns the datastructure associated with the HLS target
       */
      HLS_targetRef get_HLS_target() const;

      /**
       * Returns the backend flow
       */
      const BackendFlowRef get_backend_flow();

      /**
       * Return the specified constant in string format
       */
      std::string get_constant_string(unsigned int node, unsigned int precision);

      /**
       * Writes the current HLS project into an XML file
       */
      void xwrite(const std::string& filename);

      /**
       * Returns the values required by a vertex
       */
      std::vector<io_binding_type> get_required_values(unsigned int fun_id, const vertex& v) const;

      /**
       * helper function that return true in case the variable is register compatible
       * @param var is the variable
       * @return true in case var is register compatible
       */
      bool is_register_compatible(unsigned int var) const;

      /**
       * Returns all the implementations resulting from the synthesis
       */
      std::set<hlsRef> GetAllImplementations() const;

      /**
       * Return if single write memory is exploited
       */
      bool IsSingleWriteMemory() const;
};
///refcount definition of the class
typedef refcount<HLS_manager> HLS_managerRef;

#endif
