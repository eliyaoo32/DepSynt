#ifndef REACTIVE_SYNTHESIS_BFSS_HANDLE_UNATES_BASE_H
#define REACTIVE_SYNTHESIS_BFSS_HANDLE_UNATES_BASE_H

#include "unate_utils.h"
#include "synt_instance.h"

class HandleUnatesBase {
protected:
    spot::twa_graph_ptr m_automaton;
    SyntInstance& m_synt_instance;
    UnatesHandlerMeasures& m_unate_measures;

protected:
    virtual void resolve_unates_in_state(unsigned state) = 0;

    virtual void handle_unate_in_state(unsigned state, int varnum, UnateType unate_type, UnateEffectOnState& unate_effect_on_state);
public:
    explicit HandleUnatesBase(const spot::twa_graph_ptr& automaton, SyntInstance& synt_instance, UnatesHandlerMeasures& unate_measures, const char* algo_name)
    : m_synt_instance(synt_instance), m_unate_measures(unate_measures) {
        m_automaton = automaton;
        m_unate_measures.set_algorithm_name(algo_name);
    }

    void run();
};

#endif //REACTIVE_SYNTHESIS_BFSS_HANDLE_UNATES_BASE_H
