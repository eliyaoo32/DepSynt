//
// Created by Eliyahu Basa on 24/03/2023.
//

#include "merge_strategies_.h"


// Input: Dependent AIG, Independent AIG, Inputs, Independent Vars, Dependent Vars
// Output: Merged AIG
spot::aig_ptr merge_strategies_(spot::aig_ptr independent_strategy,
                                spot::aig_ptr dependent_strategy,
                                const vector<string>& inputs,
                                const vector<string>& independent_vars,
                                const vector<string>& dependent_vars,
                                spot::bdd_dict_ptr dict, string& model_name) {
    if (dependent_vars.empty() || dependent_strategy == nullptr) {
        return independent_strategy;
    }
    if (independent_vars.empty() || independent_strategy == nullptr) {
        // TODO: Settle the init latch to 1 and return a new AIG
    }

    // Create a BLIF file for each strategy
    BLIF deps_blif(model_name + "deps"), indeps_blif(model_name + "indeps");
    deps_blif.load_aig(dependent_strategy);
    indeps_blif.load_aig(independent_strategy);

    // Resolve Dependent BLIF by initializing the latch corresponding to init state to 1
    string deps_init_latch = deps_blif.find_latch_name_by_num(dependent_strategy->latch_var(0));
    deps_blif.init_latch_to_one(deps_init_latch);

    // Merge strategies
    auto merged_blif = BLIF::merge_dependency_strategies(indeps_blif, deps_blif, inputs, independent_vars, dependent_vars, model_name);
    return merged_blif->to_aig(dict);
}
