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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file ModuleGeneratorManager.hpp
 * @brief
 *
 *
 *
 * @author Alessandro Nacci <alenacci@gmail.com>
 * @author Gianluca Durelli <durellinux@gmail.com>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _MODULE_GENERATOR_HPP_
#define _MODULE_GENERATOR_HPP_
#include "custom_map.hpp"
#include "generic_device.hpp"
#include "graph.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(FunctionBehavior);
enum class HDLWriter_Language;
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(structural_type_descriptor);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(technology_node);
REF_FORWARD_DECL(application_manager);
class module;

class ModuleGeneratorManager
{
 protected:
   /// The HLS manager
   const HLS_managerRef HLSMgr;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   const int debug_level;

 public:
   /**
    * Constructor.
    * @param HLSMgr is the HLS manager
    * @param parameters is the set of input parameters
    */
   ModuleGeneratorManager(const HLS_managerRef HLSMgr, const ParameterConstRef parameters);

   /**
    * Destructor.
    */
   virtual ~ModuleGeneratorManager();

   structural_type_descriptorRef getDataType(unsigned int variable,
                                             const FunctionBehaviorConstRef function_behavior) const;

   void add_port_parameters(structural_objectRef generated_port, structural_objectRef original_port);

   std::string GenerateHDL(const std::string& hdl_template, structural_objectRef mod, unsigned int function_id,
                           vertex op_v, const std::vector<std::tuple<unsigned int, unsigned int>>& required_variables,
                           HDLWriter_Language language);

   std::string get_specialized_name(unsigned int firstIndexToSpecialize,
                                    const std::vector<std::tuple<unsigned int, unsigned int>>& required_variables,
                                    const FunctionBehaviorConstRef FB) const;

   void specialize_fu(const std::string& fu_name, vertex ve, const FunctionBehaviorConstRef FB,
                      const std::string& libraryId, const std::string& new_fu_name,
                      std::map<std::string, technology_nodeRef>& new_fu);

   void create_generic_module(const std::string& fu_name, vertex ve, const FunctionBehaviorConstRef FB,
                              const std::string& libraryId, const std::string& new_fu_name);
};
using ModuleGeneratorManagerRef = refcount<ModuleGeneratorManager>;
#endif
