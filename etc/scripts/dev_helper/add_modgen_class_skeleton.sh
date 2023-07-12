#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
modgen_dir="$(readlink -f ${script_dir}/../../../src/HLS/module_generator)"

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
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
EOM

if [ "$#" != "1" ]; then
   echo "usage: $0 <ClassPrefix>"
   exit -1
fi

classname="$1"

define_name="$(sed -r 's/([a-z0-9])([A-Z])/\1_\L\2/g' <<< $classname)"

out_base="${modgen_dir}/${classname}ModuleGenerator"
out_hpp="${out_base}.hpp"
out_cpp="${out_base}.cpp"

cat > ${out_cpp} << EOF
${copyright_notice}
/**
 * @file ${classname}ModuleGenerator.cpp
 * @brief
 *
 * @author Name Surname <your.contact@email.com>
 * \$Revision\$
 * \$Date\$
 * Last modified by \$Author\$
 *
 */

#include "${classname}ModuleGenerator.hpp"

#include "language_writer.hpp"

${classname}ModuleGenerator::${classname}ModuleGenerator(const HLS_managerRef& _HLSMgr) : Registrar(_HLSMgr)
{
}

void ${classname}ModuleGenerator::InternalExec(std::ostream& out, structural_objectRef /* mod */, unsigned int /* function_id */,
                                            vertex /* op_v */, const HDLWriter_Language /* language */,
                                            const std::vector<ModuleGenerator::parameter>& /* _p */,
                                            const std::vector<ModuleGenerator::parameter>& /* _ports_in */,
                                            const std::vector<ModuleGenerator::parameter>& /* _ports_out */,
                                            const std::vector<ModuleGenerator::parameter>& /* _ports_inout */)
{
   out << "${classname}" << std::endl;
}
EOF

cat > ${out_hpp} << EOF
${copyright_notice}
/**
 * @file ${classname}ModuleGenerator.hpp
 * @brief
 *
 * @author Name Surname <your.contact@email.com>
 * \$Revision\$
 * \$Date\$
 * Last modified by \$Author\$
 *
 */
#ifndef _${define_name^^}_MODULE_GENERATOR_HPP_
#define _${define_name^^}_MODULE_GENERATOR_HPP_

#include "ModuleGenerator.hpp"

class ${classname}ModuleGenerator : public ModuleGenerator::Registrar<${classname}ModuleGenerator>
{
 public:
   ${classname}ModuleGenerator(const HLS_managerRef& HLSMgr);

   void InternalExec(std::ostream& out, structural_objectRef mod, unsigned int function_id, vertex op_v,
                     const HDLWriter_Language language, const std::vector<ModuleGenerator::parameter>& _p,
                     const std::vector<ModuleGenerator::parameter>& _ports_in,
                     const std::vector<ModuleGenerator::parameter>& _ports_out,
                     const std::vector<ModuleGenerator::parameter>& _ports_inout) final;
};

#endif
EOF

echo "Module generator derive class skeleton added in ${out_base}.{hpp,cpp}"
echo "TODO:"
echo "  - Fill in the ${classname}ModuleGenerator::InternalExec with the implementation"
echo "  - Fill in the file description and @author filed at the top of each file"
echo "  - Add files to noinst_HEADERS and lib_module_generator_la_SOURCE in src/HLS/Makefile.am"
