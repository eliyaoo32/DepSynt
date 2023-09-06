#!/bin/bash

#SBATCH --job-name=find_deps
#SBATCH --output={{OUTPUT_BASE_PATH}}/%a.out
#SBATCH --error={{OUTPUT_BASE_PATH}}/%a.err
#SBATCH --array=1-{{NUM_BENCHMARKS}}
#SBATCH --ntasks=1
#SBATCH --mem=2G
#SBATCh --cpus-per-task=1

TOTAL_TIMEOUT="{{TIMEOUT}}"
CLI_TOOL="./find_dependencies"
FILEPATH="{{BENCHMARKS_DIR}}/$SLURM_ARRAY_TASK_ID.txt"

benchmark_name=$(sed -n "2p" "$FILEPATH" | tr -d '\r')
inputs_var=$(sed -n "5p" "$FILEPATH" | tr -d '\r')
outputs_var=$(sed -n "6p" "$FILEPATH" | tr -d '\r')
formula=$(sed -n "4p" "$FILEPATH" | tr -d '\r')

cmd_string="$CLI_TOOL --formula=\"$formula\" --model-name=\"$benchmark_name\" --algo=automaton"
if [[ -n "$inputs_var" ]]; then
    cmd_string="$cmd_string --input=\"$inputs_var\""
fi
if [[ -n "$outputs_var" ]]; then
    cmd_string="$cmd_string --output=\"$outputs_var\""
fi

srun bash -c "{ time timeout $TOTAL_TIMEOUT $cmd_string; }"

