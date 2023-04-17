#ifndef REACTIVE_SYNTHESIS_BFSS_SYNT_MEASURE_H
#define REACTIVE_SYNTHESIS_BFSS_SYNT_MEASURE_H

#include <boost/json.hpp>
#include <iostream>
#include <spot/twa/twa.hh>
#include <spot/twaalgos/aiger.hh>
#include <vector>

#include "synt_instance.h"
#include "utils.h"
#include "unate_utils.h"

using namespace std;

namespace json = boost::json;

struct TestedVariable {
    string name;
    Duration duration;
    bool is_dependent;
    vector<string> tested_dependency_set;
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

struct AigerDescription {
    int inputs = -1;
    int outputs = -1;
    int latches = -1;
    int gates = -1;
};

class BaseDependentsMeasures;

void dump_measures(const BaseDependentsMeasures &sm, BaseCLIOptions &cli_options);

void extract_aiger_description(AigerDescription &description_dst,
                               spot::aig_ptr &aiger);

class BaseMeasures {
private:
    // Automaton data
    bool m_is_automaton_built;
    uint m_total_automaton_states;
    string m_automaton_state_based_status;
    TimeMeasure m_aut_construct_time;

    // Generic data
    TimeMeasure m_total_time;
    SyntInstance &m_synt_instance;
    bool m_is_completed;

protected:
    virtual void get_json_object(json::object &obj) const;

public:
    explicit BaseMeasures(SyntInstance &m_synt_instance)
            : m_is_automaton_built(false),
              m_synt_instance(m_synt_instance),
              m_total_automaton_states(-1),
              m_is_completed(false) {
        m_total_time.start();
    }

    void start_automaton_construct() { m_aut_construct_time.start(); }

    void end_automaton_construct(spot::twa_graph_ptr &automaton);

    void completed() { m_is_completed = true; }

    friend ostream &operator<<(ostream &os, const BaseMeasures &sm);
};

class FindUnatesMeasures : public BaseMeasures {
private:
    // Variables data
    TimeMeasure m_variable_test_time;
    TimeMeasure m_state_test_time;
    TimeMeasure m_complement_time;

    string currently_testing_var;
    unsigned currently_testing_state;

    vector<TestedState> tested_states;
    vector<string> positive_unate;
    vector<string> negative_unate;
    vector<string> not_unate;

protected:
    void get_json_object(json::object &obj) const override;

public:
    explicit FindUnatesMeasures(SyntInstance &m_synt_instance)
            : BaseMeasures(m_synt_instance),
              currently_testing_var("") {}

    void start_testing_state(unsigned state) {
        currently_testing_state = state;
        m_state_test_time.start();
    }

    void end_testing_state() {
        m_state_test_time.end();

        tested_states.emplace_back(TestedState{
                .state = currently_testing_state,
                .total_duration = m_state_test_time.get_duration(),
                .complement_duration = m_complement_time.get_duration(),
                .positive_unate_variables = positive_unate,
                .negative_unate_variables = negative_unate,
                .not_unate_variables = not_unate,
                .removed_edges = 0,
                .impacted_edges = 0
        });

        positive_unate.clear();
        negative_unate.clear();
        not_unate.clear();
        currently_testing_state = -1;
    }

    void start_testing_var(string &var) {
        currently_testing_var = var;
        m_variable_test_time.start();
    }

    void tested_var_unate(UnateType unate_type) {
        if (unate_type == UnateType::Negative) {
            negative_unate.push_back(currently_testing_var);
        } else if (unate_type == UnateType::Positive) {
            positive_unate.push_back(currently_testing_var);
        }

        currently_testing_var = "";
        m_variable_test_time.start();
    }

    void tested_var_not_unate() {
        not_unate.push_back(currently_testing_var);

        currently_testing_var = "";
        m_variable_test_time.start();
    }

    void start_automaton_complement() {
        m_complement_time.start();
    }

