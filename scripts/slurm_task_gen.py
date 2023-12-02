#!/usr/bin/env python3

import argparse
from glob import glob
from os import path
from pathlib import Path

parser = argparse.ArgumentParser("Generate slurm tasks for finding dependency and synthesis")
parser.add_argument("--task", type=str, required=True, choices=['find_deps', 'find_deps_formula', 'depsynt', 'depsynt_measured', 'spotmodular', 'strix'], help="Type of task to generate")
parser.add_argument("--timeout", type=str, required=True, help="Timeout for each task, for example, 60m")
parser.add_argument("--benchmarks-path", type=str, required=True, help="Path for the benchmarks in text file")
parser.add_argument("--output-path", type=str, required=True, help="Path for put the output files")
parser.add_argument("--families", type=str, help="List of benchmarks families to run, separated by comma, for example, mux,adder")


def generate_depsynt(args, measured):
    benchmarks_path = args.benchmarks_path
    total_benchmarks = len([f for f in glob(path.join(benchmarks_path, "*.txt"))])
    task_template = Path(path.join(Path(__file__).parent.resolve(), 'depsynt_slurm_template.sh')).read_text()
    families = args.families.split(',')
    if len(families) == 0:
        print("Error: no families provided")
        exit(1)

    variables = {
        'OUTPUT_BASE_PATH': args.output_path,
        'NUM_BENCHMARKS': str(total_benchmarks),
        'TIMEOUT': args.timeout,
        'BENCHMARKS_DIR': benchmarks_path,
        'ALLOWED_FAMILIES': " ".join(["\""+f+"\"" for f in families]),
        'FIND_DEP_TIMEOUT': '12000',
        'MEASURES': 'True' if measured else ''
    }

    for var_name, var_value in variables.items():
        task_template = task_template.replace('{{'+var_name+'}}', var_value)

    print(task_template)


def generate_strix(args):
    benchmarks_path = args.benchmarks_path
    total_benchmarks = len([f for f in glob(path.join(benchmarks_path, "*.txt"))])
    task_template = Path(path.join(Path(__file__).parent.resolve(), 'strix_slurm_template.sh')).read_text()
    families = args.families.split(',')
    if len(families) == 0:
        print("Error: no families provided")
        exit(1)

    variables = {
        "OUTPUT_BASE_PATH": args.output_path,
        'NUM_BENCHMARKS': str(total_benchmarks),
        'TIMEOUT': args.timeout,
        'BENCHMARKS_DIR': benchmarks_path,
        'ALLOWED_FAMILIES': " ".join(["\""+f+"\"" for f in families]),
    }

    for var_name, var_value in variables.items():
        task_template = task_template.replace('{{'+var_name+'}}', var_value)

    print(task_template)


def generate_spotmodular(args):
    benchmarks_path = args.benchmarks_path
    total_benchmarks = len([f for f in glob(path.join(benchmarks_path, "*.txt"))])
    task_template = Path(path.join(Path(__file__).parent.resolve(), 'depsynt_slurm_template.sh')).read_text()
    families = args.families.split(',')
    if len(families) == 0:
        print("Error: no families provided")
        exit(1)

    variables = {
        'OUTPUT_BASE_PATH': args.output_path,
        'NUM_BENCHMARKS': str(total_benchmarks),
        'TIMEOUT': args.timeout,
        'BENCHMARKS_DIR': benchmarks_path,
        'ALLOWED_FAMILIES': " ".join(["\""+f+"\"" for f in families]),
        'FIND_DEP_TIMEOUT': "0",
    }

    for var_name, var_value in variables.items():
        task_template = task_template.replace('{{'+var_name+'}}', var_value)

    print(task_template)


def generate_find_deps(args, approach):
    if apprach not in ['formula', 'automaton']:
        print("Error: unknown approach, must be: formula, automaton")
        exit(1)
    
    benchmarks_path = args.benchmarks_path
    total_benchmarks = len([f for f in glob(path.join(benchmarks_path, "*.txt"))])
    task_template = Path(path.join(Path(__file__).parent.resolve(), 'find_deps_slurm_template.sh')).read_text()

    variables = {
        'OUTPUT_BASE_PATH': args.output_path,
        'NUM_BENCHMARKS': str(total_benchmarks),
        'TIMEOUT': args.timeout,
        'BENCHMARKS_DIR': benchmarks_path,
        'ALGORITHM': approach,
    }

    for var_name, var_value in variables.items():
        task_template = task_template.replace('{{'+var_name+'}}', var_value)

    print(task_template)


def main():
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
        generate_find_deps(args, 'automaton')
    elif args.task == 'find_deps_formula':
        generate_find_deps(args, 'formula')
    elif args.task == 'depsynt':
        generate_depsynt(args, measured=False)
    elif args.task == 'depsynt_measured':
        generate_depsynt(args, measured=True)
    elif args.task == 'strix':
        generate_strix(args)
    elif args.task == 'spotmodular':
        generate_spotmodular(args)
    else:
        print("Unknown task")
        exit(1)


if __name__ == '__main__':
    main()
