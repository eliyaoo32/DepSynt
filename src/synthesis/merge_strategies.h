#ifndef REACTIVE_SYNTHESIS_BFSS_MERGE_STRATEGIES_H
#define REACTIVE_SYNTHESIS_BFSS_MERGE_STRATEGIES_H

#include <spot/twa/bdddict.hh>
#include <spot/twaalgos/aiger.hh>
#include "BLIF.h"

using namespace std;

// Input: Dependent AIG, Independent AIG, Inputs, Independent Vars, Dependent Vars
// Output: Merged AIG
spot::aig_ptr merge_strategies(spot::aig_ptr independent_strategy,
                      spot::aig_ptr dependent_strategy,
                      const vector<string>& inputs,
                      const vector<string>& independent_vars,
                      const vector<string>& dependent_vars,
                      spot::bdd_dict_ptr dict, string& model_name);

#endif //REACTIVE_SYNTHESIS_BFSS_MERGE_STRATEGIES_H
