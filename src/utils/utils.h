#ifndef UTILS_H
#define UTILS_H

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <spot/tl/formula.hh>
#include <spot/tl/parse.hh>
#include <spot/twa/fwd.hh>
#include <spot/twaalgos/postproc.hh>
#include <spot/twaalgos/translate.hh>
#include <string>
#include <vector>

enum Algorithm { UNKNOWN = 0, FORMULA = 1, AUTOMATON = 2 };

Algorithm string_to_algorithm(const std::string &str);
std::string algorithm_to_string(const Algorithm &algo);

struct BaseCLIOptions {
    std::string formula;
    std::string inputs;
    std::string outputs;
    bool verbose;
};

struct SynthesisCLIOptions : public BaseCLIOptions {
    bool skip_dependencies;
    bool decompose_formula;
};

struct FindDependenciesCLIOptions : public BaseCLIOptions {
    Algorithm algorithm;
    bool find_input_dependencies;
};

bool parse_find_dependencies_cli(int argc, const char *argv[],
                                 FindDependenciesCLIOptions &options);

bool parse_synthesis_cli(int argc, const char *argv[], SynthesisCLIOptions &options);

void extract_variables(const std::string &str, std::vector<std::string> &dst);

std::ostream &operator<<(std::ostream &out, const std::vector<std::string> &vec);

std::ostream &operator<<(std::ostream &out, const SynthesisCLIOptions &options);

std::ostream &operator<<(std::ostream &out, const FindDependenciesCLIOptions &options);

using Duration = long;

class TimeMeasure {
   private:
    std::chrono::steady_clock::time_point m_start;
    Duration m_total_duration;
    bool m_has_started;

   public:
    TimeMeasure() : m_total_duration(-1), m_has_started(false) {}

    void start();

    bool has_started() const { return m_has_started; }

    Duration end();

    [[nodiscard]] Duration time_elapsed() const;

    [[nodiscard]] Duration get_duration(bool validate_is_ended = true) const;
};

#endif
