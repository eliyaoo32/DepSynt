#ifndef REACTIVE_SYNTHESIS_BFSS_UNATE_UTILS_H
#define REACTIVE_SYNTHESIS_BFSS_UNATE_UTILS_H

#include <vector>
#include <string>
#include <boost/json.hpp>

#include "utils.h"
using namespace std;
namespace json = boost::json;

enum class UnateType {
    Positive,
    Negative,
};


struct TestedState {
    unsigned state;
    Duration total_duration;
    Duration complement_duration;

    vector<string> positive_unate_variables;
    vector<string> negative_unate_variables;
    vector<string> not_unate_variables;

    int removed_edges;
    int impacted_edges;
};

class UnatesHandlerMeasures {
private:
    // Variables data
    TimeMeasure m_variable_test_time;
    TimeMeasure m_state_test_time;
    TimeMeasure m_complement_time;

    string currently_testing_var;
    unsigned currently_testing_state;

    // Process Data
    vector<TestedState> tested_states;
    vector<string> positive_unate;
    vector<string> negative_unate;
    vector<string> not_unate;
    TimeMeasure m_unate_handler_duration;
    int m_total_edges_after_unate;
    int m_total_states_after_unate;

protected:
    void get_json_object(json::object &obj) const;

public:
    UnatesHandlerMeasures() : currently_testing_var(""), m_total_edges_after_unate(-1) {
    }

    void start() {
        m_unate_handler_duration.start();
    }

    void end(int total_edges_after_unate, int states_after_unate) {
        m_unate_handler_duration.end();
        m_total_edges_after_unate = total_edges_after_unate;
        m_total_states_after_unate = states_after_unate;
    }

    void start_testing_state(unsigned state);

    void end_testing_state(int removed_edges, int impacted_edges);

    void start_testing_var(string &var);

    void tested_var_unate(UnateType unate_type);

    void tested_var_not_unate();

    void start_automaton_complement();

    void end_automaton_complement();
};



#endif //REACTIVE_SYNTHESIS_BFSS_UNATE_UTILS_H
