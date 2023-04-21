#include "synt_measure.h"
#include "nba_utils.h"

#include <fstream>

void aiger_description_obj(json::object& obj, const AigerDescription& description) {
    obj.emplace("total_inputs", description.inputs);
    obj.emplace("total_outputs", description.outputs);
    obj.emplace("total_latches", description.latches);
    obj.emplace("total_gates", description.gates);
}

void extract_aiger_description(AigerDescription& description_dst,
                               spot::aig_ptr& aiger) {
    description_dst.gates = aiger->num_gates();
    description_dst.latches = aiger->num_latches();
    description_dst.outputs = aiger->num_outputs();
    description_dst.inputs = aiger->num_inputs();
}

void BaseMeasures::end_automaton_construct(spot::twa_graph_ptr& automaton) {
    m_aut_construct_time.end();
    m_is_automaton_built = true;

    m_total_automaton_states = automaton->num_states();
    m_automaton_state_based_status =
        automaton->prop_state_acc().is_true()
            ? "true"
            : (automaton->prop_state_acc().is_false() ? "false" : "maybe");
}

void BaseDependentsMeasures::start_testing_variable(string& var) {
    m_variable_test_time.start();
    currently_testing_var = new string(var);
}

void BaseDependentsMeasures::end_testing_variable(bool is_dependent,
                                                  vector<string>& tested_dependency_set) {
    m_variable_test_time.end();

    m_tested_variables.push_back({*currently_testing_var,
                                  m_variable_test_time.get_duration(), is_dependent,
                                  tested_dependency_set});
    delete currently_testing_var;
    currently_testing_var = nullptr;
}

void BaseMeasures::get_json_object(json::object& obj) const {
    // General information
    json::array output_vars;
    std::transform(m_synt_instance.get_output_vars().begin(),
                   m_synt_instance.get_output_vars().end(),
                   std::back_inserter(output_vars),
                   [](const std::string& var) { return json::string(var); });
    json::array input_vars;
    std::transform(m_synt_instance.get_input_vars().begin(),
                   m_synt_instance.get_input_vars().end(),
                   std::back_inserter(input_vars),
                   [](const std::string& var) { return json::string(var); });

    obj.emplace("is_completed", m_is_completed);
    obj.emplace("output_vars", output_vars);
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
    if (this->m_prune_automaton_time.has_started()) {
        automaton["prune_automaton_duration"] =
                this->m_prune_automaton_time.get_duration();
    }
    automaton["pruned_state_based_status"] =
            this->m_prune_automaton_state_based_status;
    automaton["prune_total_states"] =
            static_cast<int>(this->m_total_prune_automaton_states);
    automaton["total_edges"] = this->m_total_automaton_edges;

    obj.emplace("automaton", automaton);

}

void BaseDependentsMeasures::get_json_object(json::object& obj) const {
    BaseMeasures::get_json_object(obj);
    obj.emplace("algorithm_type", "formula");

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
    obj.emplace("tested_dependencies", tested_vars);
}

void FindUnatesMeasures::get_json_object(json::object& obj) const {
    BaseMeasures::get_json_object(obj);
    UnatesHandlerMeasures::get_json_object(obj);
}

void AutomatonFindDepsMeasure::start_search_pair_states() {
    m_search_pair_states_time.start();
}

void AutomatonFindDepsMeasure::end_search_pair_states(int total_pair_states) {
    m_search_pair_states_time.end();
    m_total_pair_states = total_pair_states;
}

void BaseMeasures::start_prune_automaton() {
    m_prune_automaton_time.start();
}

void BaseMeasures::end_prune_automaton(
    spot::twa_graph_ptr& pruned_automaton) {
    m_prune_automaton_time.end();

    m_total_prune_automaton_states = pruned_automaton->num_states();
    m_prune_automaton_state_based_status =
        pruned_automaton->prop_state_acc().is_true()
            ? "true"
            : (pruned_automaton->prop_state_acc().is_false() ? "false" : "maybe");

    m_total_automaton_edges = count_edges(pruned_automaton);
}

