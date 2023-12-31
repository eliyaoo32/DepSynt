# Introduction
This artifact contains all tools and benchmarks that were used to obtain the experimental results in the paper "On Dependent Variables in Reactive Synthesis".
The tool **DepSynt** developed as part of this work builds on the publicly available and widely used automata manipulation and synthesis library Spot.
Our comparisons are done with the best performing entries in SYNTCOMP 2023 competition. Four of these entries are implemented in Spot. The fifth one, named Strix, is not implemented in Spot, but is provided as part of this artifact for the sake of completeness.  Our artifact contains instructions on how to install our tool and competing ones, and also describes how to run our tool on individual benchmarks and on a batch of them.
We also describe how to compare the performance of DepSynt with the five competing tools. The runtimes reported in the paper are obtained from a high performance computing cluster, and may differ from those obtained from the TACAS Artifact Evaluation VM.  However, the relative trends should be the same as those reported in the paper.

# Hyperlink to Artifact
Link to the Github repository: https://github.com/eliyaoo32/DepSynt

# Building & Installation
* The build process might take 20 minutes approximately.
* Assuming that you have the TACAS24' VM, you can build DepSynt by running the following commands:
```bash
chmod +x ./build.sh
sudo ./build.sh
```

You can verify that the installation was successful by running the following command:
```bash
./depsynt --help
```
The expected output should be:
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


# Benchmarks

The benchmarks are available in two formats:
1) TLSF - The benchmarks are located in the `scripts/benchmarks/tlsf` folder.

2) LTL Formula - The benchmarks are located in the `scripts/benchmarks-ltl` folder.

We have converted from TLSF specification to LTL using the public available tool Syfco (Included in this artifact).


# Usage
## How to run DepSynt on a specific benchmark
For example, suppose we want to run DepSynt on the benchmark `ltl2dpa10` which has input variables `b,a` and output variables `p2,p1,p0`.
LTL formula corresponding to this benchmark is:

`((G (((((p0) && (! (p1))) && (! (p2))) || (((! (p0)) && (p1)) && (! (p2)))) || (((! (p0)) && (! (p1))) && (p2)))) && (((F (G (a))) || (G (F (b)))) <-> ((G (F (p0))) || ((G (F (p2))) && (! (G (F (p1))))))))`


We want to limit the dependency finding process to 10 seconds (10,000 milliseconds).


The following command should then be executed:
```bash
./depsynt --model-name="ltl2dpa10" --input="b,a" --output="p2,p1,p0" --dependency-timeout=10000 --formula="((G (((((p0) && (! (p1))) && (! (p2))) || (((! (p0)) && (p1)) && (! (p2)))) || (((! (p0)) && (! (p1))) && (p2)))) && (((F (G (a))) || (G (F (b)))) <-> ((G (F (p0))) || ((G (F (p2))) && (! (G (F (p1))))))))"
```

The expected output for realizable specification like the above one is:
```plain
Circuit in AIG format of mealy machine representing strategy for independent outputs.
Circuit in AIG format of mealy machine representing strategy for dependent outputs.
Performance metrcies and related information like count of dependency / dependent variables in JSON format.
```

In case of unrealizable specification, the expected output is:
```plain
UNREALIZABLE
Performance metrcies and related information like count of dependency / dependent variables in JSON format.
```

## How to run Ltlsynt (Competing tool) on a specific benchmark 
Ltlsynt is one of the winning entries in SYNTCOMP 2023 and is based on Spot library.
Ltlsynt can be run with a variety of algorithms. For our experimental comparison we used the algorithms denoted by `sd,ds,lar,acd`.

The algorithm can be specified using the following command line option:
```bash
--algo=sd|ds|lar|acd
```

To run Ltlsynt on the same benchmark `ltl2dpa10` as above with algo `acd`, use the following command:
```bash
ltlsynt --aiger --algo=acd --outs="p2,p1,p0" --ins="b,a" --formula="((G (((((p0) && (! (p1))) && (! (p2))) || (((! (p0)) && (p1)) && (! (p2)))) || (((! (p0)) && (! (p1))) && (p2)))) && (((F (G (a))) || (G (F (b)))) <-> ((G (F (p0))) || ((G (F (p2))) && (! (G (F (p1))))))))" 
```

The expected output of realizable specification is:
```plain
REALIZABLE
Circuit in AIG format of mealy machine representing strategy for the specification.
```

In the case of unrealizable specification, the expected output is:
```plain
UNREALIZABLE
```

## How to run Strix (Competing tool) on a specific benchmark
Strix is one of the winning entries in SYNTCOMP 2023.

To run Strix on the same benchmark `ltl2dpa10`, use the following command:
```bash
./strix --ins="b,a" --outs="p2,p1,p0" -o aag -f "((G (((((p0) && (! (p1))) && (! (p2))) || (((! (p0)) && (p1)) && (! (p2)))) || (((! (p0)) && (! (p1))) && (p2)))) && (((F (G (a))) || (G (F (b)))) <-> ((G (F (p0))) || ((G (F (p2))) && (! (G (F (p1))))))))"
```

The expected output of realizable specification is:
```plain
REALIZABLE
Circuit in AIG format of mealy machine representing strategy for the specification.
```

In the case of unrealizable specification, the expected output is:
```plain
UNREALIZABLE
```

## How to run comparison between DepSynt, Ltlsynt and Strix on multiple benchmarks together
* This process can take very long time on the VM.
* All the execution have to be run sequentially on the VM.
* Note that the results reported in the paper were obtained on a high performance computing cluster with parallel runs using Slurm which is reliable in terms time and resources per tool.
* These times may differ from those obtained from the TACAS Artifact Evaluation VM.  However, the relative trends should be the same as those reported in the paper.

For example, suppose we want to run DepSynt, Ltlsynt and Strix on the benchmarks `ltl2dpa16,ltl2dpa10,Increment` together.
The following command should then be executed:
```bash
python3 ./run-benchmarks.py --timeout=10000 --benchmarks=ltl2dpa16,ltl2dpa10,Increment
```

Important notes:
* The benchmarks name are case-sensitive, the same as appears in the folder `scripts/benchmarks-ltl`.
* The timeout is in milliseconds.
* The script outputs in CSV with the headers: benchmark, depsynt_status, depsynt_duration, strix_status, strix_duration, ltlsynt_status, ltlsynt_duration.
* You can write the CSV to file using the `--file` option:

```bash
python3 ./run-benchmarks.py --file=./results.csv --timeout=10000 --benchmarks=ltl2dpa16,ltl2dpa10,Increment
```