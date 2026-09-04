#!/usr/bin/env python3
import argparse
import json
import pathlib
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from datetime import datetime, timezone


PATTERN_IS_PARAMETER = re.compile(r'IsParameter\("([^"]+)"\)')
PATTERN_GET_PARAMETER = re.compile(r'GetParameter<([^>]+)>\("([^"]+)"\)')
PATTERN_TRY_GET_PARAMETER_FROM_PARAMETER_OR_DEVICE = re.compile(
   r'TryGetParameterFromParameterOrDevice<([^>]+)>\(\s*"([^"]+)"'
)
PATTERN_GET_PARAMETER_FROM_PARAMETER_OR_DEVICE_OR_DEFAULT = re.compile(
   r'GetParameterFromParameterOrDeviceOrDefault<([^>]+)>\(\s*"([^"]+)"'
)


def run_rg(pattern, src_dir, cwd):
   cwd_path = pathlib.Path(cwd)
   try:
      result = subprocess.run(
         ["rg", "-n", "--no-heading", pattern, src_dir],
         check=False,
         stdout=subprocess.PIPE,
         stderr=subprocess.PIPE,
         universal_newlines=True,
         cwd=str(cwd_path),
      )
   except FileNotFoundError:
      return run_fallback_search(pattern, src_dir, cwd_path)
   if result.returncode not in (0, 1):
      raise RuntimeError(result.stderr.strip() or "rg failed with code {}".format(result.returncode))
   if not result.stdout:
      return []
   return result.stdout.splitlines()


def run_fallback_search(pattern, src_dir, cwd):
   cwd_path = pathlib.Path(cwd)
   regex = re.compile(pattern)
   src_path = pathlib.Path(src_dir)
   if not src_path.is_absolute():
      src_path = cwd_path / src_path
   exts = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
   matches = []
   for path in src_path.rglob("*"):
      if not path.is_file() or path.suffix not in exts:
         continue
      try:
         with path.open("r", encoding="utf-8", errors="ignore") as handle:
            for line_no, line in enumerate(handle, start=1):
               if regex.search(line):
                  matches.append("{}:{}:{}".format(path, line_no, line.rstrip()))
      except OSError:
         continue
   return matches


def iter_source_files(src_path):
   exts = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
   for path in src_path.rglob("*"):
      if path.is_file() and path.suffix in exts:
         yield path


def infer_type(type_hint):
   cleaned = re.sub(r"\b(const|volatile)\b", "", type_hint)
   cleaned = cleaned.replace("&", "").replace("*", "").strip()
   lower = cleaned.lower()
   if "bool" in lower:
      return "Bool"
   if "double" in lower or "float" in lower:
      return "Double"
   if re.search(r"\b(unsigned|uint|size_t|uint\d+_t)\b", lower):
      return "UInt"
   if re.search(r"\b(int|long|short|int\d+_t)\b", lower):
      return "Int"
   if "string" in lower:
      return "String"
   if cleaned:
      return "Enum"
   return "Unknown"

def infer_type_from_value(value):
   stripped = value.strip()
   if not stripped:
      return "Unknown"
   lower = stripped.lower()
   if lower in ("true", "false"):
      return "Bool"
   if re.fullmatch(r"-?\d+", stripped):
      return "Int" if stripped.startswith("-") else "UInt"
   if re.fullmatch(r"-?\d+(\.\d+)?([eE][-+]?\d+)?", stripped):
      return "Double"
   return "String"


def parse_bool(value):
   return value.strip().lower() in ("1", "true", "yes")


def add_occurrence(data, name, kind, file_path, line_no, type_hint=None, value=None):
   entry = data.setdefault(name, {"name": name, "occurrences": [], "types": set()})
   if type_hint is not None:
      inferred = infer_type(type_hint)
   elif value is not None:
      inferred = infer_type_from_value(value)
   else:
      inferred = "Unknown"
   entry["types"].add(inferred)
   occurrence = {"file": file_path, "line": line_no, "kind": kind}
   if type_hint is not None:
      occurrence["type_hint"] = type_hint
   if value is not None:
      occurrence["value"] = value
   entry["occurrences"].append(occurrence)


def parse_rg_lines(lines, pattern, kind, data):
   for line in lines:
      parts = line.split(":", 2)
      if len(parts) != 3:
         continue
      file_path, line_str, content = parts
      try:
         line_no = int(line_str)
      except ValueError:
         continue
      for match in pattern.finditer(content):
         if kind in (
            "GetParameter",
            "TryGetParameterFromParameterOrDevice",
            "GetParameterFromParameterOrDeviceOrDefault",
         ):
            type_hint, name = match.group(1), match.group(2)
            add_occurrence(data, name, kind, file_path, line_no, type_hint=type_hint)
         else:
            name = match.group(1)
            add_occurrence(data, name, kind, file_path, line_no)


