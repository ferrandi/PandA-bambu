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
 *   Copyright (C) 2022-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
