#!/usr/bin/env python3

import argparse
from glob import glob
from os import path
from pathlib import Path

parser = argparse.ArgumentParser("Generate slurm tasks for finding dependency and synthesis")
parser.add_argument("--task", type=str, required=True, choices=['find_deps'], help="Type of task to generate")
parser.add_argument("--timeout", type=str, required=True, help="Timeout for each task, for example, 60m")
parser.add_argument("--benchmarks-path", type=str, required=True, help="Path for the benchmarks in text file")
parser.add_argument("--output-path", type=str, required=True, help="Path for put the output files")

if __name__ == '__main__':
    args = parser.parse_args()
    benchmarks_path = args.benchmarks_path

    if not path.exists(benchmarks_path):
        print("Error: benchmarks path does not exist")
        exit(1)

    total_benchmarks = len([f for f in glob(path.join(benchmarks_path, "*.txt"))])
    if total_benchmarks == 0:
        print("Error: no benchmarks found in the path: {}".format(benchmarks_path))
        exit(1)

    if args.task == 'find_deps':
        task_template = Path(path.join(Path(__file__).parent.resolve(), 'find_deps_slurm_template.sh')).read_text()

        variables = {
            'OUTPUT_BASE_PATH': args.output_path,
            'NUM_BENCHMARKS': str(total_benchmarks),
            'TIMEOUT': args.timeout,
            'BENCHMARKS_DIR': benchmarks_path
        }

        for var_name, var_value in variables.items():
            task_template = task_template.replace('{{'+var_name+'}}', var_value)

        print(task_template)
    else:
        print("Unknown task")
        exit(1)
