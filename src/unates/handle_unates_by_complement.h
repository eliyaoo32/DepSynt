#ifndef REACTIVE_SYNTHESIS_BFSS_HANDLE_UNATES_BY_COMPLEMENT_H
#define REACTIVE_SYNTHESIS_BFSS_HANDLE_UNATES_BY_COMPLEMENT_H

#include <string>
#include <set>
#include "handle_unates_base.h"
#include "unate_utils.h"
#include "synt_instance.h"

#define COMPLEMENT_MAXIMAL_MULTIPLIER 5

struct UnateEffectOnState {
    set<void*> impacted_edges; // Storing the address of the changed edges
    set<void*> removed_edges; // Storing the address of the changed edges
};

class HandleUnatesByComplement: public HandleUnatesBase {
private:
    // Clone of the original automaton, will be complemented many times on and its initial state will be changed
    spot::twa_graph_ptr m_automaton_base;
    // A prime automaton as described in the algorithm
    spot::twa_graph_ptr m_automaton_prime;

    unsigned m_prime_init_state;
    unsigned m_original_init_state;

    /**
     * TODO: create a struct called `AutomatonSize` which store how many states and edges there are.
     * It can be used in various places, such as, here, in `HandleUnatesByComplement::run()`, in `HandleUnatesByComplement::resolve_unates_in_state()`, `BaseMeasures`
     */
    unsigned m_original_automaton_total_edges;

    bool is_var_unate_in_state(unsigned state, int varnum, spot::twa_graph_ptr& base_automaton_complement, UnateType unate_type);

    void handle_unate_in_state(unsigned state, int varnum, UnateType unate_type, UnateEffectOnState& unate_effect_on_state);

protected:
    void resolve_unates_in_state(unsigned state) override;

public:
    explicit HandleUnatesByComplement(const spot::twa_graph_ptr& automaton, SyntInstance& synt_instance, UnatesHandlerMeasures& unate_measures);
};

#endif
