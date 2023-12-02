import json
import os.path
import shutil
import sys
import csv

csv.field_size_limit(sys.maxsize)


def get_current_dir():
    return os.path.dirname(os.path.realpath(__file__))


def make_tmp_dir():
    tmp_dir = os.path.join(get_current_dir(), '../tasks_output/tmp_cactus')
    if os.path.exists(tmp_dir):
        print('Error: temp directory already exists')
        exit(1)
    os.makedirs(tmp_dir)
    return tmp_dir


def delete_tmp_dir():
    tmp_dir = os.path.join(get_current_dir(), '../tasks_output/tmp_cactus')
    shutil.rmtree(tmp_dir)


def get_filename(filepath):
    return os.path.basename(filepath).split('.')[0]


def load_csv(csv_path, tool_name):
    stats = {}
    with open(csv_path, 'r') as fp:
        reader = csv.DictReader(fp)
        rows = list(reader)
        for row in rows:
            success_ = row['Status'] == 'Success'
            stats[row['Benchmark Name']] = {
                'status': success_,
            }
            if success_:
                rtime = float(row['Total Duration']) / 1000.0  # In seconds
                if rtime < 0.0001:
                    rtime = 0.0001
                stats[row['Benchmark Name']]['rtime'] = rtime
    
    if len(stats.keys()) == 0:
        raise Exception('No stats found ' + csv_path)
    return {
        'preamble': {
            'program': tool_name,
            'prog_alias': tool_name,
            'benchmark': 'SYNTCOMP',
        },
        'stats': stats,
    }


def save_cactus_to_file(filename='cactus.png', title=None, ylog=False):
    # TODO: check that mkplot is installed
    cactus_gen_command = "python ./scripts/mkplot/mkplot.py --ylabel=\"Time (seconds)\" --ymin 0.0001 {} -t 3600 {} -b png --save-to {} ./tasks_output/tmp_cactus/*.json".format(
        "--ylog" if ylog else "",
        f"--plot-title \"{title}\"" if title is not None else "",
        f"\"{filename}\""
    )
    os.system(cactus_gen_command)


def dump_stats(csv_path, tmp_dir, tool_name):
    stats = load_csv(csv_path, tool_name)
    tool_json_file = os.path.join(tmp_dir, tool_name + '.json')
    with open(tool_json_file, 'w') as fp:
        json.dump(stats, fp, indent=4)


def main():
    tmp_dir = make_tmp_dir()

    by_formula_csv = os.path.join(get_current_dir(), f'../tasks_output/find_deps_formula.csv')
    by_automaton_csv = os.path.join(get_current_dir(), f'../tasks_output/find_deps.csv')

    dump_stats(by_automaton_csv, tmp_dir, "Automaton approach")
    dump_stats(by_formula_csv, tmp_dir, "Formula approach")

    save_cactus_to_file(filename='cactus_find_deps.png', title='Find Dependencies', ylog=True)

    delete_tmp_dir()


if __name__ == '__main__':
    main()
