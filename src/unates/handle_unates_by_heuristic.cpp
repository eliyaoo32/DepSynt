#include "handle_unates_by_heuristic.h"


/**
 * * Denote β_i to the i-th edge of the state in the automaton
 * * β_i(x=0) = Restriction x to 0 in β_i
 * - If [β_1(x=0) -> β_1(x=1)] ^ ... ^ [β_n(x=0) -> β_n(x=1)] is valid (i.e. bddtrue) then it's positive unate
 * - If [β_1(x=1) -> β_1(x=0)] ^ ... ^ [β_n(x=1) -> β_n(x=0)] is valid (i.e. bddtrue) then it's negative unate
 * - If [β_1(x=0) | ... | β_n(x=0)] & !([β_1(x=1) | ... | β_n(x=1)]) is SAT (i.e. NOT bddfalse) then it's NOT positive Unate
 * - If [β_1(x=1) | ... | β_n(x=1)] & !([β_1(x=0) | ... | β_n(x=0)]) is SAT (i.e. NOT bddfalse) then it's NOT positive Unate
 */
void HandleUnatesByHeuristic::resolve_unates_in_state(unsigned int state) {
    m_unate_measures.start_testing_state(state);

    vector<string> untested_vars( m_synt_instance.get_output_vars() );
    vector<string> not_positive_unate;
    vector<string> not_negative_unate;
    vector<string> unknown_unate;
    UnateEffectOnState unate_effect_on_state;

    auto test_unknown = [&]() {
        untested_vars.insert(untested_vars.end(), unknown_unate.begin(), unknown_unate.end());
        unknown_unate.clear();
    };

    while(!untested_vars.empty()) {
        string var = untested_vars.back();
        untested_vars.pop_back();

        m_unate_measures.start_testing_var(var);
        int varnum = m_automaton->register_ap(var);

        bdd var_positive = bdd_ithvar(varnum);
        bdd var_negative = bdd_nithvar(varnum);

        bdd positive_unate_sufficient_condition = bddtrue;
        bdd negative_unate_sufficient_condition = bddtrue;
        bdd positive_edges_condition = bddfalse;
        bdd negative_edges_condition = bddfalse;

        for(auto& edge : m_automaton->out(state)) {
            bdd edge_positive = bdd_restrict(edge.cond, var_positive);
            bdd edge_negative = bdd_restrict(edge.cond, var_negative);

            positive_unate_sufficient_condition &= bdd_imp(edge_negative, edge_positive);
            negative_unate_sufficient_condition &= bdd_imp(edge_positive, edge_negative);

            positive_edges_condition |= edge_positive;
            negative_edges_condition |= edge_negative;
        }

        if(positive_unate_sufficient_condition == bddtrue) {
            this->handle_unate_in_state(state, varnum, UnateType::Positive, unate_effect_on_state);
            m_unate_measures.tested_var_unate(UnateType::Positive);
            test_unknown();
        } else if (negative_unate_sufficient_condition == bddtrue) {
            this->handle_unate_in_state(state, varnum, UnateType::Negative, unate_effect_on_state);
            m_unate_measures.tested_var_unate(UnateType::Negative);
            test_unknown();
        } else {
            bdd not_positive_unate_sufficient_condition = negative_edges_condition & !positive_edges_condition;
            bdd not_negative_unate_sufficient_condition = positive_edges_condition & !negative_edges_condition;

            if(not_positive_unate_sufficient_condition != bddfalse) {
                not_positive_unate.push_back(var);
                m_unate_measures.tested_var_not_unate();
            } else if (not_negative_unate_sufficient_condition != bddfalse) {
                not_negative_unate.push_back(var);
                m_unate_measures.tested_var_not_unate();
            } else {
                unknown_unate.push_back(var);
                m_unate_measures.tested_var_unknown();
            }
        }
    }

    m_unate_measures.end_testing_state(
            static_cast<int>(unate_effect_on_state.removed_edges.size()),
            static_cast<int>(unate_effect_on_state.impacted_edges.size())
    );
}