#include "dependents_synthesiser.h"

#include <algorithm>
#include <iostream>
#include <spot/tl/parse.hh>
#include <spot/tl/print.hh>
#include <spot/twaalgos/aiger.hh>
#include <spot/twaalgos/translate.hh>
#include <string>
#include <vector>

using namespace std;
using namespace spot;

void DependentsSynthesiser::init_aiger() {
    /**
     * Create AIGER
     * Input = Input Vars + Indep Vars
     * Output = Dep Vars
     * Latches = #States
     */
    std::vector<std::string> aiger_inputs;
    std::copy(m_input_vars.begin(), m_input_vars.end(),
              std::back_inserter(aiger_inputs));
    std::copy(m_indep_vars.begin(), m_indep_vars.end(),
              std::back_inserter(aiger_inputs));

    unsigned num_latches = m_nba_with_deps->num_states();
    m_aiger = std::make_shared<aig>(aiger_inputs, m_dep_vars, num_latches,
                                    m_nba_with_deps->get_dict());

    /**
     * Preprocessed data
     */
    for (auto& var : m_dep_vars) {
        deps_bdd_vars.insert(this->ap_to_bdd_varnum(var));
    }
}

void DependentsSynthesiser::define_next_latches() {
    /**
     * Define next latches values
     *
     * unordered_map<Dst, vector<(Src, Gate)> dst_transitions means that the
     * transition (src, gate, dst) exists in the projected NBA
     */
    unordered_map<State, std::vector<StateGatePair>> dst_transitions;
    for (State state = 0; state < m_nba_without_deps->num_states(); state++) {
        for (auto& transition : m_nba_without_deps->out(state)) {
            State src = transition.src;
            State dst = transition.dst;
            bdd& cond = transition.cond;

            dst_transitions[dst].emplace_back(src, m_aiger->bdd2INFvar(cond));
        }
    }

    for (auto& trans_to_dst : dst_transitions) {
        State dst = trans_to_dst.first;
        auto& trans = trans_to_dst.second;

        vector<Gate> next_latch_conds;
        for (auto& src_and_cond : trans) {
            // i.e. the transition (src, cond, dst)
            State src_gate = m_aiger->latch_var(src_and_cond.first);
            Gate cond_gate = src_and_cond.second;

            next_latch_conds.emplace_back(m_aiger->aig_and(src_gate, cond_gate));
        }

        assert(next_latch_conds.size() > 0);
        Gate next_latch_gate;
        if (next_latch_conds.size() == 1) {
            next_latch_gate = next_latch_conds[0];
        } else {
            next_latch_gate = m_aiger->aig_or(next_latch_conds);
        }
        /**
         * TODO: check maybe it should be dst_gate instead of
         * m_aiger->latch_var(dst);
         */
        m_aiger->set_next_latch(dst, next_latch_gate);
    }
}

void DependentsSynthesiser::define_output_gates() {
    /**
     * Create for each state which corresponds if any transiiton of the state is
     * activate based on Input and Indep Vars
     */
    unordered_map<State, Gate> active_state_gate;
    for (State state = 0; state < m_nba_without_deps->num_states(); state++) {
        const auto& state_outs = m_nba_without_deps->out(state);
        vector<Gate> state_trans_gates;

        std::transform(state_outs.begin(), state_outs.end(),
                       std::back_inserter(state_trans_gates),
                       [this](auto& transition) {
                           return m_aiger->bdd2INFvar(transition.cond);
                       });

        active_state_gate[state] = m_aiger->aig_and(
            m_aiger->latch_var(state), m_aiger->aig_or(state_trans_gates));
    }

    /**
     * Define output for the dependent variables
     */
    for (unsigned dep_idx = 0; dep_idx < m_dep_vars.size(); dep_idx++) {
        /**
         * TODO: we can small optimization here by removing the
         * partial_impl_cache of current dependent varaible after finished
         */
        /** TODO: For the same BDD the process PartialImpl(BDD,d1) and
         * PartialImpl(BDD, d2) shares the same construction except assignment of
         * neg_n_v  */
        /**
         * TODO: optimization - BDD1 may be included in BDD2, so in the process
         * BDD2 we can conver BDD1. */

        string& dep_var = m_dep_vars[dep_idx];
        vector<Gate> dep_partial_impls;

        // For all transitions (src, cond, dst)
        for (State state = 0; state < m_nba_with_deps->num_states(); state++) {
            for (auto& transition : m_nba_with_deps->out(state)) {
                State src = transition.src;
                bdd& cond = transition.cond;

                Gate partial_impl = get_partial_impl(cond, dep_var);

                dep_partial_impls.emplace_back(
                    m_aiger->aig_and(active_state_gate[src], partial_impl));
            }
        }

        m_aiger->set_output(dep_idx, m_aiger->aig_or(dep_partial_impls));
    }
}

Gate DependentsSynthesiser::get_partial_impl(const bdd& cond, string& dep_var) {
    string partial_impl_key = std::to_string(cond.id()) + "#" + dep_var;

    // If exists in cache
    if (partial_impl_cache.find(partial_impl_key) != partial_impl_cache.end()) {
        return partial_impl_cache[partial_impl_key];
    }

    // Create partial implementation
    unordered_map<int, Gate> bdds_partial_impl;
    Gate partial_impl = calc_partial_impl(cond, dep_var, bdds_partial_impl);

    // Store to cache and return it
    partial_impl_cache[partial_impl_key] = partial_impl;
    return partial_impl_cache[partial_impl_key];
}

Gate DependentsSynthesiser::calc_partial_impl(
    const bdd& cond, string& dep_var, unordered_map<int, Gate>& bdd_partial_impl) {
    if (cond == bddtrue) {
        return m_aiger->aig_true();
    }
    if (cond == bddfalse) {
        return m_aiger->aig_false();
    }
    if (bdd_partial_impl.find(cond.id()) != bdd_partial_impl.end()) {
        return bdd_partial_impl[cond.id()];
    }

    // Post-order traversal
    Gate high_gate = calc_partial_impl(bdd_high(cond), dep_var, bdd_partial_impl);
    Gate low_gate = calc_partial_impl(bdd_low(cond), dep_var, bdd_partial_impl);

    bool is_dep_var = deps_bdd_vars.find(bdd_var(cond)) != deps_bdd_vars.end();
    Gate n_v, neg_n_v;

    if (!is_dep_var) {
        n_v = m_aiger->bdd2aigvar(bdd_ithvar(bdd_var(cond)));
        neg_n_v = m_aiger->aig_not(n_v);
    } else {
        bool is_bdd_var_cur_dep = bdd_var(cond) == ap_to_bdd_varnum(dep_var);
        neg_n_v = is_bdd_var_cur_dep ? m_aiger->aig_false() : m_aiger->aig_true();
        n_v = m_aiger->aig_true();
    }

    bdd_partial_impl[cond.id()] = m_aiger->aig_or(
        m_aiger->aig_and(neg_n_v, low_gate), m_aiger->aig_and(n_v, high_gate));
    return bdd_partial_impl[cond.id()];
}
