#include "utils.h"

#include <boost/program_options.hpp>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace Options = boost::program_options;
using namespace std;

void parse_cli_common(BaseCLIOptions &options, Options::options_description &desc) {
    desc.add_options()("formula,f",
                       Options::value<string>(&options.formula)->required(),
                       "LTL formula")(
        "output,o", Options::value<string>(&options.outputs)->default_value(""),

        "Output variables")("input,i",
                            Options::value<string>(&options.inputs)->default_value(""),
                            "Input variables")(
        "verbose,v", Options::bool_switch(&options.verbose), "Verbose messages")(
        "measures-path",
        Options::value<string>(&options.measures_path)->default_value(""));
}

/**
 * Options:
 * - Decompostion (Only possible if skip synt dependencies)
 * - Skip eject dependencies
 * - Skip synt dependencies
 */

bool parse_synthesis_cli(int argc, const char *argv[],
                         SynthesisCLIOptions &options) {
    Options::options_description desc(
        "Tool to synthesis LTL specification using dependencies and Unates concept");
    parse_cli_common(options, desc);
    desc.add_options()(
            "model-name",
            Options::value<string>(&options.model_name)->required(),
            "Unique model name of the specification"
        )
        ("dependency-timeout",
         Options::value<int>(&options.dependency_timeout)->required(),
         "Timeout for finding dependencies in milliseconds, if 0 then the process skips finding dependencies"
         )
        (
        "skip-unates",
        Options::bool_switch(&options.skip_unates)->default_value(false),
        "Should skip using unates to synthesise a strategy"
        )
        (
        "merge-strategies",
        Options::bool_switch(&options.merge_strategies)->default_value(false),
        "Should merge the independent and dependent strategies"
        )
        (
        "model-checking",
        Options::bool_switch(&options.apply_model_checking)->default_value(false),
        "Should apply model checking to the synthesized strategy"
        )(
        "measure-bdd",
        Options::bool_switch(&options.measure_bdd)->default_value(false),
        "Should measure the BDD size of NBAs"
        );

    try {
        Options::command_line_parser parser{argc, argv};
        parser.options(desc).allow_unregistered().style(
            Options::command_line_style::default_style |
            Options::command_line_style::allow_slash_for_short);
        Options::parsed_options parsed_options = parser.run();

        Options::variables_map vm;
        Options::store(parsed_options, vm);
        Options::notify(vm);

        if(!options.skip_unates) {
            cerr << "Currently, unates are not supported. Please use --skip-unates option" << endl;
            return false;
        }
        if(!options.merge_strategies && options.apply_model_checking) {
            cerr << "Model checking can only be applied if the strategies are merged. Please use --merge-strategies option." << endl;
            return false;
        }

        return true;
    } catch (const Options::error &ex) {
        cerr << ex.what() << '\n';
        cout << desc << endl;
        return false;
    }
}

bool parse_find_unates_cli(int argc, const char *argv[],
                                 FindUnatesCLIOptions &options) {
    Options::options_description desc(
            "Tool to unates in LTL specification");
    parse_cli_common(options, desc);

    try {
        Options::command_line_parser parser{argc, argv};
        parser.options(desc).allow_unregistered().style(
                Options::command_line_style::default_style |
                Options::command_line_style::allow_slash_for_short);
        Options::parsed_options parsed_options = parser.run();

        Options::variables_map vm;
        Options::store(parsed_options, vm);
        Options::notify(vm);

        return true;
    } catch (const Options::error &ex) {
        cerr << ex.what() << '\n';
        cout << desc << endl;
        return false;
    }
}

