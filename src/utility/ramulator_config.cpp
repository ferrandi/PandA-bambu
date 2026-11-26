// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "ramulator_config.hpp"
#include "ramulator_sha256.hpp"
#include <yaml-cpp/yaml.h>
#include <pugixml.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace
{
namespace fs = std::filesystem;
using Decimal = boost::multiprecision::cpp_rational;
const std::array<const char*, 4> delay_names{{"read_forward", "write_forward", "read_return", "write_return"}};
void require(bool test, const std::string& message)
{
   if(!test)
      throw std::runtime_error("Ramulator: " + message);
}
long double number(const std::string& value)
{
   size_t end = 0;
   const auto result = std::stold(value, &end);
   require(end == value.size() && std::isfinite(result) && result >= 0, "invalid nonnegative time: " + value);
   return result;
}
uint64_t clock_femtoseconds(long double ns)
{
   require(std::isfinite(ns) && ns >= 0 && ns * 1.e6L <= std::numeric_limits<int64_t>::max(),
           "time is outside the supported femtosecond range");
   const auto scaled = ns * 1.e6L;
   const auto nearest = std::round(scaled);
   return static_cast<uint64_t>(nearest);
}
uint64_t delay_femtoseconds(const Decimal& ns)
{
   const Decimal scaled = ns * 1000000;
   require(scaled >= 0 && scaled <= Decimal(std::numeric_limits<int64_t>::max()),
           "delay is outside the supported femtosecond range");
   auto result = scaled.convert_to<uint64_t>();
   if(Decimal(result) < scaled)
      ++result;
   return result;
}
Decimal time_ns(const std::string& value)
{
   require(value.size() <= 10000, "time has too many decimal digits");
   (void)number(value); // Validate finite nonnegative input before decimal parsing.
   const auto start = value.find_first_not_of(" \t\r\n");
   require(start != std::string::npos, "empty time");
   auto pos = start + (value[start] == '+' || value[start] == '-');
   const auto end = value.find_first_of("eE", pos);
   const auto mantissa_end = end == std::string::npos ? value.size() : end;
   boost::multiprecision::cpp_int coefficient = 0;
   long long fraction_digits = 0;
   bool fraction = false;
   for(; pos < mantissa_end; ++pos)
   {
      if(value[pos] == '.' && !fraction)
      {
         fraction = true;
         continue;
      }
      require(value[pos] >= '0' && value[pos] <= '9', "invalid decimal time: " + value);
      coefficient *= 10;
      coefficient += value[pos] - '0';
      if(fraction)
         ++fraction_digits;
   }
   if(coefficient == 0)
      return Decimal(0);
   const auto exponent = end == std::string::npos ? 0LL : std::stoll(value.substr(end + 1));
   require(exponent >= -20000 && exponent <= 20000, "decimal time exponent is outside the supported range");
   const auto power = exponent - fraction_digits;
   require(power >= -20000 && power <= 20000, "decimal time exponent is outside the supported range");
   boost::multiprecision::cpp_int scale = 1;
   for(long long i = 0; i < (power < 0 ? -power : power); ++i)
      scale *= 10;
   if(power >= 0)
      return Decimal(coefficient * scale);
   return Decimal(coefficient) / scale;
}
std::string decimal_text(const Decimal& value)
{
   // Decimal inputs and their sums have finite decimal expansions. Keep the
   // report exact as well as the arithmetic used for upward cycle rounding.
   const auto divisor = denominator(value);
   const boost::multiprecision::cpp_int whole = numerator(value) / divisor;
   boost::multiprecision::cpp_int remainder = numerator(value) % divisor;
   auto text = whole.str();
   if(remainder != 0)
   {
      text += '.';
      while(remainder != 0)
      {
         remainder *= 10;
         const boost::multiprecision::cpp_int digit = remainder / divisor;
         text += static_cast<char>('0' + digit.convert_to<unsigned>());
         remainder %= divisor;
      }
   }
   return text;
}
void attributes(pugi::xml_node node, const std::set<std::string>& allowed)
{
   std::set<std::string> seen;
   for(auto a : node.attributes())
   {
      require(seen.insert(a.name()).second, "duplicate XML attribute " + std::string(a.name()));
      require(allowed.count(a.name()), "unknown XML attribute " + std::string(a.name()));
   }
}
void merge_xml(pugi::xml_node target, pugi::xml_node source)
{
   for(auto attr : source.attributes())
   {
      auto dest = target.attribute(attr.name());
      if(!dest)
         dest = target.append_attribute(attr.name());
      dest.set_value(attr.value());
   }
   for(auto child : source.children())
   {
      if(child.type() != pugi::node_element)
         continue;
      // An imported raw controller configuration or override list replaces the
      // complete corresponding section. Scalar sections merge provided fields.
      const std::string name = child.name();
      auto dest = target.child(child.name());
      if(name == "controller_configuration" || name == "ramulator" || name == "provenance")
      {
         if(dest)
            target.remove_child(dest);
         target.append_copy(child);
      }
      else
      {
         if(!dest)
            dest = target.append_child(child.name());
         merge_xml(dest, child);
      }
   }
}
void canonical_copy(pugi::xml_node target, pugi::xml_node source)
{
   std::map<std::string, std::string> sorted_attributes;
   for(auto attr : source.attributes())
      sorted_attributes.emplace(attr.name(), attr.value());
   for(const auto& attr : sorted_attributes)
      target.append_attribute(attr.first.c_str()) = attr.second.c_str();
   std::vector<pugi::xml_node> children;
   std::set<std::string> names;
   bool unique = true;
   for(auto child : source.children())
      if(child.type() == pugi::node_element)
      {
         children.push_back(child);
         unique = names.insert(child.name()).second && unique;
      }
   if(unique)
      std::sort(children.begin(), children.end(),
                [](pugi::xml_node a, pugi::xml_node b) { return std::string(a.name()) < std::string(b.name()); });
   for(auto child : children)
      canonical_copy(target.append_child(child.name()), child);
   if(children.empty())
   {
      const std::string text = source.text().get();
      if(text.find_first_not_of(" \t\r\n") != std::string::npos)
         target.text() = text.c_str();
   }
}
std::string canonical(pugi::xml_node node)
{
   pugi::xml_document document;
   canonical_copy(document.append_child(node.name()), node);
   std::ostringstream stream;
   document.save(stream, "", pugi::format_raw | pugi::format_no_declaration);
   return stream.str();
}
std::string profile_of(pugi::xml_node root)
{
   const std::string explicit_profile = root.attribute("profile").value();
   return !explicit_profile.empty() ? explicit_profile : root.child("identity").attribute("board_model").value();
}
void read_record(const fs::path& path, pugi::xml_document& result, bool overlay)
{
   pugi::xml_document source;
   const auto parsed = source.load_file(path.c_str());
   require(parsed, "cannot read XML " + path.string() + ": " + parsed.description());
   auto root = source.document_element();
   require(std::string(root.name()) == "ramulator_configuration", "expected ramulator_configuration root");
   attributes(root, {"schema_version", "status", "profile"});
   const std::string status = root.attribute("status").as_string("experimental");
   require(status == "draft" || status == "experimental" || status == "calibrated" || status == "not_validated",
           "unknown calibration status");
   require(root.attribute("schema_version").as_string() == std::string("1"), "unsupported configuration schema");
   std::set<std::string> seen;
   for(auto child : root.children())
   {
      if(child.type() != pugi::node_element)
         continue;
      const std::string name = child.name();
      require(seen.insert(name).second, "duplicate XML section " + name);
      require(name == "identity" || name == "clocks_hz" || name == "delay_ns" || name == "hbm_switch" ||
                  name == "controller_configuration" || name == "ramulator" || name == "provenance" ||
                  name == "ramulator_model",
              "unknown XML section " + name);
      if(name == "identity" || name == "clocks_hz" || name == "delay_ns" || name == "hbm_switch" || name == "ramulator_model")
         require(!child.first_child(), "scalar XML section cannot contain children: " + name);
      if(name == "ramulator_model")
      {
         attributes(child, {"sha256"});
         const std::string hash = child.attribute("sha256").value();
         require(hash.size() == 64 && hash.find_first_not_of("0123456789abcdef") == std::string::npos,
                 "ramulator_model sha256 must contain 64 lowercase hexadecimal digits");
      }
      if(name == "identity")
         attributes(child, {"board_model", "fpga_speed_grade", "dram_model", "dram_speed_grade", "configuration_id"});
      if(name == "clocks_hz")
         attributes(child, {"accelerator", "axi_frontend", "dram"});
      if(name == "delay_ns")
         attributes(child, {"read_forward", "write_forward", "read_return", "write_return"});
      if(name == "hbm_switch")
         attributes(child, {"enabled", "read_correction_ns", "write_correction_ns"});
   }
   require(status != "calibrated" || root.child("ramulator_model").attribute("sha256"),
           "calibrated record requires ramulator_model sha256 from the resolved model report");
   if(overlay && result.document_element())
   {
      auto old_id = result.document_element().child("identity");
      for(auto attr : root.child("identity").attributes())
      {
         if(std::string(attr.name()) == "configuration_id" &&
            std::string(result.document_element().attribute("status").value()) != "calibrated")
            continue;
         auto old = old_id.attribute(attr.name());
         require(!old || !*old.value() || std::string(old.value()) == "unknown" ||
                     std::string(attr.value()) == old.value(),
                 "probe identity differs from selected configuration: " + std::string(attr.name()));
      }
      auto previous_config = result.document_element().child("controller_configuration");
      auto next_config = root.child("controller_configuration");
      const bool was_calibrated = std::string(result.document_element().attribute("status").value()) == "calibrated";
      const auto previous_overrides = result.document_element().child("ramulator");
      const auto next_overrides = root.child("ramulator");
      const bool changed_overrides =
          next_overrides && (!previous_overrides || canonical(previous_overrides) != canonical(next_overrides));
      const bool changed_controller =
          previous_config && next_config && canonical(previous_config) != canonical(next_config);
      // The AXI controller frontend frequency is part of the calibration key.
      // Compare numeric Hz, so equivalent decimal/exponent spellings select
      // the same measurements. A different frequency needs a complete probe
      // explicitly qualified for that frequency, rather than inherited delays.
      const auto previous_axi = result.document_element().child("clocks_hz").attribute("axi_frontend");
      const auto next_axi = root.child("clocks_hz").attribute("axi_frontend");
      const bool changed_axi =
          next_axi && (!previous_axi || time_ns(previous_axi.value()) != time_ns(next_axi.value()));
      if(was_calibrated && changed_axi)
      {
         require(status == "calibrated",
                 "AXI frontend frequency differs from calibration key; supply a complete calibrated probe "
                 "for the new frequency");
         for(auto name : delay_names)
            require(root.child("delay_ns").attribute(name),
                    "changed AXI frontend frequency requires a complete new probe: missing " + std::string(name));
         require(root.child("clocks_hz").attribute("accelerator") && root.child("clocks_hz").attribute("dram"),
                 "changed AXI frontend frequency requires a complete new probe with all clocks");
      }
      if(was_calibrated && (changed_controller || (changed_overrides && status == "calibrated")))
      {
         for(auto name : delay_names)
            require(root.child("delay_ns").attribute(name),
                    "changed controller configuration requires a complete new probe");
         require(root.child("clocks_hz").attribute("accelerator") &&
                     root.child("clocks_hz").attribute("axi_frontend") && root.child("clocks_hz").attribute("dram"),
                 "changed controller configuration requires new clocks");
      }
      merge_xml(result.document_element(), root);
      if(was_calibrated && changed_overrides && status != "calibrated")
      {
         result.document_element().attribute("status") = "experimental";
         auto provenance = result.document_element().child("provenance");
         if(!provenance)
            provenance = result.document_element().append_child("provenance");
         provenance.append_child("note").text() =
             "Simulator overrides changed; retained delays are experimental estimates.";
      }
   }
   else
      result.append_copy(root);
}
YAML::Node detached(const YAML::Node& node, unsigned depth = 0)
{
   require(depth < 64, "YAML nesting or alias cycle exceeds supported depth");
   if(node.IsMap())
   {
      YAML::Node result(YAML::NodeType::Map);
      std::set<std::string> keys;
      for(auto item : node)
      {
         const auto key = item.first.as<std::string>();
         require(keys.insert(key).second, "duplicate YAML key " + key);
         result[key] = detached(item.second, depth + 1);
      }
      return result;
   }
   if(node.IsSequence())
   {
      YAML::Node result(YAML::NodeType::Sequence);
      for(auto item : node)
         result.push_back(detached(item, depth + 1));
      return result;
   }
   return YAML::Clone(node);
}
YAML::Node load_yaml(const fs::path& path, std::set<fs::path>& stack);
void includes(YAML::Node node, const fs::path& directory, std::set<fs::path>& stack, unsigned depth = 0)
{
   require(depth < 64, "YAML nesting or alias cycle exceeds supported depth");
   if(node.Tag() == "!include")
   {
      auto resolved = load_yaml(directory / node.as<std::string>(), stack);
      node.reset(resolved);
      return;
   }
   if(node.IsMap())
      for(auto it : node)
      {
         auto child = it.second;
         if(child.Tag() == "!include")
            node[it.first.as<std::string>()] = load_yaml(directory / child.as<std::string>(), stack);
         else
            includes(child, directory, stack, depth + 1);
      }
   else if(node.IsSequence())
      for(size_t i = 0; i < node.size(); ++i)
      {
         auto child = node[i];
         if(child.Tag() == "!include")
            node[i] = load_yaml(directory / child.as<std::string>(), stack);
         else
            includes(child, directory, stack, depth + 1);
      }
}
YAML::Node load_yaml(const fs::path& path, std::set<fs::path>& stack)
{
   const auto absolute = fs::weakly_canonical(fs::absolute(path));
   require(fs::is_regular_file(absolute), "missing YAML " + absolute.string());
   require(stack.insert(absolute).second, "cyclic YAML include " + absolute.string());
   auto node = YAML::LoadFile(absolute.string());
   if(node.Tag() == "!include")
      node = load_yaml(absolute.parent_path() / node.as<std::string>(), stack);
   else
      includes(node, absolute.parent_path(), stack);
   stack.erase(absolute);
   return detached(node);
}
void merge_yaml(YAML::Node target, const YAML::Node& source)
{
   require(source.IsMap(), "discovery YAML must contain a native Ramulator mapping");
   for(auto entry : source)
   {
      const auto key = entry.first.as<std::string>();
      if(target[key].IsMap() && entry.second.IsMap())
         merge_yaml(target[key], entry.second);
      else
         target[key] = YAML::Clone(entry.second);
   }
}
void apply_set(YAML::Node node, const std::string& path, const std::string& value)
{
   require(path.size() > 1 && path.front() == '/' && path.back() != '/', "override path must start with /");
   std::istringstream stream(path.substr(1));
   std::vector<std::string> parts;
   std::string part;
   while(std::getline(stream, part, '/'))
   {
      require(!part.empty(), "empty override path component");
      parts.push_back(part);
   }
   for(size_t i = 0; i < parts.size(); ++i)
   {
      YAML::Node child;
      if(node.IsSequence())
      {
         require(std::all_of(parts[i].begin(), parts[i].end(), [](char c) { return c >= '0' && c <= '9'; }),
                 "invalid sequence index");
         const auto index = std::stoull(parts[i]);
         require(index < node.size(), "override index out of range: " + path);
         child.reset(node[index]);
      }
      else
      {
         require(node.IsMap() && static_cast<bool>(node[parts[i]]), "unknown override path: " + path);
         child.reset(node[parts[i]]);
      }
      if(i + 1 == parts.size())
      {
         require(child.IsScalar(), "XML overrides may change existing scalars only: " + path);
         child = value;
      }
      else
         node.reset(child);
   }
}
// Versioned, length-prefixed encoding of the resolved YAML data tree.
// Maps sort by scalar key; sequences preserve order; aliases are expanded by
// load_yaml. Scalar spelling is retained; quoting, comments and style are not.
void canonical_yaml(std::ostream& out, const YAML::Node& node)
{
   if(node.IsMap())
   {
      std::map<std::string, YAML::Node> entries;
      for(auto item : node) entries.emplace(item.first.as<std::string>(), item.second);
      out << 'M' << entries.size() << ':';
      for(const auto& entry : entries)
      {
         out << 'S' << entry.first.size() << ':' << entry.first;
         canonical_yaml(out, entry.second);
      }
   }
   else if(node.IsSequence())
   {
      out << 'Q' << node.size() << ':';
      for(auto item : node) canonical_yaml(out, item);
   }
   else if(node.IsScalar())
      out << 'S' << node.Scalar().size() << ':' << node.Scalar();
   else out << 'N';
}
void atomic_text(const fs::path& path, const std::string& value)
{
   auto temporary = path;
   temporary += ".tmp";
   {
      std::ofstream stream(temporary);
      require(static_cast<bool>(stream), "cannot create " + temporary.string());
      stream << value;
      stream.close();
      require(static_cast<bool>(stream), "cannot write " + temporary.string());
   }
   fs::rename(temporary, path);
}
} // namespace

