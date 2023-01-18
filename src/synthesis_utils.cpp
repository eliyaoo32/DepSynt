#include "synthesis_utils.h"

using namespace std;
using namespace spot;

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
                                     AutomatonSyntMeasure& synt_measures,
                                     SyntInstance& synt_instance) {
    auto tobdd = [&nba](const std::string& ap_name) {
        return bdd_ithvar(nba->register_ap(ap_name));
    };
    const vector<string>& output_vars = synt_instance.get_output_vars();

    synt_measures.start_split_2step();
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
    synt_measures.end_split_2step();

    synt_measures.start_nba_to_dpa();
    auto dpa = ntgba2dpa(splitted, gi.force_sbacc);
    // Transform an automaton into a parity game by propagating players.
    alternate_players(dpa);
    // Merge states knows about players
    dpa->merge_states();
    set_synthesis_outputs(dpa, outs);
    synt_measures.end_nba_to_dpa();

    return dpa;
}

spot::twa_graph_ptr get_nba_for_synthesis(SyntInstance& synt_instance,
                                          synthesis_info& gi,
                                          AutomatonSyntMeasure& synt_measures,
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

    auto automaton = trans.run(synt_instance.get_formula_parsed());
    synt_measures.end_automaton_construct(automaton);

    verbose << "=> Pruning Automaton" << endl;
    synt_measures.start_prune_automaton();
    auto pruned_automaton = spot::scc_filter_states(automaton);  // Prune automaton
    synt_measures.end_prune_automaton(pruned_automaton);

    return pruned_automaton;
}

void find_and_remove_dependents(const twa_graph_ptr& automaton,
                                SyntInstance& synt_instance,
                                AutomatonSyntMeasure& synt_measures,
                                vector<string>& dependent_variables_dst,
                                vector<string>& independent_variables_dst,
                                std::ostream& verbose) {
    verbose << "=> Finding Dependent Variables" << endl;

    FindDepsByAutomaton automaton_dependencies(synt_instance, synt_measures,
                                               automaton, false);
    automaton_dependencies.find_dependencies(dependent_variables_dst,
                                             independent_variables_dst);
    verbose << "=> Found " << dependent_variables_dst.size()
            << " dependent variables" << endl;

    verbose << "=> Remove Dependent Variables" << endl;
    synt_measures.start_remove_dependent_ap();
    remove_ap_from_automaton(automaton, dependent_variables_dst);
    synt_measures.end_remove_dependent_ap();
}

// Return if realizable
bool synthesis_nba_to_mealy(spot::synthesis_info& gi,
                            AutomatonSyntMeasure& synt_measures,
                            twa_graph_ptr& automaton, SyntInstance& synt_instance,
                            std::ostream& verbose, bool should_split_mealy,
                            spot::mealy_like& ml) {
    // =================== Step 1: Build a determanstic-parity-game from the NBA
    auto arena = get_dpa_from_nba(automaton, gi, synt_measures, synt_instance);

    // =================== Step 2: Solve the determanstic-parity-game
    synt_measures.start_solve_game();
    bool is_solved = spot::solve_game(arena, gi);
    synt_measures.end_solve_game();

    if (!is_solved) {
        synt_measures.completed();
        return false;
    }

    // =================== Step 3: Convert the solved game to mealy_machine
    synt_measures.start_dpa_to_mealy();
    ml.success = spot::mealy_like::realizability_code::REALIZABLE_REGULAR;
    ml.mealy_like = spot::solved_game_to_mealy(arena, gi);
    simplify_mealy_here(ml.mealy_like, gi, should_split_mealy);
    synt_measures.end_dpa_to_mealy();

    synt_measures.completed();
    return true;
}
