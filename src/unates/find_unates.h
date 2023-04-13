#ifndef REACTIVE_SYNTHESIS_BFSS_FIND_UNATES_H
#define REACTIVE_SYNTHESIS_BFSS_FIND_UNATES_H

#include <string>
#include "synt_instance.h"


class FindUnates {
private:
    spot::twa_graph_ptr m_automaton_base, m_automaton_prime;
    SyntInstance& m_synt_instance;

    unsigned m_prime_init_state;
    unsigned m_original_init_state;
public:
    explicit FindUnates(const spot::twa_graph_ptr& automaton, SyntInstance& synt_instance);

    bool handle_unates_in_state(unsigned state);

    bool is_var_unate_in_state(unsigned state, int varnum, spot::twa_graph_ptr& base_automaton_complement);

    void handle_unate(unsigned state, int varnum);
}

#endif
