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
 * @file language_writer.hpp
 * @brief This class writes different HDL based descriptions (VHDL, Verilog, SystemC) starting from a structural representation.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef LANGUAGE_WRITER_HPP
#define LANGUAGE_WRITER_HPP

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_FROM_C_BUILT.hpp"

/// utility include
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "refcount.hpp"
#include "simple_indent.hpp"

#include <list>
#include <string>
#include <vector>

/**
 * @name Forward declarations.
 */
//@{
REF_FORWARD_DECL(IndentedOutputStream);
REF_FORWARD_DECL(language_writer);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(structural_type_descriptor);
CONSTREF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(structural_object);
CONSTREF_FORWARD_DECL(technology_manager);
//@}

#define MEM_PREFIX "MEM_"
#define BITSIZE_PREFIX "BITSIZE_"
#define PORTSIZE_PREFIX "PORTSIZE_"
#define NUM_ELEM_PREFIX "NUM_ELEM_"
enum class HDLWriter_Language
{
   VERILOG = 0,
   SYSTEM_VERILOG,
#if HAVE_EXPERIMENTAL
   SYSTEMC,
   BLIF,
   EDIF,
#endif
   VHDL
};

/**
 * HDL writer base class used to specify the interface of the different language writers
 */
class language_writer
{
 protected:
   inline std::string encode_one_hot(unsigned int n_states, unsigned int val) const
   {
      std::string res;
      for(unsigned int i = 0; i < n_states; ++i)
         res = (val == i ? "1" : "0") + res;
      return res;
   }

   /// Represents the stream we are currently writing to
   const IndentedOutputStreamRef indented_output_stream;

   /// list of library imported (e.g., includes).
   CustomOrderedSet<std::string> list_of_lib;

   /// list of customized gates
   CustomOrderedSet<std::string> list_of_customized_gates;

   /// the set of input parameters
   const ParameterConstRef parameters;

   /// debugging level of the class
   int debug_level;

