#ifndef REACTIVE_SYNTHESIS_BFSS_HANDLE_UNATES_BY_HEURISTIC_H
#define REACTIVE_SYNTHESIS_BFSS_HANDLE_UNATES_BY_HEURISTIC_H

#include "handle_unates_base.h"

class HandleUnatesByHeuristic : public HandleUnatesBase {
public:
    explicit HandleUnatesByHeuristic(const spot::twa_graph_ptr& automaton, SyntInstance& synt_instance, UnatesHandlerMeasures& unate_measures)
        : HandleUnatesBase(automaton, synt_instance, unate_measures) {};

    void run();
};


#endif //REACTIVE_SYNTHESIS_BFSS_HANDLE_UNATES_BY_HEURISTIC_H
