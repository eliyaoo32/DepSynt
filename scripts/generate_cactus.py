import pathlib
from typing import Callable

from .results_summarizer import load_find_deps, load_depsynt, DepSyntBenchmark
import csv
import json
import os.path
import shutil

TOOLS = [
    'strix',
    'depsynt',
    'spot-acd',
    # 'spot-ds',
    'spot-lar',
    # 'spot-sd',
    'spotmodular',
]


def get_depsynt_item(benchmark_id) -> DepSyntBenchmark:
    depsynt_results = os.path.join(pathlib.Path(__file__).parent.resolve(), "../tasks_output/depsynt")
    benchmark_file = os.path.join(pathlib.Path(__file__).parent.resolve(), "../tasks_output/generated_benchmarks/text/"+benchmark_id+".txt")
    if not os.path.exists(benchmark_file):
        raise Exception(f'Benchmark file {benchmark_file} does not exist')

    return load_depsynt(depsynt_results, benchmark_file)


def should_include_row_by_depsynt(depsynt_cond: Callable[[DepSyntBenchmark], bool]):
    def should_include_row(row):
        benchmark_id = row['Benchmark Id']
        depsynt_summary = get_depsynt_item(benchmark_id)
        has_deps = len(depsynt_summary.dependent_variables) > 0

        return has_deps and depsynt_cond(depsynt_summary)

    return should_include_row


def should_include_row_by_deps(indep_cond=None, dep_cond=None):
    def should_include_row(row):
        benchmark_id = row['Benchmark Id']
        find_deps_results = os.path.join(pathlib.Path(__file__).parent.resolve(), "../tasks_output/find_deps")
        benchmark_file = os.path.join(pathlib.Path(__file__).parent.resolve(), "../tasks_output/generated_benchmarks/text/"+benchmark_id+".txt")
        find_deps_summary = load_find_deps(find_deps_results, benchmark_file)

        has_deps = len(find_deps_summary.dependent_variables) > 0

        passed_indep_cond = indep_cond(find_deps_summary.independent_variables) if indep_cond is not None else True
        passed_dep_cond = dep_cond(find_deps_summary.dependent_variables) if dep_cond is not None else has_deps

        return passed_indep_cond and passed_dep_cond

    return should_include_row


def load_stats_from_csv(tool_csv_file, should_include_row):
    stats = {}

    with open(tool_csv_file, 'r') as fp:
        reader = csv.DictReader(fp)
        rows = list(reader)
        for row in rows:
            if not should_include_row(row):
                # print("Skipping row: " + row['Benchmark Id'])
                continue
            success_ = row['Status'] == 'Success'
            stats[row['Benchmark Name']] = {
                'status': success_,
            }
            if success_:
                rtime = float(row['Total Duration']) / 1000.0  # In seconds
                if rtime < 0.0001:
                    rtime = 0.0001
                stats[row['Benchmark Name']]['rtime'] = rtime

    return stats


def get_filename(filepath):
    return os.path.basename(filepath).split('.')[0]


def get_current_dir():
    return os.path.dirname(os.path.realpath(__file__))


def load_tool_json(tool_csv_file, should_include_row):
    stats = load_stats_from_csv(tool_csv_file, should_include_row)
    if len(stats.keys()) == 0:
        raise Exception('No stats found ' + tool_csv_file)

    tool_name = get_filename(tool_csv_file)
    return {
        'preamble': {
            'program': tool_name,
            'prog_alias': tool_name,
            'benchmark': 'SYNTCOMP',
        },
        'stats': stats,
    }


def save_cactus_to_file(filename='cactus.png', title=None, ylog=True):
    # TODO: check that mkplot is installed
    # cactus_gen_command = "python ./scripts/mkplot/mkplot.py --ylabel=\"Time (seconds)\" --ymin 0.0001 --ymax 15 {} -t 10800 {} -b png --save-to {} ./tasks_output/tmp_cactus/*.json".format(
    cactus_gen_command = "python ./scripts/mkplot/mkplot.py --ylabel=\"Time (seconds)\" --ymin 0.0001 {} -t 10800 {} -b png --save-to {} ./tasks_output/tmp_cactus/*.json".format(
        "--ylog" if ylog else "",
        f"--plot-title \"{title}\"" if title is not None else "",
        f"\"{filename}\""
    )
    os.system(cactus_gen_command)


