#ifndef REACTIVE_SYNTHESIS_UTILS_H
#define REACTIVE_SYNTHESIS_UTILS_H

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
    std::string measures_path;
};

struct SynthesisCLIOptions : public BaseCLIOptions {
    int dependency_timeout;
    bool skip_unates;
    bool merge_strategies;
    bool apply_model_checking;
    std::string model_name;
};

struct FindUnatesCLIOptions : public BaseCLIOptions {
};

struct FindDependenciesCLIOptions : public BaseCLIOptions {
    Algorithm algorithm{UNKNOWN};
    bool find_input_dependencies{};
};

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);

        return h1 ^ h2;
    }
};


bool parse_find_dependencies_cli(int argc, const char *argv[],
                                 FindDependenciesCLIOptions &options);

bool parse_find_unates_cli(int argc, const char *argv[], FindUnatesCLIOptions &options);

bool parse_synthesis_cli(int argc, const char *argv[], SynthesisCLIOptions &options);

void extract_variables(const std::string &str, std::vector<std::string> &dst);

void exec(const char* cmd, std::string &dst);

std::ostream &operator<<(std::ostream &out, const std::vector<std::string> &vec);

std::ostream &operator<<(std::ostream &out, const SynthesisCLIOptions &options);

std::ostream &operator<<(std::ostream &out,
                         const FindDependenciesCLIOptions &options);

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

    [[nodiscard]] Duration get_duration(bool validate_is_ended = false) const;
};

#endif
