#ifndef REACTIVE_SYNTHESIS_BFSS_UNATE_UTILS_H
#define REACTIVE_SYNTHESIS_BFSS_UNATE_UTILS_H

#include <unordered_set>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

#include "utils.h"
using namespace std;

using json = nlohmann::json;


enum class UnateType {
    Positive,
    Negative,
};

struct UnateEffectOnState {
    unordered_set<void*> impacted_edges; // Storing the address of the changed edges
    unordered_set<void*> removed_edges; // Storing the address of the changed edges
};

struct TestedState {
    unsigned state;
    Duration total_duration;
    Duration complement_duration;

    vector<string> positive_unate_variables;
    vector<string> negative_unate_variables;
    vector<string> not_unate_variables;
    vector<string> unknown_unate_variables;

    int removed_edges;
    int impacted_edges;

    bool complement_succeeded;
};

class UnatesHandlerMeasures {
private:
    // Variables data
    TimeMeasure m_state_test_time;
    TimeMeasure m_complement_time;
    TimeMeasure m_postprocess_unate_time;

    string currently_testing_var;
    unsigned currently_testing_state;

    // Process Data
    vector<TestedState> tested_states;
    vector<string> positive_unate;
    vector<string> negative_unate;
    vector<string> unknown_unate;
    vector<string> not_unate;
    TimeMeasure m_unate_handler_duration;
    int m_total_edges_after_unate;
    int m_total_states_after_unate;

    string m_algorithm_name;

protected:
    void get_json_object(json &obj) const;

public:
    UnatesHandlerMeasures() : currently_testing_var(""), m_total_edges_after_unate(-1), m_algorithm_name("UNKNOWN") {
    }

    void start() {
        m_unate_handler_duration.start();
    }

    void end(int total_edges_after_unate, int states_after_unate) {
        m_unate_handler_duration.end();
        m_total_edges_after_unate = total_edges_after_unate;
        m_total_states_after_unate = states_after_unate;
    }

    void set_algorithm_name(const char* algorithm_name) {
        m_algorithm_name = algorithm_name;
    }

    void start_testing_state(unsigned state);

    void end_testing_state(int removed_edges, int impacted_edges);

    void start_postprocess_automaton() {m_postprocess_unate_time.start();}

    void end_postprocess_automaton() {m_postprocess_unate_time.end();}

    void failed_complement();

    void start_testing_var(string &var);

    void tested_var_unate(UnateType unate_type);

    void tested_var_not_unate();

    void tested_var_unknown();

    void start_automaton_complement();

    void end_automaton_complement();
};



#endif //REACTIVE_SYNTHESIS_BFSS_UNATE_UTILS_H
