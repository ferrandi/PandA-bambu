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
 * @file HDLGeneratorManager.hpp
 * @brief Declaration of the manager responsible for instantiating and driving HDL generators.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _HDL_GENERATOR_MANAGER_HPP_
#define _HDL_GENERATOR_MANAGER_HPP_
#include "custom_map.hpp"
#include "generic_device.hpp"
#include "graph.hpp"
#include "refcount.hpp"

class module_o;
CONSTREF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(Parameter);
enum class HDLWriter_Language;
REF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(structural_type_descriptor);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(technology_node);

class HDLGeneratorManager
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
   HDLGeneratorManager(const HLS_managerRef HLSMgr, const ParameterConstRef parameters);

   virtual ~HDLGeneratorManager() = default;

   std::string GenerateHDL(const std::string& hdl_template, structural_objectRef mod, const FunctionBehaviorConstRef FB,
                           gc_vertex_descriptor op_v,
                           const std::vector<std::tuple<unsigned int, unsigned int>>& required_variables,
                           HDLWriter_Language language);

   std::string getModuleNameSuffix(unsigned int firstIndexToSpecialize,
                                   const std::vector<std::tuple<unsigned int, unsigned int>>& required_variables) const;

   technology_nodeRef specialize_fu(const std::string& fu_name, gc_vertex_descriptor ve,
                                    const FunctionBehaviorConstRef FB, const std::string& libraryId,
                                    const std::string& new_fu_name);

   technology_nodeRef create_generic_module(const std::string& fu_name, gc_vertex_descriptor ve,
                                            const FunctionBehaviorConstRef FB, const std::string& libraryId,
                                            const std::string& new_fu_name);
};
using HDLGeneratorManagerRef = refcount<HDLGeneratorManager>;
#endif
