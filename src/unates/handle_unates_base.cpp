#include "nba_utils.h"
#include "handle_unates_base.h"

void HandleUnatesBase::run() {
    m_unate_measures.start();

    for(unsigned state = 0; state < m_automaton->num_states(); state++) {
        this->resolve_unates_in_state(state);
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

