#include "synt_measure.h"

void SyntMeasures::end_automaton_construct(spot::twa_graph_ptr& automaton) {
    m_aut_construct_time.end();
    m_is_automaton_built = true;

    m_total_automaton_states = automaton->num_states();
    m_automaton_state_based_status =
        automaton->prop_state_acc().is_true()
            ? "true"
            : (automaton->prop_state_acc().is_false() ? "false" : "maybe");
}

void SyntMeasures::start_testing_variable(string& var) {
    m_variable_test_time.start();
    currently_testing_var = new string(var);
}

void SyntMeasures::end_testing_variable(bool is_dependent,
                                        vector<string>& tested_dependency_set) {
    m_variable_test_time.end();

    m_tested_variables.push_back({*currently_testing_var,
                                  m_variable_test_time.get_duration(), is_dependent,
                                  tested_dependency_set});
    delete currently_testing_var;
    currently_testing_var = nullptr;
}

void SyntMeasures::get_json_object(json::object& obj) const {
    // General information
    json::array output_vars;
    std::transform(m_synt_instance.get_output_vars().begin(),
                   m_synt_instance.get_output_vars().end(),
                   std::back_inserter(output_vars),
                   [](const std::string& var) { return json::string(var); });
    json::array input_vars;
    std::transform(m_synt_instance.get_input_vars().begin(),
                   m_synt_instance.get_input_vars().end(), std::back_inserter(input_vars),
                   [](const std::string& var) { return json::string(var); });

    obj.emplace("is_completed", m_is_completed);
    obj.emplace("output_vars", output_vars);
    obj.emplace("algorithm_type", "formula");
    obj.emplace("input_vars", input_vars);
    obj.emplace("formula", this->m_synt_instance.get_formula_str());
    obj["total_time"] = this->m_total_time.time_elapsed();

    // Automaton information
    json::object automaton;
    automaton["is_built"] = this->m_is_automaton_built;
    if (this->m_is_automaton_built) {
        automaton["build_duration"] = this->m_aut_construct_time.get_duration();
        automaton["total_states"] = static_cast<int>(this->m_total_automaton_states);
        automaton["state_based_status"] = this->m_automaton_state_based_status;
    }
    obj.emplace("automaton", automaton);

    // Dependency information
    json::array tested_vars;
    for (const auto& var : this->m_tested_variables) {
        json::object var_obj;
        var_obj["name"] = var.name;
        var_obj["duration"] = var.duration;
        var_obj["is_dependent"] = var.is_dependent;
        var_obj.emplace("tested_dependency_set",
                        json::array(var.tested_dependency_set.begin(),
                                    var.tested_dependency_set.end()));

        tested_vars.emplace_back(var_obj);
    }
    obj.emplace("tested_variables", tested_vars);
}

void AutomatonFindDepsMeasure::start_search_pair_states() {
    m_search_pair_states_time.start();
}

void AutomatonFindDepsMeasure::end_search_pair_states(int total_pair_states) {
    m_search_pair_states_time.end();
    m_total_pair_states = total_pair_states;
}

void AutomatonFindDepsMeasure::start_prune_automaton() { m_prune_automaton_time.start(); }

void AutomatonFindDepsMeasure::end_prune_automaton(
    spot::twa_graph_ptr& pruned_automaton) {
    m_prune_automaton_time.end();

    m_total_prune_automaton_states = pruned_automaton->num_states();
    m_prune_automaton_state_based_status =
        pruned_automaton->prop_state_acc().is_true()
            ? "true"
            : (pruned_automaton->prop_state_acc().is_false() ? "false" : "maybe");
}

void AutomatonFindDepsMeasure::get_json_object(json::object& obj) const {
    SyntMeasures::get_json_object(obj);
    obj["algorithm_type"] = "automaton";

    json::object automaton_algo_obj;
    automaton_algo_obj["skipped_dependecies"] = this->m_skipped_dependency_check;
    automaton_algo_obj["type"] = "automaton";
    automaton_algo_obj["total_pair_state"] = this->m_total_pair_states;
    if (this->m_search_pair_states_time.has_started()) {
        automaton_algo_obj["search_pair_state_duration"] =
            this->m_search_pair_states_time.get_duration();
    }
    if (this->m_prune_automaton_time.has_started()) {
        automaton_algo_obj["prune_automaton_duration"] =
            this->m_prune_automaton_time.get_duration();
    }
    automaton_algo_obj["pruned_state_based_status"] =
        this->m_prune_automaton_state_based_status;
    automaton_algo_obj["prune_total_states"] =
        static_cast<int>(this->m_total_prune_automaton_states);

    obj.emplace("algorithm", automaton_algo_obj);
}

void AutomatonSyntMeasure::get_json_object(json::object& obj) const {
    AutomatonFindDepsMeasure::get_json_object(obj);

    json::object synthesis_process_obj;
    if (m_remove_dependent_ap.has_started()) {
        synthesis_process_obj.emplace("remove_dependent_ap_duration",
                                      m_remove_dependent_ap.get_duration());
    }

    synthesis_process_obj.emplace("split_2step_duration", m_split_2step.get_duration());
    synthesis_process_obj.emplace("nba_to_dpa_duration", m_nba_to_dpa.get_duration());
    synthesis_process_obj.emplace("solve_game_duration", m_solve_game.get_duration());
    synthesis_process_obj.emplace("dpa_to_mealy_duration",
                                  m_dpa_to_mealy.get_duration(false));

    obj.emplace("synthesis_process", synthesis_process_obj);
}

ostream& operator<<(ostream& os, const SyntMeasures& sm) {
    json::object obj;
    sm.get_json_object(obj);
    os << json::serialize(obj);

    return os;
}
