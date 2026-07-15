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
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file evaluation_mode.hpp
 * @brief Evaluation modes
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef EVALUATION_MODE_HPP
#define EVALUATION_MODE_HPP

#include <array>
#include <string>
#include <string_view>

#define FOR_EACH_EVALUATION_MODE(X) \
   X(NONE)                          \
   X(DRY_RUN)                       \
   X(SIMULATION)                    \
   X(BACKEND)                       \
   X(FULL)

struct EvaluationMode
{
   enum evaluation_mode
   {
#define EVALUATION_MODE_ENUMERATOR(name) name,
      FOR_EACH_EVALUATION_MODE(EVALUATION_MODE_ENUMERATOR)
#undef EVALUATION_MODE_ENUMERATOR
          ERROR
   };

   static constexpr auto namedModes = std::array{
#define EVALUATION_MODE_NAME(name) std::string_view{#name},
       FOR_EACH_EVALUATION_MODE(EVALUATION_MODE_NAME)
#undef EVALUATION_MODE_NAME
   };

   static const std::string& to_string(enum evaluation_mode v);

   static enum evaluation_mode to_evaluation_mode(const std::string& v);
};

#undef FOR_EACH_EVALUATION_MODE

#endif // EVALUATION_MODE_HPP
