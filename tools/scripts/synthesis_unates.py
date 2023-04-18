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


def process_benchmark(benchmark, timeout, output_dir, synthesis_unates_tool, skip_unates):
    benchmark_name = benchmark['benchmark_name']
    input_vars = benchmark['input_vars']
    output_vars = benchmark['output_vars']
    ltl_formula = benchmark['ltl_formula']

    print("Processing {}...".format(benchmark_name))

    config = {
        'process_timeout': timeout,
        'synthesis_unates_cli_path': synthesis_unates_tool,
        'formula': ltl_formula,
        'inputs': input_vars,
        'outputs': output_vars
    }
    cli_cmd = 'time timeout --signal=HUP {process_timeout} {synthesis_unates_cli_path} --formula="{formula}" --input="{inputs}" --output="{outputs}"'.format(
        **config)
    if skip_unates:
        cli_cmd += ' --skip-unates'

    with Popen(cli_cmd, stdout=PIPE, stderr=PIPE, shell=True, preexec_fn=os.setsid) as process:
        process_communicate = process.communicate()
        cli_stdout = process_communicate[0].decode("utf-8")
        cli_stderr = process_communicate[1].decode("utf-8")
        result = cli_stdout + cli_stderr

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
        '--synthesis_unates_tool', help="Path to synthesis Unates tool", type=str, required=True)
    parser.add_argument(
        '--timeout', help="Timeout of each benchmark", type=str, default='40m')
    parser.add_argument(
        '--workers', help="Number of workers", type=int, default=16)
    parser.add_argument('--skip_unates', help="Skip unates in synthesis process", default=False, action='store_true')

    args = parser.parse_args()

    workers = args.workers
    benchmark_name_filter = args.name
    benchmarks_path = args.benchs_list
    benchmarks_timeout = args.timeout
    output_dir = args.output_dir
    synthesis_unates_tool = args.synthesis_unates_tool
    skip_unates = args.skip_unates

    """
    Search for benchmarks by configuration
    """
    create_folder(output_dir)
    benchmarks = get_all_benchmarks(
        output_dir=output_dir,
        ignore_if_output_exists=False,
        benchmark_name_filter=benchmark_name_filter,
        benchmarks_path=benchmarks_path
    )
    print("Found {} benchmarks.".format(len(benchmarks)))
    shuffle(benchmarks)

    """
    Apply the algorithm
    """
    process_benchmark_args = [
        (benchmark, benchmarks_timeout, output_dir, synthesis_unates_tool, skip_unates)
        for benchmark in benchmarks
    ]
    with Pool(workers) as pool:
        pool.starmap(process_benchmark, process_benchmark_args)


if __name__ == "__main__":
    main()
