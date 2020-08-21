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
 * @file NP_functionality.hpp
 * @brief Not parsed functionality manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef NP_FUNCTIONALITY_HPP
#define NP_FUNCTIONALITY_HPP
#include "custom_map.hpp"
#include "refcount.hpp"
#include <ostream>
#include <string>
#include <vector>

REF_FORWARD_DECL(NP_functionality);

/**
 * @name Forward decl of xml Element.
 */
class xml_element;
//@}

/**
 * @name LIBRARY parameters positions.
 */
//@{
#define START_LIBRARY_PARAMETER 1
#define LIBRARY_NAME 0
//@}

/// LIBRARY Declaration extension. It has probably mean only for the systemC backend.
#define LIBRARY_DECL_SUFFIX_EXT "_DECL"

/**
 * Not parsed functionality descriptor of a module.
 * Usually a behavior is parsed through the C/C++/SystemC front-end.
 * This class is used in all cases where an alternative description is provided:
 *  - true tables
 *  - library based descriptions
 *  - graph based descriptions(e.g. NFA)
 *  - SystemC provided descriptions (not parsed)
 *  - VHDL provided descriptions
 *  - Verilog provided descriptions
 * Library based descriptions (e.g., LIBRARY) are based on a string containing the name of the library,
 * a list of port names for which a parameters has to be specified.
 * Port vectors are parametrized with the number of port associated,
 * while ports are parametrized in case the type is a integer with the number of bits.
 */
class NP_functionality
{
 public:
   /// functionality type descriptors.
   /// Currently, it is foreseen six type of descriptions:
   /// - True Tables
   /// - Library based descriptions
   /// - Graph based descriptions like Non deterministic finite automata (NFA)
   /// - Finite state machine description of Moore/Mealy machine
   /// - SystemC provided descriptions (not parsed)
   /// - VHDL provided descriptions
   /// - Verilog provided descriptions
   /// - System  Verilog provided descriptions
   enum NP_functionaly_type
   {
      TABLE = 0,
      EQUATION,
      PORT_LIST,
      LIBRARY,
      GRAPH,
      FSM,
      FSM_CS,
      SC_PROVIDED,
      VHDL_PROVIDED,
      VERILOG_PROVIDED,
      SYSTEM_VERILOG_PROVIDED,
      VERILOG_GENERATOR,
      VHDL_GENERATOR,
      FLOPOCO_PROVIDED,
      BAMBU_PROVIDED,
      BLIF_DESCRIPTION,
      AIGER_DESCRIPTION,
      IP_COMPONENT,
      IP_LIBRARY,
      VERILOG_FILE_PROVIDED,
      VHDL_FILE_PROVIDED,
      UNKNOWN
   };

 private:
   /// Store the description of the functionality.
   std::map<NP_functionaly_type, std::string> descriptions;
   /// store the names of the enumerative NP_functionaly_type.
   static const char* NP_functionaly_typeNames[];
   /**
    * Convert a string into the corresponding NP_functionaly_type enumerative type
    * @param val is the string version of the enum.
    */
   NP_functionaly_type to_NP_functionaly_type(const std::string& val);

 public:
   /// Constructor.
   NP_functionality() = default;

   /// Constructor.
   explicit NP_functionality(const NP_functionalityRef& obj);

   /// Destructor.
   ~NP_functionality() = default;

   /**
    * Add a non SystemC based description.
    */
   void add_NP_functionality(NP_functionaly_type type, const std::string& functionality_description);

   /**
    * Return the description provided the type
    */
   std::string get_NP_functionality(NP_functionaly_type type) const;

   /**
    * Return true in case there exist a functionaly of the given type
    */
   bool exist_NP_functionality(NP_functionaly_type type) const;

   /**
    * return the name of the library in case it there exists a LIBRARY based description.
    */
   std::string get_library_name() const;

   /**
    * fill a vector with the library parameters in case it there exists a LIBRARY based description.
    * @param parameters is the filled vector.
    */
   void get_library_parameters(std::vector<std::string>& parameters) const;

   void get_port_list(std::map<unsigned int, std::map<std::string, std::string>>& InPortMap, std::map<unsigned int, std::map<std::string, std::string>>& OutPortMap) const;

   /**
    * Load a NP_functionality starting from an xml file.
    * @param node is a node of the xml tree.
    */
   void xload(const xml_element* Enode);

   /**
    * Add a NP_functionality to an xml tree.
    * @param rootnode is the root node at which the xml representation of the non SystemC based description is attached.
    */
   void xwrite(xml_element* rootnode);

   /**
    * Print the Non-SystemC based functionality description (for debug purpose).
    * @param os is the output stream
    */
   void print(std::ostream& os) const;

   /**
    * Definition of get_kind_text()
    */
   std::string get_kind_text() const
   {
      return std::string("NP_functionality");
   }
};

/**
 * RefCount type definition of the connection class structure.
 */
typedef refcount<NP_functionality> NP_functionalityRef;

#endif
