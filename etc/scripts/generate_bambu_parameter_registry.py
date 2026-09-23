#!/usr/bin/env python3
import argparse
import json
import os
import pathlib
import re
import sys

MIN_PYTHON = (3, 4)
METADATA_FIELDS = ("type", "default_value", "description", "category", "declared_in", "allowed_values")
DEFAULT_METADATA_SCHEMA = {
   "required": ["name"],
   "optional": ["type", "default_value", "description", "category", "declared_in", "allowed_values"],
}


def check_python_version():
   if sys.version_info < MIN_PYTHON:
      sys.stderr.write(
         "generate_bambu_parameter_registry.py requires Python >= {}.{} (found {}.{}.{}).\n".format(
            MIN_PYTHON[0], MIN_PYTHON[1], sys.version_info[0], sys.version_info[1], sys.version_info[2]
         )
      )
      return False
   return True


def sanitize_group(name):
   sanitized = re.sub(r"[^a-zA-Z0-9]+", "_", name.strip()).strip("_").lower()
   return sanitized or "misc"


def write_if_changed(path, content):
   if path.exists():
      existing = path.read_text()
      if existing == content:
         return
   path.write_text(content)


def escape_cpp_string(value):
   return (
      value.replace("\\", "\\\\")
      .replace("\n", "\\n")
      .replace("\t", "\\t")
      .replace('"', '\\"')
   )


def normalize_type(value):
   key = value.strip().lower()
   mapping = {
      "bool": "Bool",
      "int": "Int",
      "uint": "UInt",
      "uint32": "UInt",
      "uint32_t": "UInt",
      "uint64": "UInt",
      "uint64_t": "UInt",
      "size_t": "UInt",
      "double": "Double",
      "float": "Double",
      "string": "String",
      "enum": "Enum",
      "unknown": "Unknown",
   }
   return mapping.get(key, value.strip())


def resolve_type_token(entry):
   explicit = entry.get("type")
   inferred = entry.get("inferred_type")
   if explicit:
      normalized = normalize_type(str(explicit))
   elif inferred:
      normalized = normalize_type(str(inferred))
   else:
      normalized = "Unknown"
   return "PandaParamType::{}".format(normalized)


def normalize_allowed_values(values):
   if isinstance(values, list):
      return [str(value) for value in values if str(value)]
   if isinstance(values, str):
      return [value.strip() for value in values.split(",") if value.strip()]
   return []


def to_absolute_path(path, base_dir=None):
   candidate = pathlib.Path(path)
   if not candidate.is_absolute():
      if base_dir is None:
         base_dir = pathlib.Path.cwd()
      candidate = pathlib.Path(base_dir) / candidate
   # Python 3.5: Path.resolve() may raise on non-existing paths.
   return pathlib.Path(os.path.abspath(str(candidate)))


def format_cpp_vector(values):
   if not values:
      return "{}"
   escaped = ['"{}"'.format(escape_cpp_string(value)) for value in values]
   return "{ " + ", ".join(escaped) + " }"


def group_for_occurrence(file_path):
   parts = pathlib.Path(file_path).parts
   if "libtech" in parts:
      return "technology"
   if "src" in parts:
      src_indices = [index for index, part in enumerate(parts) if part == "src"]
      src_index = src_indices[-1]
      if len(parts) > src_index + 1:
         candidate = parts[src_index + 1]
         if pathlib.Path(candidate).suffix:
            return "core"
         return candidate
      return "core"
   return "misc"


def load_metadata(path):
   if not path.exists():
      return {"schema": DEFAULT_METADATA_SCHEMA, "parameters": []}, {}
   raw = json.loads(path.read_text())
   if isinstance(raw, dict):
      entries = raw.get("parameters", [])
      document = dict(raw)
   elif isinstance(raw, list):
      entries = raw
      document = {"parameters": raw}
   else:
      raise RuntimeError("Metadata must be a list or an object with a 'parameters' list.")
   metadata = {}
   for entry in entries:
      if not isinstance(entry, dict):
         continue
      name = entry.get("name")
      if not name:
         continue
      metadata[str(name)] = entry
   return document, metadata


def merge_metadata(entry, metadata):
   for field in METADATA_FIELDS:
      if field in metadata:
         entry[field] = metadata[field]


def metadata_entry_sort_key(entry):
   return entry["name"]


def build_metadata_entries(parameters, metadata):
   entries = []
   for parameter in sorted(parameters, key=metadata_entry_sort_key):
      name = parameter["name"]
      merged = {"name": name}
      if name in metadata:
         merge_metadata(merged, metadata[name])
      elif parameter.get("inferred_type") and parameter["inferred_type"] != "Unknown":
         merged["type"] = parameter["inferred_type"]
      occurrences = parameter.get("occurrences", [])
      file_path = occurrences[0]["file"] if occurrences else ""
      if "category" not in merged and file_path:
         merged["category"] = group_for_occurrence(file_path)
      entries.append(merged)
   return entries


def sync_metadata_file(path, document, parameters, metadata):
   if "schema" not in document:
      document["schema"] = DEFAULT_METADATA_SCHEMA
   document["parameters"] = build_metadata_entries(parameters, metadata)
   write_if_changed(path, json.dumps(document, indent=2) + "\n")


def render_autogen(entries, category):
   lines = []
   for entry in sorted(entries, key=lambda x: x["name"]):
      name = escape_cpp_string(entry["name"])
      type_token = resolve_type_token(entry)
      default_value = escape_cpp_string(str(entry.get("default_value", "")))
      description = escape_cpp_string(str(entry.get("description", "")))
      cat = escape_cpp_string(str(entry.get("category", category)))
      declared_in = escape_cpp_string(str(entry.get("declared_in", entry.get("first_declared_in", ""))))
      allowed_values = normalize_allowed_values(entry.get("allowed_values", []))
      allowed_values_cpp = format_cpp_vector(allowed_values)
      lines.append(
         "RegisterPandaParameter(PandaParameterInfo{"
         + '"{}", {}, "{}", "{}", "{}", "{}", {}'.format(
            name, type_token, default_value, description, cat, declared_in, allowed_values_cpp
         )
         + "});"
      )
   return "\n".join("   {}".format(line) for line in lines)


