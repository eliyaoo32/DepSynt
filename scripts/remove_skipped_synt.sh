#!/bin/bash

# Get the directory from command-line argument
dir="$1"

# Check if directory is supplied
if [[ -z "$dir" ]]; then
  echo "Usage: $0 <directory>"
  exit 1
fi

# Loop over the .out files
for file in $dir/*.out
do
  # If the file contains "Family .* is skipped"
  if grep -q "Family .* is skipped" $file; then
    # Get the base number (removing the .out)
    base=${file%.out}

    # Remove the .out and .err files
    rm "$base.out"
    rm "$base.err"
  fi
done
