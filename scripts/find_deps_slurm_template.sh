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

benchmark_name=$(sed -n "1p" "$FILEPATH" | tr -d '\r')
inputs_var=$(sed -n "2p" "$FILEPATH" | tr -d '\r')
outputs_var=$(sed -n "3p" "$FILEPATH" | tr -d '\r')
formula=$(sed -n "4p" "$FILEPATH" | tr -d '\r')

srun bash -c "{ time timeout $TOTAL_TIMEOUT $CLI_TOOL --formula=\"$formula\" --input=\"$inputs_var\" --output=\"$outputs_var\" --model-name=\"$benchmark_name\" --algo=automaton; }"