    void end_automaton_complement() {
        m_complement_time.end();
    }
};

class BaseDependentsMeasures : public BaseMeasures {
private:
    // Variables data
    TimeMeasure m_variable_test_time;
    string *currently_testing_var;
    vector<TestedVariable> m_tested_variables;

protected:
    void get_json_object(json::object &obj) const override;

public:
    explicit BaseDependentsMeasures(SyntInstance &m_synt_instance)
            : BaseMeasures(m_synt_instance),
              currently_testing_var(nullptr) {}

    ~BaseDependentsMeasures() { delete currently_testing_var; }

    void start_testing_variable(string &var);

    void end_testing_variable(bool is_dependent,
                              vector<string> &tested_dependency_set);
};

class AutomatonFindDepsMeasure : public BaseDependentsMeasures {
private:
    TimeMeasure m_prune_automaton_time;
    string m_prune_automaton_state_based_status;
    uint m_total_prune_automaton_states;
    TimeMeasure m_search_pair_states_time;
    int m_total_pair_states;
    bool m_skipped_dependency_check;
    TimeMeasure m_total_find_deps_duration;

protected:
    void get_json_object(json::object &obj) const override;

public:
    explicit AutomatonFindDepsMeasure(SyntInstance &m_synt_instance,
                                      bool skipped_dependency_check)
            : BaseDependentsMeasures(m_synt_instance),
              m_total_pair_states(-1),
              m_total_prune_automaton_states(-1),
              m_skipped_dependency_check(skipped_dependency_check) {}

    void start_find_deps() { m_total_find_deps_duration.start(); }

    void end_find_deps() { m_total_find_deps_duration.end(); }

    void start_search_pair_states();

    void end_search_pair_states(int total_pair_states);

    void start_prune_automaton();

    void end_prune_automaton(spot::twa_graph_ptr &pruned_automaton);
};

class SynthesisMeasure : public AutomatonFindDepsMeasure {
private:
    TimeMeasure m_remove_dependent_ap;
    TimeMeasure m_independents_total_duration;
    TimeMeasure m_dependents_total_duration;
    TimeMeasure m_clone_nba_with_deps;
    TimeMeasure m_clone_nba_without_deps;
    TimeMeasure m_model_checking;
    TimeMeasure m_merge_strategies;

    AigerDescription m_independent_strategy;
    AigerDescription m_dependent_strategy;
    AigerDescription m_final_strategy;

    string m_independents_realizable;
    string m_model_checking_status;

protected:
    void get_json_object(json::object &obj) const override;

public:
    explicit SynthesisMeasure(SyntInstance &m_synt_instance,
                              bool skipped_dependency_check)
            : AutomatonFindDepsMeasure(m_synt_instance, skipped_dependency_check),
              m_independents_realizable("UNKNOWN"),
              m_model_checking_status("UNKNOWN") {}

    void start_remove_dependent_ap() { m_remove_dependent_ap.start(); }

    void end_remove_dependent_ap() { m_remove_dependent_ap.end(); }

    void start_clone_nba_with_deps() { m_clone_nba_with_deps.start(); }

    void end_clone_nba_with_deps() { m_clone_nba_with_deps.end(); }

    void start_clone_nba_without_deps() { m_clone_nba_without_deps.start(); }

    void end_clone_nba_without_deps() { m_clone_nba_without_deps.end(); }

    void start_model_checking() { m_model_checking.start(); }

    void end_model_checking(const char *status) {
        m_model_checking.end();
        m_model_checking_status = status;
    }

    void start_independents_synthesis() { m_independents_total_duration.start(); }

    void end_independents_synthesis(spot::aig_ptr &aiger_strat);

    void start_dependents_synthesis() { m_dependents_total_duration.start(); }

    void end_dependents_synthesis(spot::aig_ptr &aiger_strat) {
        if (aiger_strat != nullptr) {
            extract_aiger_description(m_dependent_strategy, aiger_strat);
        }
        m_dependents_total_duration.end();
    }

    void start_merge_strategies() { m_merge_strategies.start(); }

    void end_merge_strategies(spot::aig_ptr &aiger_strat) {
        m_merge_strategies.end();
        extract_aiger_description(m_final_strategy, aiger_strat);
    }
};

#endif  // REACTIVE_SYNTHESIS_BFSS_SYNT_MEASURE_H
