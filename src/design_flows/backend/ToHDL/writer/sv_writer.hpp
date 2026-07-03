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
 * @brief Class for system verilog writing. Currently only system verilog provided descriptions are managed.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef SYSTEM_VERILOG_WRITER_HPP
#define SYSTEM_VERILOG_WRITER_HPP

#include "verilog_writer.hpp"

class system_verilog_writer : public verilog_writer
{
 public:
   /**
    * Return the name of the language writer.
    */
   std::string get_name() const override
   {
      return "system_verilog";
   }
   /**
    * Return the filename extension associted with the verilog_writer.
    */
   std::string get_extension() const override
   {
      return ".sv";
   }

   /**
    * Write in the proper language the behavioral description of the module described in "Not Parsed" form.
    * @param cir is the component.
    */
   void write_NP_functionalities(const structural_objectRef& cir) override;

   explicit system_verilog_writer(const ParameterConstRef parameters);
};

#endif
