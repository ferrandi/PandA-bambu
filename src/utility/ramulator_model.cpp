// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ramulator_model.hpp"
#include "ramulator_config.hpp"
#include "Parameter.hpp"
#include "config_PANDA_DATA_INSTALLDIR.hpp"
#include "fileIO.hpp"
#include "exceptions.hpp"
#include <map>
#include <mutex>
#include <sstream>
#include <iostream>
#include <iomanip>
RamulatorModel resolve_ramulator_model(const ParameterConstRef& parameters)
{
   RamulatorConfigInputs input;
   input.library = relocate_install_path(PANDA_DATA_INSTALLDIR) / "libmdpi";
   input.output = parameters->getOption<std::filesystem::path>(OPT_output_hls_directory);
   input.clock_period_ns = parameters->getOption<double>(OPT_clock_period);
   if(parameters->isOption(OPT_ramulator_profile))
      input.profile = parameters->getOption<std::string>(OPT_ramulator_profile);
   if(parameters->isOption(OPT_ramulator_calibration))
      input.calibration = parameters->getOption<std::string>(OPT_ramulator_calibration);
   if(parameters->isOption(OPT_ramulator_import))
      input.imports = parameters->getOption<std::vector<std::string>>(OPT_ramulator_import);
   // All generators in one invocation must consume the same resolved input.
   // Length-prefix fields so filenames cannot collide with separators.
   std::ostringstream key;
   key << std::setprecision(17) << input.clock_period_ns;
   for(const auto& field : {input.library.string(), input.output.string(), input.profile, input.calibration})
      key << ':' << field.size() << ':' << field;
   for(const auto& field : input.imports)
      key << ':' << field.size() << ':' << field;
   static std::mutex mutex;
   static std::map<std::string, RamulatorModel> cache;
   const std::lock_guard<std::mutex> lock(mutex);
   auto previous = cache.find(key.str());
   if(previous != cache.end())
      return previous->second;
   try
   {
      const auto resolved = resolve_ramulator_config(input);
      RamulatorModel result;
      result.cycles = resolved.cycles;
      result.configuration = resolved.configuration;
      cache.emplace(key.str(), result);
      if(!result.configuration.empty())
         std::clog << "[Ramulator] configuration=" << result.configuration
                   << "; transport cycles (read forward, write forward, read return, write return)=" << result.cycles[0]
                   << ',' << result.cycles[1] << ',' << result.cycles[2] << ',' << result.cycles[3]
                   << "; status=" << resolved.status
                   << "; provenance=" << result.configuration.parent_path() / "resolved-memory-model.xml" << '\n';
      return result;
   }
   catch(const std::exception& error)
   {
      THROW_ERROR(error.what());
      throw;
   }
}