void AutomatonFindDepsMeasure::get_json_object(json::object& obj) const {
    BaseDependentsMeasures::get_json_object(obj);
    obj["algorithm_type"] = "automaton";

    json::object automaton_algo_obj;

    automaton_algo_obj["total_duration"] =
        this->m_total_find_deps_duration.get_duration(false);
    automaton_algo_obj["skipped_dependencies"] = this->m_skipped_dependency_check;
    automaton_algo_obj["total_pair_state"] = this->m_total_pair_states;
    if (this->m_search_pair_states_time.has_started()) {
        automaton_algo_obj["search_pair_state_duration"] =
            this->m_search_pair_states_time.get_duration();
    }

    obj.emplace("dependencies", automaton_algo_obj);
}

void SynthesisMeasure::end_independents_synthesis(spot::aig_ptr& aiger_strat) {
    if (aiger_strat != nullptr) {
        extract_aiger_description(m_independent_strategy, aiger_strat);
        m_independents_realizable = "REALIZABLE";
    } else {
        m_independents_realizable = "UNREALIZABLE";
    }

    m_independents_total_duration.end();
}

void SynthesisMeasure::get_json_object(json::object& obj) const {
    AutomatonFindDepsMeasure::get_json_object(obj);
    obj.emplace("algorithm_type", "aiger_synthesis");

    // Unate
    json::object unate_obj;
    unate_obj.emplace("skipped_unate", m_skipped_unate);
    if(!m_skipped_unate) {
        UnatesHandlerMeasures::get_json_object(unate_obj);
    }
    obj.emplace("unate", unate_obj);

    // Synthesis process
    json::object synthesis_process_obj;

    // Report remove depenedents
    if (m_remove_dependent_ap.has_started()) {
        synthesis_process_obj.emplace("remove_dependent_ap_duration",
                                      m_remove_dependent_ap.get_duration());
    }
    if (m_clone_nba_with_deps.has_started()) {
        synthesis_process_obj.emplace("clone_nba_with_dep",
                                      m_clone_nba_with_deps.get_duration());
    }
    if (m_clone_nba_without_deps.has_started()) {
        synthesis_process_obj.emplace("clone_nba_without_dep",
                                      m_clone_nba_without_deps.get_duration());
    }
    if (m_dependents_total_duration.has_started()) {
        synthesis_process_obj.emplace("synthesis_dependents_duration",
                                      m_dependents_total_duration.get_duration());
    }
    synthesis_process_obj.emplace("model_checking_status", m_model_checking_status);
    if (m_model_checking.has_started()) {
        synthesis_process_obj.emplace("model_checking_duration",
                                      m_model_checking.get_duration());
    }
    if(m_merge_strategies.has_started()) {
        synthesis_process_obj.emplace("merge_strategies_duration",
                                      m_merge_strategies.get_duration());
    }

    json::object independent_strategy_obj, dependent_strategy_obj;

    independent_strategy_obj.emplace("duration",
                                     m_independents_total_duration.get_duration());
    independent_strategy_obj.emplace("realizability", m_independents_realizable);
    aiger_description_obj(independent_strategy_obj, m_independent_strategy);

    dependent_strategy_obj.emplace("duration",
                                   m_dependents_total_duration.get_duration());
    aiger_description_obj(dependent_strategy_obj, m_dependent_strategy);

    if(m_merge_strategies.has_started()) {
        json::object final_strategy_obj;
        final_strategy_obj.emplace("merge_duration",
                                       m_merge_strategies.get_duration());
        aiger_description_obj(final_strategy_obj, m_final_strategy);
        synthesis_process_obj.emplace("final_strategy", dependent_strategy_obj);
    }

    synthesis_process_obj.emplace("independent_strategy", independent_strategy_obj);
    synthesis_process_obj.emplace("dependent_strategy", dependent_strategy_obj);

    obj.emplace("synthesis", synthesis_process_obj);
}

ostream& operator<<(ostream& os, const BaseMeasures& sm) {
    json::object obj;
    sm.get_json_object(obj);
    os << json::serialize(obj);

    return os;
}

void dump_measures(const BaseDependentsMeasures& sm, BaseCLIOptions& cli_options) {
    if (cli_options.measures_path.empty()) {
        cout << sm << endl;
        return;
    }

    ofstream measure_file(cli_options.measures_path);

    if (measure_file.is_open()) {
        measure_file << sm << endl;
        measure_file.close();

        cout << "Measures written to file: " << cli_options.measures_path << endl;
    } else {
        cerr << "Failed to open fail: " << cli_options.measures_path << endl;
        cout << sm << endl;
    }
}
