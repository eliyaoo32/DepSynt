import os
import subprocess
import time
from dataclasses import dataclass
import argparse
import csv
from io import StringIO
from typing import List

# Initialize parser
parser = argparse.ArgumentParser(description="Run benchmarks with specific timeouts")

# Adding arguments
parser.add_argument("--benchmarks", type=str, required=True, help="List of benchmarks separated by comma")
parser.add_argument("--timeout", type=int, required=True,  help="Timeout for each benchmark in milliseconds")
parser.add_argument("--file", type=str, help="File to write the results to (CSV format)")

DEPENDENCY_TIMEOUT = 10000


@dataclass
class ToolResult:
    tool_name: str
    duration: float
    model_name: str
    status: str


@dataclass
class Benchmark:
    ltl_formula: str
    model_name: str
    input_vars: str
    output_vars: str

def get_tool_status(tool_output: str) -> str:
    if 'slurmstepd: error' in tool_output and 'oom-kill' in tool_output:
        return 'Out-Of-Memory'
    if 'java.lang.OutOfMemoryError' in tool_output:
        return 'Out-Of-Memory'
    if 'ltlsynt: Too many acceptance sets used' in tool_output:
        return 'Error' # Error message =  'Spot Limited Accepting State'
    if 'ltlsynt: alternate_players(): Odd cycle detected.' in tool_output:
        return 'Error' # Error message = 'Spot, Odd Cycle Detected'
    if 'Exited with exit code 124' in tool_output:
        return 'Timeout'
    if 'should follow immediately after the equal sign' in tool_output:
        return 'Error'
    if 'Detected 1 oom-kill event' in tool_output:
        return 'Out-Of-Memory'
    if 'Argument list too long' in tool_output:
        return 'Error' # Error message = 'Argument List Too Long'

    return 'Success'

"""
Run benchmarks for the following tools:
    - depsynt
    - ltlsynt
    - strix
Timeout is given in seconds.
"""
def run_tool(tool_name: str, model_name: str, input_vars: str, output_vars: str, formula: str, timeout: int) -> ToolResult:
    if tool_name == "depsynt":
        command = (f'./depsynt --model-name="{model_name}" --input="{input_vars}" --output="{output_vars}" '
                   f'--dependency-timeout={DEPENDENCY_TIMEOUT} --formula="{formula}"')
    elif tool_name == "ltlsynt":
        command = f'ltlsynt --aiger --outs="{output_vars}" --ins="{input_vars}" --formula="{formula}"'
    elif tool_name == "strix":
        command = f'./strix --ins="{input_vars}" --outs="{output_vars}" -o aag -f "{formula}"'
    else:
        raise ValueError(f"Unknown tool name: {tool_name}")

    command_with_timeout = f'timeout {timeout}s {command}'
    start_time = time.time()
    result = subprocess.run(command_with_timeout, shell=True, text=True, capture_output=True)
    status = get_tool_status(result.stdout + result.stderr)
    duration = time.time() - start_time
    return ToolResult(
        tool_name=tool_name,
        duration=duration,
        model_name=model_name,
        status=status
    )


def get_benchmark(benchmark_name: str) -> Benchmark:
    benchmark_file = get_benchmark_path(benchmark_name)
    if not is_benchmark_exists(benchmark_name):
        raise ValueError(f"Benchmark file {benchmark_file} does not exist")

    with open(benchmark_file, 'r') as f:
        lines = f.readlines()
        model_name = lines[0].replace("Name:", "").strip()
        ltl_formula = lines[2].replace("Formula:", "").strip()
        input_vars = lines[3].replace("Input:", "").strip()
        output_vars = lines[4].replace("Output:", "").strip()
        return Benchmark(
            ltl_formula=ltl_formula,
            model_name=model_name,
            input_vars=input_vars,
            output_vars=output_vars
        )


def get_benchmark_path(benchmark_name: str) -> str:
    return f'./scripts/benchmarks-ltl/{benchmark_name}.txt'


def is_benchmark_exists(benchmark_name: str) -> bool:
    benchmark_file = get_benchmark_path(benchmark_name)
    return os.path.exists(benchmark_file)


def print_results(results: List[ToolResult], file: str | None = None):
    # Prepare a dictionary to hold the combined results
    combined = {}
    for result in results:
        if result.model_name not in combined:
            combined[result.model_name] = {
                "benchmark": result.model_name,
                "depsynt_status": "",
                "depsynt_duration": "",
                "strix_status": "",
                "strix_duration": "",
                "ltlsynt_status": "",
                "ltlsynt_duration": ""
            }
        combined[result.model_name][f"{result.tool_name.lower()}_status"] = result.status
        combined[result.model_name][f"{result.tool_name.lower()}_duration"] = result.duration

        # Write the dictionary to a CSV
        output = StringIO() if (file is None or file.strip() == "") else open(file.strip(), 'w', newline='')

        # Write the dictionary to CSV
        writer = csv.DictWriter(output, fieldnames=["benchmark", "depsynt_status", "depsynt_duration", "strix_status", "strix_duration", "ltlsynt_status", "ltlsynt_duration"])
        writer.writeheader()
        for entry in combined.values():
            writer.writerow(entry)

        # If writing to StringIO, print the results, otherwise close the file
        if isinstance(output, StringIO):
            print(output.getvalue())
        else:
            output.close()


def main():
    args = parser.parse_args()
    benchmarks = [x.strip() for x in args.benchmarks.split(',')] if args.benchmarks else []
    try:
        timeout = args.timeout
        if timeout <= 0:
            raise argparse.ArgumentTypeError(f"Timeout must be a positive integer, got {args.timeout}")
    except ValueError:
        raise argparse.ArgumentTypeError(f"Invalid timeout value: {args.timeout}. It must be an integer.")

    # Validate benchmark exists
    for benchmark in benchmarks:
        if not is_benchmark_exists(benchmark):
            raise ValueError(f"Benchmark {benchmark} does not exist")

    # Run benchmarks
    benchmarks_results = []
    for benchmark in benchmarks:
        ltl_benchmark = get_benchmark(benchmark)
        for tool_name in ["depsynt", "ltlsynt", "strix"]:
            print(f"[Start] Apply benchmark {benchmark} with tool {tool_name}")
            tool_result = run_tool(
                tool_name=tool_name,
                model_name=ltl_benchmark.model_name,
                input_vars=ltl_benchmark.input_vars,
                output_vars=ltl_benchmark.output_vars,
                formula=ltl_benchmark.ltl_formula,
                timeout=timeout
            )
            benchmarks_results.append(tool_result)
            print(f"[Done] Apply benchmark {benchmark} with tool {tool_name}")

    # Summarize Results
    print_results(benchmarks_results, args.file)


if __name__ == "__main__":
    main()
