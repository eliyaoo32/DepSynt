from subprocess import PIPE, Popen
from multiprocessing import Pool
import os
import csv
from random import shuffle

# Applying the files relative to the current file's path
file_path = os.path.dirname(__file__)
if file_path != "":
    os.chdir(file_path)

BENCHMARK_OUTPUT_FILE_FORMAT = '{}.json'


def get_benchmark_output_path(benchmark_name, output_dir):
    return os.path.join(output_dir, BENCHMARK_OUTPUT_FILE_FORMAT.format(benchmark_name))


def is_benchmark_output_exists(benchmark_name, output_dir):
    return os.path.exists(get_benchmark_output_path(benchmark_name, output_dir))


def get_all_benchmarks(output_dir, benchmarks_path, ignore_if_output_exists=True, benchmark_name_filter=''):
    def is_benchmark_valid(benchmark_name):
        if ignore_if_output_exists and is_benchmark_output_exists(benchmark_name, output_dir):
            return False
        if benchmark_name_filter not in benchmark_name:
            return False

        return True

    with open(benchmarks_path, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        benchmarks = [
            {
                'benchmark_name': row['benchmark_name'],
                'input_vars': row['input_vars'],
                'output_vars': row['output_vars'],
                'ltl_formula': row['ltl_formula']
            }
            for row in reader
            if is_benchmark_valid(row['benchmark_name'])
        ]

    return benchmarks


def process_benchmark(benchmark, timeout, output_dir, find_deps_tool, algorithm, dependents_type):
    benchmark_name = benchmark['benchmark_name']
    input_vars = benchmark['input_vars']
    output_vars = benchmark['output_vars']
    ltl_formula = benchmark['ltl_formula']

    print("Processing {}...".format(benchmark_name))

    config = {
        'process_timeout': timeout,
        'find_deps_cli_path': find_deps_tool,
        'formula': ltl_formula,
        'inputs': input_vars,
        'outputs': output_vars,
        'algorithm': algorithm
    }
    find_deps_cli = 'timeout --signal=HUP {process_timeout} {find_deps_cli_path} --formula="{formula}" --input="{inputs}" --output="{outputs}" --algo={algorithm}'.format(
        **config)

    if dependents_type == 'input':
        find_deps_cli += ' --find-input-only'

    with Popen(find_deps_cli, stdout=PIPE, stderr=PIPE, shell=True, preexec_fn=os.setsid) as process:
        result = process.communicate()[0].decode("utf-8")

    print("Done Processing {}!".format(benchmark_name))
    with open(get_benchmark_output_path(benchmark_name, output_dir), "w+") as outfile:
        outfile.write(result)


def create_folder(folder_name):
    if not os.path.exists(folder_name):
        os.makedirs(folder_name)


def main():
    """
    Parse CLI arguments and run the tool.
    """
    from argparse import ArgumentParser
    parser = ArgumentParser(add_help=True)
    parser.add_argument(
        '--name', help="Filter by benchmarks name", type=str, default='')
    parser.add_argument(
        '--benchs_list', help="A path to csv file which store benchmark instances", type=str, required=True)
    parser.add_argument(
        '--output_dir', help="Path to the directory which the output files are stored into", type=str, required=True)
    parser.add_argument(
        '--find_deps_tool', help="Path to the find dependency tool", type=str, required=True)
    parser.add_argument(
        '--timeout', help="Timeout of each benchmark", type=str, default='40m')
    parser.add_argument('--all', help="Apply altough output file already exists ",
                        default=False, action='store_true')
    parser.add_argument(
        '--workers', help="Number of workers", type=int, default=16)
    parser.add_argument('--algorithm', help="Algorithm to use",
                        type=str, choices=['automaton', 'formula'], required=True)
    parser.add_argument('--dependents_type', help="Which type of dependents variable to search for (input or output)",
                        type=str, choices=['input', 'output'], default='output')
    args = parser.parse_args()

    algorithm = args.algorithm
    workers = args.workers
    benchmark_name_filter = args.name
    ignore_existing_output = not args.all
    benchmarks_path = args.benchs_list
    benchmarks_timeout = args.timeout
    output_dir = args.output_dir
    find_deps_tool = args.find_deps_tool
    dependents_type = args.dependents_type

    """
    Search for benchmarks by configuration
    """
    create_folder(output_dir)
    benchmarks = get_all_benchmarks(
        output_dir=output_dir,
        ignore_if_output_exists=ignore_existing_output,
        benchmark_name_filter=benchmark_name_filter,
        benchmarks_path=benchmarks_path
    )
    print("Found {} benchmarks.".format(len(benchmarks)))
    shuffle(benchmarks)

    """
    Apply the algorithm
    """
    process_benchmark_args = [
        (benchmark, benchmarks_timeout, output_dir, find_deps_tool, algorithm, dependents_type)
        for benchmark in benchmarks
    ]
    with Pool(workers) as pool:
        pool.starmap(process_benchmark, process_benchmark_args)


if __name__ == "__main__":
    main()