# TODO: ignore instances that have 0 dependent variables
def generate_cactus(filename, title, ylog, should_include_row):
    # Create tools CSV paths
    tools_csv_paths = [
        os.path.join(get_current_dir(), f'../tasks_output/{t}.csv')
        for t in TOOLS
    ]

    # Verify if all tools CSV files exist
    for tool_path in tools_csv_paths:
        if not os.path.exists(tool_path):
            print(f'Error: tool CSV file {tool_path} does not exist in the path f{tool_path}')
            exit(1)

    # Create temp directory
    tmp_dir = os.path.join(get_current_dir(), '../tasks_output/tmp_cactus')
    if os.path.exists(tmp_dir):
        print('Error: temp directory already exists')
        exit(1)
    os.makedirs(tmp_dir)

    # Write JSON files
    for tool_path in tools_csv_paths:
        tool_json = load_tool_json(tool_path, should_include_row)
        tool_name = get_filename(tool_path)
        tool_json_file = os.path.join(tmp_dir, tool_name + '.json')
        with open(tool_json_file, 'w') as fp:
            json.dump(tool_json, fp, indent=4)

    save_cactus_to_file(filename, title, ylog)

    shutil.rmtree(tmp_dir)


def main():
    # Non-Grouped Cactus plot
    # generate_cactus(
    #     filename=f'cactus-deps>0.png',
    #     title=f'Y Log, Dep vars > 0',
    #     ylog=True,
    #     should_include_row=should_include_row_by_deps()
    # )

    # Group elements by number of independent variables
    indeps_conds = [
        # {
        #     'name': 'indep=0',
        #     'cond': lambda indep_vars: len(indep_vars) == 0,
        # },
        # {
        #     'name': 'indep=1',
        #     'cond': lambda indep_vars: len(indep_vars) == 1,
        # },
        # {
        #     'name': 'indep=2',
        #     'cond': lambda indep_vars: len(indep_vars) == 2,
        # },
        # {
        #     'name': 'indep=3',
        #     'cond': lambda indep_vars: len(indep_vars) == 3,
        # },
        # {
        #     'name': 'indep=4',
        #     'cond': lambda indep_vars: len(indep_vars) == 4,
        # },
        # {
        #     'name': 'indep>4',
        #     'cond': lambda indep_vars: len(indep_vars) > 4,
        # },
        # {
        #     'name': 'indep=0,1,2,3,4',
        #     'cond': lambda indep_vars: len(indep_vars) <= 4,
        # },
        # {
        #     'name': 'indep=0,1,2,3',
        #     'cond': lambda indep_vars: len(indep_vars) <= 3,
        # },
        {
            'name': 'indep>=4',
            'cond': lambda indep_vars: len(indep_vars) >=4,
        },
    ]

    for indep_cond in indeps_conds:
        generate_cactus(
            filename=f'cactus-ylog-{indep_cond["name"]}.png',
            title=f'Y Log, Deps > 0, {indep_cond["name"]}',
            ylog=True,
            should_include_row=should_include_row_by_deps(
                indep_cond=indep_cond['cond'],
            )
        )
        # generate_cactus(
        #     filename=f'cactus-{indep_cond["name"]}.png',
        #     title=f'Deps > 0, {indep_cond["name"]}',
        #     ylog=False,
        #     should_include_row=should_include_row_by_deps(
        #         indep_cond=indep_cond['cond'],
        #     )
        # )

    return

    # Group elements by depsynt duration
    # depsynt_duration_conds = [
    #     {
    #         'name': 'depsynt<=100ms',
    #         'cond': should_include_row_by_depsynt(lambda x: x.total_duration <= 100)
    #     },
    #     {
    #         'name': '100ms<depsynt<=500ms',
    #         'cond': should_include_row_by_depsynt(lambda x: 100 < x.total_duration <= 500)
    #     },
    #     {
    #         'name': '500ms<depsynt<=1s',
    #         'cond': should_include_row_by_depsynt(lambda x: 500 < x.total_duration <= 1000)
    #     },
    #     {
    #         'name': '1s<depsynt<=100s',
    #         'cond': should_include_row_by_depsynt(lambda x: 1000 < x.total_duration <= 100000)
    #     },
    #     {
    #         'name': '100s<depsynt<=500s',
    #         'cond': should_include_row_by_depsynt(lambda x: 100000 < x.total_duration <= 500000)
    #     },
    #     {
    #         'name': '500s<depsynt<=1000s',
    #         'cond': should_include_row_by_depsynt(lambda x: 500000 < x.total_duration <= 1000000)
    #     },
    #     {
    #         'name': '1000s<depsynt',
    #         'cond': should_include_row_by_depsynt(lambda x: 1000000 < x.total_duration)
    #     },
    # ]
    # for depsynt_cond in depsynt_duration_conds:
    #     generate_cactus(
    #         filename=f'cactus-{depsynt_cond["name"]}.png',
    #         title=f'Deps > 0, {depsynt_cond["name"]}',
    #         ylog=False,
    #         should_include_row=depsynt_cond['cond']
    #     )
    #     generate_cactus(
    #         filename=f'cactus-ylog-{depsynt_cond["name"]}.png',
    #         title=f'Y Log, Deps > 0, {depsynt_cond["name"]}',
    #         ylog=True,
    #         should_include_row=depsynt_cond['cond']
    #     )


if __name__ == '__main__':
    main()