bool parse_find_dependencies_cli(int argc, const char *argv[],
                                 FindDependenciesCLIOptions &options) {
    Options::options_description desc(
        "Tool to find dependencies in LTL specification");
    parse_cli_common(options, desc);
    desc.add_options()("algo,algorithm", Options::value<string>(),
                       "Which algorithm to use: formula, automaton")(
        "find-input-only",
        Options::bool_switch(&options.find_input_dependencies)->default_value(false),
        "Search for input dependent variables instead of output dependent "
        "variables");

    try {
        Options::command_line_parser parser{argc, argv};
        parser.options(desc).allow_unregistered().style(
            Options::command_line_style::default_style |
            Options::command_line_style::allow_slash_for_short);
        Options::parsed_options parsed_options = parser.run();

        Options::variables_map vm;
        Options::store(parsed_options, vm);
        Options::notify(vm);

        if (vm.count("algo")) {
            options.algorithm = string_to_algorithm(vm["algo"].as<string>());
        } else {
            options.algorithm = Algorithm::UNKNOWN;
        }

        if (options.algorithm != Algorithm::AUTOMATON &&
            options.find_input_dependencies) {
            cerr << "Input dependencies can only be found using the automaton "
                    "algorithm"
                 << endl;
            return false;
        }

        return true;
    } catch (const Options::error &ex) {
        cerr << ex.what() << '\n';
        cout << desc << endl;
        return false;
    }
}

Algorithm string_to_algorithm(const std::string &str) {
    if (str == "formula") {
        return Algorithm::FORMULA;
    } else if (str == "automaton") {
        return Algorithm::AUTOMATON;
    } else {
        return Algorithm::UNKNOWN;
    }
}

std::string algorithm_to_string(const Algorithm &algo) {
    switch (algo) {
        case Algorithm::FORMULA:
            return "formula";
        case Algorithm::AUTOMATON:
            return "automaton";
        default:
            return "unknown";
    }
}

ostream &operator<<(ostream &out, const vector<string> &vec) {
    for (const string &s : vec) {
        out << s << ", ";
    }
    return out;
}

void extract_variables(const std::string &str, std::vector<std::string> &dst) {
    if(str.empty()) {
        return;
    }

    boost::split(dst, str, boost::is_any_of(","));
}

std::ostream &operator<<(std::ostream &out,
                         const FindDependenciesCLIOptions &options) {
    out << " - Formula: " << options.formula << endl;
    out << " - Inputs: " << options.inputs << endl;
    out << " - Outputs: " << options.outputs << endl;
    out << " - Verbose: " << options.verbose << endl;

    out << " - Algorithm: " << algorithm_to_string(options.algorithm) << endl;
    out << " - Type of dependent variables: "
        << (options.find_input_dependencies ? "input" : "output") << endl;

    return out;
}

std::ostream &operator<<(std::ostream &out, const SynthesisCLIOptions &options) {
    out << boolalpha;
    out << " - Formula: " << options.formula << endl;
    out << " - Inputs: " << options.inputs << endl;
    out << " - Outputs: " << options.outputs << endl;
    out << " - Verbose: " << options.verbose << endl;
    out << " - Skip synthesis dependencies synthesis: "
        << (options.dependency_timeout <= 0) << endl;
    out << " - Find dependency dependencies timeout: "
        << options.dependency_timeout << endl;

    return out;
}

/// TimeMeasure implementation
void TimeMeasure::start() {
    m_start = std::chrono::steady_clock::now();
    m_has_started = true;
}

Duration TimeMeasure::end() {
    m_total_duration = time_elapsed();
    return m_total_duration;
}

Duration TimeMeasure::time_elapsed() const {
    auto end = std::chrono::steady_clock::now();
    return static_cast<Duration>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start)
            .count());
}

Duration TimeMeasure::get_duration(bool validate_is_ended) const {
    if (validate_is_ended && m_total_duration == -1) {
        throw std::runtime_error(
            "TimeMeasure::get_total_duration() called before "
            "TimeMeasure::end()");
    }

    return m_total_duration;
}

void exec(const char* cmd, std::string &dst) {
    char buffer[128];
    auto pipe = popen(cmd, "r");
    if (!pipe) throw runtime_error("popen() failed!");

    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != nullptr) {
            dst += buffer;
        }
    }

    pclose(pipe);
}
