#include "find_unates.h"
#include <spot/twaalgos/contains.hh>
#include <spot/twaalgos/complement.hh>

FindUnates::FindUnates(const spot::twa_graph_ptr& automaton) {
    // TODO: check if I can avoid clone the base automaton - and make sure I restore the original init state
    m_automaton_base = clone_nba(automaton);

    // Create prime automaton
    m_automaton_prime = clone_nba(automaton);
    m_prime_init_state = m_automaton_prime->new_state();
    m_automaton_prime->set_init_state(m_prime_init_state);
}

bool FindUnates::is_unate_by_state(unsigned state, std::string& var) {
    // Set init state
    m_automaton_base->set_init_state(state);

    // Set up prime init state
    m_automaton_prime->kill_state(m_prime_init_state);

    // Create common edge
    int var_num = m_automaton_base->register_ap(var);
    for(auto& edge : m_automaton_base->out(state)) {
        bdd cond = bdd_restrict(edge.cond, bdd_nithvar(var_num)) & bdd_ithvar(var_num);
        if(cond != bddfalse) {
            m_automaton_prime->new_edge(m_prime_init_state, edge.dst, cond, edge.acc);
        }
    }

    bool is_unate = contains(m_automaton_base, m_automaton_prime);
    return is_unate;

}
