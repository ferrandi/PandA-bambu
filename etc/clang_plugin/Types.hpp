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
#include "panda_clang_compat.hpp"

#ifndef TYPES_HPP
#define TYPES_HPP

#if PANDA_LLVM_CLANG_MAJOR >= 13

#include "OpenMPTaskPassConfig.hpp"

#include <set>
#include <unordered_map>
#include <vector>

namespace Bambu
{
   namespace OpenMP
   {
      namespace Task
      {
         /**
          * @enum InputType
          * @brief Enum representing the type of input.
          *
          * This enum is used to specify the type of input in the context of OpenMP tasks.
          *
          * @ref InputType::Base
          * Represents a base reference in the SSA (Static Single Assignment) form of LLVM IR, not the memory address
          * itself.
          *
          * @ref InputType::Offset
          * Represents an offset amount.
          */
         enum class InputType
         {
            Base,
            Offset
         };

         using BaseRef = int;
         using OffsetAmount = unsigned long;

         using InputId = unsigned;
         using InputIdIndex = unsigned;

         using Inputs = std::vector<std::tuple<InputType, BaseRef, OffsetAmount>>;
         using ConstInputs = std::vector<unsigned>;
         using AdjMap = std::unordered_map<std::string, std::set<std::string>>;
         using Nodes = std::vector<std::string>;

      } // namespace Task
   }    // namespace OpenMP
} // namespace Bambu

#endif

#endif
