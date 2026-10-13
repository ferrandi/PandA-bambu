# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Check the straight-line vector-mask fixtures and their optimized LLVM IR.

The checker ignores SSA renaming, inferred alignment, metadata and pointer
syntax, and canonicalizes the equivalent scalar zext/shift and select used
to pack an extracted bit, masked extensions, packed-byte extraction and lanes
overwritten by an exclusive chain of inserts. Other vector instructions,
widths and mixed-lane constants must remain intact. Use --function for an
internalized Bambu top module. The regression harness requires Python 3.9+.
"""

import argparse
import pathlib
import re
import sys


FUNCTION = re.compile(
    r"^define\b(?P<header>[^\n]*@(?P<name>[\w.]+)\([^\n]*\)[^\n{]*)\{\n"
    r"(?P<body>.*?)^}", re.MULTILINE | re.DOTALL
)


INSERT = re.compile(r"(%[\w.]+) = insertelement <(\d+) x i(\d+)> (.+), i\3 ([^,]+), i\d+ (\d+)")


def consumers(lines, value):
    """Return every operand use, excluding the SSA definition itself."""
    return [index for index, line in enumerate(lines)
            for _ in re.finditer(re.escape(value) + r"(?![\w.])", line.split(" = ", 1)[-1])]


def normalize_insertions(lines):
    insertions = {index: match.groups() for index, line in enumerate(lines)
                  for match in [INSERT.fullmatch(line)] if match}
    result = lines[:]
    for index, (value, count, width, initial, scalar, position) in insertions.items():
        count, position = int(count), int(position)
        if position >= count:
            continue
        overwritten = {position}
        current = value
        # The intermediate vector must have exactly one use: the next insert.
        # A read, branch or extra operand use makes later lanes observable.
        while True:
            users = consumers(lines, current)
            following = insertions.get(users[0]) if len(users) == 1 else None
            if not following or following[1:3] != (str(count), width) or following[3] != current:
                break
            if int(following[5]) >= count:
                break
            overwritten.add(int(following[5]))
            current = following[0]
        literal = re.fullmatch(r"<([^>]*)>", initial)
        if literal:
            lanes = literal.group(1).split(", ")
            if len(lanes) == count:
                for lane in overwritten:
                    lanes[lane] = "i{} 0".format(width)
                initial = "zeroinitializer" if all(lane == "i{} 0".format(width) for lane in lanes) else (
                    "<" + ", ".join(lanes) + ">")
        result[index] = "{} = insertelement <{} x i{}> {}, i{} {}, i32 {}".format(
            value, count, width, initial, width, scalar, position)
    return result


def normalize_packed_bytes(lines):
    """Replace exclusively used scalar byte slices with the vector-byte form."""
    result, removed = lines[:], set()
    for index, line in enumerate(lines):
        cast = re.fullmatch(r"(%[\w.]+) = bitcast <(\d+) x i1> (%[\w.]+) to i\2", line)
        if not cast:
            continue
        value, width, mask = cast.groups()
        bits = int(width)
        if bits <= 8 or bits % 8:
            continue
        slices, shifts = {}, set()
        valid = True
        for user in consumers(lines, value):
            low = re.fullmatch(r"(%[\w.]+) = trunc i" + width + " " + re.escape(value) + " to i8", lines[user])
            shift = re.fullmatch(r"(%[\w.]+) = lshr i" + width + " " + re.escape(value) + r", (\d+)", lines[user])
            if low:
                slices[user] = (low.group(1), 0)
            elif shift and int(shift.group(2)) % 8 == 0 and int(shift.group(2)) < bits:
                shifted = shift.group(1)
                users = consumers(lines, shifted)
                high = re.fullmatch(r"(%[\w.]+) = trunc i" + width + " " + re.escape(shifted) + " to i8",
                                    lines[users[0]]) if len(users) == 1 else None
                if not high:
                    valid = False
                    break
                slices[users[0]] = (high.group(1), int(shift.group(2)) // 8)
                shifts.add(user)
            else:
                valid = False
                break
        if valid and slices:
            result[index] = "{} = bitcast <{} x i1> {} to <{} x i8>".format(value, bits, mask, bits // 8)
            for user, (output, byte) in slices.items():
                result[user] = "{} = extractelement <{} x i8> {}, i32 {}".format(output, bits // 8, value, byte)
            removed.update(shifts)
    return [line for index, line in enumerate(result) if index not in removed]


def normalize_masked_extensions(lines):
    result = lines[:]
    for index, line in enumerate(lines):
        trunc = re.fullmatch(r"(%[\w.]+) = trunc i(\d+) (%[\w.]+) to i(\d+)", line)
        if not trunc:
            continue
        value, source_width, operand, narrow_width = trunc.groups()
        users = consumers(lines, value)
        extension = re.fullmatch(r"(%[\w.]+) = zext i" + narrow_width + " " + re.escape(value) + r" to i(\d+)",
                                 lines[users[0]]) if len(users) == 1 else None
        if extension and int(narrow_width) < int(source_width) < int(extension.group(2)):
            result[index] = "{} = and i{} {}, {}".format(value, source_width, operand, (1 << int(narrow_width)) - 1)
            result[users[0]] = "{} = zext i{} {} to i{}".format(
                extension.group(1), source_width, value, extension.group(2))
    return result


def normalize_constant_indices(line):
    # Positive constant indices into these small fixed vectors/arrays do not
    # depend on whether LLVM chooses a 32-bit or 64-bit index type.
    extract = re.fullmatch(r"(.* = extractelement <(\d+) x i8> %[\w.]+), i(?:32|64) (\d+)", line)
    if extract and int(extract.group(3)) < min(int(extract.group(2)), 1 << 31):
        return "{}, i32 {}".format(extract.group(1), extract.group(3))
    gep = re.fullmatch(r"(.* = getelementptr \[(\d+) x i\d+\], ptr %[\w.]+), i(?:32|64) 0, i(?:32|64) (\d+)", line)
    if gep and int(gep.group(3)) < min(int(gep.group(2)), 1 << 31):
        return "{}, i32 0, i32 {}".format(gep.group(1), gep.group(3))
    return line


def parse_ir(path):
    text = path.read_text()
    groups = dict(re.findall(r"^attributes\s+#(\d+)\s*=\s*{([^}]*)}", text, re.MULTILINE))
    result = {}
    for match in FUNCTION.finditer(text):
        header = match.group("header")
        signature = re.search(r"@" + re.escape(match.group("name")) + r"\((.*)\)(.*)$", header)
        attributes = signature.group(2)
        for group in re.findall(r"#(\d+)", attributes):
            attributes += " " + groups.get(group, "")
        attributes = re.sub(r";[^\n]*", "", attributes)
        attributes = re.sub(r'"[^"\n]*"', "", attributes)
        words = set(re.findall(r"\b\w+\b", attributes))
        if not {"noinline", "optnone"} <= words:
            raise ValueError("{}: {} lacks real noinline/optnone attributes".format(path, match.group("name")))

        names = {}

        def rename(token):
            name = token.group(0)
            if name not in names:
                names[name] = "%v{}".format(len(names))
            return names[name]

        parameters = signature.group(1)
        re.sub(r"%[\w.]+", rename, parameters)
        lines = []
        for line in match.group("body").splitlines():
            line = line.split(";", 1)[0].strip()
            if not line or line.endswith(":"):
                continue
            line = re.sub(r",\s*!.*$", "", line)
            line = re.sub(r",\s*align\s+\d+", "", line)
            line = re.sub(r"\b(?:nsw|nuw|exact|inbounds)\s+", "", line)
            line = re.sub(r"(?:<[^>\n]+>|\[[^\]\n]+\]|i\d+)\s*\*", "ptr", line)
            lines.append(normalize_constant_indices(" ".join(line.split())))

        lines = normalize_insertions(lines)
        lines = normalize_packed_bytes(lines)
        lines = normalize_masked_extensions(lines)

        # Bambu's LLVM 16/19 pipeline turns zext(i1) << K into a select.
        # Only fold this scalar pattern when the extension has one use.
        canonical = []
        index = 0
        while index < len(lines):
            extension = re.fullmatch(r"(%[\w.]+) = zext i1 (%[\w.]+) to i(\d+)", lines[index])
            if extension and index + 1 < len(lines):
                value, bit, width = extension.groups()
                shift = re.fullmatch(r"(%[\w.]+) = shl i" + width + r" " + re.escape(value) + r", (\d+)",
                                     lines[index + 1])
                uses = re.findall(re.escape(value) + r"(?![\w.])", "\n".join(lines))
                if shift and len(uses) == 2 and int(shift.group(2)) < int(width):
                    canonical.append("{} = select i1 {}, i{} {}, i{} 0".format(
                        shift.group(1), bit, width, 1 << int(shift.group(2)), width))
                    index += 2
                    continue
            canonical.append(lines[index])
            index += 1

        instructions = []
        for line in canonical:
            line = re.sub(r"%[\w.]+", rename, line)
            commutative = re.fullmatch(r"(%v\d+ = or i\d+) (%v\d+), (%v\d+)", line)
            if commutative:
                left, right = sorted(commutative.groups()[1:])
                line = "{} {}, {}".format(commutative.group(1), left, right)
            instructions.append(line)
        result[match.group("name")] = instructions
    if not result:
        raise ValueError("{}: no fixture functions found".format(path))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("--optimized", type=pathlib.Path)
    parser.add_argument("--function")
    args = parser.parse_args()
    try:
        source = parse_ir(args.source)
        if args.optimized:
            optimized = parse_ir(args.optimized)
            selected = [args.function] if args.function else sorted(source)
            for name in selected:
                if name not in source or name not in optimized:
                    raise ValueError("{}: missing function {}".format(args.optimized, name))
                if source[name] != optimized[name]:
                    raise ValueError("{}: {} instructions changed\nexpected: {}\nactual: {}".format(
                        args.optimized, name, source[name], optimized[name]))
            print("PASS: {} functions retain their protected instructions".format(len(selected)))
        else:
            print("PASS: {} functions have noinline/optnone".format(len(source)))
    except (ValueError, OSError) as error:
        print("FAIL: {}".format(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
