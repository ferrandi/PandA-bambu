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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file NP_functionality.hpp
 * @brief Not parsed functionality manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
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

class xml_element;

#define START_LIBRARY_PARAMETER 1
#define LIBRARY_NAME 0

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
      FSM_STAGES,
      SC_PROVIDED,
      VHDL_PROVIDED,
      VERILOG_PROVIDED,
      SYSTEM_VERILOG_PROVIDED,
      VERILOG_GENERATOR,
      VHDL_GENERATOR,
      BAMBU_PROVIDED,
      IP_COMPONENT,
      IP_INCLUDE,
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
   NP_functionality() = default;

   explicit NP_functionality(const NP_functionalityRef& obj);

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

   void get_port_list(std::map<unsigned int, std::map<std::string, std::string>>& InPortMap,
                      std::map<unsigned int, std::map<std::string, std::string>>& OutPortMap) const;

   /**
    * Load a NP_functionality starting from an xml file.
    * @param Enode is a node of the xml tree.
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
using NP_functionalityRef = refcount<NP_functionality>;

#endif
