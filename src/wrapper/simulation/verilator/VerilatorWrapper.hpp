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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
/**
 * @file VerilatorWrapper.hpp
 * @brief Wrapper to Verilator simulator
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef VERILATOR_WRAPPER_HPP
#define VERILATOR_WRAPPER_HPP
#include "SimulationTool.hpp"

#include <filesystem>

/**
 * @class VerilatorWrapper
 * Main class for wrapping Verilator.
 */
class VerilatorWrapper : public SimulationTool
{
   void GenerateVerilatorMain(const std::filesystem::path& filename) const;

   std::string GenerateScript(std::ostream& script, const std::string& top_filename,
                              const std::list<std::string>& file_list) override;

 public:
   /**
    * Constructor
    * @param Param is the set of parameters
    */
   VerilatorWrapper(const ParameterConstRef& Param, const std::string& top_fname, const std::string& inc_dirs);
};

#endif
