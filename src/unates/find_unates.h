#ifndef REACTIVE_SYNTHESIS_BFSS_FIND_UNATES_H
#define REACTIVE_SYNTHESIS_BFSS_FIND_UNATES_H

#include <string>
#include "unate_summary.h"
#include "synt_instance.h"


class FindUnates {
private:
    spot::twa_graph_ptr m_automaton_base, m_automaton_prime;
    SyntInstance& m_synt_instance;

    unsigned m_prime_init_state;
    unsigned m_original_init_state;

    bool is_var_unate_in_state(unsigned state, int varnum, spot::twa_graph_ptr& base_automaton_complement, UnateType unate_type);

    void handle_unate(unsigned state, int varnum, UnateType unate_type);

public:

    explicit FindUnates(const spot::twa_graph_ptr& automaton, SyntInstance& synt_instance);

    void resolve_unates_in_state(unsigned state);
};

#endif
