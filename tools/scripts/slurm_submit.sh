#!/bin/bash

#SBATCH --job-name=synthesis-unates
#SBATCH --output=benchmark_%A_%a.out
#SBATCH --error=benchmark_%A_%a.err
#SBATCH --array=1-202
#SBATCH --time=1:00:00
#SBATCH --ntasks=1
#SBATCH --mem=2G

CLI_TOOL="<PATH_TO_CLI_TOOL>"
BENCHMARKS_DIR="<BENCHMARK_DIR>"  # The benchmark directory generator by the script: generate_benchmarks.py
MEASURES_BASE_PATH="<OUTPUT_PATH>"  # The path to the directory to put the output on

filepath="$BENCHMARKS_DIR/$SLURM_ARRAY_TASK_ID.txt"
benchmark_name=$(sed -n "1p" $filepath)
inputs_var=$(sed -n "2p" $filepath)
outputs_var=$(sed -n "3p" $filepath)
formula=$(sed -n "4p" $filepath)
measure_file="$MEASURES_BASE_PATH/$SLURM_ARRAY_TASK_ID.json"
output_file="$MEASURES_BASE_PATH/$SLURM_ARRAY_TASK_ID.hoa"

# Optional flags: --skip-unates --skip-dependencies
srun $CLI_TOOL --skip-dependencies --formula="$formula" --input="$inputs_var" --output="$outputs_var" --measures-path="$measure_file" --model-name="$benchmark_name" > $output_file 2>&1
