#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
modgen_dir="$(readlink -f ${script_dir}/../../../src/HLS/hdl_generators)"

read -r -d '' copyright_notice << EOM
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
 *              Copyright (C) $(date +"%Y")-$(date +"%Y") Politecnico di Milano
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
