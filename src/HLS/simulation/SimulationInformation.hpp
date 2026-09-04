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
 *   Copyright (C) 2024-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file SimulationInformation.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 */
#ifndef SIMULATION_INFORMATION_HPP
#define SIMULATION_INFORMATION_HPP

#include <map>
#include <string>
#include <vector>

class SimulationInformation
{
 public:
   /// every element of this vector maps the parameters of the top function
   //  to be tested onto strings representing their values for in a certain
   //  test vector
   std::vector<std::map<std::string, std::string>> test_vectors;
};

#endif
