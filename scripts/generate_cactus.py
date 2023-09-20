import pathlib

from .results_summarizer import load_find_deps
import csv
import json
import os.path
import shutil

TOOLS = [
    'strix',
    'depsynt',
    'spot-acd',
    'spot-ds',
    'spot-lar',
    'spot-sd',
]


def should_include_row(row):
    benchmark_id = row['Benchmark Id']
    find_deps_results = os.path.join(pathlib.Path(__file__).parent.resolve(), "../tasks_output/find_deps")
    benchmark_file = os.path.join(pathlib.Path(__file__).parent.resolve(), "../tasks_output/generated_benchmarks/text/"+benchmark_id+".txt")
    find_deps_summary = load_find_deps(find_deps_results, benchmark_file)

    total_dependent_vars = len(find_deps_summary.dependent_variables)
    return total_dependent_vars > 0


def load_stats_from_csv(tool_csv_file):
    stats = {}

    with open(tool_csv_file, 'r') as fp:
        reader = csv.DictReader(fp)
        rows = list(reader)
        for row in rows:
            if not should_include_row(row):
                print("Skipping row: " + row['Benchmark Id'])
                continue
            success_ = row['Status'] == 'Success'
            stats[row['Benchmark Name']] = {
                'status': success_,
            }
            if success_:
                stats[row['Benchmark Name']]['rtime'] = float(row['Total Duration']) / 1000.0   #  In seconds

    return stats


def get_filename(filepath):
    return os.path.basename(filepath).split('.')[0]


def get_current_dir():
    return os.path.dirname(os.path.realpath(__file__))


def load_tool_json(tool_csv_file):
    stats = load_stats_from_csv(tool_csv_file)
    tool_name = get_filename(tool_csv_file)
    return {
        'preamble': {
            'program': tool_name,
            'prog_alias': tool_name,
            'benchmark': 'SYNTCOMP',
        },
        'stats': stats,
    }


def create_cactus_file():
    """
    TODO: check that mkplot is installed
    command :  python ./scripts/mkplot/mkplot.py -b png --save-to cactus.png ./tasks_output/tmp_cactus/*.json
    :return:
    """
    os.system("python ./scripts/mkplot/mkplot.py --ylabel=\"Time (ms)\" -t 10800 --ylog -b png --save-to cactus.png ./tasks_output/tmp_cactus/*.json")


# TODO: ignore instances that have 0 dependent variables
def main():
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
        tool_json = load_tool_json(tool_path)
        tool_name = get_filename(tool_path)
        tool_json_file = os.path.join(tmp_dir, tool_name + '.json')
        with open(tool_json_file, 'w') as fp:
            json.dump(tool_json, fp, indent=4)

    create_cactus_file()

    shutil.rmtree(tmp_dir)


if __name__ == '__main__':
    main()
