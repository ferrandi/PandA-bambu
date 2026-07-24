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
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
