#include "unate_utils.h"
#include <boost/algorithm/string/join.hpp>

void UnatesHandlerMeasures::start_testing_state(unsigned state) {
    currently_testing_state = state;
    m_state_test_time.start();
}

void UnatesHandlerMeasures::end_testing_state(int removed_edges, int impacted_edges) {
    m_state_test_time.end();

    tested_states.emplace_back(TestedState{
            .state = currently_testing_state,
            .total_duration = m_state_test_time.get_duration(),
            .complement_duration = m_complement_time.get_duration(),
            .positive_unate_variables = positive_unate,
            .negative_unate_variables = negative_unate,
            .not_unate_variables = not_unate,
            .removed_edges = removed_edges,
            .impacted_edges = impacted_edges,
            .complement_succeeded = true
    });

    positive_unate.clear();
    negative_unate.clear();
    not_unate.clear();
    currently_testing_state = -1;
}



void UnatesHandlerMeasures::failed_complement() {
    m_state_test_time.end();

    tested_states.emplace_back(TestedState{
            .state = currently_testing_state,
            .total_duration = m_state_test_time.get_duration(),
            .complement_duration = m_complement_time.get_duration(),
            .positive_unate_variables = {},
            .negative_unate_variables = {},
            .not_unate_variables = {},
            .removed_edges = 0,
            .impacted_edges = 0,
            .complement_succeeded = false
    });

    positive_unate.clear();
    negative_unate.clear();
    not_unate.clear();
    currently_testing_state = -1;
}



void UnatesHandlerMeasures::start_testing_var(string &var) {
    currently_testing_var = var;
    m_variable_test_time.start();
}

void UnatesHandlerMeasures::tested_var_unate(UnateType unate_type) {
    if (unate_type == UnateType::Negative) {
        negative_unate.push_back(currently_testing_var);
    } else if (unate_type == UnateType::Positive) {
        positive_unate.push_back(currently_testing_var);
    }

    currently_testing_var = "";
    m_variable_test_time.start();
}

void UnatesHandlerMeasures::tested_var_not_unate() {
    not_unate.push_back(currently_testing_var);

    currently_testing_var = "";
    m_variable_test_time.start();
}

void UnatesHandlerMeasures::start_automaton_complement() {
    m_complement_time.start();
}

void UnatesHandlerMeasures::end_automaton_complement() {
    m_complement_time.end();
}


void UnatesHandlerMeasures::get_json_object(json::object& obj) const {
    json::array unate_states;
    for(const auto& state : this->tested_states) {
        json::object state_obj;
        state_obj["state"] = state.state;
        state_obj["total_duration"] = state.total_duration;
        state_obj["removed_edges"] = state.removed_edges;
        state_obj["impacted_edges"] = state.impacted_edges;
        state_obj["complement_succeeded"] = state.complement_succeeded;
        state_obj["complement_duration"] = state.complement_duration;
        state_obj["negative_unate_variables"] = boost::algorithm::join(state.negative_unate_variables, ",");
        state_obj["positive_unate_variables"] = boost::algorithm::join(state.positive_unate_variables, ",");
        state_obj["not_unate_variables"] = boost::algorithm::join(state.not_unate_variables, ",");
        unate_states.emplace_back(state_obj);
    }

    obj.emplace("unate_states", unate_states);
    obj.emplace("total_unate_duration", m_unate_handler_duration.get_duration());
    obj.emplace("automaton_postprocess_duration", m_postprocess_unate_time.get_duration());
    obj.emplace("total_edges_after_unate", m_total_edges_after_unate);
    obj.emplace("total_states_after_unate", m_total_states_after_unate);
}
