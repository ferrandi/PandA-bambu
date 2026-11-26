// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#pragma once
#include <array>
#include <filesystem>
#include <string>
#include <vector>
struct RamulatorConfigInputs
{
   std::filesystem::path library, output;
   std::string profile, calibration;
   std::vector<std::string> imports;
   double clock_period_ns = 10;
};
struct RamulatorResolvedConfig
{
   std::array<unsigned, 4> cycles{{1, 1, 1, 1}};
   std::filesystem::path configuration;
   std::string status = "generic";
};
RamulatorResolvedConfig resolve_ramulator_config(const RamulatorConfigInputs& input);