def update_file(path, autogen_block, shard_func_name):
   marker_begin = "// BEGIN AUTOGEN"
   marker_end = "// END AUTOGEN"
   old_include = '#include "PandaParameterRegistry.hpp"'
   new_include = '#include "BambuParameterRegistry.hpp"'
   shard_template = (
      "{}\n\n".format(new_include)
      + "void {}()\n".format(shard_func_name)
      + "{\n"
      + "   {}\n".format(marker_begin)
      + "{}\n".format(autogen_block)
      + "   {}\n".format(marker_end)
      + "}\n"
   )
   if path.exists():
      content = path.read_text()
      if old_include in content:
         content = content.replace(old_include, new_include, 1)
      indented_begin = "\n   {}".format(marker_begin)
      indented_end = "\n   {}".format(marker_end)
      if indented_begin in content and indented_end in content and shard_func_name in content:
         before, rest = content.split(marker_begin, 1)
         _, after = rest.split(marker_end, 1)
         new_content = before + marker_begin + "\n" + autogen_block + "\n" + marker_end + after
         write_if_changed(path, new_content)
         return
   write_if_changed(path, shard_template)


def main():
   if not check_python_version():
      return 2

   parser = argparse.ArgumentParser(description="Generate bambu-parameter registry shards from discovery output.")
   parser.add_argument(
      "--input",
      default="build/bambu_parameters_discovered.json",
      help="Discovery JSON input (default: build/bambu_parameters_discovered.json)",
   )
   parser.add_argument(
      "--output-dir",
      default="build/bambu_parameters_registry",
      help="Output directory for registry shards (default: build/bambu_parameters_registry)",
   )
   parser.add_argument(
      "--metadata",
      default="documentation/bambu_parameters_metadata.json",
      help="Metadata JSON with descriptions (default: documentation/bambu_parameters_metadata.json)",
   )
   parser.add_argument(
      "--sync-metadata",
      action="store_true",
      help="Rewrite the metadata JSON so it matches the currently discovered parameter set.",
   )
   args = parser.parse_args()

   repo_root = pathlib.Path(__file__).resolve().parents[2]
   cwd = pathlib.Path.cwd()

   input_path = pathlib.Path(args.input)
   if not input_path.is_absolute():
      cwd_input = to_absolute_path(input_path, cwd)
      if cwd_input.exists():
         input_path = cwd_input
      else:
         input_path = to_absolute_path(input_path, repo_root)
   else:
      input_path = to_absolute_path(input_path)

   output_dir = pathlib.Path(args.output_dir)
   if not output_dir.is_absolute():
      cwd_output = to_absolute_path(output_dir, cwd)
      if cwd_output.parent.exists():
         output_dir = cwd_output
      else:
         output_dir = to_absolute_path(output_dir, repo_root)
   else:
      output_dir = to_absolute_path(output_dir)

   metadata_path = pathlib.Path(args.metadata)
   if not metadata_path.is_absolute():
      cwd_metadata = to_absolute_path(metadata_path, cwd)
      if cwd_metadata.exists():
         metadata_path = cwd_metadata
      else:
         metadata_path = to_absolute_path(metadata_path, repo_root)
   else:
      metadata_path = to_absolute_path(metadata_path)
   if not input_path.exists():
      sys.stderr.write("Discovery JSON not found: {}\n".format(input_path))
      return 2

   data = json.loads(input_path.read_text())
   parameters = data.get("parameters", [])
   try:
      metadata_document, metadata = load_metadata(metadata_path)
   except RuntimeError as exc:
      sys.stderr.write("{}\n".format(exc))
      return 2
   if metadata:
      parameters_by_name = {entry["name"]: entry for entry in parameters if "name" in entry}
      for name, meta in metadata.items():
         if name in parameters_by_name:
            merge_metadata(parameters_by_name[name], meta)
   if args.sync_metadata:
      sync_metadata_file(metadata_path, metadata_document, parameters, metadata)

   grouped = {}
   for entry in parameters:
      occurrences = entry.get("occurrences", [])
      file_path = occurrences[0]["file"] if occurrences else ""
      if not file_path and entry.get("category"):
         group = str(entry["category"])
      else:
         group = group_for_occurrence(file_path)
      grouped.setdefault(group, []).append(entry)

   output_dir.mkdir(parents=True, exist_ok=True)
   shard_functions = []
   for group in sorted(grouped.keys()):
      entries = grouped[group]
      category = group
      group_token = sanitize_group(group)
      shard_func_name = "RegisterBambuParameterRegistryShard_{}".format(group_token)
      shard_functions.append(shard_func_name)
      filename = "registry_{}.cpp".format(group_token)
      target = output_dir / filename
      autogen = render_autogen(entries, category)
      update_file(target, autogen, shard_func_name)

   registry_all = output_dir / "registry_all.cpp"
   lines = ['#include "BambuParameterRegistry.hpp"', ""]
   for func in sorted(shard_functions):
      lines.append("void {}();".format(func))
   lines.append("")
   lines.append("void RegisterBambuParameterRegistryShards()")
   lines.append("{")
   for func in sorted(shard_functions):
      lines.append("   {}();".format(func))
   lines.append("}")
   write_if_changed(registry_all, "\n".join(lines) + "\n")

   return 0


if __name__ == "__main__":
   raise SystemExit(main())
