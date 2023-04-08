#ifndef REACTIVE_SYNTHESIS_BFSS_FIND_UNATES_H
#define REACTIVE_SYNTHESIS_BFSS_FIND_UNATES_H

#include <string>
#include <spot/twaalgos/synthesis.hh>

#include "nba_utils.h"

class FindUnates {
private:
    spot::twa_graph_ptr automaton_base, automaton_prime;

    unsigned prime_init_state;
public:
    FindUnates(const spot::twa_graph_ptr& automaton) {
        automaton_base = clone_nba(automaton);
        automaton_prime = clone_nba(automaton);

//        original_init_state = automaton_base->get_init_state_number();
        prime_init_state = automaton_prime->new_state();
        automaton_prime->set_init_state(prime_init_state);
    }

    bool is_unate_by_state(unsigned state, std::string var) {
        // Set init state
        automaton_base->set_init_state(state);

        // Set up prime init state
        automaton_prime->kill_state(prime_init_state);

        // Create common edge
        int var_num = automaton_base->register_ap(var);
        bdd beta = bddtrue;
        for(auto& x : automaton_base->out(state)) {
            beta |= bdd_restrict(x.cond, bdd_nithvar(var_num));
        }
        beta &= bdd_ithvar(var_num);

        // Add edges to prime state
        for(auto& x : automaton_base->out(state)) {
            automaton_prime->new_edge(prime_init_state, x.dst, x.cond & beta, x.acc);
        }

        // Check if prime is sub-language of base
        bool is_unate = !automaton_prime->intersects(automaton_base_complement);
        return is_unate;
    }
};


#endif
