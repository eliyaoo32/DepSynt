#!/bin/bash

#SBATCH --job-name=strix
#SBATCH --output={{OUTPUT_BASE_PATH}}/%a.out
#SBATCH --error={{OUTPUT_BASE_PATH}}/%a.err
#SBATCH --array=1-{{NUM_BENCHMARKS}}
#SBATCH --ntasks=1
#SBATCH --mem=2G
#SBATCh --cpus-per-task=1

TOTAL_TIMEOUT="{{TIMEOUT}}"
CLI_TOOL="strix"
FILEPATH="{{BENCHMARKS_DIR}}/$SLURM_ARRAY_TASK_ID.txt"

benchmark_family=$(sed -n "3p" "$FILEPATH" | tr -d '\r')
benchmark_name=$(sed -n "2p" "$FILEPATH" | tr -d '\r')
inputs_var=$(sed -n "5p" "$FILEPATH" | tr -d '\r')
outputs_var=$(sed -n "6p" "$FILEPATH" | tr -d '\r')
formula=$(sed -n "4p" "$FILEPATH" | tr -d '\r')

allowed_family=({{ALLOWED_FAMILIES}})
if [[ ! " ${allowed_family[@]} " =~ " ${benchmark_family} " ]]; then
    echo "Family $benchmark_family is skipped"
    exit 1
fi


srun bash -c "{ time timeout $TOTAL_TIMEOUT $CLI_TOOL -f \"$formula\" --ins=\"$inputs_var\" --outs=\"$outputs_var\" -o aag -e bfs --solver zlk; }"
