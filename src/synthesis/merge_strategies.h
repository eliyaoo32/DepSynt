#ifndef MERGE_AIGERS_H
#define MERGE_AIGERS_H

#include <boost/algorithm/string/join.hpp>
#include <fstream>
#include <spot/twa/bdddict.hh>
#include <spot/twaalgos/aiger.hh>
#include <string>
#include <vector>

#include "utils.h"

using namespace std;

inline string blif_wired_var(string var) { return "In" + var; }

spot::aig_ptr blif_file_to_aiger(string& blif_path, spot::bdd_dict_ptr dict,
                                 string& model_name);

void aiger_to_blif(spot::aig_ptr aiger, string& blif_dst, string blif_name);

void merge_strategies_blifs(ostream& out, string& indeps_blif, string& deps_blif,
                            string& indeps_model_name, string& deps_model_name,
                            const vector<string>& inputs,
                            const vector<string>& independent_vars,
                            const vector<string>& dependent_vars,
                            string& model_name);

spot::aig_ptr merge_strategies(spot::aig_ptr independent_strategy,
                               spot::aig_ptr dependent_strategy,
                               const vector<string>& inputs,
                               const vector<string>& independent_vars,
                               const vector<string>& dependent_vars,
                               spot::bdd_dict_ptr dict, string& model_name);

#endif
