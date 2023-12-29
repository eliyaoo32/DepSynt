# DepSynt - Reactive Synthesis Tool
# Introduction
DepSynt is a reactive synthesis and LTL tool that elevates the concept of dependency to LTL.

This project expose 2 tools:
1) DepSynt - Synthesis a reactive synthesis specification, separating the problem into 2 sub-problems: A) synthesis strategy of independent out variables. B) synthesis strategy of dependent variables.

2) Find dependencies - find maximal set of dependent varialbes in LTL formula.

# Build & Run on TACAS24' VM
1. Explanation & download of TACAS24' VM: https://zenodo.org/records/7113223
2. Run build script on sudo mode:
```bash
sudo ./scripts/build.sh
```
make sure the file is executable:
```bash
chmod +x ./scripts/build.sh
```


# 3rd Parties Dependencies
This tool uses the following 3rd parties:
- [Boost](https://www.boost.org/) - Common C++ libraries, mainly used for JSON creation and CLI parsing.
- [Spot](https://spot.lrde.epita.fr/) - Library for LTL and omega-automata manipulation
- [ABC](https://people.eecs.berkeley.edu/~alanmi/abc/) - Library for software system for synthesis and verification of binary sequential logic circuits.
  - Used mainly for AIG optimization
- [AIGER](https://github.com/arminbiere/aiger) - AIGER is a format, library and set of utilities for And-Inverter Graphs (AIGs).

# Using the tool
## Synthesis
Usability of the CLI tool (using `./depsynt <args>` or `docker run depsynt <args>`):

For example, help command:
```bash
./depsynt --help
```
```plain
Tool to synthesis LTL specification using dependencies and Unates concept:
  -f [ --formula ] arg     LTL formula
  -o [ --output ] arg      Output variables
  -i [ --input ] arg       Input variables
  -v [ --verbose ]         Verbose messages
  --measures-path arg
  --model-name arg         Unique model name of the specification
  --dependency-timeout arg Timeout for finding dependencies in milliseconds, if
                           0 then the process skips finding dependencies
  --merge-strategies       Should merge the independent and dependent 
                           strategies
  --model-checking         Should apply model checking to the synthesized 
                           strategy
  --measure-bdd            Should measure the BDD size of NBAs
```

Synthesis [ltl2dpa10](https://github.com/SYNTCOMP/benchmarks/blob/288f8f313d3a4c1e1bafff97e7c5533fc43b3a71/tlsf/ltl2dpa/ltl2dpa16.tlsf):
```bash
./depsynt --input="b,a" --output="p2,p1,p0" --dependency-timeout=10000 --model-name="ltl2dpa16" --formula="((G (((((p0) && (! (p1))) && (! (p2))) || (((! (p0)) && (p1)) && (! (p2)))) || (((! (p0)) && (! (p1))) && (p2)))) && (((F (G (a))) || (G (F (b)))) <-> ((G (F (p0))) || ((G (F (p2))) && (! (G (F (p1))))))))"
```

### Synthesis a SYNTCOMP benchmark:
1. Reactive synthesis benchmarks in SYNTCOMP are given in TLSF file.
2. TLSF file are converted to LTL formula using [Syfco](https://github.com/reactive-systems/syfco).
3. Since Syfco return the input and output variables with space and comma between and DepSynt expect comma separated list, we need to remove the spaces.
4. The benchmarks are exist in path `./scripts/benchmarks/tlsf` as Git submodule, also can be found in [Github](https://github.com/SYNTCOMP/benchmarks). 

Example of running the ltl2dpa10 benchmark from its TLSF format:
```bash
LTL=$(syfco -f ltlxba -m fully ./scripts/benchmarks/tlsf/ltl2dpa/ltl2dpa10.tlsf)
OUT=$(syfco --print-output-signals ./scripts/benchmarks/tlsf/ltl2dpa/ltl2dpa10.tlsf | tr -d ' ')
IN=$(syfco --print-input-signals ./scripts/benchmarks/tlsf/ltl2dpa/ltl2dpa10.tlsf | tr -d ' ')
./depsynt --input="$IN" --output="$OUT" --dependency-timeout=10000 --model-name="ltl2dpa10" --formula="$LTL"
``` 

## Find Dependencies
Find dependency is a standalone tool that finds the maximal set of dependent variables in LTL formula, without time limitation and without synthesising process.
The CLI tool source code is available in `bins/findDeps.cpp`.

# Run on Docker
1. Docker file can be downloaded from https://figshare.com/articles/software/DepSynt_docker_image/24915732
2. Load the docker image from the file:
```bash
docker load --input depsynt.tar.gz
```
The output suppose to be:
```plain
Loaded image: depsynt:latest
```

3. Run the tool:
```plain
docker run depsynt --help
```
```bash
docker run depsynt --input="b,a" --output="p2,p1,p0" --dependency-timeout=10000 --model-name="ltl2dpa16" --formula="((G (((((p0) && (! (p1))) && (! (p2))) || (((! (p0)) && (p1)) && (! (p2)))) || (((! (p0)) && (! (p1))) && (p2)))) && (((F (G (a))) || (G (F (b)))) <-> ((G (F (p0))) || ((G (F (p2))) && (! (G (F (p1))))))))"
```


# Build on Docker
1) Fetch the git submodules:
```bash
git submodule update --init
```

2. Build the docker image:
```bash
docker build -t depsynt .
```

Now we can run the DepSynt tool: `docker run depsynt`


Example for synthesising the LTL specification [ltl2dpa10](https://github.com/SYNTCOMP/benchmarks/blob/288f8f313d3a4c1e1bafff97e7c5533fc43b3a71/tlsf/ltl2dpa/ltl2dpa16.tlsf):
```bash
docker run depsynt --input="b,a" --output="p2,p1,p0" --dependency-timeout=10000 --model-name="ltl2dpa16" --formula="((G (((((p0) && (! (p1))) && (! (p2))) || (((! (p0)) && (p1)) && (! (p2)))) || (((! (p0)) && (! (p1))) && (p2)))) && (((F (G (a))) || (G (F (b)))) <-> ((G (F (p0))) || ((G (F (p2))) && (! (G (F (p1))))))))"
```

# Build on Ubuntu (For Development mode)
1) Fetch the used submodules:
```bash
git submodule update --init
```

2) Install Boost, cmake, wget, gnupg2
```bash
apt-get update && apt-get install -y \
    g++ \
    cmake \
    libboost-all-dev \
    wget \
    gnupg2
```

3) Install Boost-JSON (Available in debian sid repository)
```bash
echo "deb http://deb.debian.org/debian sid main" >> /etc/apt/sources.list
apt-get update && apt-get install -y --no-install-recommends  libboost-json-dev
```

3) Install Spot, following [Spot's Debian installation guidelines](https://spot.lre.epita.fr/install.html#Debian):
```bash
wget -q -O - https://www.lrde.epita.fr/repo/debian.gpg | apt-key add -
echo 'deb http://www.lrde.epita.fr/repo/debian/ stable/' >> /etc/apt/sources.list
apt-get update && apt-get install -y \
    spot=2.11.4.0-1 libspot-dev=2.11.4.0-1 \
    libspot0=2.11.4.0-1 libspotltsmin0=2.11.4.0-1 libspotgen0=2.11.4.0-1 \
    libbddx0=2.11.4.0-1 libbddx-dev=2.11.4.0-1
```

4) Install ABC (Codebase is available in ./libs/abc).
ABC need to be added to environment variable `LD_LIBRARY_PATH`. The following command does it, make sure to replace `{REPO_PATH}` with the path where this repo is installed.
```bash
cd ./libs/abc && make ABC_USE_NO_READLINE=1 ABC_USE_PIC=1 libabc.so
LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:{REPO_PATH}/libs/abc"
```


5) Compile source-code:
```bash
cmake .
make synthesis
```
# Documentation
## File Structure
```
bins/ - Executable files, such as: synthesis, findDeps, etc.
libs/ - 3rd party libraries, includes AIGER, Spot and ABC
src/
  findDeps/ - Find dependent variables logic
  synthesis/ - Synthesis functions, synthesis dependent variables strategy, synthesis an NBA, merge strategies.
  utils/ - Utility functions
  unates/ - (Still in progress, currently not relevant for synthesis and dependency)
```

## Algorithm Implementations
DepSynt - Algorithm to synthesize a reactive synthesis specification.
* Entrypoint for algorithm is `bins/synthesis.cpp`

The algorithm has the following steps:
1. Construct NBA A of the LTL specification (Implemented by Spot)
2. Find a maximal set of dependent variables (Source code: `src/findDeps/find_deps_by_automaton.cpp`)
3.  If no dependent variable was found, synthesis NBA A using Spot and close the process.
4. Construct NBA A' which is a clone of A with the dependent variables removed. (Source code: `src/synthesis/synthesis_utils.cpp`)
5. Synthesis a strategy as AIG for A' using Spot. (Source code: `src/synthesis/synthesis_utils.cpp`).
 5.1. If A' is not realizable, the specification is not realizable and the process is closed.
6. Synthesis a strategy as AIG format for dependent variables (Source code: `src/synthesis/dependents_synthesiser.cpp`)
7. (If requested) Merge the 2 strategies (Source code: `src/synthesis/merge_strategies.cpp`)
   7.1. This step uses AIGER and BLIF format to merge the strategy.
   7.2. ABC is used to balance and final strategy.
8. (If requested) Model checking the synthesized strategy - by checking if the negation of the specification intersects with the language of strategy.


# Useful Tools for analyzing
- [Syfco](https://github.com/reactive-systems/syfco) - A tool for converting from TLSF files to LTL formulas.
- [SYNTCOMP Benchmarks](https://github.com/SYNTCOMP/benchmarks) - Repoistory contains all the benchmarks used by SYNTCOMP competition, we use the benchmarks for LTL reactive synthesis encoded in [TLSF](https://github.com/SYNTCOMP/benchmarks#:~:text=synthesis%20encoded%20in-,TLSF,-format%2C) format.
- [Taskfile](https://taskfile.dev/) - Simple task runner.
- Model checking was done with [combine_aiger](https://github.com/reactive-systems/aiger-ltl-model-checker).
    - The tool was applied on the [SYNTCOMP](https://github.com/SYNTCOMP/benchmarks) benchmarks.
- [MKPlot](https://github.com/alexeyignatiev/mkplot.git) - Python script to generate Cactus plots.
