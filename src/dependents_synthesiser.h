#ifndef DEPENDENTS_SYNTHESISER_H
#define DEPENDENTS_SYNTHESISER_H

#include <algorithm>
#include <iostream>
#include <spot/tl/parse.hh>
#include <spot/twaalgos/aiger.hh>
#include <string>
#include <utility>
#include <vector>

class DependentsSynthesiser {
   private:
    spot::twa_graph_ptr m_nba_without_deps;
    std::vector<std::string> m_input_vars;
    std::vector<std::string> m_indep_vars;
    std::vector<std::string> m_dep_vars;

    aig_ptr synthesised_aiger;
    std::vector<std::string> aiger_inputs;
    std::vector<std::string> aiger_outputs;

    // <Dest State Id, Array<(Src State Id, BDD ID)>
    unordered_map<unsigned, vector<pair<unsigned, int>>> states_dst_to_srcs;

    // <State, Outs BDDs>
    unordered_map<unsigned, unordered_set<int>> states_to_outs_bdds;

    // <BDD ID, Gate Num>
    unordered_map<int, unsigned> bdds_to_gate;

    void initilize_aiger() {
        uint num_latches = 2 * m_nba_without_deps->num_states();

        // AIGER Inputs = Independets Vars + Input Vars
        std::copy(m_input_vars.begin(), m_input_vars.end(),
                  std::back_inserter(aiger_inputs));
        std::copy(m_indep_vars.begin(), m_indep_vars.end(),
                  std::back_inserter(aiger_inputs));

        // AIGER Outputs = Dependent Vars
        std::copy(m_dep_vars.begin(), m_dep_vars.end(),
                  std::back_inserter(aiger_outputs));

        synthesised_aiger =
            std::make_shared<aig>(aiger_inputs, aiger_outputs, num_latches,
                                  m_nba_without_deps->get_dict());

        // Initlize states_dst_to_srcs and states_to_outs_bdds
        // for (uint state = 0; state < m_nba_without_deps->num_states(); state++) {
        //     states_dst_to_srcs.emplace(state, {});
        // }
    }

    unsigned aig1_latech(unsigned state) { return state; }

    unsigned aig3_latech(unsigned state) {
        return m_nba_without_deps->num_states() + state;
    }

    void preprocess_states() {
        for (uint state = 0; state < m_nba_without_deps->num_states(); state++) {
            for (auto& trans : m_nba_without_deps->out(state)) {
                int bdd_id = trans.cond.id();

                // Create a gate to bdd if not exists
                if (bdds_to_gate.find(bdd_id) == bdds_to_gate.end()) {
                    bdds_to_gate[bdd_id] = synthesised_aiger->bdd2INFvar(trans.cond);
                }

                // Add the transition to the state
                states_to_outs_bdds[trans.src].insert(bdd_id);

                // Add the transition to the states_dst_to_srcs
                states_dst_to_srcs[trans.dst].emplace_back(trans.src, bdd_id);
            }
        }
    }

    void synthesis_aig1() {
        /**
         * Each latech S next value of AIG1 is the following:
         * OR(
         *  AND(src, cond)
         *  ∀(src, cond, dst) if dst = S
         * )
         */
        for (uint state = 0; state < m_nba_without_deps->num_states(); state++) {
            std::vector<unsigned> or_gates;

            for (auto& [src_state, bdd_id] : states_dst_to_srcs[state]) {
                uint src_latches_gate = synthesised_aiger->latch_var(src_state);
                uint bdd_gate = bdds_to_gate[bdd_id];

                or_gates.push_back(
                    synthesised_aiger->aig_and(src_latches_gate, bdd_gate));
            }

            uint next_latch_gate = synthesised_aiger->aig_or(or_gates);
            synthesised_aiger->set_next_latch(aig1_latech(state), next_latch_gate);
        }
    }

   public:
    DependentsSynthesiser(spot::twa_graph_ptr nba_without_deps,
                          std::vector<std::string>& input_vars,
                          std::vector<std::string>& indep_vars,
                          std::vector<std::string>& dep_vars)
        : m_nba_without_deps(nba_without_deps),
          m_input_vars(input_vars),
          m_indep_vars(indep_vars),
          m_dep_vars(dep_vars){};

    spot::aig_ptr synthesis() {
        initilize_aiger();

        preprocess_states();

        synthesis_aig1();
    }
};

#endif