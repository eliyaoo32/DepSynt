# DepSynt - Reactive Synthesis Tool
# Introduction
This is a reactive synthesis tool that uses the concept of dependency in LTL.
Currently, we are working on elevating the Unate concept to the LTL level.

# 3rd Parties

## Libraries
This tool uses the following 3rd parties:
- [Boost](https://www.boost.org/) - Common C++ libraries, mainly used for JSON creation and CLI parsing.
- [Spot](https://spot.lrde.epita.fr/) - Library for LTL and omega-automata manipulation
- [ABC](https://people.eecs.berkeley.edu/~alanmi/abc/) - Library for software system for synthesis and verification of binary sequential logic circuits.
  - Used mainly for AIG optimization
- [AIGER](https://github.com/arminbiere/aiger) - AIGER is a format, library and set of utilities for And-Inverter Graphs (AIGs).

## Useful Tools
- [Syfco](https://github.com/reactive-systems/syfco) - A tool for converting from TLSF files to LTL formulas.
- [Taskfile](https://taskfile.dev/) - Simple task runner.
- Model checking was done with [combine_aiger](https://github.com/reactive-systems/aiger-ltl-model-checker).
  - The tool was applied on the [SYNTCOMP](https://github.com/SYNTCOMP/benchmarks) benchmarks.
- [MKPlot](https://github.com/alexeyignatiev/mkplot.git) - Python script to generate Cactus plots.

# Build & Usage Instruction
1) Fetch the used submodules:
```bash
git submodule update --init
```

2) Make sure you add to `CPATH` enviornment variable a path to Spot headers, for example: `CPATH=~/spot/include:$CPATH`.

3) Compile binaries with CMake and Makefile:
```bash
cmake .
make
```

# Flows
## Generate Benchmarks
For each benchmark it generates a file in a dedicated folder with the format:

```text
id
benchmark_name
benchmark_family
ltl_formula
input_variables
output_variables
```

Generating benchmark with Taskfile:
```bash
task generate_benchmarks
```

## Find Dependencies
The flow applies the following steps:

1. Generate a bash file to runs slurm job on the generated benchmarks folder.
2. Run the bash file on the slurm cluster.

### Summarize Dependency Results

## Synthesis with DepSynt
TBD.

### Summarize Synthesis Results

## Model Checking
TBD.



# File Structure
```
bins/ - Executable files
libs/ - 3rd party libraries, includes AIGER and ABC
src/
  findDeps/ - Classes to find dependent variables in LTL formulas
  synthesis/ - Synthesis functions, synthesis dependent variables strategy, synthesis an NBA, merge strategies.
  unates/ - Classes to find unate variables in LTL formulas (Still in progress)
  utils/ - Utility functions
```