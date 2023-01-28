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

    void end_testing_variable(bool is_dependent,
                              vector<string>& tested_dependency_set);

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
    TimeMeasure m_total_find_deps_duration;

   protected:
    void get_json_object(json::object& obj) const override;

   public:
    explicit AutomatonFindDepsMeasure(SyntInstance& m_synt_instance,
                                      bool skipped_dependency_check)
        : SyntMeasures(m_synt_instance),
          m_total_pair_states(-1),
          m_total_prune_automaton_states(-1),
          m_skipped_dependency_check(skipped_dependency_check) {}

    void start_find_deps() { m_total_find_deps_duration.start(); }

    void end_find_deps() { m_total_find_deps_duration.end(); }

    void start_search_pair_states();

    void end_search_pair_states(int total_pair_states);

    void start_prune_automaton();

    void end_prune_automaton(spot::twa_graph_ptr& pruned_automaton);
};

class SynthesisMeasure : public AutomatonFindDepsMeasure {
   private:
    TimeMeasure m_remove_dependent_ap;
    TimeMeasure m_independents_total_duration;
    TimeMeasure m_dependents_total_duration;

    string m_independents_realizable;

   protected:
    void get_json_object(json::object& obj) const override;

   public:
    explicit SynthesisMeasure(SyntInstance& m_synt_instance,
                              bool skipped_dependency_check)
        : AutomatonFindDepsMeasure(m_synt_instance, skipped_dependency_check),
          m_independents_realizable("UNKNOWN") {}

    void start_remove_dependent_ap() { m_remove_dependent_ap.start(); }
    void end_remove_dependent_ap() { m_remove_dependent_ap.end(); }

    void start_independents_synthesis() { m_independents_total_duration.start(); }
    void end_independents_synthesis() { m_independents_total_duration.end(); }

    void start_dependents_synthesis() { m_dependents_total_duration.start(); }
    void end_dependents_synthesis() { m_dependents_total_duration.end(); }

    void set_independents_realizability(const char* realizability) {
        m_independents_realizable = realizability;
    }
};

#endif  // REACTIVE_SYNTHESIS_BFSS_SYNT_MEASURE_H
