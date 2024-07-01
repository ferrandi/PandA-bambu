#!python3

import argparse
import csv
import math
import os
import sys
from collections import defaultdict

benchname_col = 'Benchmark'
score_col = 'USER SCORE'


class SplitArgs(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, values.split(','))


def read_results_from_csv(csv_file):
    data_dict = defaultdict(object)
    with open(csv_file, newline='') as csv_data:
        csv_dict = csv.DictReader(csv_data)
        for row in csv_dict:
            perf_point = defaultdict(object)
            for col in row:
                perf_point[col.strip(' ')] = row[col]
            data_dict[row[benchname_col]] = perf_point
    return data_dict


def compare_results(base_dict, new_dict, datapoints, score_weights, compute_score):
    comp_dict = defaultdict(lambda: [])
    degraded_set = []
    improved_set = []
    for bench_name in base_dict:
        base_bench = base_dict[bench_name]
        if bench_name in new_dict:
            new_bench = new_dict[bench_name]
            comp_point = [bench_name]
            degraded = False
            improved = False
            new_score = 0.0
            for idx, pp in enumerate(datapoints):
                base_val = float(base_bench[pp])
                new_val = float(new_bench[pp])
                cmp_val = 0.0
                if base_val == 0 and new_val == 0:
                    comp_point.append(1.0)
                elif base_val == 0:
                    comp_point.append(new_val)
                    cmp_val = -new_val
                elif new_val == 0:
                    comp_point.append(0.0)
                    cmp_val = base_val
                else:
                    comp_point.append(new_val / base_val)
                    cmp_val = base_val / new_val - 1.0
                new_score += cmp_val * score_weights[idx]
                if cmp_val < 0.0:
                    degraded = True
                elif cmp_val > 0.0:
                    improved = True
            comp_point.append(new_score)
            if compute_score:
                degraded = new_score < 0.0
                improved = new_score > 0.0
            if degraded:
                degraded_set.append(comp_point)
            elif improved:
                improved_set.append(comp_point)
            comp_dict[bench_name] = comp_point
        else:
            print('Base benchmark "' + bench_name +
                  '" not present in new results')
    return [comp_dict, degraded_set, improved_set]


def diff_string(diff, base_val=None, new_val=None):
    diff = diff - 1.0
    if base_val != None and new_val != None:
        base_val = float(base_val)
        new_val = float(new_val)
        prec = 2
        if base_val.is_integer() and new_val.is_integer():
            prec = 0
        if diff == 0.0:
            return "{0: .2%} ({1:.{2}f})".format(diff, base_val, prec)
        return "{0: .2%} ({2:.{3}f}/{1:.{3}f})".format(diff, base_val, new_val, prec)
    else:
        return "{0: .2%}".format(diff)


def print_results(comp_list, base_dict, new_dict, datapoints, score_computed):
    if len(comp_list) > 0:
        row_format = "{:50}" + "{:<30}" * (len(datapoints))
        means = [0.0 for x in datapoints]
        vars = [0.0 for x in datapoints]
        print(row_format.format('Benchmark', *datapoints))
        for it in comp_list:
            it_score = it.pop()
            base_bench = base_dict[it[0]]
            new_bench = new_dict[it[0]]
            for idx, x in enumerate(it[1:]):
                means[idx] += x - 1.0
                vars[idx] += (x - 1.0) ** 2
            if score_computed:
                print(row_format.format(
                    it[0], *[diff_string(x, base_bench[datapoints[idx]], new_bench[datapoints[idx]]) for idx, x in enumerate(it[1:])], "{: .2f}".format(it_score)))
            else:
                print(row_format.format(
                    it[0], *[diff_string(x, base_bench[datapoints[idx]], new_bench[datapoints[idx]]) for idx, x in enumerate(it[1:])]))
        print('--------------------------------------------------')
        print(row_format.format(
            'Mean', *[diff_string(x / len(comp_list) + 1.0) for x in means]))
        print(row_format.format('Standard deviation', *
              [diff_string(math.sqrt(abs((x / len(comp_list)) - ((means[idx] / len(comp_list)) ** 2))) + 1.0) for idx, x in enumerate(vars)]))


def print_results_to_csv(comp_list, base_dict, new_dict, datapoints, score_weights, score_computed, csv_file, first_col='Benchmark'):
    if len(comp_list) > 0:
        with open(csv_file, mode='a') as result_csv:
            header = first_col + ',' + \
                ','.join(
                    [x + ',' + x + '_base,diff' for x in datapoints]) + '\n'
            if score_computed:
                header = header[:-6] + '\n'
            result_csv.write(header)
            if score_computed:
                result_csv.write(
                    'Weights,' + ''.join([str(x) + ',,,' for x in score_weights]) + ',\n')
            for it in comp_list:
                base_perf = base_dict[it[0]]
                new_perf = new_dict[it[0]]
                bench_line = base_perf[benchname_col]
                idx = 1
                for pp in datapoints:
                    if pp == score_col:
                        bench_line += ',' + str(it[idx]) + ',0.0'
                    else:
                        bench_line += ',' + new_perf[pp] + ',' + base_perf[pp] + \
                            ',' + diff_string(it[idx])
                    idx += 1
                result_csv.write(bench_line + '\n')


