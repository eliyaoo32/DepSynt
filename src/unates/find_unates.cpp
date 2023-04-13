#include <algorithm>
#include <vector>
#include <spot/twaalgos/complement.hh>

#include "nba_utils.h"
#include "find_unates.h"

using namespace std;


FindUnates::FindUnates(const spot::twa_graph_ptr& automaton, SyntInstance& synt_instance) : m_synt_instance(synt_instance) {
    m_automaton_base = automaton;
    m_original_init_state = automaton->get_init_state_number();

    // Create prime automaton
    m_automaton_prime = clone_nba(automaton);
    m_prime_init_state = m_automaton_prime->new_state();
    m_automaton_prime->set_init_state(m_prime_init_state);
}


bool FindUnates::resolve_unates_in_state(unsigned state) {
    // Update automaton init state
    m_automaton_base->set_init_state(state);

    // Find complement of the automaton
    spot::twa_graph_ptr complement;
    if(state == m_original_init_state) {
        complement = construct_automaton_negation(m_synt_instance, m_automaton_base->get_dict());
    } else {
        // TODO: add timeout
        complement = spot::complement(m_automaton_base);
    }

    // Check for Unate for all variables
    vector<string> untested_vars( m_synt_instance.get_output_vars() );
    vector<string> tested_vars;

    while(!untested_vars.empty()) {
        string var = untested_vars.back();
        untested_vars.pop_back();

        int varnum = m_automaton_base->register_ap(var);
        if(is_var_unate_in_state(state, varnum, complement)) {
            this->handle_unate(state, varnum);

            // Retesting all the already tested variables
            untested_vars.insert(untested_vars.end(), tested_vars.begin(), tested_vars.end());
            tested_vars.clear();
        }

        tested_vars.push_back(var);
    }

    // Restore automaton init state
    m_automaton_base->set_init_state(m_original_init_state);

    // Restore prime automaton
    m_automaton_prime->kill_state(m_prime_init_state);
}

bool FindUnates::is_var_unate_in_state(unsigned state, int varnum, spot::twa_graph_ptr& base_automaton_complement) {
    // Create the prime state in prime automaton
    m_automaton_prime->kill_state(m_prime_init_state);

    for(auto& edge : m_automaton_base->out(state)) {
        // For each edge with BDD Ψ_i, we create the BDD: β_i = [Ψ_i(var=0) & Var]
        bdd cond = bdd_restrict(edge.cond, bdd_nithvar(varnum)) & bdd_ithvar(varnum);
        if(cond != bddfalse) {
            m_automaton_prime->new_edge(m_prime_init_state, edge.dst, cond, edge.acc);
        }
    }

    // TODO: (optimization) add sufficient conditions for unate variables

    bool is_unate = !base_automaton_complement->intersects(m_automaton_prime);
    return is_unate;
}

void FindUnates::handle_unate(unsigned state, int varnum) {
    auto var_bdd = bdd_ithvar(varnum);

    for(auto& edge : m_automaton_base->out(state)) {
        edge.cond = bdd_restrict(edge.cond, var_bdd) & var_bdd;
    }
}
