#!/usr/bin/env python3

import argparse
import csv
import json
import os
import re
from dataclasses import dataclass, field
from glob import glob
from pathlib import Path
from typing import List, Optional


parser = argparse.ArgumentParser("Analyze Find Dependency Results")
parser.add_argument("--result-path", type=str, required=True, help="The path to the results")
parser.add_argument("--benchmarks-path", type=str, required=True, help="The path to the benchmark text files")
parser.add_argument("--summary-output", type=str, required=True, help="The path to store the summary CSV file")
parser.add_argument("--tool", type=str, choices=['find_deps','depsynt','strix'])


@dataclass
class NBABDDSummary:
    avg_bdd_size: Optional[int] = None
    max_bdd_size: Optional[int] = None
    min_bdd_size: Optional[int] = None
    total_bdds: Optional[int] = None
    total_bdds_size_not_repeated: Optional[int] = None
    total_bdds_size_repeated: Optional[int] = None

    def summary(self):
        return {
            'Average BDD Size': self.avg_bdd_size,
            'Max BDD Size': self.max_bdd_size,
            'Min BDD Size': self.min_bdd_size,
            'Total BDDs Instances': self.total_bdds,
            'Total BDDs Size (Not Repeated)': self.total_bdds_size_not_repeated,
            'Total BDDs Size (Repeated)': self.total_bdds_size_repeated,
        }

    @staticmethod
    def load_object(obj):
        summary = NBABDDSummary()
        summary.avg_bdd_size = obj.get('avg_bdd_size', None)
        summary.max_bdd_size = obj.get('max_bdd_size', None)
        summary.min_bdd_size = obj.get('min_bdd_size', None)
        summary.total_bdds = obj.get('total_bdds', None)
        summary.total_bdds_size_not_repeated = obj.get('total_bdds_size_not_repeated', None)
        summary.total_bdds_size_repeated = obj.get('total_bdds_size_repeated', None)
        return summary


@dataclass
class BaseBenchmark:
    benchmark_id: str = ""
    benchmark_name: str = ""
    benchmark_family: str = ""
    ltl_formula: str = ""
    input_vars: str = ""
    output_vars: str = ""

    status: str = "UNKNOWN"    # Success, Timeout, Out-Of-Memory, Irrelevant, Other Error. TODO: make it enum.
    error_message: str = ""
    total_duration: Optional[float] = None

    def load_benchmark_file(self, text_file_path):
        benchmark_id = Path(text_file_path).stem

        with open(text_file_path, 'r') as file:
            self.benchmark_id = file.readline().strip()
            assert(self.benchmark_id == benchmark_id)

            self.benchmark_name = file.readline().strip()
            self.benchmark_family = file.readline().strip()
            self.ltl_formula = file.readline().strip()
            self.input_vars = file.readline().strip()
            self.output_vars = file.readline().strip()

    def summary(self):
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
        }


@dataclass
class StrixBenchmark(BaseBenchmark):
    realizability: str = ""

    def summary(self):
        base_summary = super().summary()

        return {
            **base_summary,

            # Strix Information
            'Realizability': self.realizability,
        }

    def load_output(self, out_content, err_content):
        # Load error
        if 'slurmstepd: error' in err_content and 'oom-kill' in err_content:
            self.status = 'Out-Of-Memory'
            return
        if 'java.lang.OutOfMemoryError' in err_content:
            self.status = 'Out-Of-Memory'
            return
        if 'ltlsynt: Too many acceptance sets used' in err_content:
            self.status = 'Error'
            self.error_message = 'Spot Limited Accepting State'
            return
        if 'ltlsynt: alternate_players(): Odd cycle detected.' in err_content:
            self.status = 'Error'
            self.error_message = 'Spot, Odd Cycle Detected'
            return

        # Load Timeout
        if 'Exited with exit code 124' in err_content:
            self.status = 'Timeout'
            return

        # Load Duration
        match = re.search(r'real\t(\d+)m(\d+\.\d+)s', err_content)
        if match:
            minutes = int(match.group(1))
            seconds = float(match.group(2))
            total_time_in_seconds = minutes * 60 + seconds
            self.total_duration = total_time_in_seconds * 1000

        # Load Realizabilty
        if 'UNREALIZABLE' in out_content:
            self.realizability = 'UNREALIZABLE'
        elif 'REALIZABLE' in out_content:
            self.realizability = 'REALIZABLE'
        else:
            self.realizability = 'UNKNOWN'

        self.status = 'Success'


