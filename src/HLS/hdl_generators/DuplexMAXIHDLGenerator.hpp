/*
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#ifndef _DUPLEX_MAXI_HDL_GENERATOR_HPP_
#define _DUPLEX_MAXI_HDL_GENERATOR_HPP_

#include "HDLGenerator.hpp"

class DuplexMAXIHDLGenerator : public HDLGenerator::Registrar<DuplexMAXIHDLGenerator>
{
 public:
   DuplexMAXIHDLGenerator(const HLS_managerRef& HLSMgr);

   void InternalExec(std::ostream& out, structural_objectRef mod, unsigned int function_id, gc_vertex_descriptor op_v,
                     const HDLWriter_Language language, const std::vector<HDLGenerator::parameter>& _p,
                     const std::vector<HDLGenerator::parameter>& _ports_in,
                     const std::vector<HDLGenerator::parameter>& _ports_out,
                     const std::vector<HDLGenerator::parameter>& _ports_inout) final;
};

#endif
