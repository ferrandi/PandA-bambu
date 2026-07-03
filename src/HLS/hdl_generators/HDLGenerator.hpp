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
 *              Copyright (C) 2022-2026 Politecnico di Milano
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
 * @file HDLGenerator.hpp
 * @brief Base interface and registration helpers for plugin-based HDL generators.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _HDL_GENERATOR_HPP_
#define _HDL_GENERATOR_HPP_

#include "Factory.hpp"
#include "graph.hpp"
#include "refcount.hpp"

#include <functional>
#include <iostream>
#include <string>
#include <vector>

class module_o;
enum class HDLWriter_Language;
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(structural_object);

class HDLGenerator : public Factory<HDLGenerator, const HLS_managerRef&>
{
 public:
   struct parameter
   {
      std::string name;
      std::string type;
      unsigned long long type_size;
      unsigned long long alignment;

      parameter() = default;
      parameter(const structural_objectRef& port);
   };

 protected:
   const HLS_managerRef HLSMgr;

   HDLGenerator(Key, const HLS_managerRef& _HLSMgr) : HLSMgr(_HLSMgr)
   {
   }

   std::string add_port(const structural_objectRef& circuit, const std::string& port_name, int dir, unsigned port_size,
                        bool parametric = false) const;

   virtual void InternalExec(std::ostream& out, structural_objectRef mod, unsigned int function_id,
                             gc_vertex_descriptor op_v, const HDLWriter_Language language,
                             const std::vector<parameter>& _p, const std::vector<parameter>& _ports_in,
                             const std::vector<parameter>& _ports_out, const std::vector<parameter>& _ports_inout) = 0;

 public:
   virtual ~HDLGenerator() = default;

   void Exec(std::ostream& out, structural_objectRef mod, unsigned int function_id, gc_vertex_descriptor op_v,
             const std::vector<parameter>& _p, const HDLWriter_Language language);
};

#endif
