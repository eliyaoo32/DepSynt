#ifndef REACTIVE_SYNTHESIS_BFSS_FIND_UNATES_H
#define REACTIVE_SYNTHESIS_BFSS_FIND_UNATES_H

#include <string>
#include <spot/twaalgos/synthesis.hh>

#include "nba_utils.h"

class FindUnates {
private:
    spot::twa_graph_ptr m_automaton_base, m_automaton_prime;
    unsigned m_prime_init_state;
public:
    FindUnates(const spot::twa_graph_ptr& automaton);

    bool is_unate_by_state(unsigned state, std::string& var);

    /// \brief Test how many out edges of \a state can be removed if var is positive Unate in \a state.
    int removable_edges_by_state(unsigned state, std::string& var);
};


#endif
