#ifndef REACTIVE_SYNTHESIS_BFSS_HANDLE_UNATES_BASE_H
#define REACTIVE_SYNTHESIS_BFSS_HANDLE_UNATES_BASE_H

#include "unate_utils.h"
#include "synt_instance.h"

class HandleUnatesBase {
protected:
    spot::twa_graph_ptr m_automaton;
    SyntInstance& m_synt_instance;
    UnatesHandlerMeasures& m_unate_measures;
public:
    explicit HandleUnatesBase(const spot::twa_graph_ptr& automaton, SyntInstance& synt_instance, UnatesHandlerMeasures& unate_measures)
    : m_synt_instance(synt_instance), m_unate_measures(unate_measures) {
        m_automaton = automaton;
    }

    virtual void run() = 0;
};

#endif //REACTIVE_SYNTHESIS_BFSS_HANDLE_UNATES_BASE_H
