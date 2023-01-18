#ifndef REACTIVE_SYNTHESIS_BFSS_SYNTHESIS_UTILS_H
#define REACTIVE_SYNTHESIS_BFSS_SYNTHESIS_UTILS_H

#include <iostream>
#include <spot/misc/optionmap.hh>
#include <spot/twaalgos/degen.hh>
#include <spot/twaalgos/determinize.hh>
#include <spot/twaalgos/game.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twaalgos/isdet.hh>
#include <spot/twaalgos/mealy_machine.hh>
#include <spot/twaalgos/sbacc.hh>
#include <spot/twaalgos/sccfilter.hh>
#include <spot/twaalgos/synthesis.hh>
#include <spot/twaalgos/translate.hh>
#include <vector>

#include "find_deps_by_automaton.h"
#include "synt_instance.h"
#include "utils.h"

spot::twa_graph_ptr ntgba2dpa(const spot::twa_graph_ptr& aut, bool force_sbacc);

void remove_ap_from_automaton(const spot::twa_graph_ptr& automaton,
                              vector<string>& variables);

spot::twa_graph_ptr get_dpa_from_nba(spot::twa_graph_ptr nba,
                                     spot::synthesis_info& gi,
                                     AutomatonSyntMeasure& synt_measures,
                                     SyntInstance& synt_instance);

spot::twa_graph_ptr get_nba_for_synthesis(SyntInstance& synt_instance,
                                          spot::synthesis_info& gi,
                                          AutomatonSyntMeasure& synt_measures,
                                          std::ostream& verbose);

void find_and_remove_dependents(const spot::twa_graph_ptr& automaton,
                                SyntInstance& synt_instance,
                                AutomatonSyntMeasure& synt_measures,
                                vector<string>& dependent_variables_dst,
                                vector<string>& independent_variables_dst,
                                std::ostream& verbose);

bool synthesis_nba_to_mealy(spot::synthesis_info& gi,
                            AutomatonSyntMeasure& synt_measures,
                            spot::twa_graph_ptr& automaton,
                            SyntInstance& synt_instance, std::ostream& verbose,
                            bool should_split_mealy, spot::mealy_like& ml);

#endif