 public:
   /**
    * Constructor
    */
   language_writer(char open_char, char close_char, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   virtual ~language_writer();

   /**
    * Creates the specialization of the writer based on the desired language
    * @param language_w is the desired language
    * @param TM is the technology manager
    * @param parameters is the set of input parameters
    */
   static language_writerRef create_writer(HDLWriter_Language language, const technology_managerConstRef TM, const ParameterConstRef parameters);

   /**
    * Returns the name of the language writer.
    */
   virtual std::string get_name() const = 0;

   /**
    * Returns the filename extension associated with the specification.
    */
   virtual std::string get_extension() const = 0;

   /**
    * Writes a raw string into the stream.
    * @param rawString is the string to be written.
    */
   virtual void write(const std::string& rawString);

   /**
    * Writes the header part of the file. Write some lines of comments and possibly global libraries.
    */
   virtual void write_header();

   /**
    * Prints a comment.
    * @param comment_string is the string to be printed as a comment.
    */
   virtual void write_comment(const std::string& comment_string) = 0;

   /**
    * Return a language based type string given a structural_type_descriptor.
    * @param Type is the structural_type_descriptor.
    */
   virtual std::string type_converter(structural_type_descriptorRef Type) = 0;

   /**
    * Return a language based size string given the object
    * @param cir is the object
    */
   virtual std::string type_converter_size(const structural_objectRef& cir) = 0;

   /**
    * Write the declaration of the library.
    * @param cir is the component for which the library declarations are written.
    */
   virtual void write_library_declaration(const structural_objectRef& cir) = 0;
   /**
    * Write the declaration of the module.
    * @param cir is the module to be written.
    */
   virtual void write_module_declaration(const structural_objectRef& cir) = 0;
   /**
    * Write the declaration of internal objects of the module.
    * @param cir is the module to be written.
    */
   virtual void write_module_internal_declaration(const structural_objectRef& cir) = 0;
   /**
    * Write the port declaration starting from a port object.
    * @param cir is the port to be written.
    */
   virtual void write_port_declaration(const structural_objectRef& cir, bool first_port_analyzed) = 0;
   /**
    * Write the declaration of componets
    * @param cir is the component to be declared.
    */
   virtual void write_component_declaration(const structural_objectRef& cir) = 0;
   /**
    * Write the declaration of a signal
    * @param cir is the signal to be declared.
    */
   virtual void write_signal_declaration(const structural_objectRef& cir) = 0;
   /**
    * Write the begin part in a module declaration.
    * @param cir is the top component to be declared.
    */
   virtual void write_module_definition_begin(const structural_objectRef& cir) = 0;
   /**
    * Write the initial part of the instance of a module.
    * @param cir is the module to be instanced.
    * @param component_name is the name of the module to be instanced. It has to be specified since VHDL and verilog can print in different ways
    */
   virtual void write_module_instance_begin(const structural_objectRef& cir, const std::string& module_name, bool write_parametrization) = 0;
   /**
    * Write the ending part of the instance of a module.
    * @param cir is the module to be instanced.
    */
   virtual void write_module_instance_end(const structural_objectRef& cir) = 0;
   /**
    * Write the binding of a port. It follows the name binding style.
    * @param port is the port to be bounded.
    * @param top is the component owner of the component that has the port to be bounded.
    */
   virtual void write_port_binding(const structural_objectRef& port, const structural_objectRef& top, bool& first_port_analyzed) = 0;

   virtual void write_vector_port_binding(const structural_objectRef& port, bool& first_port_analyzed) = 0;
   /**
    * Write the end part in a module declaration.
    * @param cir is the top component to be declared.
    */
   virtual void write_module_definition_end(const structural_objectRef& cir) = 0;
   /**
    * Write some code managing primary ports to signals connections.
    * Loop signals are present for example in this bench circuit:
    * INPUT(X)
    * OUTPUT(Z)
    * Y = DFF(Z)
    * Z = AND(X,Y)
    * The circuit builder adds an internal signal Z_sign allowing the write and read of the Z values.
    * @param port is the primary port for which this problem happens.
    * @param sig is the attached signal.
    */
   virtual void write_io_signal_post_fix(const structural_objectRef& port, const structural_objectRef& sig) = 0;
   /**
    * Module can be parametrized with respect different features. Port vectors are parametrized with the number of port associated,
    * while ports are parametrized in case the type is a integer with the number of bits. The id of the module is modified
    * by adding the parameters at its end. For example an AND_GATE with a port_vector of 2 will be declared as: AND_GATE_2.
    * Moreover, a multiplier with the first input of four bits, the second input with eight bits and an output of twelve bits will be
    * declared as: MULT_4_8_12.
    * Note that parametrization has a meaning only in case the functionality come from the STD technology library.
    * @param cir is the component to be declared.
    */
   virtual void write_module_parametrization(const structural_objectRef& cir) = 0;
   /**
    * Write the tail part of the file. Write some lines of comments and some debugging code.
    * @param cir is the top component.
    */
   virtual void write_tail(const structural_objectRef& cir) = 0;
   /**
    * write the declaration of all the states of the finite state machine.
    * @param list_of_states is the list of all the states.
    */
   virtual void write_state_declaration(const structural_objectRef& cir, const std::list<std::string>& list_of_states, const std::string& reset_port, const std::string& reset_state, bool one_hot) = 0;
   /**
    * write the present_state update process
    * @param reset_state is the reset state.
    * @param reset_port is the reset port.
    * @param clock_port is the clock port.
    * @param synch_reset when true the FSM will have an synchronous reset
    */
   virtual void write_present_state_update(const structural_objectRef cir, const std::string& reset_state, const std::string& reset_port, const std::string& clock_port, const std::string& reset_type, bool connect_present_next_state_signals) = 0;
   /**
    * Write the transition and output functions.
    * @param cir is the component.
    * @param reset_port is the reset port.
    * @param clock_port is the clock port.
    * @param first if the first iterator of the state table.
    * @param end if the end iterator of the state table.
    * @param n_states is the number of states.
    */
   virtual void write_transition_output_functions(bool single_proc, unsigned int output_index, const structural_objectRef& cir, const std::string& reset_state, const std::string& reset_port, const std::string& start_port, const std::string& clock_port,
                                                  std::vector<std::string>::const_iterator& first, std::vector<std::string>::const_iterator& end) = 0;

   /**
    * Write in the proper language the behavioral description of the module described in "Not Parsed" form.
    * @param cir is the component.
    */
   virtual void write_NP_functionalities(const structural_objectRef& cir) = 0;

   /**
    * Write the header for generics
    */
   virtual void write_port_decl_header() = 0;

   /**
    * Write the tail for generics
    */
   virtual void write_port_decl_tail() = 0;

   /**
    * Write the declaration of the module parameters
    */
   virtual void write_module_parametrization_decl(const structural_objectRef& cir) = 0;

   virtual void write_assign(const std::string& op0, const std::string& op1) = 0;

   virtual bool has_output_prefix() const = 0;

   /**
    * Counts the number of bits in an unsigned int.
    * @param n is the number.
    */
   static unsigned int bitnumber(unsigned long long n);

   virtual bool check_keyword(std::string id) const = 0;

   /**
    * Write a builtin component
    * @param component is the component to be printed
    */
   virtual void WriteBuiltin(const structural_objectConstRef component) = 0;

   virtual void write_timing_specification(const technology_managerConstRef TM, const structural_objectRef& cir);

   /**
    * Dump the content of the write as a string
    */
   const std::string WriteString() const;

   /**
    * Write content to a file
    * @param std::stringg filename;
    */
   void WriteFile(const std::string& filename) const;

   /**
    * Return the names of auxiliary signals which will be used by backend
    */
   CustomSet<std::string> GetHDLReservedNames() const;

   /**
    * Write the license
    */
   void WriteLicense();
};
/// RefCount definition of the class
typedef refcount<language_writer> language_writerRef;

#endif
