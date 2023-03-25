#include "merge_strategies.h"

void load_dependent_strategy(BLIF& deps_blif, spot::aig_ptr& dependent_strategy) {
    deps_blif.load_aig(dependent_strategy);
    string deps_init_latch = deps_blif.find_latch_name_by_num(dependent_strategy->latch_var(0));
    deps_blif.init_latch_to_one(deps_init_latch);
}

spot::aig_ptr merge_strategies(spot::aig_ptr independent_strategy,
                                spot::aig_ptr dependent_strategy,
                                const vector<string>& inputs,
                                const vector<string>& independent_vars,
                                const vector<string>& dependent_vars,
                                spot::bdd_dict_ptr dict, string& model_name) {
    if (dependent_vars.empty() || dependent_strategy == nullptr) {
        return independent_strategy;
    }
    if (independent_vars.empty() || independent_strategy == nullptr) {
        // Only fix the latch corresponding to the initial state
        BLIF deps_blif(model_name + "deps");
        load_dependent_strategy(deps_blif, dependent_strategy);
        return deps_blif.to_aig(dict);
    }

    // Create a BLIF file for each strategy
    BLIF deps_blif(model_name + "deps"), indeps_blif(model_name + "indeps");
    indeps_blif.load_aig(independent_strategy);
    load_dependent_strategy(deps_blif, dependent_strategy);

    // Merge strategies
    auto merged_blif = BLIF::merge_dependency_strategies(indeps_blif, deps_blif, inputs, independent_vars, dependent_vars, model_name);
    return merged_blif->to_aig(dict);
}


