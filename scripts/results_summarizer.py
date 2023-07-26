#!/usr/bin/env python3

import argparse
import csv
import json
import os
from dataclasses import dataclass, field, asdict
from glob import glob
from pathlib import Path
from typing import List, Optional

parser = argparse.ArgumentParser("Analyze Find Dependency Results")
parser.add_argument("--result-path", type=str, required=True, help="The path to the find deps results")
parser.add_argument("--benchmarks-path", type=str, required=True, help="The path to the benchmark text files")
parser.add_argument("--summary-output", type=str, required=True, help="The path to store the summary CSV file")


@dataclass
class Benchmark:
    benchmark_id: str = ""
    benchmark_name: str = ""
    benchmark_family: str = ""
    ltl_formula: str = ""
    input_vars: str = ""
    output_vars: str = ""

    status: str = "UNKNOWN"    # Success, Timeout, Out-Of-Memory, Irrelevant, Other Error. TODO: make it enum.
    error_message: str = ""
    total_duration: float = -1

    dependent_variables: List[str] = field(default_factory=list)
    independent_variables: List[str] = field(default_factory=list)

    is_automaton_built: bool = False
    automaton_build_duration: float = -1.0
    automaton_total_states: Optional[int] = None
    automaton_total_edges: Optional[int] = None

    total_pair_states: Optional[int] = None
    find_pair_states_duration: Optional[float] = None
    find_dependency_duration: Optional[float] = None

    def summary(self):
        total_dependent_vars = len(self.dependent_variables)
        total_output_vars = len(self.output_vars.split(','))
        return {
            # Benchmark Metadata
            'Benchmark Id': self.benchmark_id,
            'Benchmark Name': self.benchmark_name,
            'Benchmark Family': self.benchmark_family,
            'LTL Formula': self.ltl_formula,
            'Input Variables': self.input_vars,
            'Output Variables': self.output_vars,
            'Total Input Variables': len(self.input_vars.split(',')),
            'Total Output Variables': total_output_vars,

            # Find Deps Information
            'Status': self.status,
            'Error Message': self.error_message,
            'Total Duration': self.total_duration,

            # Dependent Variables
            'Dependent Variables': self.dependent_variables,
            'Total Dependent Variables': total_dependent_vars,
            'Independent Variables': self.independent_variables,
            'Total Independent Variables': len(self.independent_variables),
            'Dependency Ratio': total_dependent_vars / total_output_vars if total_output_vars > 0 else 0,

            # Automaton Information
            'Is Automaton Built': self.is_automaton_built,
            'Automaton Build Duration': self.automaton_build_duration,
            'Automaton Total States': self.automaton_total_states,
            'Automaton Total Edges': self.automaton_total_edges,

            # Find Dependency Process
            'Total Pair States': self.total_pair_states,
            'Find Pair States Duration': self.find_pair_states_duration,
            'Find Dependency Duration': self.find_dependency_duration,
        }


def load_benchmark(results_path, text_file_path) -> Benchmark:
    benchmark = Benchmark()
    benchmark_id = Path(text_file_path).stem

    find_deps_out_path = os.path.join(results_path, benchmark_id + ".out")
    find_deps_err_path = os.path.join(results_path, benchmark_id + ".err")

    if not os.path.exists(find_deps_out_path):
        print("Error: find deps out not found for benchmark {}".format(benchmark_id))
        exit(1)

    if not os.path.exists(find_deps_err_path):
        print("Error: find deps err not found for benchmark {}".format(benchmark_id))
        exit(1)

    # Read text file
    with open(text_file_path, 'r') as file:
        benchmark.benchmark_id = file.readline().strip()
        assert(benchmark.benchmark_id == benchmark_id)

        benchmark.benchmark_name = file.readline().strip()
        benchmark.benchmark_family = file.readline().strip()
        benchmark.ltl_formula = file.readline().strip()
        benchmark.input_vars = file.readline().strip()
        benchmark.output_vars = file.readline().strip()

    # Read find deps output file
    find_deps_out = Path(find_deps_out_path).read_text()
    find_deps_err = Path(find_deps_err_path).read_text()

    # Case: No output/input variables, therefore, irrelevant
    if 'should follow immediately after the equal sign' in find_deps_err:
        benchmark.status = 'Irrelevant'
        return benchmark

    # Case: Argument list too long
    if 'Argument list too long' in find_deps_err:
        benchmark.status = 'Error'
        benchmark.error_message = 'Argument list too long'
        return benchmark

    # Case: Out of Memory
    if 'Detected 1 oom-kill event' in find_deps_err:
        benchmark.status = 'Out-Of-Memory'
        return benchmark

    benchmark_json = json.loads(find_deps_out)
    benchmark.total_duration = benchmark_json['total_time']

    # Find Dependent Variables
    dependent_vars, independent_vars = [], []
    for var_description in benchmark_json["dependency"]['tested_dependencies']:
        if var_description['is_dependent']:
            dependent_vars.append(var_description['name'])
        else:
            independent_vars.append(var_description['name'])

    # Dependent Information
    benchmark.dependent_variables = dependent_vars
    benchmark.independent_variables = independent_vars
    benchmark.total_pair_states = benchmark_json['dependency'].get('total_pair_state', None)
    benchmark.find_pair_states_duration = benchmark_json['dependency'].get('search_pair_state_duration', None)
    benchmark.find_dependency_duration = benchmark_json['dependency'].get('total_duration', None)

    # Automaton Information
    benchmark.is_automaton_built = benchmark_json['automaton']['is_built']
    benchmark.automaton_build_duration = benchmark_json['automaton'].get('build_duration', None)
    benchmark.automaton_total_states = benchmark_json['automaton'].get('total_states', None) if benchmark.is_automaton_built else None
    benchmark.automaton_total_edges = benchmark_json['automaton'].get('total_edges', None) if benchmark.is_automaton_built else None

    # Case 2: Timeout
    if 'Exited with exit code 124' in find_deps_err:
        benchmark.status = 'Timeout'
        return benchmark

    # Case 3: Success
    benchmark.status = 'Success'
    return benchmark


if __name__ == "__main__":
    args = parser.parse_args()
    results_path = args.result_path
    benchmarks_path = args.benchmarks_path

    if not os.path.exists(results_path):
        print("Error: find deps path does not exist")
        exit(1)

    if not os.path.exists(benchmarks_path):
        print("Error: benchmarks path does not exist")
        exit(1)

    # List all benchmarks
    benchmarks = glob(f"{benchmarks_path}/*.txt")
    if len(benchmarks) == 0:
        print("Error: no benchmarks found in the path: {}".format(benchmarks_path))
        exit(1)
    print("Found {} benchmarks".format(len(benchmarks)))

    summary = []
    for benchmark_path in benchmarks:
        benchmark = load_benchmark(results_path, benchmark_path)
        summary.append(benchmark.summary())

    # Write summary to CSV
    with open(args.summary_output, 'w+', newline='') as output_file:
        keys = summary[0].keys()
        dict_writer = csv.DictWriter(output_file, keys)
        dict_writer.writeheader()
        dict_writer.writerows(summary)

