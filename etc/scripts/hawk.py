#!/usr/bin/env python3
import argparse
import sys
import xml.etree.ElementTree as ET
from copy import deepcopy

score_col = 'USER SCORE'


class SplitArgs(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, values.split(','))


def find_simulation_runs(app):
    runs = app.findall('.//timing//simulation//run')
    if runs:
        return runs
    return app.findall('.//timing//evaluation//run')


def get_metric_element(app):
    eval_elm = app.find('evaluation')
    if eval_elm is not None:
        return eval_elm
    return app.find('resources')


def read_results_from_xml(xml_file):
    """Parse the results.xml format and return a dict mapping benchmark -> (application_element, resources_attr_dict)
    """
    tree = ET.parse(xml_file)
    root = tree.getroot()
    data = {}
    for app in root.findall('application'):
        bench = app.get('benchmark')
        if bench is None:
            continue
        res = app.find('resources')
        eval_elm = app.find('evaluation')
        metric_elm = eval_elm if eval_elm is not None else res
        res_attrs = {}
        res_text_attrs = {}
        raw_res_text_attrs = {}
        if res is not None:
            for k, v in res.items():
                raw_res_text_attrs[k] = v
                res_text_attrs[k] = v
                try:
                    res_attrs[k] = float(v)
                except Exception:
                    res_attrs[k] = v
        if eval_elm is not None:
            for k, v in eval_elm.items():
                res_text_attrs[k] = v
                try:
                    res_attrs[k] = float(v)
                except Exception:
                    res_attrs[k] = v
        elif metric_elm is None and find_simulation_runs(app):
            metric_elm = ET.SubElement(app, 'evaluation')
        normalize_resource_attrs(app, metric_elm, res_attrs, res_text_attrs, raw_res_text_attrs)
        data[bench] = (deepcopy(app), res_attrs)
    return data


def set_resource_attr(res, res_attrs, res_text_attrs, key, value):
    if key in res_attrs:
        return
    text_value = str(value)
    res_text_attrs[key] = text_value
    try:
        res_attrs[key] = float(text_value)
    except Exception:
        res_attrs[key] = text_value
    if res is not None:
        res.set(key, text_value)


def normalize_resource_attrs(app, res, res_attrs, res_text_attrs, raw_res_text_attrs):
    runs = []
    for run in find_simulation_runs(app):
        if run is None or run.text is None:
            continue
        try:
            runs.append(float(run.text.strip()))
        except Exception:
            continue
    if runs:
        total = sum(runs)
        set_resource_attr(res, res_attrs, res_text_attrs, 'TOTAL_CYCLES', f"{total:.0f}")
        set_resource_attr(res, res_attrs, res_text_attrs, 'CYCLES', f"{total / len(runs):.3f}")
        set_resource_attr(res, res_attrs, res_text_attrs, 'RUNS', str(len(runs)))

    for normalized, source in (('PERIOD', 'DELAY'), ('CLOCK_SLACK', 'SLACK'), ('AREA', 'SLICES')):
        if source in raw_res_text_attrs:
            set_resource_attr(res, res_attrs, res_text_attrs, normalized, raw_res_text_attrs[source])


def compare_results(base_dict, new_dict, datapoints, score_weights, compute_score):
    """Compare two result dicts. Returns comp_dict mapping bench->(compare_attrs,score,cmp_vals_list),
    plus lists of degraded and improved benchmarks.
    """
    comp_dict = {}
    degraded = []
    improved = []
    unmatched_base = []
    unmatched_new = []
    for bench, (base_app, base_res) in base_dict.items():
        if bench not in new_dict:
            # collect unmatched baseline-only applications
            unmatched_base.append((bench, base_app))
            continue
        new_app, new_res = new_dict[bench]
        compare_attrs = {}
        score = 0.0
        any_degraded = False
        any_improved = False
        cmp_vals = []
        for idx, dp in enumerate(datapoints):
            if dp not in base_res or dp not in new_res:
                compare_attrs[dp] = 'nan'
                cmp_vals.append(None)
                continue
            base_val = base_res[dp]
            new_val = new_res[dp]
            # ensure floats
            try:
                base_val_f = float(base_val)
                new_val_f = float(new_val)
            except Exception:
                compare_attrs[dp] = 'nan'
                cmp_vals.append(None)
                continue
            if base_val_f == 0 and new_val_f == 0:
                cmp_val = 0.0
            elif base_val_f == 0:
                cmp_val = -new_val_f
            elif new_val_f == 0:
                cmp_val = base_val_f
            else:
                cmp_val = base_val_f / new_val_f - 1.0
            cmp_vals.append(cmp_val)
            score += cmp_val * score_weights[idx]
            if cmp_val < 0:
                any_degraded = True
            elif cmp_val > 0:
                any_improved = True
            percent = cmp_val * 100.0
            compare_attrs[dp] = f"{percent:.2f}%"
        
        if compute_score:
            any_degraded = score < 0.0
            any_improved = score > 0.0
        comp_dict[bench] = (compare_attrs, score, cmp_vals, base_app, new_app)
        if any_degraded:
            degraded.append(bench)
        elif any_improved:
            improved.append(bench)
    
    for bench, (new_app, new_res) in new_dict.items():
        if bench not in base_dict:
            unmatched_new.append((bench, new_app))

    return comp_dict, degraded, improved, unmatched_base, unmatched_new

