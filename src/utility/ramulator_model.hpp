// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef RAMULATOR_MODEL_HPP
#define RAMULATOR_MODEL_HPP
#include "refcount.hpp"
#include <array>
#include <filesystem>
#include <string>
CONSTREF_FORWARD_DECL(Parameter);
struct RamulatorModel
{
   // read forward, write forward, read return, write return; integer TB cycles.
   std::array<unsigned, 4> cycles{{1, 1, 1, 1}};
   std::filesystem::path configuration;
};
// Resolve inputs without modifying the installed library. A generic legacy run
// returns the default cycles and an empty explicit configuration path.
RamulatorModel resolve_ramulator_model(const ParameterConstRef& parameters);
#endif
