import argparse
import xml.etree.ElementTree as ET
import sys
import os

def remove_cells_by_prefix(input_path, output_path, prefixes):
    try:
        with open(input_path, 'r', encoding='utf-8') as f:
            original_content = f.read()
        parser = ET.XMLParser(target=ET.TreeBuilder(insert_comments=True))
        root = ET.fromstring(original_content, parser=parser)
        tree = ET.ElementTree(root)
    except ET.ParseError as e:
        print(f"Skipping {input_path} (XML parse error: {e})", file=sys.stderr)
        return

    libraries = root.findall('.//library')
    if not libraries:
        print(f"Skipping {input_path} (no <library> elements found)", file=sys.stderr)
        return

    total_removed = 0
    for library in libraries:
        removed_count = 0
        for cell in list(library.findall('cell')):
            name_elem = cell.find('name')
            if name_elem is not None and name_elem.text:
                if any(name_elem.text.startswith(prefix) for prefix in prefixes):
                    library.remove(cell)
                    removed_count += 1
        total_removed += removed_count

    if total_removed == 0:
        print(f"[{os.path.basename(input_path)}] No matching cells found. File left unchanged.")
        return
    
    tree.write(output_path, encoding='utf-8')

    print(f"[{os.path.basename(input_path)}] Removed {total_removed} cell(s)")

def process_single_file(input_file, output_file, prefixes):
    remove_cells_by_prefix(input_file, output_file, prefixes)

def process_directory(input_dir, prefixes):
    for fname in os.listdir(input_dir):
        if fname.endswith('.xml'):
            full_path = os.path.join(input_dir, fname)
            remove_cells_by_prefix(full_path, full_path, prefixes)

def main():
    parser = argparse.ArgumentParser(description='Remove <cell> elements by <name> prefix from XML files.')
    parser.add_argument('input_path', help='Path to input XML file or directory')
    parser.add_argument('--remove-prefix', type=str, required=True,
                        help='Comma-separated list of prefixes to remove (e.g., widen_sum_expr,foo_)')
    parser.add_argument('-o', '--output', help='Output file path (only valid if input is a single file)')
    parser.add_argument('-i', '--inplace', action='store_true', help='Modify input file(s) in place')

    args = parser.parse_args()
    prefixes = [p.strip() for p in args.remove_prefix.split(',') if p.strip()]

    if not prefixes:
        print("Error: No valid prefixes provided.", file=sys.stderr)
        sys.exit(1)

    if os.path.isdir(args.input_path):
        if args.output:
            print("Error: Cannot use -o with a directory input.", file=sys.stderr)
            sys.exit(1)
        if not args.inplace:
            print("Error: -i is required when input is a directory.", file=sys.stderr)
            sys.exit(1)
        process_directory(args.input_path, prefixes)

    elif os.path.isfile(args.input_path):
        output_path = args.input_path if args.inplace else args.output
        if output_path is None:
            print("Error: Must specify -o or -i when input is a file.", file=sys.stderr)
            sys.exit(1)
        process_single_file(args.input_path, output_path, prefixes)

    else:
        print("Error: Input path is neither a file nor a directory.", file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main()
