#ifndef REACTIVE_SYNTHESIS_BFSS_SYNT_MEASURE_H
#define REACTIVE_SYNTHESIS_BFSS_SYNT_MEASURE_H

#include <boost/json.hpp>
#include <iostream>
#include <spot/twa/twa.hh>
#include <vector>

#include "synt_instance.h"
#include "utils.h"
using namespace std;

namespace json = boost::json;

struct TestedVariable {
    string name;
    Duration duration;
    bool is_dependent;
    vector<string> tested_dependency_set;
};

class SyntMeasures {
   private:
    // Automaton data
    bool m_is_automaton_built;
    uint m_total_automaton_states;
    string m_automaton_state_based_status;
    TimeMeasure m_aut_construct_time;

    // Variables data
    TimeMeasure m_variable_test_time;
    string* currently_testing_var;
    vector<TestedVariable> m_tested_variables;

    // Generic data
    TimeMeasure m_total_time;
    SyntInstance& m_synt_instance;
    bool m_is_completed;

   protected:
    virtual void get_json_object(json::object& obj) const;

   public:
    explicit SyntMeasures(SyntInstance& m_synt_instance)
        : m_is_automaton_built(false),
          currently_testing_var(nullptr),
          m_synt_instance(m_synt_instance),
          m_total_automaton_states(-1),
          m_is_completed(false) {
        m_total_time.start();
    }

    ~SyntMeasures() { delete currently_testing_var; }

    void start_automaton_construct() { m_aut_construct_time.start(); }

    void end_automaton_construct(spot::twa_graph_ptr& automaton);

    void start_testing_variable(string& var);

    void end_testing_variable(bool is_dependent, vector<string>& tested_dependency_set);

    void completed() { m_is_completed = true; }

    friend ostream& operator<<(ostream& os, const SyntMeasures& sm);
};

class AutomatonFindDepsMeasure : public SyntMeasures {
   private:
    TimeMeasure m_prune_automaton_time;
    string m_prune_automaton_state_based_status;
    uint m_total_prune_automaton_states;
    TimeMeasure m_search_pair_states_time;
    int m_total_pair_states;
    bool m_skipped_dependency_check;

   protected:
    void get_json_object(json::object& obj) const override;

   public:
    explicit AutomatonFindDepsMeasure(SyntInstance& m_synt_instance,
                                      bool skipped_dependency_check)
        : SyntMeasures(m_synt_instance),
          m_total_pair_states(-1),
          m_total_prune_automaton_states(-1),
          m_skipped_dependency_check(skipped_dependency_check) {}

    void start_search_pair_states();

    void end_search_pair_states(int total_pair_states);

    void start_prune_automaton();

    void end_prune_automaton(spot::twa_graph_ptr& pruned_automaton);
};

class AutomatonSyntMeasure : public AutomatonFindDepsMeasure {
   private:
    TimeMeasure m_remove_dependent_ap;
    TimeMeasure m_split_2step;
    TimeMeasure m_nba_to_dpa;
    TimeMeasure m_solve_game;
    TimeMeasure m_dpa_to_mealy;

   protected:
    void get_json_object(json::object& obj) const override;

   public:
    explicit AutomatonSyntMeasure(SyntInstance& m_synt_instance,
                                  bool skipped_dependency_check)
        : AutomatonFindDepsMeasure(m_synt_instance, skipped_dependency_check) {}

    void start_remove_dependent_ap() { m_remove_dependent_ap.start(); }
    void start_split_2step() { m_split_2step.start(); }
    void start_nba_to_dpa() { m_nba_to_dpa.start(); }
    void start_solve_game() { m_solve_game.start(); }
    void start_dpa_to_mealy() { m_dpa_to_mealy.start(); }

    void end_remove_dependent_ap() { m_remove_dependent_ap.end(); }
    void end_split_2step() { m_split_2step.end(); }
    void end_nba_to_dpa() { m_nba_to_dpa.end(); }
    void end_solve_game() { m_solve_game.end(); }
    void end_dpa_to_mealy() { m_dpa_to_mealy.end(); }
};

#endif  // REACTIVE_SYNTHESIS_BFSS_SYNT_MEASURE_H
