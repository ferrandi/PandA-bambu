#!/usr/bin/python3

import argparse
import csv
from collections import defaultdict


class SplitArgs(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, values.split(','))


parser = argparse.ArgumentParser(
    description="Compare benchmark results from two spider generated csv files", fromfile_prefix_chars="@")
parser.add_argument(
    "files", help="The csv spider-generated files to be compared", nargs='+', action="append")
parser.add_argument('-d', "--datapoints", help="Comma separated list of row names to compare",
                    required=True, action=SplitArgs)
parser.add_argument('-s', "--score", help="Comma separated list of weights to aggregate datapoints as a score",
                    required=False, action=SplitArgs)
parser.add_argument("--returnfail", help="Return FAILURE in case at least one perf point is worse (default=false)",
                    default=False, action="store_true")
parser.add_argument('-o', "--output", help="The output file to fill with aggregated benchmark results",
                    default="")

args = parser.parse_args()

benchname_col = 'Benchmark'
score_col = 'Score'

print('Selected datapoints: ' + ', '.join(args.datapoints))
base_bench_dict = defaultdict(object)
new_bench_dict = defaultdict(object)
with open(args.files[0][0], newline='') as csv_file:
    bench_dict = csv.DictReader(csv_file)
    for row in bench_dict:
        perf_point = defaultdict(object)
        for col in row:
            perf_point[col.strip(' ')] = row[col]
        base_bench_dict[row[benchname_col]] = perf_point

with open(args.files[0][1], newline='') as csv_file:
    bench_dict = csv.DictReader(csv_file)
    for row in bench_dict:
        perf_point = defaultdict(str)
        for col in row:
            perf_point[col.strip(' ')] = row[col]
        new_bench_dict[row[benchname_col]] = perf_point

pair_count = 0
perf_dict = defaultdict(lambda: [])
score_weight = [1.0 for x in args.datapoints]
use_score = args.score != None and len(args.score) > 0
if use_score:
    idx = 0
    for w in args.score:
        score_weight[idx] *= float(w)
        idx += 1
score = 0.0
perf_mean = ['Mean', *[0.0 for x in args.datapoints], 0.0]
bad_perf = []

for bench_name in base_bench_dict:
    base_bench = base_bench_dict[bench_name]
    if new_bench_dict[bench_name] != None:
        pair_count += 1
        new_bench = new_bench_dict[bench_name]
        perf_point = [bench_name]
        is_bad = False
        idx = 1
        new_score = 0.0
        for pp in args.datapoints:
            base_val = float(base_bench[pp])
            new_val = float(new_bench[pp])
            if base_val == 0 and new_val == 0:
                perf_point.append(0.0)
            elif base_val == 0:
                perf_point.append(1.0)
            else:
                perf_point.append(new_val / base_val)
            new_score += (1.0 / perf_point[idx] - 1.0) * score_weight[idx - 1]
            perf_mean[idx] += perf_point[idx]
            if 1 / perf_point[idx] - 1.0 < 0.0:
                is_bad = True
            idx += 1
        perf_point.append(new_score)
        perf_mean[idx] += new_score
        if use_score:
            is_bad = new_score < 0.0
        if is_bad:
            bad_perf.append(perf_point)
        perf_dict[bench_name] = perf_point
    else:
        print('Base benchmark "' + bench_name + '" not present in new results')


print('Valid pairs compared: ' + str(pair_count) +
      '/' + str(len(base_bench_dict)))
if pair_count == 0:
    exit(-1)

if use_score:
    args.datapoints.append(score_col)

perf_var = ['Variance', *[0.0 for x in args.datapoints]]
for idx in range(1, len(args.datapoints) + 1):
    perf_mean[idx] /= pair_count
    for bench_name in perf_dict:
        perf_var[idx] += (perf_dict[bench_name][idx] - perf_mean[idx]) ** 2
    perf_var[idx] /= pair_count

if len(args.output) > 0:
    print('Writing output csv to "' + args.output + '"')
    with open(args.output, mode='w') as result_csv:
        header = 'Bad performant,' + \
            ','.join([x + ',diff' for x in args.datapoints]) + '\n'
        if use_score:
            header = header[:-6] + '\n'
        result_csv.write(header)
        if use_score:
            result_csv.write(
                'Weights,' + ',,'.join([str(x) for x in score_weight]) + ',,\n')
        for bp in bad_perf:
            base_perf = base_bench_dict[bp[0]]
            new_perf = new_bench_dict[bp[0]]
            base_line = base_perf[benchname_col]
            new_line = ''
            idx = 1
            for pp in args.datapoints:
                if pp == score_col:
                    base_line += ','
                    new_line += ',' + str(bp[idx])
                else:
                    base_line += ',' + base_perf[pp] + ','
                    new_line += ',' + new_perf[pp] + \
                        ',' + "{0:.4%}".format(bp[idx] - 1.0)
                idx += 1
            result_csv.write(base_line + '\n')
            result_csv.write(new_line + '\n')

row_format = "{:50}" + "{:<15}" * (len(args.datapoints))
print(row_format.format('Benchmark', *args.datapoints))
if use_score:
    for bp in bad_perf:
        bp_score = bp.pop()
        print(row_format.format(
            bp[0], *["{0:.4%}".format(x - 1.0) for x in bp[1:]], "{:.4f}".format(bp_score)))
    print('-----------------')
    mean_score = perf_mean.pop()
    print(row_format.format(perf_mean[0], *["{0:.4%}".format(x - 1.0)
                                            for x in perf_mean[1:]], "{:.4f}".format(mean_score)))
    var_score = perf_var.pop()
    print(row_format.format(perf_var[0], *["{0:.4%}".format(x)
                                           for x in perf_var[1:]], "{:.4f}".format(var_score)))
else:
    for bp in bad_perf:
        print(row_format.format(
            bp[0], *["{0:.4%}".format(x - 1.0) for x in bp[1:]]))
    print('-----------------')
    print(row_format.format(
        perf_mean[0], *["{0:.4%}".format(x - 1.0) for x in perf_mean[1:]]))
    print(row_format.format(
        perf_var[0], *["{0:.4%}".format(x) for x in perf_var[1:]]))

if len(bad_perf) > 0 and args.returnfail:
    exit(len(bad_perf))
