#!/usr/bin/env python3

import argparse
import json
from glob import glob
from pathlib import Path
import shutil
import os

OUTPUT = """{id}
{benchmark_name}
{benchmark_family}
{ltl_formula}
{input_vars}
{output_vars}
"""

parser = argparse.ArgumentParser("Convert TLSF files to text format")
parser.add_argument("tlsf_path", type=str, help="The path which includes all the TLSF files flatten")
parser.add_argument("text_path", type=str, help="The path to put the text files")


def get_tlsf_files(basepath):
    return glob(f"{basepath}/*.tlsf")


def omit_spaces(string):
    return ','.join([x.strip() for x in string.split(",")])


def analyize_tlsf_file(tlsf_path):
    benchmark_name = Path(tlsf_path).stem
    input_vars = omit_spaces(os.popen('syfco {} -ins'.format(tlsf_path)).read()).lower()
    output_vars = omit_spaces(os.popen('syfco {} -outs'.format(tlsf_path)).read()).lower()
    ltl_formula = os.popen('syfco -f ltlxba -m fully {}'.format(tlsf_path)).read().strip()

    return {
        "benchmark_name": benchmark_name,
        "input_vars": input_vars,
        "output_vars": output_vars,
        "ltl_formula": ltl_formula,
    }


def load_benchmarks_families(tlsf_basepath):
    structure_file = os.path.join(tlsf_basepath, 'structure.json')
    if not os.path.exists(structure_file):
        print("Error: structure.json not found")
        exit(1)

    with open(structure_file, 'r') as file:
        benchmarks_structure = json.load(file)

    def get_benchmark_family(benchmark_name):
        for family, benchmarks in benchmarks_structure.items():
            if benchmark_name in benchmarks or "{}.tlsf".format(benchmark_name) in benchmarks:
                return family
        return None

    return get_benchmark_family


if __name__ == "__main__":
    if shutil.which('syfco') is None:
        print("Error: SYFCO not found in PATH")
        exit(1)

    args = parser.parse_args()
    tlsf_basepath = args.tlsf_path
    text_basepath = args.text_path

    tlsf_files = get_tlsf_files(tlsf_basepath)
    get_benchmark_family = load_benchmarks_families(tlsf_basepath)

    id_counter = 1
    for tlsf_f in tlsf_files:
        tlsf_info = analyize_tlsf_file(tlsf_f)
        tlsf_info["benchmark_family"] = get_benchmark_family(tlsf_info["benchmark_name"])
        tlsf_info["id"] = id_counter
        id_counter += 1

        text_f = os.path.join(text_basepath, "{}.txt".format(tlsf_info["id"]))
        with open(text_f, 'w') as file:
            file.write(OUTPUT.format(**tlsf_info))