def main():
    parser = argparse.ArgumentParser(
        description="Compare benchmark results from two spider generated csv files", fromfile_prefix_chars="@")
    parser.add_argument(
        "base", help="The base csv spider-generated benchmark results file", type=str)
    parser.add_argument(
        "other", help="The other csv spider-generated benchmark results file", type=str)
    parser.add_argument('-d', "--datapoints", help="Comma separated list of row names to compare",
                        required=True, action=SplitArgs)
    parser.add_argument('-s', "--score", help="Comma separated list of weights to aggregate datapoints as a score",
                        required=False, action=SplitArgs)
    parser.add_argument("--returnfail", help="Return FAILURE in case at least one perf point is worse (default=false)",
                        default=False, action="store_true")
    parser.add_argument('-o', "--output", help="The output file to fill with aggregated benchmark results",
                        default="")

    args = parser.parse_args()

    selected_datapoints = args.datapoints
    user_weights = args.score
    outfile = args.output
    opt_format = "{:20}|" + "{:^11}|" * (len(selected_datapoints))
    print(opt_format.format('Selected datapoints ', *selected_datapoints))
    score_weight = [1.0 for x in selected_datapoints]
    user_score = user_weights != None and len(user_weights) > 0
    if user_score:
        for idx, w in enumerate(user_weights):
            score_weight[idx] *= float(w)
        print(opt_format.format('Datapoints weight ', *score_weight))

    base_bench_dict = read_results_from_csv(args.base)
    new_bench_dict = read_results_from_csv(args.other)

    [perf_dict, bad_perf, good_perf] = compare_results(
        base_bench_dict, new_bench_dict, selected_datapoints, score_weight, user_score)

    print('Valid pairs compared: ' + str(len(perf_dict)) +
          '/' + str(len(base_bench_dict)))
    if len(perf_dict) == 0:
        print('-1', file=sys.stderr)
        exit(0)

    if user_score:
        selected_datapoints.append(score_col)

    perf_mean = ['Mean', *[0.0 for x in selected_datapoints], 0.0]
    perf_sd = ['Standard deviation', *[0.0 for x in selected_datapoints]]
    for bench_name in perf_dict:
        perf_point = perf_dict[bench_name]
        for idx in range(1, len(selected_datapoints) + 1):
            perf_mean[idx] += perf_point[idx]
            perf_sd[idx] += perf_point[idx] * perf_point[idx]

    for idx in range(1, len(selected_datapoints) + 1):
        perf_mean[idx] /= len(perf_dict)
        perf_sd[idx] = math.sqrt(
            (perf_sd[idx] / len(perf_dict)) - (perf_mean[idx] * perf_mean[idx]))

    if len(outfile) > 0:
        if os.path.exists(outfile):
            os.remove(outfile)
        print('Writing output csv to "' + outfile + '"')
        print_results_to_csv(bad_perf, base_bench_dict, new_bench_dict,
                             selected_datapoints, score_weight, user_score, outfile, 'Degraded')
        print_results_to_csv(good_perf, base_bench_dict,
                             new_bench_dict, selected_datapoints, score_weight, user_score, outfile, 'Improved')

    print('==================================================')
    print('Degraded benchmarks: ' + str(len(bad_perf)))
    print_results(bad_perf, base_bench_dict,
                  new_bench_dict, selected_datapoints, user_score)
    print('==================================================')
    print('Improved benchmarks: ' + str(len(good_perf)))
    print_results(good_perf, base_bench_dict,
                  new_bench_dict, selected_datapoints, user_score)
    print('==================================================')

    row_format = "{:50}" + "{:<30}" * (len(selected_datapoints))
    print(row_format.format('Benchmark', *selected_datapoints))
    if user_score:
        mean_score = perf_mean.pop()
        print(row_format.format(perf_mean[0], *[diff_string(x)
                                                for x in perf_mean[1:]], "{: .2f}".format(mean_score)))
        var_score = perf_sd.pop()
        print(row_format.format(perf_sd[0], *[diff_string(x + 1.0)
                                              for x in perf_sd[1:]], "{: .2f}".format(var_score)))
    else:
        print(row_format.format(
            perf_mean[0], *[diff_string(x) for x in perf_mean[1:]]))
        print(row_format.format(
            perf_sd[0], *[diff_string(x + 1.0) for x in perf_sd[1:]]))

    print(str(len(bad_perf)), file=sys.stderr)
    if args.returnfail:
        exit(len(bad_perf))
    exit(0)


if __name__ == "__main__":
    main()
