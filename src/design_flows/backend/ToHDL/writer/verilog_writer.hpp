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
 * @file verilog_writer.hpp
 * @brief This class defines the methods to write Verilog descriptions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef VERILOG_WRITER_HPP
#define VERILOG_WRITER_HPP

#include "language_writer.hpp"

/// STD include
#include <string>

/// STL include
#include "custom_set.hpp"
#include <list>
#include <map>
#include <vector>

class verilog_writer : public language_writer
{
 protected:
   static const char* tokenNames[];
   /// map putting into relation standard gates with the corresponding built-in Verilog statements.
   static const std::map<std::string, std::string> builtin_to_verilog_keyword;

   CustomOrderedSet<std::string> keywords;

 public:
   /**
    * Return the name of the language writer.
    */
   std::string get_name() const override
   {
      return "verilog";
   }
   /**
    * Return the filename extension associated with the verilog_writer.
    */
   std::string get_extension() const override
   {
      return ".v";
   }
   /**
    * Print a comment.
    * @param comment_string is the string to be printed as a comment.
    */
   void write_comment(const std::string& comment_string) override;
   /**
    * Return a language based type string given a structural_type_descriptor.
    * In case of arrays the range of the array is print by the type_converter_size function.
    * @param Type is the structural_type_descriptor.
    */
   std::string type_converter(structural_type_descriptorRef Type) override;
   /**
    * Return a language based type string given a structural_type_descriptor for the range of the array. In case of scalar type it return an empty string.
    * @param cir is the object for which the size has been computed.
    */
   std::string type_converter_size(const structural_objectRef& cir) override;

   /**
    * return the slice in case of a port owned by a port vector
    * @param port is the port
    * @return a string in case of a port owned by a port vector
    */
   std::string may_slice_string(const structural_objectRef& port);

   /**
    * Write the #include for each used library.
    * @param cir is the component for which the library declarations are written.
    */
   void write_library_declaration(const structural_objectRef& cir) override;
   /**
    * Write the declaration of the module.
    * @param cir is the module to be written.
    */
   void write_module_declaration(const structural_objectRef& cir) override;
   /**
    * Write the declaration of internal objects of the module.
    * @param cir is the module to be written.
    */
   void write_module_internal_declaration(const structural_objectRef& cir) override;
   /**
    * Write the port declaration starting from a port object.
    * @param cir is the port to be written.
    */
   void write_port_declaration(const structural_objectRef& cir, bool last_port_to_analyze) override;
   /**
    * Write the declaration of a component
    * @param cir is the component to be declared.
    */
   void write_component_declaration(const structural_objectRef& cir) override;
   /**
    * Write the declaration of a signal.
    * @param cir is the signal to be declared.
    */
   void write_signal_declaration(const structural_objectRef& cir) override;
   /**
    * Write the top constructor declaration.
    * @param cir is the top component to be declared.
    */
   void write_module_definition_begin(const structural_objectRef& cir) override;
   /**
    * Write the initial part of the instance of a module.
    * @param cir is the module to be instanced.
    * @param component_name is the name of the module to be instanced. It has to be specified since VHDL and verilog can print in different ways
    * @param write_parametrization specified if parameters have to be written
    */
   void write_module_instance_begin(const structural_objectRef& cir, const std::string& component_name, bool write_parametrization) override;

   /**
    * Write the ending part of the instance of a module.
    * @param cir is the module to be instanced.
    */
   void write_module_instance_end(const structural_objectRef& cir) override;
   /**
    * Write the binding of a port. It follows the name binding style.
    * @param port is the port to be bounded.
    * @param top is the component owner of the component that has the port to be bounded.
    */
   void write_port_binding(const structural_objectRef& port, const structural_objectRef& top, bool& first_port_analyzed) override;
   void write_vector_port_binding(const structural_objectRef& port, bool& first_port_analyzed) override;
   /**
    * Write the end part in a module declaration.
    * @param cir is the top component to be declared.
    */
   void write_module_definition_end(const structural_objectRef& cir) override;
   /**
    * Write some code managing primary ports to signals connections.
    * Loop signals are present for example in this bench circuit:
    * INPUT(X)
    * OUTPUT(Z)
    * Y = DFF(Z)
    * Z = AND(X,Y).
    * The circuit builder adds an internal signal Z_sign allowing the write and read of the Z values.
    * In verilog, this problem has been solved in this way:
    * ???;
    * @param port is the primary port for which this problem happens.
    * @param sig is the attached signal.
    */
   void write_io_signal_post_fix(const structural_objectRef& port, const structural_objectRef& sig) override;
   /**
    * Module can be parameterized with respect different features. Port vectors areparameterizedd with the number of port associated,
    * while ports are parameterized in case the type is a integer with the number of bits. The id of the module is modified
    * by adding the parameters at its end. For example an AND_GATE with a port_vector of 2 will be declared as: AND_GATE_2.
    * Moreover, a multiplier with the first input of four bits, the second input with eight bits and an output of twelve bits will be
    * declared as: MULT_4_8_12.
    * Note that parametrization has a meaning only in case the functionality come from the STD technology library.
    * @param cir is the component to be declared.
    */
   void write_module_parametrization(const structural_objectRef& cir) override;
   /**
    * Write the tail part of the file. Write some lines of comments and some debugging code.
    * @param cir is the top component.
    */
   void write_tail(const structural_objectRef& cir) override;
   /**
    * write the declaration of all the states of the finite state machine.
    * @param list_of_states is the list of all the states.
    */
   void write_state_declaration(const structural_objectRef& cir, const std::list<std::string>& list_of_states, const std::string& reset_port, const std::string& reset_state, bool one_hot) override;
   /**
    * write the present_state update process
    * @param reset_state is the reset state.
    * @param reset_port is the reset port.
    * @param clock_port is the clock port.
    * @param reset_type specify the type of the reset
    */
   void write_present_state_update(const structural_objectRef cir, const std::string& reset_state, const std::string& reset_port, const std::string& clock_port, const std::string& reset_type, bool connect_present_next_state_signals) override;

   /**
    * Write the transition and output functions.
    * @param cir is the component.
    * @param reset_port is the reset port.
    * @param clock_port is the clock port.
    * @param first if the first iterator of the state table.
    * @param end if the end iterator of the state table.
    * @param n_states is the number of states.
    */
   void write_transition_output_functions(bool single_proc, unsigned int output_index, const structural_objectRef& cir, const std::string& reset_state, const std::string& reset_port, const std::string& start_port, const std::string& clock_port,
                                          std::vector<std::string>::const_iterator& first, std::vector<std::string>::const_iterator& end) override;

   /**
    * Write in the proper language the behavioral description of the module described in "Not Parsed" form.
    * @param cir is the component.
    */
   void write_NP_functionalities(const structural_objectRef& cir) override;

   /**
    * Write the header for port_decl
    */
   void write_port_decl_header() override;

   /**
    * Write the tail for port_decl
    */
   void write_port_decl_tail() override;

   /**
    * Write the declaration of the module parameters
    */
   void write_module_parametrization_decl(const structural_objectRef& cir) override;

   /**
    * Constructor
    */
   explicit verilog_writer(const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~verilog_writer() override;

   void write_assign(const std::string& op0, const std::string& op1) override;

   bool has_output_prefix() const override
   {
      return true;
   }

   bool check_keyword(std::string id) const override;

   void write_timing_specification(const technology_managerConstRef TM, const structural_objectRef& cir) override;

   void write_header() override;

   /**
    * Write a builtin component
    * @param component is the component to be printed
    */
   void WriteBuiltin(const structural_objectConstRef component) override;
};

#endif
