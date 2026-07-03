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
 * @file verilog_writer.hpp
 * @brief This class defines the methods to write Verilog descriptions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef VERILOG_WRITER_HPP
#define VERILOG_WRITER_HPP
#include "language_writer.hpp"

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

class verilog_writer : public language_writer
{
 protected:
   /// map putting into relation standard gates with the corresponding built-in Verilog statements.
   static const std::map<std::string, std::string> builtin_to_verilog_keyword;

   static const std::set<std::string> keywords;

   void WriteBuiltin_ASSIGN_GATE_STD(const structural_objectConstRef component);

 public:
   explicit verilog_writer(const ParameterConstRef parameters);

   std::string get_name() const override
   {
      return "verilog";
   }

   std::string get_extension() const override
   {
      return ".v";
   }

   void write_comment(const std::string& comment_string) override;

   std::string type_converter(structural_type_descriptorRef Type) override;

   std::string type_converter_size(const structural_objectRef& cir) override;

   /**
    * return the slice in case of a port owned by a port vector
    * @param cir is the port
    * @return a string in case of a port owned by a port vector
    */
   std::string may_slice_string(const structural_objectRef& cir);

   void write_library_declaration(const structural_objectRef& cir) override;

   void write_module_declaration(const structural_objectRef& cir, bool is_library) override;

   void write_module_internal_declaration(const structural_objectRef& cir) override;

   void write_port_declaration(const structural_objectRef& cir, bool last_port_to_analyze) override;

   void write_component_declaration(const structural_objectRef& cir) override;

   void write_signal_declaration(const structural_objectRef& cir) override;

   void write_module_definition_begin(const structural_objectRef& cir) override;

   void write_module_instance_begin(const structural_objectRef& cir, const std::string& module_name,
                                    bool write_parametrization) override;

   void write_module_instance_end(const structural_objectRef& cir) override;

   void write_port_binding(const structural_objectRef& port, const structural_objectRef& object_bounded,
                           bool first_port_analyzed) override;

   void write_vector_port_binding(const structural_objectRef& port, bool first_port_analyzed) override;

   void write_module_definition_end(const structural_objectRef& cir, bool is_library) override;

   void write_io_signal_post_fix(const structural_objectRef& port, const structural_objectRef& sig) override;

   void write_io_signal_post_fix_vector(const structural_objectRef& port, const structural_objectRef& sig) override;

   void write_module_parametrization(const structural_objectRef& cir) override;

   void write_state_declaration(const structural_objectRef& cir, const std::list<std::string>& list_of_states,
                                const std::string& reset_port, const std::string& reset_state, bool one_hot) override;

   void write_stage_declaration(const structural_objectRef& cir, int nStages) override;

   void write_present_state_update(const structural_objectRef cir, const std::string& reset_state,
                                   const std::string& reset_port, const std::string& clock_port,
                                   const std::string& reset_type, bool connect_present_next_state_signals) override;

   void write_present_stages_update(const structural_objectRef cir, const std::string& reset_port,
                                    const std::string& clock_port, const std::string& reset_type,
                                    const std::string& start_port, int nStages) override;

   void write_transition_output_functions(
       bool single_proc, unsigned int output_index, const structural_objectRef& cir, const std::string& reset_state,
       const std::string& reset_port, const std::string& start_port, const std::string& clock_port,
       std::vector<std::string>::const_iterator& first, std::vector<std::string>::const_iterator& end, bool is_yosys,
       const std::map<unsigned int, std::map<std::string, std::set<unsigned int>>>& bypass_signals,
       const std::string& fsm_stage_i) override;

   void write_NP_functionalities(const structural_objectRef& cir) override;

   void write_port_decl_header() override;

   void write_port_decl_tail() override;

   void write_module_parametrization_decl(const structural_objectRef& cir) override;

   void write_assign(const std::string& op0, const std::string& op1) override;

   bool has_output_prefix() const override
   {
      return true;
   }

   bool check_keyword(const std::string& id) const override;

   void write_header(bool) override;

   void WriteBuiltin(const structural_objectConstRef component) override;

   static bool check_keyword_verilog(const std::string& word);
};

#endif