def discover_device_xml_parameters(data, repo_root):
   libtech_dir = repo_root / "etc" / "libtech"
   if not libtech_dir.exists():
      return
   for xml_path in libtech_dir.rglob("*-seed.xml"):
      try:
         stack = []
         device_depth = None
         for event, elem in ET.iterparse(str(xml_path), events=("start", "end")):
            if event == "start":
               stack.append(elem.tag)
               if elem.tag == "device":
                  device_depth = len(stack) - 1
                  continue
               if device_depth is not None and len(stack) == device_depth + 2:
                  is_bash_var = elem.attrib.get("is_bash_var", "")
                  if is_bash_var and parse_bool(is_bash_var):
                     continue
                  value = elem.attrib.get("value", "")
                  add_occurrence(
                     data,
                     elem.tag,
                     "DeviceXml",
                     str(xml_path),
                     0,
                     value=value,
                  )
            else:
               if elem.tag == "device":
                  device_depth = None
               stack.pop()
      except ET.ParseError:
         continue


def discover_multiline_parameter_calls(data, src_path, patterns):
   for path in iter_source_files(src_path):
      try:
         content = path.read_text(encoding="utf-8", errors="ignore")
      except OSError:
         continue
      for pattern, kind in patterns:
         for match in pattern.finditer(content):
            type_hint, name = match.group(1), match.group(2)
            line_no = content.count("\n", 0, match.start()) + 1
            add_occurrence(data, name, kind, str(path), line_no, type_hint=type_hint)


def finalize_entries(data):
   entries = []
   for name in sorted(data.keys()):
      entry = data[name]
      types = sorted(entry["types"])
      known_types = [t for t in types if t != "Unknown"]
      if len(known_types) == 1:
         inferred_type = known_types[0]
      else:
         inferred_type = "Unknown"
      occurrences = sorted(entry["occurrences"], key=lambda x: (x["file"], x["line"]))
      first_declared = ""
      if occurrences:
         first = occurrences[0]
         first_declared = "{}:{}".format(first["file"], first["line"])
      entries.append(
         {
            "name": name,
            "inferred_type": inferred_type,
            "types": types,
            "occurrences": occurrences,
            "first_declared_in": first_declared,
         }
      )
   return entries


def write_if_changed(path, content):
   if path.exists():
      existing = path.read_text()
      if existing == content:
         return
   path.write_text(content)


def write_outputs(entries, build_dir):
   build_dir.mkdir(parents=True, exist_ok=True)
   output_json = {
      "generated_at": datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z"),
      "parameters": entries,
   }
   json_path = build_dir / "bambu_parameters_discovered.json"
   write_if_changed(json_path, json.dumps(output_json, indent=2, sort_keys=True) + "\n")

   md_lines = [
      "| name | inferred_type | occurrences | example |",
      "| --- | --- | --- | --- |",
   ]
   for entry in entries:
      example = entry["first_declared_in"] or "-"
      md_lines.append(
         "| {} | {} | {} | {} |".format(
            entry["name"], entry["inferred_type"], len(entry["occurrences"]), example
         )
      )
   md_path = build_dir / "bambu_parameters_discovered.md"
   write_if_changed(md_path, "\n".join(md_lines) + "\n")

   unknown = [entry["name"] for entry in entries if entry["inferred_type"] == "Unknown"]
   unknown_path = build_dir / "bambu_parameters_unknown_type.txt"
   write_if_changed(unknown_path, "\n".join(unknown) + ("\n" if unknown else ""))


def main():
   parser = argparse.ArgumentParser(description="Discover bambu-parameters from source code and device seeds.")
   parser.add_argument("--src-dir", default="src", help="Source directory to scan (default: src)")
   parser.add_argument("--build-dir", default="build", help="Output directory (default: build)")
   args = parser.parse_args()

   repo_root = pathlib.Path(__file__).resolve().parents[2]
   cwd = pathlib.Path.cwd()
   src_path = pathlib.Path(args.src_dir)
   if not src_path.is_absolute():
      cwd_path = (cwd / src_path).resolve()
      if cwd_path.exists():
         src_path = cwd_path
      else:
         src_path = (repo_root / src_path).resolve()
   else:
      src_path = src_path.resolve()
   build_dir = pathlib.Path(args.build_dir)
   if not build_dir.is_absolute():
      cwd_build = (cwd / build_dir).resolve()
      if cwd_build.parent.exists():
         build_dir = cwd_build
      else:
         build_dir = (repo_root / build_dir).resolve()
   else:
      build_dir = build_dir.resolve()
   if not src_path.exists():
      sys.stderr.write("Source directory not found: {}\n".format(src_path))
      return 2

   data = {}
   try:
      lines = run_rg(r'IsParameter\("[^"]+"\)', str(src_path), repo_root)
      parse_rg_lines(lines, PATTERN_IS_PARAMETER, "IsParameter", data)
      lines = run_rg(r'GetParameter<[^>]+>\("[^"]+"\)', str(src_path), repo_root)
      parse_rg_lines(lines, PATTERN_GET_PARAMETER, "GetParameter", data)
      discover_multiline_parameter_calls(
         data,
         src_path,
         [
            (PATTERN_TRY_GET_PARAMETER_FROM_PARAMETER_OR_DEVICE, "TryGetParameterFromParameterOrDevice"),
            (
               PATTERN_GET_PARAMETER_FROM_PARAMETER_OR_DEVICE_OR_DEFAULT,
               "GetParameterFromParameterOrDeviceOrDefault",
            ),
         ],
      )
      discover_device_xml_parameters(data, repo_root)
   except RuntimeError as exc:
      sys.stderr.write("{}\n".format(exc))
      return 2

   entries = finalize_entries(data)
   write_outputs(entries, build_dir)
   return 0


if __name__ == "__main__":
   raise SystemExit(main())
