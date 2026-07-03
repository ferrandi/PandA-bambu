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
 *              Copyright (C) 2025-2026 Politecnico di Milano
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
