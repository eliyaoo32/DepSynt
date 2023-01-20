#ifndef DEPENDENTS_SPOT_SYNTHESISER_H
#define DEPENDENTS_SPOT_SYNTHESISER_H

#include <spot/twa/twagraph.hh>
#include <spot/twaalgos/aiger.hh>
#include <string>
#include <vector>

#include "synthesis_utils.h"

using namespace std;

class DependentsSpotSynthesiser {
   private:
    vector<string>& m_input_vars;
    vector<string>& m_indeps_vars;
    vector<string>& m_deps_vars;
    spot::twa_graph_ptr m_nba_with_deps;

   public:
    DependentsSpotSynthesiser(vector<string>& input_vars,
                              vector<string>& indeps_vars, vector<string>& dep_vars,
                              spot::twa_graph_ptr nba_with_deps)
        : m_input_vars(input_vars),
          m_indeps_vars(indeps_vars),
          m_deps_vars(dep_vars),
          m_nba_with_deps(nba_with_deps){};

    spot::aig_ptr synthesis(synthesis_info& gi) {
        vector<string> strategy_ins(m_indeps_vars);
        std::copy(m_input_vars.begin(), m_input_vars.end(),
                  std::back_inserter(strategy_ins));

        vector<string>& strategy_outs = m_deps_vars;

        spot::mealy_like ml;
        auto arena = get_dpa_from_nba(m_nba_with_deps, gi, m_deps_vars);
        bool is_solved = spot::solve_game(arena, gi);
        if (!is_solved) {
            return nullptr;  // UNREALIZABLE
        }

        ml.success = spot::mealy_like::realizability_code::REALIZABLE_REGULAR;
        ml.mealy_like = spot::solved_game_to_mealy(arena, gi);
        simplify_mealy_here(ml.mealy_like, gi, true);

        return spot::mealy_machines_to_aig({ml}, AIGER_MODE, strategy_ins,
                                           {strategy_outs});
    }
};

#endif