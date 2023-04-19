import os
from subprocess import PIPE, Popen
from multiprocessing import Pool
from datetime import datetime
from random import shuffle

from lookup_dependencies import create_folder, get_all_benchmarks

BENCHMARK_OUTPUT_FILE_FORMAT = '{}.hoa'
BENCHMARK_MEASURES_FILE_FORMAT = '{}.json'
TOOLS = ['ltlsynt-sd', 'ltlsynt-ds',
         'ltlsynt-lar.old', 'ltlsynt-lar', 'ltlsynt-ps', 'bfss-synt', 'bfss-synt-eject-deps', 'bfss-synt-skip-deps']


def get_benchmark_output_path(tool_name, benchmark_name, output_dir):
    return os.path.join(output_dir, BENCHMARK_OUTPUT_FILE_FORMAT.format(tool_name + " - " + benchmark_name))


def get_benchmark_measures_path(tool_name, benchmark_name, output_dir):
    return os.path.join(output_dir, BENCHMARK_MEASURES_FILE_FORMAT.format(tool_name + " - " + benchmark_name))


def process_benchmark(benchmark, timeout, output_dir, synt_tool, tool_path, decompose, model_checking):
    benchmark_name = benchmark['benchmark_name']
    input_vars = benchmark['input_vars']
    output_vars = benchmark['output_vars']
    ltl_formula = benchmark['ltl_formula']

    print("Processing {}...".format(benchmark_name))

    # Find the cli command of the tool
    if 'ltlsynt' in synt_tool:
        _, algorithm = synt_tool.split('-')
        cli_cmd = 'time timeout --signal=HUP {timeout} ltlsynt --verbose --formula="{formula}" --ins="{inputs}" --outs="{outputs}" --algo={algo}'.format(
            timeout=timeout, formula=ltl_formula, inputs=input_vars, outputs=output_vars, algo=algorithm)
        if not decompose:
            cli_cmd += " --decompose=no"
        cli_cmd += " --aiger"   # Output in the aiger fromat
        if model_checking:
            cli_cmd += " --verify"
    elif 'bfss-synt' in synt_tool:
        algorithm = synt_tool

        cli_cmd = 'time timeout --signal=HUP {timeout} {tool_path} --formula="{formula}" --input="{inputs}" --output="{outputs}" --algo=automaton --measures-path="{measures_path}" --model-name={model_name}'.format(
            timeout=timeout, formula=ltl_formula, inputs=input_vars, outputs=output_vars, model_name=benchmark_name,
            tool_path=tool_path if tool_path != '' else 'bfss-synt',
            measures_path=get_benchmark_measures_path(algorithm, benchmark_name, output_dir)
        )
        if 'skip-deps' in synt_tool:
            cli_cmd += ' --skip-eject-deps --skip-synt-deps'
        elif 'eject-deps' in synt_tool:
            cli_cmd += ' --skip-synt-deps'
        if decompose:
            cli_cmd += ' --decompose'
        if model_checking:
            cli_cmd += " --model-checking"
    else:
        raise Exception("Unknown tool {}".format(synt_tool))

    # Apply the CLI command
    start_time = datetime.now()
    with Popen(cli_cmd, stdout=PIPE, stderr=PIPE, shell=True, preexec_fn=os.setsid) as process:
        process_communicate = process.communicate()
        cli_stdout = process_communicate[0].decode("utf-8")
        cli_stderr = process_communicate[1].decode("utf-8")
        result = cli_stdout + cli_stderr

    print("Done Processing {}!".format(benchmark_name))
    with open(get_benchmark_output_path(algorithm, benchmark_name, output_dir), "w+") as outfile:
        outfile.write(result)


def main():
    from argparse import ArgumentParser

    parser = ArgumentParser(add_help=True)
    parser.add_argument(
        '--name', help="Filter by benchmarks name", type=str, default='')
    parser.add_argument(
        '--benchs_list', help="A path to csv file which store benchmark instances", type=str, required=True)
    parser.add_argument(
        '--output_dir', help="Path to the directory which the output files are stored into", type=str, required=True)
    parser.add_argument(
        '--timeout', help="Timeout of each benchmark", type=str, default='40m')
    parser.add_argument('--all', help="Apply altough output file already exists ",
                        default=False, action='store_true')
    parser.add_argument(
        '--workers', help="Number of workers", type=int, default=16)
    parser.add_argument('--tool', help="Which ltl synt tool to use",
                        type=str, choices=TOOLS, required=True)
    parser.add_argument('--tool_path', help="A path to executable file of the tool",
                        type=str, required=False, default='')
    parser.add_argument('--decompose', help="Apply the synthesis tool with decompose option",
                        default=False, action='store_true')
    parser.add_argument('--model_checking', help="Apply model checking on the tool", default=False, action='store_true')
    args = parser.parse_args()

    workers = args.workers
    benchmark_name_filter = args.name
    ignore_existing_output = not args.all
    benchmarks_path = args.benchs_list
    benchmarks_timeout = args.timeout
    output_dir = args.output_dir
    model_checking = args.model_checking
    tool_path = args.tool_path
    synt_tool = args.tool
    decompose = args.decompose

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

    """
    Apply the algorithm
    """
    process_benchmark_args = [
        (benchmark, benchmarks_timeout, output_dir, synt_tool, tool_path, decompose, model_checking)
        for benchmark in benchmarks
    ]
    with Pool(workers) as pool:
        pool.starmap(process_benchmark, process_benchmark_args)


if __name__ == "__main__":
    main()
