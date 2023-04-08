#include "find_unates.h"
#include <spot/twaalgos/contains.hh>

FindUnates::FindUnates(const spot::twa_graph_ptr& automaton) {
    // TODO: check if I can avoid clone the base automaton - and make sure I restore the original init state
    m_automaton_base = clone_nba(automaton);

    // Create prime automaton
    m_automaton_prime = clone_nba(automaton);
    m_prime_init_state = m_automaton_prime->new_state();
    m_automaton_prime->set_init_state(m_prime_init_state);
}

bool FindUnates::is_unate_by_state(unsigned state, std::string var) {
    // Set init state
    m_automaton_base->set_init_state(state);

    // Set up prime init state
    m_automaton_prime->kill_state(m_prime_init_state);

    // Create common edge
    int var_num = m_automaton_base->register_ap(var);
    bdd beta = bddtrue;
    for(auto& x : m_automaton_base->out(state)) {
        beta |= bdd_restrict(x.cond, bdd_nithvar(var_num));
    }
    beta &= bdd_ithvar(var_num);

    // Add edges to prime state
    for(auto& x : m_automaton_base->out(state)) {
        m_automaton_prime->new_edge(m_prime_init_state, x.dst, x.cond & beta, x.acc);
    }

    // Check if prime is sub-language of base
//    auto automaton_base_complement = spot::complement(m_automaton_base);
//    bool is_unate = !m_automaton_prime->intersects(automaton_base_complement);
    bool is_unate = contains(m_automaton_base, m_automaton_prime);
    return is_unate;
}
