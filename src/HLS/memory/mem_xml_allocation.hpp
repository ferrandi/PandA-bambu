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
 * @file mem_xml_allocation.hpp
 * @brief Parsing of memory allocation described in XML
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef _MEMORY_XML_ALLOCATION_HPP_
#define _MEMORY_XML_ALLOCATION_HPP_

#include "memory_allocation.hpp"
REF_FORWARD_DECL(memory_symbol);
#include "custom_map.hpp"

class mem_xml_allocation : public memory_allocation
{
   /**
    * Imports the memory configuration from XML file
    */
   bool parse_xml_allocation(const std::string& xml_file);

   /**
    * Parses the memory allocation related to external variables
    */
   void parse_external_allocation(const xml_element* node);

   /**
    * Parses the memory allocation related to internal variables
    */
   void parse_internal_allocation(const xml_element* node);

   /**
    * Performs a final analysis of the memory allocation to finalize the datastructure
    */
   void finalize_memory_allocation();

   /// map of symbols that are externally allocated
   std::map<unsigned int, memory_symbolRef> ext_variables;

   /// map of symbols that are internally allocated
   std::map<unsigned int, std::map<unsigned int, memory_symbolRef>> int_variables;

   /// map of symbols that refer to parameters
   std::map<unsigned int, std::map<unsigned int, memory_symbolRef>> param_variables;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    */
   mem_xml_allocation(const ParameterConstRef Param, const HLS_managerRef HLSMgr, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~mem_xml_allocation() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif
