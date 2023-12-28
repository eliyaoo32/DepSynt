# DepSynt - Reactive Synthesis Tool
# Introduction
DepSynt is a reactive synthesis and LTL tool that elevates the concept of dependency to LTL.

This project expose 2 tools:
1) DepSynt - Synthesis a reactive synthesis specification, separating the problem into 2 sub-problems: A) synthesis strategy of independent out variables. B) synthesis strategy of dependent variables.

2) Find dependencies - find maximal set of dependent varialbes in LTL formula.


# 3rd Parties Dependencies
This tool uses the following 3rd parties:
- [Boost](https://www.boost.org/) - Common C++ libraries, mainly used for JSON creation and CLI parsing.
- [Spot](https://spot.lrde.epita.fr/) - Library for LTL and omega-automata manipulation
- [ABC](https://people.eecs.berkeley.edu/~alanmi/abc/) - Library for software system for synthesis and verification of binary sequential logic circuits.
  - Used mainly for AIG optimization
- [AIGER](https://github.com/arminbiere/aiger) - AIGER is a format, library and set of utilities for And-Inverter Graphs (AIGs).

## Useful Tools for analyzing
- [Syfco](https://github.com/reactive-systems/syfco) - A tool for converting from TLSF files to LTL formulas.
- [SYNTCOMP Benchmarks](https://github.com/SYNTCOMP/benchmarks) - Repoistory contains all the benchmarks used by SYNTCOMP competition, we use the benchmarks for LTL reactive synthesis encoded in [TLSF](https://github.com/SYNTCOMP/benchmarks#:~:text=synthesis%20encoded%20in-,TLSF,-format%2C) format.
- [Taskfile](https://taskfile.dev/) - Simple task runner.
- Model checking was done with [combine_aiger](https://github.com/reactive-systems/aiger-ltl-model-checker).
  - The tool was applied on the [SYNTCOMP](https://github.com/SYNTCOMP/benchmarks) benchmarks.
- [MKPlot](https://github.com/alexeyignatiev/mkplot.git) - Python script to generate Cactus plots.


# Build & Running With Docker (Recommended)
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
docker run depsynt --formula="((G (((((p0) && (! (p1))) && (! (p2))) || (((! (p0)) && (p1)) && (! (p2)))) || (((! (p0)) && (! (p1))) && (p2)))) && (((F (G (a))) || (G (F (b)))) <-> ((G (F (p0))) || ((G (F (p2))) && (! (G (F (p1))))))))" --input="b,a" --output="p2,p1,p0" --dependency-timeout=10000 --model-name="ltl2dpa16"
```

# Build & Running on Ubuntu/Debian
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

# Running the tool
## Synthesis
Usability of the CLI tool (using `docker run depsynt --help`):
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

## Find Dependencies
TBD.

# Documentation
## File Structure
```
bins/ - Executable files
libs/ - 3rd party libraries, includes AIGER, Spot and ABC
src/
  findDeps/ - Classes to find dependent variables in LTL formulas
  synthesis/ - Synthesis functions, synthesis dependent variables strategy, synthesis an NBA, merge strategies.
  unates/ - Classes to find unate variables in LTL formulas (Still in progress, currently not relevant for synthesis and dependency)
  utils/ - Utility functions
```

## Algorithm Implementations
TBD.

## Available Utils Commands
TBD.

