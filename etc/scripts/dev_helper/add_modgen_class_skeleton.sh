#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
modgen_dir="$(readlink -f ${script_dir}/../../../src/HLS/hdl_generators)"

read -r -d '' copyright_notice << EOM
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
 *   Copyright (C) $(date +"%Y")-$(date +"%Y") Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
EOM

if [ "$#" != "1" ]; then
   echo "usage: $0 <ClassPrefix>"
   exit -1
fi

classname="$1"

define_name="$(sed -r 's/([a-z0-9])([A-Z])/\1_\L\2/g' <<< $classname)"

out_base="${modgen_dir}/${classname}HDLGenerator"
out_hpp="${out_base}.hpp"
out_cpp="${out_base}.cpp"

cat > ${out_cpp} << EOF
${copyright_notice}
/**
 * @file ${classname}HDLGenerator.cpp
 * @brief
 *
 * @author Name Surname <your.contact@email.com>
 * \$Revision\$
 * \$Date\$
 * Last modified by \$Author\$
 *
 */

#include "${classname}HDLGenerator.hpp"

#include "language_writer.hpp"

${classname}HDLGenerator::${classname}HDLGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void ${classname}HDLGenerator::InternalExec(std::ostream& out, structural_objectRef /* mod */, unsigned int /* function_id */,
                                            vertex /* op_v */, const HDLWriter_Language /* language */,
                                            const std::vector<HDLGenerator::parameter>& /* _p */,
                                            const std::vector<HDLGenerator::parameter>& /* _ports_in */,
                                            const std::vector<HDLGenerator::parameter>& /* _ports_out */,
                                            const std::vector<HDLGenerator::parameter>& /* _ports_inout */)
{
   out << "${classname}" << std::endl;
}
EOF

cat > ${out_hpp} << EOF
${copyright_notice}
/**
 * @file ${classname}HDLGenerator.hpp
 * @brief
 *
 * @author Name Surname <your.contact@email.com>
 *
 */
#ifndef _${define_name^^}_HDL_GENERATOR_HPP_
#define _${define_name^^}_HDL_GENERATOR_HPP_

#include "HDLGenerator.hpp"

class ${classname}HDLGenerator : public HDLGenerator::Registrar<${classname}HDLGenerator>
{
 public:
   ${classname}HDLGenerator(const HLS_managerRef& HLSMgr);

   void InternalExec(std::ostream& out, structural_objectRef mod, unsigned int function_id, vertex op_v,
                     const HDLWriter_Language language, const std::vector<HDLGenerator::parameter>& _p,
                     const std::vector<HDLGenerator::parameter>& _ports_in,
                     const std::vector<HDLGenerator::parameter>& _ports_out,
                     const std::vector<HDLGenerator::parameter>& _ports_inout) final;
};

#endif
EOF

echo "Module generator derive class skeleton added in ${out_base}.{hpp,cpp}"
echo "TODO:"
echo "  - Fill in the ${classname}HDLGenerator::InternalExec with the implementation"
echo "  - Fill in the file description and @author filed at the top of each file"
echo "  - Add files to noinst_HEADERS and lib_hdl_generators_la_SOURCE in src/HLS/Makefile.am"