@dataclass
class FindDepsBenchmark(BaseBenchmark):
    dependent_variables: List[str] = field(default_factory=list)
    independent_variables: List[str] = field(default_factory=list)

    is_automaton_built: bool = False
    automaton_build_duration: Optional[float] = None
    automaton_total_states: Optional[int] = None
    automaton_total_edges: Optional[int] = None

    total_pair_states: Optional[int] = None
    find_pair_states_duration: Optional[float] = None
    find_dependency_duration: Optional[float] = None

    def summary(self):
        total_output_vars = len(self.output_vars.split(','))
        base_summary = super().summary()

        if self.status == "Success":
            total_dependent_vars = len(self.dependent_variables)
            dependency_ratio = total_dependent_vars / total_output_vars if total_output_vars > 0 else 0
        else:
            total_dependent_vars = None
            dependency_ratio = None

        return {
            **base_summary,

            # Dependent Variables
            'Dependent Variables': self.dependent_variables,
            'Total Dependent Variables': total_dependent_vars,
            'Independent Variables': self.independent_variables,
            'Total Independent Variables': len(self.independent_variables),
            'Dependency Ratio': dependency_ratio,

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

    def load_from_dict(self, data):
        self.total_duration = data['total_time']

        # Find Dependent Variables
        dependent_vars, independent_vars = [], []
        for var_description in data["dependency"]['tested_dependencies']:
            if var_description['is_dependent']:
                dependent_vars.append(var_description['name'])
            else:
                independent_vars.append(var_description['name'])

        # Dependent Information
        self.dependent_variables = dependent_vars
        self.independent_variables = independent_vars
        self.total_pair_states = data['dependency'].get('total_pair_state', None)
        self.find_pair_states_duration = data['dependency'].get('search_pair_state_duration', None)
        self.find_dependency_duration = data['dependency'].get('total_duration', None)

        # Automaton Information
        self.is_automaton_built = data['automaton']['is_built']
        self.automaton_build_duration = data['automaton'].get('build_duration', None)
        self.automaton_total_states = data['automaton'].get('total_states', None) if self.is_automaton_built else None
        self.automaton_total_edges = data['automaton'].get('total_edges', None) if self.is_automaton_built else None


@dataclass
class DepSyntBenchmark(FindDepsBenchmark):
    realizability: str = ""
    independents_synthesis_duration: Optional[float] = None
    dependents_synthesis_duration: Optional[float] = None
    merge_strategies_duration: Optional[float] = None
    strategies_synthesis_duration: Optional[float] = None

    projected_bdd_summary: Optional[NBABDDSummary] = None
    origin_bdd_summary: Optional[NBABDDSummary] = None

    def __init__(self):
        super().__init__()

    def load_from_dict(self, data):
        super().load_from_dict(data)

        self.realizability = data['synthesis']['independent_strategy']['realizability']
        self.independents_synthesis_duration = data['synthesis']['independent_strategy']['duration']

        deps_synthesis_duration = data['synthesis']['dependent_strategy']['duration']
        self.dependents_synthesis_duration = deps_synthesis_duration if deps_synthesis_duration != -1 else 0
        self.merge_strategies_duration = data['synthesis'].get('merge_strategies_duration', None)
        self.strategies_synthesis_duration = self.independents_synthesis_duration + self.dependents_synthesis_duration

        # BDD Summary
        origin_bdd_summary = data['bdd_summary'].get('origin_nba', None)
        projected_bdd_summary = data['bdd_summary'].get('projected_nba', None)

        if origin_bdd_summary is not None:
            self.origin_bdd_summary = NBABDDSummary.load_object(origin_bdd_summary)

        if projected_bdd_summary is not None:
            self.projected_bdd_summary = NBABDDSummary.load_object(projected_bdd_summary)

    def summary(self):
        summary = super().summary()
        summary.update({
            # Synthesis Information
            'Realizability': self.realizability,
            'Independent Synthesis Duration': self.independents_synthesis_duration,
            'Dependent Synthesis Duration': self.dependents_synthesis_duration,
            'Merge Strategies Duration': self.merge_strategies_duration,
            'Synthesis Duration': self.strategies_synthesis_duration,
        })

        if self.projected_bdd_summary is not None:
            projected_bdd_summary = self.projected_bdd_summary.summary()
            projected_bdd_summary = {
                "Projected " + key: value
                for key, value in projected_bdd_summary.items()
            }
            summary.update(projected_bdd_summary)

        if self.origin_bdd_summary is not None:
            origin_bdd_summary = self.origin_bdd_summary.summary()
            origin_bdd_summary = {
                "Origin " + key: value
                for key, value in origin_bdd_summary.items()
            }
            summary.update(origin_bdd_summary)

        return summary


def extract_status(out_file, err_file):
    # Case: No output/input variables, therefore, irrelevant
    if 'should follow immediately after the equal sign' in err_file:
        return {
            'status': 'Irrelevant',
        }

    # Case 2: Timeout
    if 'Exited with exit code 124' in err_file:
        return {
            'status': 'Timeout',
        }

    # Case: Out of Memory
    if 'Detected 1 oom-kill event' in err_file:
        return {
            'status': 'Out-Of-Memory',
        }

    # Case: Argument list too long
    if 'Argument list too long' in err_file:
        return {
            'status': 'Error',
            'message': 'Argument List Too Long',
        }

    return {
        'status': 'Success',
    }


def load_find_deps(results_path, text_file_path):
    benchmark_id = Path(text_file_path).stem
    out_path = os.path.join(results_path, benchmark_id + ".out")
    err_path = os.path.join(results_path, benchmark_id + ".err")

    if not os.path.exists(out_path):
        print("Error: Couldn't find out file for benchmark {}".format(benchmark_id))
        exit(1)

    if not os.path.exists(err_path):
        print("Error: Couldn't find err file for benchmark {}".format(benchmark_id))
        exit(1)

    out_file = Path(out_path).read_text()
    err_file = Path(err_path).read_text()

    # Loading Benchmark File
    benchmark = FindDepsBenchmark()
    benchmark.load_benchmark_file(text_file_path)

    # Extracting status
    extracted_status = extract_status(out_file, err_file)
    benchmark.status = extracted_status['status']
    if benchmark.status != 'Success':
        benchmark.error_message = extracted_status.get('message', None)
        return benchmark

    # Load JSON Measure
    benchmark_json = json.loads(out_file)
    benchmark.load_from_dict(benchmark_json)

    return benchmark


def get_second_last_line(text):
    lines = text.split('\n')
    if len(lines) < 2:
        return None  # Return None or some default value if there aren't at least two lines.
    return lines[-2]  # Negative indexing gets lines from the end. -1 would be the last line, -2 is the second to last line.


def load_strix(results_path, text_file_path):
    benchmark_id = Path(text_file_path).stem
    out_path = os.path.join(results_path, benchmark_id + ".out")
    err_path = os.path.join(results_path, benchmark_id + ".err")

    if not os.path.exists(out_path):
        print("Error: Couldn't find out file for benchmark {}".format(benchmark_id))
        return

    if not os.path.exists(err_path):
        print("Error: Couldn't find err file for benchmark {}".format(benchmark_id))
        return

    out_file = Path(out_path).read_text()
    err_file = Path(err_path).read_text()

    # Loading Benchmark File
    benchmark = StrixBenchmark()
    benchmark.load_benchmark_file(text_file_path)
    benchmark.load_output(out_file, err_file)

    return benchmark


def load_depsynt(results_path, text_file_path):
    benchmark_id = Path(text_file_path).stem
    out_path = os.path.join(results_path, benchmark_id + ".out")
    err_path = os.path.join(results_path, benchmark_id + ".err")

    if not os.path.exists(out_path):
        print("Error: Couldn't find out file for benchmark {}".format(benchmark_id))
        return

    if not os.path.exists(err_path):
        print("Error: Couldn't find err file for benchmark {}".format(benchmark_id))
        return

    out_file = Path(out_path).read_text()
    err_file = Path(err_path).read_text()

    # Loading Benchmark File
    benchmark = DepSyntBenchmark()
    benchmark.load_benchmark_file(text_file_path)

    # Extracting status
    extracted_status = extract_status(out_file, err_file)
    benchmark.status = extracted_status['status']
    if benchmark.status != 'Success':
        benchmark.error_message = extracted_status.get('message', None)
        return benchmark

    # Load JSON Measure
    benchmark_json = json.loads(get_second_last_line(out_file))   # Last line is empty
    benchmark.load_from_dict(benchmark_json)

    return benchmark


def should_benchmark_be_included(summary: BaseBenchmark):
    return True


def list_benchmarks(results_path, benchmarks_path, target_tool):
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

    listed_benchmarks = []
    for benchmark_path in benchmarks:
        if target_tool == 'find_deps':
            benchmark = load_find_deps(results_path, benchmark_path)
        elif target_tool == 'depsynt':
            benchmark = load_depsynt(results_path, benchmark_path)
        elif target_tool == 'strix':
            benchmark = load_strix(results_path, benchmark_path)
        else:
            print("Error: unknown tool")
            exit(1)

        if benchmark is not None and should_benchmark_be_included(benchmark):
            listed_benchmarks.append(benchmark)

    return listed_benchmarks


def main():
    args = parser.parse_args()
    results_path = args.result_path
    benchmarks_path = args.benchmarks_path
    target_tool = args.tool

    listed_benchmarks = list_benchmarks(results_path, benchmarks_path, target_tool)

    # Write summary to CSV
    summary = [
        x.summary()
        for x in listed_benchmarks
    ]
    with open(args.summary_output, 'w+', newline='') as output_file:
        keys = summary[0].keys()
        dict_writer = csv.DictWriter(output_file, keys)
        dict_writer.writeheader()
        dict_writer.writerows(summary)


if __name__ == "__main__":
    main()