RamulatorResolvedConfig resolve_ramulator_config(const RamulatorConfigInputs& input)
{
   RamulatorResolvedConfig resolved;
   require(std::isfinite(input.clock_period_ns) && input.clock_period_ns > 0, "accelerator period must be positive");
   if(input.profile.empty() && input.calibration.empty() && input.imports.empty())
      return resolved;
   const auto period_fs = clock_femtoseconds(input.clock_period_ns);
   require(period_fs > 0, "accelerator period is below femtosecond precision");
   std::string profile = input.profile;
   fs::path discovery, probe;
   for(const auto& file : input.imports)
   {
      fs::path path(file);
      if(path.extension() == ".yaml" || path.extension() == ".yml")
      {
         require(discovery.empty(), "only one discovery YAML may be imported");
         discovery = path;
      }
      else if(path.extension() == ".xml")
      {
         require(probe.empty(), "only one probe XML may be imported");
         probe = path;
      }
      else
         throw std::runtime_error("Ramulator: import expects discovery YAML or probe XML: " + file);
   }
   pugi::xml_document imported;
   if(!probe.empty())
   {
      read_record(probe, imported, false);
      if(profile.empty())
         profile = profile_of(imported.document_element());
   }
   pugi::xml_document record;
   fs::path template_path;
   if(!profile.empty())
   {
      template_path = fs::path(profile).has_extension() ? fs::path(profile)
                                                        : input.library / "configs/boards" / (profile + ".yaml");
      if(!fs::path(profile).has_extension())
      {
         const auto default_record = input.library / "calibrations" / profile / "default.xml";
         if(input.calibration.empty() && fs::exists(default_record))
            read_record(default_record, record, false);
      }
   }
   if(!input.calibration.empty())
   {
      require(!profile.empty() || fs::path(input.calibration).has_extension(),
              "a configuration ID requires --ramulator-profile; an XML file may identify its board");
      const auto path = fs::path(input.calibration).has_extension()
                            ? fs::path(input.calibration)
                            : input.library / "calibrations" / profile / (input.calibration + ".xml");
      read_record(path, record, false);
      const std::string associated = profile_of(record.document_element());
      require(profile.empty() || fs::path(profile).has_extension() || associated.empty() || associated == profile,
              "configuration belongs to another profile");
      if(template_path.empty() && !associated.empty())
      {
         profile = associated;
         template_path = input.library / "configs/boards" / (profile + ".yaml");
      }
   }
   if(!probe.empty())
   {
      const std::string associated = profile_of(imported.document_element());
      require(profile.empty() || fs::path(profile).has_extension() || associated.empty() || associated == profile,
              "probe belongs to another profile");
      read_record(probe, record, true);
   }
   if(!record.document_element())
   {
      auto root = record.append_child("ramulator_configuration");
      root.append_attribute("schema_version") = 1;
      root.append_attribute("status") = "experimental";
   }
   auto root = record.document_element();
   require(!template_path.empty() || !discovery.empty(), "select a profile or provide a complete discovery YAML");
   std::set<fs::path> stack;
   auto config = template_path.empty() ? YAML::Node(YAML::NodeType::Map) : load_yaml(template_path, stack);
   const auto selected_template = YAML::Dump(config);
   if(!discovery.empty())
      merge_yaml(config, load_yaml(discovery, stack));
   if(!discovery.empty() && YAML::Dump(config) != selected_template &&
      std::string(root.attribute("status").value()) == "calibrated")
   {
      bool new_calibration =
          !probe.empty() && std::string(imported.document_element().attribute("status").value()) == "calibrated";
      for(auto name : delay_names)
         new_calibration =
             new_calibration && static_cast<bool>(imported.document_element().child("delay_ns").attribute(name));
      if(!new_calibration)
      {
         root.attribute("status") = "experimental";
         auto provenance = root.child("provenance");
         if(!provenance)
            provenance = root.append_child("provenance");
         provenance.append_child("note").text() = "Discovery changed the selected model; retained delays are estimates "
                                                  "until compatible calibration is supplied.";
      }
   }
   require(config.IsMap() && config["frontend"] && config["memory_system"],
           "profile must use native Ramulator 2.1 YAML");
   for(auto entry : config)
      require(entry.first.as<std::string>() == "frontend" || entry.first.as<std::string>() == "memory_system",
              "Bambu metadata is forbidden in native YAML");
   for(auto setting : root.child("ramulator").children())
   {
      require(std::string(setting.name()) == "set", "unknown ramulator XML override");
      attributes(setting, {"path", "value"});
      require(setting.attribute("value"), "override is missing value");
      const std::string path = setting.attribute("path").value();
      require(path.find("/dram/") == std::string::npos,
              "DRAM changes require a resolved discovery YAML including timing constraints");
      const std::string value = setting.attribute("value").value();
      require((path != "/memory_system/clock_ratio" && path != "/frontend/clock_ratio") || value == "1",
              "MDPI owns the clock; standalone clock_ratio overrides must be 1");
      require(path != "/frontend/impl" || value == "External", "Bambu requires the External frontend");
      apply_set(config, path, value);
   }
   auto system = config["memory_system"];
   require(system["impl"].as<std::string>() == "GenericDRAM", "profiled flow requires GenericDRAM");
   require(system["controllers"].IsSequence() && system["controllers"].size() > 0, "profile has no controllers");
   uint64_t common_tick_ps = 0;
   const auto channels = system["controllers"].size();
   require(channels <= 256 && (channels & (channels - 1)) == 0,
           "profile channel count must be a power of two no greater than 256");
   for(auto controller : system["controllers"])
   {
      const auto dram = controller["dram"];
      const auto timing = dram["timing"];
      require(timing.IsSequence() && timing.size() > 0, "native DRAM timing array is missing");
      const auto tick_ps = timing[timing.size() - 1].as<int64_t>();
      require(tick_ps > 0, "native DRAM period must be positive");
      require(!common_tick_ps || common_tick_ps == static_cast<uint64_t>(tick_ps),
              "heterogeneous DRAM clocks are not supported");
      common_tick_ps = static_cast<uint64_t>(tick_ps);
      const auto counts = dram["org"]["count"];
      require(counts.IsSequence() && counts.size() >= 2, "native DRAM organization is missing");
      for(auto count : counts)
      {
         const auto value = count.as<int64_t>();
         require(value > 0 && (value & (value - 1)) == 0, "organization counts must be positive powers of two");
      }
   }
   // External MDPI owns the clock and request source in co-simulation.
   config["frontend"]["impl"] = "External";
   config["frontend"]["clock_ratio"] = 1;
   system["clock_ratio"] = 1;
   std::array<Decimal, 4> delays;
   delays.fill(Decimal(period_fs) / 1000000); // Default is one current accelerator cycle.
   auto delay = root.child("delay_ns");
   const bool calibrated = std::string(root.attribute("status").value()) == "calibrated";
   resolved.status = calibrated ? "calibrated" :
                     (std::string(root.attribute("status").value()) == "not_validated" ? "not_validated" : "experimental");
   for(size_t i = 0; i < delays.size(); ++i)
   {
      const auto attr = delay.attribute(delay_names[i]);
      require(!calibrated || attr, "calibrated record is missing " + std::string(delay_names[i]));
      if(attr)
         delays[i] = time_ns(attr.value());
   }
   auto switch_node = root.child("hbm_switch");
   const auto enabled = std::string(switch_node.attribute("enabled").as_string("false"));
   require(enabled == "true" || enabled == "false", "hbm_switch enabled must be true or false");
   for(const auto* name : {"read_correction_ns", "write_correction_ns"})
      if(switch_node.attribute(name))
         (void)delay_femtoseconds(time_ns(switch_node.attribute(name).value()));
   if(enabled == "true")
   {
      require(switch_node.attribute("read_correction_ns") && switch_node.attribute("write_correction_ns"),
              "enabled HBM switch requires read/write corrections");
      delays[2] += time_ns(switch_node.attribute("read_correction_ns").value());
      delays[3] += time_ns(switch_node.attribute("write_correction_ns").value());
   }
   const auto accelerator = root.child("clocks_hz").attribute("accelerator");
   if(accelerator)
   {
      auto hz = number(accelerator.value());
      require(hz > 0, "accelerator clock must be positive");
      require(clock_femtoseconds(1.e9L / hz) == period_fs, "calibration clock differs from --clock-period");
   }
   for(auto clock : root.child("clocks_hz").attributes())
      require(number(clock.value()) > 0, "clock must be positive");
   const auto dram_clock = root.child("clocks_hz").attribute("dram");
   if(dram_clock)
   {
      const auto expected_fs = clock_femtoseconds(1.e9L / number(dram_clock.value()));
      for(auto controller : system["controllers"])
      {
         const auto timing = controller["dram"]["timing"];
         require(timing.IsSequence() && timing.size() > 0, "native DRAM timing array is missing");
         const auto tick_ps = timing[timing.size() - 1].as<uint64_t>();
         const auto actual_fs = tick_ps * 1000;
         require((actual_fs > expected_fs ? actual_fs - expected_fs : expected_fs - actual_fs) < 1000,
                 "DRAM clock differs from resolved Ramulator timing beyond its picosecond representation");
      }
   }
   const auto id = root.child("identity");
   if(calibrated)
   {
      for(auto name : {"board_model", "fpga_speed_grade", "dram_model", "dram_speed_grade", "configuration_id"})
         require(id.attribute(name) && *id.attribute(name).value() &&
                     std::string(id.attribute(name).value()) != "unknown",
                 "calibrated identity is missing " + std::string(name));
      const auto raw_configuration = root.child("controller_configuration");
      require(raw_configuration && (raw_configuration.first_attribute() || raw_configuration.first_child()) &&
                  accelerator && root.child("clocks_hz").attribute("dram") &&
                  root.child("clocks_hz").attribute("axi_frontend"),
              "calibrated record requires controller configuration and all clocks");
   }
   std::ostringstream canonical_model;
   canonical_model << "bambu-ramulator-yaml-v1\n";
   canonical_yaml(canonical_model, config);
   const auto model_hash = ramulator_sha256(canonical_model.str());
   auto binding = root.child("ramulator_model");
   const std::string input_hash = binding.attribute("sha256").value();
   require(!calibrated || !input_hash.empty(),
           "calibrated record requires ramulator_model sha256 from the resolved model report");
   if(!input_hash.empty() && input_hash != model_hash)
   {
      resolved.status = "not_validated";
      auto status = root.attribute("status");
      if(!status) status = root.append_attribute("status");
      status = resolved.status.c_str();
      std::cerr << "Warning: Ramulator calibration YAML SHA-256 mismatch: expected " << input_hash
                << ", resolved " << model_hash
                << "; status=not_validated. Continuing with the supplied latency estimates.\n";
   }
   // The emitted record always identifies the actual final model. Experimental
   // estimates may be reused; retain their previous binding in the report.
   if(!binding) binding = root.append_child("ramulator_model");
   auto hash_attr = binding.attribute("sha256");
   if(!hash_attr) hash_attr = binding.append_attribute("sha256");
   hash_attr = model_hash.c_str();
   auto report = root.append_child("resolved");
   report.append_attribute("status") = resolved.status.c_str();
   auto model_report = report.append_child("ramulator_model");
   model_report.append_attribute("sha256") = model_hash.c_str();
   model_report.append_attribute("canonicalization") = "bambu-ramulator-yaml-v1";
   if(!input_hash.empty() && input_hash != model_hash)
      model_report.append_attribute("previous_sha256") = input_hash.c_str();
   // Derived key: preserve raw input above and expose an order-independent form
   // for the future discovery/probe/update-library tools. Repeated XML children
   // retain their order, because lists can encode physical port associations.
   auto key = report.append_child("selection_key");
   canonical_copy(key.append_child("identity"), id);
   canonical_copy(key.append_child("ramulator_model"), binding);
   if(root.child("controller_configuration"))
      canonical_copy(key.append_child("controller_configuration"), root.child("controller_configuration"));
   if(root.child("clocks_hz"))
   {
      auto clocks_key = key.append_child("clocks_hz");
      canonical_copy(clocks_key, root.child("clocks_hz"));
      auto axi_key = clocks_key.attribute("axi_frontend");
      if(axi_key)
         axi_key.set_value(decimal_text(time_ns(axi_key.value())).c_str());
   }
   if(root.child("ramulator"))
      canonical_copy(key.append_child("ramulator"), root.child("ramulator"));
   if(dram_clock)
   {
      auto clock_report = report.append_child("dram_clock");
      const auto expected = clock_femtoseconds(1.e9L / number(dram_clock.value()));
      const auto timing = system["controllers"][0]["dram"]["timing"];
      const auto effective = timing[timing.size() - 1].as<uint64_t>() * 1000;
      clock_report.append_attribute("requested_period_fs").set_value(static_cast<unsigned long long>(expected));
      clock_report.append_attribute("effective_period_fs").set_value(static_cast<unsigned long long>(effective));
      clock_report.append_attribute("representation_error_fs")
          .set_value(static_cast<long long>(effective) - static_cast<long long>(expected));
   }
   report.append_attribute("accelerator_period_fs").set_value(static_cast<unsigned long long>(period_fs));
   for(size_t i = 0; i < delays.size(); ++i)
   {
      const auto delay_fs = delay_femtoseconds(delays[i]);
      const auto cycles = delay_fs / period_fs + (delay_fs % period_fs != 0);
      require(cycles <= static_cast<uint64_t>(std::numeric_limits<int>::max()),
              "transport delay exceeds integer TB range");
      resolved.cycles[i] = static_cast<unsigned>(cycles);
      auto value = report.append_child("delay");
      value.append_attribute("name") = delay_names[i];
      value.append_attribute("requested_ns") = decimal_text(delays[i]).c_str();
      value.append_attribute("requested_fs").set_value(static_cast<unsigned long long>(delay_fs));
      value.append_attribute("cycles") = resolved.cycles[i];
      value.append_attribute("represented_fs").set_value(static_cast<unsigned long long>(cycles * period_fs));
      value.append_attribute("source") = delay.attribute(delay_names[i]) ? "configuration" : "default-one-cycle";
   }
   auto sources = report.append_child("sources");
   sources.append_attribute("profile") = template_path.string().c_str();
   sources.append_attribute("discovery") = discovery.string().c_str();
   sources.append_attribute("probe") = probe.string().c_str();
   const auto output = fs::absolute(input.output / "memory-model");
   fs::create_directories(output);
   resolved.configuration = output / "ramulator2_config.yaml";
   std::ostringstream document;
   record.save(document, "  ");
   atomic_text(output / "resolved-memory-model.xml", document.str());
   atomic_text(resolved.configuration, YAML::Dump(config) + "\n");
   atomic_text(output / "ramulator2_config.canonical", canonical_model.str());
   return resolved;
}