def build_output_xml(comp_dict, selected_datapoints, degraded_list, improved_list, unmatched_base, unmatched_new, output_file=None):
    root = ET.Element('benchmarks')
    total_compared = len(comp_dict)
    root.set('total', str(total_compared))
    root.set('improved', str(len(improved_list)))
    root.set('degraded', str(len(degraded_list)))
    root.set('unmatched', str(len(unmatched_base) + len(unmatched_new)))
    
    def minimal_app_copy(orig_app):
        """Return a new application element containing only attributes and resources/timing children."""
        new_app_elem = ET.Element('application', dict(orig_app.attrib))
        for child_name in ('resources', 'evaluation', 'timing'):
            child = orig_app.find(child_name)
            if child is not None:
                new_app_elem.append(deepcopy(child))
        return new_app_elem
    
    for bench, (compare_attrs, score, cmp_vals, base_app, new_app) in comp_dict.items():
        app_out = ET.SubElement(root, 'application', {'benchmark': bench})
        
        is_degraded = 'true' if bench in degraded_list else 'false'
        is_improved = 'true' if bench in improved_list else 'false'
        app_out.set('degraded', is_degraded)
        app_out.set('improved', is_improved)
        
        baseline = ET.SubElement(app_out, 'baseline')
        baseline.append(minimal_app_copy(base_app))

        current = ET.SubElement(app_out, 'current')
        current.append(minimal_app_copy(new_app))
        
        compare = ET.SubElement(app_out, 'compare')
        for dp in selected_datapoints:
            if dp in compare_attrs:
                compare.set(dp, str(compare_attrs[dp]))
        compare.set('score', f"{score:.4f}")
    
    if len(unmatched_base) > 0 or len(unmatched_new) > 0:
        missing = ET.SubElement(root, 'missing')
        if len(unmatched_base) > 0:
            base_m = ET.SubElement(missing, 'baseline')
            for bench, app in unmatched_base:
                base_m.append(minimal_app_copy(app))
        if len(unmatched_new) > 0:
            new_m = ET.SubElement(missing, 'current')
            for bench, app in unmatched_new:
                new_m.append(minimal_app_copy(app))
    tree = ET.ElementTree(root)

    try:
        ET.indent(tree, space="  ", level=0)
    except Exception:
        pass
    if output_file:
        tree.write(output_file, encoding='utf-8', xml_declaration=True)
    else:
        ET.dump(tree)


def print_terminal_results(comp_dict, base_bench_dict, new_bench_dict, selected_datapoints, degraded_list, improved_list, score_computed):
    """Print degraded and improved benchmarks to terminal similar to old behavior."""
    def format_entry(bench, compare_attrs, base_app, new_app):
        res_base = get_metric_element(base_app)
        res_new = get_metric_element(new_app)
        parts = []
        for dp in selected_datapoints:
            perc = compare_attrs.get(dp, 'nan')
            base_val = res_base.get(dp) if res_base is not None else 'nan'
            new_val = res_new.get(dp) if res_new is not None else 'nan'
            parts.append(f"{perc} ({base_val}/{new_val})")
        return parts

    hdr = ["Benchmark"] + selected_datapoints
    print(" | ".join(hdr))
    print('==================================================')
    print('Degraded benchmarks: ' + str(len(degraded_list)))
    for bench in degraded_list:
        compare_attrs, score, cmp_vals, base_app, new_app = comp_dict[bench]
        parts = format_entry(bench, compare_attrs, base_app, new_app)
        print(bench + ' | ' + ' | '.join(parts) + (f' | score={score:.4f}' if score is not None else ''))
    print('==================================================')
    print('Improved benchmarks: ' + str(len(improved_list)))
    for bench in improved_list:
        compare_attrs, score, cmp_vals, base_app, new_app = comp_dict[bench]
        parts = format_entry(bench, compare_attrs, base_app, new_app)
        print(bench + ' | ' + ' | '.join(parts) + (f' | score={score:.4f}' if score is not None else ''))
    print('==================================================')

def main():
    parser = argparse.ArgumentParser(
        description="Compare benchmark results from two results.xml files", fromfile_prefix_chars="@")
    parser.add_argument("base", help="The base xml results file", type=str)
    parser.add_argument("other", help="The other xml results file", type=str)
    parser.add_argument('-d', "--datapoints", help="Comma separated list of resource names to compare",
                        required=True, action=SplitArgs)
    parser.add_argument('-s', "--score", help="Comma separated list of weights to aggregate datapoints as a score",
                        required=False, action=SplitArgs)
    parser.add_argument("--returnfail", help="Return FAILURE in case at least one perf point is worse (default=false)",
                        default=False, action="store_true")
    parser.add_argument('-o', "--output", help="The output xml file to write", default="")

    args = parser.parse_args()

    selected_datapoints = args.datapoints
    user_weights = args.score
    outfile = args.output if len(args.output) > 0 else None

    score_weight = [1.0 for _ in selected_datapoints]
    user_score = user_weights is not None and len(user_weights) > 0
    if user_score:
        for idx, w in enumerate(user_weights):
            score_weight[idx] *= float(w)

    base_bench_dict = read_results_from_xml(args.base)
    new_bench_dict = read_results_from_xml(args.other)

    comp_dict, bad_perf, good_perf, unmatched_base, unmatched_new = compare_results(
        base_bench_dict, new_bench_dict, selected_datapoints, score_weight, user_score)

    build_output_xml(comp_dict, selected_datapoints, bad_perf, good_perf, unmatched_base, unmatched_new, outfile)

    # If output is written to a file, print degraded/improved summaries to stdout
    if outfile is not None:
        print_terminal_results(comp_dict, base_bench_dict, new_bench_dict, selected_datapoints, bad_perf, good_perf, user_score)

    print(str(len(bad_perf)), file=sys.stderr)
    if args.returnfail:
        sys.exit(len(bad_perf))
    sys.exit(0)


if __name__ == "__main__":
    main()
