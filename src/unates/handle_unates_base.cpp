#include "nba_utils.h"
#include "handle_unates_base.h"
#include "bdd_var_cacher.h"


void HandleUnatesBase::run() {
    m_unate_measures.start();

    for(unsigned state = 0; state < m_automaton->num_states(); state++) {
        // TODO: Move start testing state and end testing test to the run function
        this->resolve_unates_in_state(state);
        // TODO: move handle_unate_state to the run function
    }

    // Clean the automaton after Unate
    /**
     * purge_dead_states
     *  - (1) Remove all the states that cannot be part of an infinite run of the automaton
     *  - (2) Transition labeled by bddfalse are also removed
     */
    m_unate_measures.start_postprocess_automaton();
    m_automaton->purge_dead_states();
    m_unate_measures.end_postprocess_automaton();

    // Report automaton size
    int total_states = static_cast<int>(m_automaton->num_states());
    int total_edges = count_edges(m_automaton);

    m_unate_measures.end(total_edges, total_states);
}

void HandleUnatesBase::handle_unate_in_state(unsigned state, int varnum, UnateType unate_type, UnateEffectOnState& unate_effect_on_state) {
    auto var_bdd = (unate_type == UnateType::Positive)
                   ? bdd_ithvar(varnum)
                   : bdd_nithvar(varnum);

    // Update the original automaton
    for(auto& edge : m_automaton->out(state)) {
        // If variable is positive Unate, we transform the edge e: [∃x(e) ^ x] (Negative Unate is [∃x(e) ^ ~x])
        bdd cond_before = edge.cond;
        edge.cond = bdd_exist(edge.cond, bdd_ithvar(varnum)) & var_bdd;

        bool is_impacted = edge.cond != cond_before;
        if(is_impacted) {
            unate_effect_on_state.impacted_edges.insert(&edge);
        }
    }
}
