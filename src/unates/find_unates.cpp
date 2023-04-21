#include <algorithm>
#include <vector>
#include <spot/twaalgos/complement.hh>

#include "nba_utils.h"
#include "bdd_var_cacher.h"
#include "find_unates.h"

using namespace std;

FindUnates::FindUnates(const spot::twa_graph_ptr& automaton, SyntInstance& synt_instance, UnatesHandlerMeasures& unate_measures)
    : m_synt_instance(synt_instance), m_unate_measures(unate_measures) {
    m_automaton_original = automaton;
    m_automaton_base = clone_nba(automaton);
    m_original_init_state = automaton->get_init_state_number();
    m_original_automaton_total_edges = count_edges(automaton);

    // Create prime automaton
    m_automaton_prime = clone_nba(automaton);

    m_prime_init_state = m_automaton_prime->new_state();
    m_automaton_prime->set_init_state(m_prime_init_state);
}

void FindUnates::run() {
    m_unate_measures.start();

    for(unsigned state = 0; state < m_automaton_original->num_states(); state++) {
        this->resolve_unates_in_state(state);
    }

    // Report automaton size
    int total_states = static_cast<int>(m_automaton_original->num_states());
    int total_edges = count_edges(m_automaton_original);

    m_unate_measures.end(total_edges, total_states);
}


void FindUnates::resolve_unates_in_state(unsigned state) {
    m_unate_measures.start_testing_state(state);

    // Update automaton init state
    m_automaton_base->set_init_state(state);

    ////////////// Find complement of the automaton
    m_unate_measures.start_automaton_complement();
    spot::twa_graph_ptr complement;
    if(state == m_original_init_state) {
        complement = construct_automaton_negation(m_synt_instance, m_automaton_base->get_dict());
    } else {
        spot::output_aborter complement_aborter(
            m_automaton_base->num_states() * COMPLEMENT_MAXIMAL_MULTIPLIER,
            m_original_automaton_total_edges * COMPLEMENT_MAXIMAL_MULTIPLIER
        );
        complement = spot::complement(m_automaton_base, &complement_aborter);

        if(!complement) {
            m_automaton_base->set_init_state(m_original_init_state);
            m_automaton_prime->kill_state(m_prime_init_state);
            m_unate_measures.end_automaton_complement();
            m_unate_measures.failed_complement();
            return;
        }
    }
    m_unate_measures.end_automaton_complement();

    ////////////// Check for Unate in all variables
    vector<string> untested_vars( m_synt_instance.get_output_vars() );
    vector<string> not_unate_vars;
    UnateEffectOnState unate_effect_on_state;

    while(!untested_vars.empty()) {
        string var = untested_vars.back();
        untested_vars.pop_back();

        m_unate_measures.start_testing_var(var);
        int varnum = m_automaton_base->register_ap(var);

        if(is_var_unate_in_state(state, varnum, complement, UnateType::Positive)) {
            this->handle_unate(state, varnum, UnateType::Positive, unate_effect_on_state);

            // Retesting all the already tested variables
            untested_vars.insert(untested_vars.end(), not_unate_vars.begin(), not_unate_vars.end());
            not_unate_vars.clear();

            // Report var result
            m_unate_measures.tested_var_unate(UnateType::Positive);
        } else if(is_var_unate_in_state(state, varnum, complement, UnateType::Negative)) {
            this->handle_unate(state, varnum, UnateType::Negative, unate_effect_on_state);

            // Retesting all the already tested variables
            untested_vars.insert(untested_vars.end(), not_unate_vars.begin(), not_unate_vars.end());
            not_unate_vars.clear();

            // Report var result
            m_unate_measures.tested_var_unate(UnateType::Negative);
        } else {
            not_unate_vars.push_back(var);

            // Report var result
            m_unate_measures.tested_var_not_unate();
        }
    }

    // Restore automaton init state
    m_automaton_base->set_init_state(m_original_init_state);

    // Restore prime automaton
    m_automaton_prime->kill_state(m_prime_init_state);

    m_unate_measures.end_testing_state(
static_cast<int>(unate_effect_on_state.removed_edges.size()),
static_cast<int>(unate_effect_on_state.impacted_edges.size())
    );
}

bool FindUnates::is_var_unate_in_state(unsigned state, int varnum, spot::twa_graph_ptr& base_automaton_complement, UnateType unate_type) {
    // Create the prime state in prime automaton
    m_automaton_prime->kill_state(m_prime_init_state);

    for(auto& edge : m_automaton_base->out(state)) {
        /**
         *  For each edge with BDD Ψ_i, we create a BDD β_i:
         *   - If unate type is positive: β_i = [Ψ_i(var=0) & Var]
         *   - If unate type is negative: β_i = [Ψ_i(var=1) & !Var]
         */
        bdd cond = (unate_type == UnateType::Positive)
                ? bdd_restrict(edge.cond, bdd_nithvar(varnum)) & bdd_ithvar(varnum)
                : bdd_restrict(edge.cond, bdd_ithvar(varnum)) & bdd_nithvar(varnum);

        if(cond != bddfalse) {
            m_automaton_prime->new_edge(m_prime_init_state, edge.dst, cond, edge.acc);
        }
    }

    bool is_unate = !base_automaton_complement->intersects(m_automaton_prime);
    return is_unate;
}

void FindUnates::handle_unate(unsigned state, int varnum, UnateType unate_type, UnateEffectOnState& unate_effect_on_state) {
    auto var_bdd = (unate_type == UnateType::Positive)
            ? bdd_ithvar(varnum)
            : bdd_nithvar(varnum);

    // Update the automaton base
    for(auto& edge : m_automaton_base->out(state)) {
        // If variable is positive Unate, and the edge could have an assigment to false, then the edge is impacted (Negative Unate is effected correspondingly)
        bool is_impacted = can_restrict_variable(edge.cond, varnum, unate_type == UnateType::Positive ? false : true);
        edge.cond = bdd_restrict(edge.cond, var_bdd) & var_bdd;

        if(edge.cond == bddfalse) { // I.e. the edge was removed
            unate_effect_on_state.removed_edges.insert(&edge);
        }
        if(is_impacted) {
            unate_effect_on_state.impacted_edges.insert(&edge);
        }
    }

    // Update the original automaton
    for(auto& edge : m_automaton_original->out(state)) {
        edge.cond = bdd_restrict(edge.cond, var_bdd) & var_bdd;
    }
}

