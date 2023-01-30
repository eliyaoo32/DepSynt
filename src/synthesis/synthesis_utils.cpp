#include "synthesis_utils.h"

using namespace std;
using namespace spot;

void remove_ap_from_automaton(const twa_graph_ptr& automaton,
                              vector<string>& variables) {
    bdd vars = bddtrue;
    for (string& ap_name : variables) {
        vars &= bdd_ithvar(automaton->register_ap(ap_name));
    }

    // Apply exists operator on all edges
    for (int i = 0; i < automaton->num_states(); i++) {
        for (auto& edge : automaton->out(i)) {
            edge.cond = bdd_exist(edge.cond, vars);
        }
    }

    // Unregister dependent variables
    for (string& var_to_remove : variables) {
        int ap = automaton->register_ap(var_to_remove);  // Get AP number
        automaton->unregister_ap(ap);
    }
}

spot::twa_graph_ptr get_dpa_from_nba(spot::twa_graph_ptr nba, synthesis_info& gi,
                                     const vector<string>& output_vars) {
    auto tobdd = [&nba](const std::string& ap_name) {
        return bdd_ithvar(nba->register_ap(ap_name));
    };

    auto is_out = [&output_vars](const std::string& ao) -> bool {
        return std::find(output_vars.begin(), output_vars.end(), ao) !=
               output_vars.end();
    };

    bdd outs = bddtrue;
    for (auto&& aap : nba->ap()) {
        if (is_out(aap.ap_name())) {
            outs &= tobdd(aap.ap_name());
        }
    }
    auto splitted = split_2step(nba, outs, true);

    auto dpa = ntgba2dpa(splitted, gi.force_sbacc);
    // Transform an automaton into a parity game by propagating players.
    alternate_players(dpa);
    // Merge states knows about players
    dpa->merge_states();
    set_synthesis_outputs(dpa, outs);

    return dpa;
}

spot::twa_graph_ptr get_nba_for_synthesis(const spot::formula& formula,
                                          synthesis_info& gi,
                                          SynthesisMeasure& synt_measures,
                                          std::ostream& verbose) {
    option_map& extra_options = gi.opt;
    const bdd_dict_ptr& dict = gi.dict;

    extra_options.set_if_unset("simul", 0);
    extra_options.set_if_unset("tls-impl", 1);
    extra_options.set_if_unset("wdba-minimize", gi.minimize_lvl);

    synt_measures.start_automaton_construct();
    translator trans(dict, &extra_options);
    trans.set_type(spot::postprocessor::Buchi);
    trans.set_pref(spot::postprocessor::SBAcc);

    auto automaton = trans.run(formula);
    synt_measures.end_automaton_construct(automaton);

    verbose << "=> Pruning Automaton" << endl;
    synt_measures.start_prune_automaton();
    auto pruned_automaton = spot::scc_filter_states(automaton);  // Prune automaton
    synt_measures.end_prune_automaton(pruned_automaton);

    return pruned_automaton;
}

// Return if realizable
bool synthesis_nba_to_mealy(spot::synthesis_info& gi, twa_graph_ptr& automaton,
                            const vector<string>& output_vars, std::ostream& verbose,
                            bool should_split_mealy, spot::mealy_like& ml) {
    // =================== Step 1: Build a determanstic-parity-game from the NBA
    auto arena = get_dpa_from_nba(automaton, gi, output_vars);

    // =================== Step 2: Solve the determanstic-parity-game
    bool is_solved = spot::solve_game(arena, gi);

    if (!is_solved) {
        return false;
    }

    // =================== Step 3: Convert the solved game to mealy_machine
    ml.success = spot::mealy_like::realizability_code::REALIZABLE_REGULAR;
    ml.mealy_like = spot::solved_game_to_mealy(arena, gi);
    simplify_mealy_here(ml.mealy_like, gi, should_split_mealy);
    return true;
}

spot::aig_ptr synthesis_nba_to_aiger(spot::synthesis_info& gi,
                                     spot::twa_graph_ptr& automaton,
                                     const vector<string>& outs,
                                     const vector<string>& ins,
                                     std::ostream& verbose) {
    mealy_like mealy;
    bool should_split = true;  // Because it's an AIGER
    bool is_realizable =
        synthesis_nba_to_mealy(gi, automaton, outs, verbose, should_split, mealy);

    if (!is_realizable) {
        return nullptr;
    }

    spot::aig_ptr aiger_strategy =
        mealy_machines_to_aig({mealy}, AIGER_MODE, ins, {outs});

    return aiger_strategy;
}

// This code is taken from: spot/twaalgos/synthesis.cc
twa_graph_ptr ntgba2dpa(const twa_graph_ptr& aut, bool force_sbacc) {
    // if the input automaton is deterministic, degeneralize it to be sure to
    // end up with a parity automaton
    auto dpa = tgba_determinize(degeneralize_tba(aut), false, true, true, false);
    dpa->merge_edges();
    if (force_sbacc) dpa = sbacc(dpa);
    reduce_parity_here(dpa, true);
    assert(([&dpa]() -> bool {
        bool max, odd;
        return dpa->acc().is_parity(max, odd);
    }()));
    assert(is_deterministic(dpa));
    return dpa;
}
