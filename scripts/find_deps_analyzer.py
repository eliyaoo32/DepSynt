#!/usr/bin/env python3

import argparse
import json
import os
from dataclasses import dataclass, field
from glob import glob
from pathlib import Path
from typing import List, Optional

parser = argparse.ArgumentParser("Analyze Find Dependency Results")
parser.add_argument("--find-deps-result-path", type=str, required=True, help="The path to the find deps results")
parser.add_argument("--benchmarks-path", type=str, required=True, help="The path to the benchmark text files")


@dataclass
class Benchmark:
    benchmark_id: str = ""
    benchmark_name: str = ""
    benchmark_family: str = ""
    ltl_formula: str = ""
    input_vars: str = ""
    output_vars: str = ""

    status: str = "UNKNOWN"    # Success, Timeout, Out-Of-Memory, Other Error. TODO: make it enum.
    total_duration: float = -1

    dependent_variables: List[str] = field(default_factory=list)
    independent_variables: List[str] = field(default_factory=list)

    is_automaton_built: bool = False
    automaton_build_duration: float = -1.0
    automaton_total_states: Optional[int] = None
    automaton_total_edges: Optional[int] = None

    total_pair_states: Optional[int] = None
    find_pair_states_duration: Optional[float] = None


def load_benchmark(find_deps_path, text_file_path):
    benchmark = Benchmark()
    benchmark_id = Path(text_file_path).stem

    find_deps_out_path = os.path.join(find_deps_path, benchmark_id + ".out")
    find_deps_err_path = os.path.join(find_deps_path, benchmark_id + ".err")

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

    # Case 1: Out of Memory
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
    benchmark.total_pair_states = benchmark_json['dependency'].get('total_pair_state',None)
    benchmark.find_pair_states_duration = benchmark_json['dependency'].get('search_pair_state_duration',None)

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
    find_deps_path = args.find_deps_result_path
    benchmarks_path = args.benchmarks_path

    if not os.path.exists(find_deps_path):
        print("Error: find deps path does not exist")
        exit(1)

    if not os.path.exists(benchmarks_path):
        print("Error: benchmarks path does not exist")
        exit(1)

    # List all benchmarks
    benchmarks = glob(f"{benchmarks_path}/*.txt")
    print("Found {} benchmarks".format(len(benchmarks)))

    for benchmark_path in benchmarks:
        print(load_benchmark(find_deps_path, benchmark_path))
        break
