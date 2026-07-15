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
 * @file ReadAxisHDLGenerator.hpp
 * @brief Declaration of the HDL generator for an AXI-Stream read interface.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _READ_AXIS_HDL_GENERATOR_HPP_
#define _READ_AXIS_HDL_GENERATOR_HPP_

#include "HDLGenerator.hpp"

class ReadAxisHDLGenerator : public HDLGenerator::Registrar<ReadAxisHDLGenerator>
{
 public:
   ReadAxisHDLGenerator(const HLS_managerRef& HLSMgr);

   void InternalExec(std::ostream& out, structural_objectRef mod, unsigned int function_id, gc_vertex_descriptor op_v,
                     const HDLWriter_Language language, const std::vector<HDLGenerator::parameter>& _p,
                     const std::vector<HDLGenerator::parameter>& _ports_in,
                     const std::vector<HDLGenerator::parameter>& _ports_out,
                     const std::vector<HDLGenerator::parameter>& _ports_inout) final;
};

#endif
