#include "find_deps_by_automaton.h"

#include <boost/range/join.hpp>
#include <cmath>
#include <spot/twaalgos/sccfilter.hh>
#include <string>

#include "utils.h"

using namespace std;

void FindDepsByAutomaton::find_dependencies(vector<string>& dependent_variables,
                                            vector<string>& independent_variables) {
    m_measures.start_find_deps();

    // Find Dependencies
    std::vector<std::string> candidates;
    this->find_dependencies_candidates(candidates);

    // Find PairStates
    m_measures.start_search_pair_states();
    vector<PairState> compatibleStates;
    get_all_compatible_states(compatibleStates, m_automaton);
    m_measures.end_search_pair_states(static_cast<int>(compatibleStates.size()));

    if(m_stop_flag.load()) {
        // Add to independent variables all the candidates
        independent_variables.insert(independent_variables.end(), candidates.begin(),
                                     candidates.end());
        m_measures.end_find_deps(false);
        m_is_done.store(true);
        return;
    }

    while (!candidates.empty() && !m_stop_flag.load()) {
        std::string dependent_var = candidates.back();
        candidates.pop_back();
        m_measures.start_testing_variable(dependent_var);

        vector<string> dependency_set;
        this->extract_dependency_set(dependency_set, candidates,
                                     independent_variables);

        // Check if candidates variable is dependent
        if (FindDepsByAutomaton::is_variable_dependent(dependent_var, dependency_set,
                                                       compatibleStates)) {
            dependent_variables.push_back(dependent_var);
            m_measures.end_testing_variable(true, dependency_set);
        } else {
            independent_variables.push_back(dependent_var);
            m_measures.end_testing_variable(false, dependency_set);
        }
    }

    // Add to independent variables all the candidates that left
    independent_variables.insert(independent_variables.end(), candidates.begin(),
                                 candidates.end());

    m_measures.end_find_deps(!m_stop_flag.load());
    m_is_done.store(true);
}

void FindDepsByAutomaton::find_dependencies_candidates(
    std::vector<std::string>& candidates_dst) {
    candidates_dst.clear();

    const vector<string>& candidates =
        m_dependent_variable_type == DependentVariableType::Output
            ? m_synt_instance.get_output_vars()
            : m_synt_instance.get_input_vars();

    std::copy(candidates.begin(), candidates.end(),
              std::back_inserter(candidates_dst));
}

void FindDepsByAutomaton::extract_dependency_set(
    std::vector<std::string>& dependency_set_dst,
    std::vector<std::string>& current_candidates,
    std::vector<std::string>& current_independents) {
    dependency_set_dst.clear();

    if (m_dependent_variable_type == DependentVariableType::Output) {
        // Dependency Set = Input Vars + Candidates Vars + Independent Vars
        auto target_deps_set =
            boost::join(m_synt_instance.get_input_vars(),
                        boost::join(current_candidates, current_independents));

        copy(target_deps_set.begin(), target_deps_set.end(),
             back_inserter(dependency_set_dst));
    } else {
        // Dependency Set = Candidates Vars + Independent Vars
        auto target_deps_set = boost::join(current_candidates, current_independents);
        copy(target_deps_set.begin(), target_deps_set.end(),
             back_inserter(dependency_set_dst));
    }
}

bool FindDepsByAutomaton::is_variable_dependent(std::string dependent_var,
                                                vector<std::string>& dependency_vars,
                                                vector<PairState>& pairStates) {
    // Extract variables indexes
    vector<VarIndexer> reset_vars_nums;
    vector<int> dependency_vars_nums;
    int dependent_var_num = m_bdd_cacher->get_variable_index(dependent_var);

    for (auto& var : dependency_vars) {
        dependency_vars_nums.push_back(m_bdd_cacher->get_variable_index(var));
    }
    vector<string> reset_vars;
    vector<string> excluded_vars(dependency_vars);
    excluded_vars.emplace_back(dependent_var);
    m_synt_instance.all_vars_excluded(reset_vars, excluded_vars);
    for (auto& var : reset_vars) {
        reset_vars_nums.push_back({m_bdd_cacher->get_variable_index(var),
                                   m_bdd_cacher->get_prime_variable_index(var)});
    }

    // For each pair-state, Can we move to an accepting state with different
    // value of dependent_var? If yes, then dependent_var is not dependent
    for (auto pairState : pairStates) {
        for (auto& t1 : m_automaton->out(pairState.first)) {
            for (auto& t2 : m_automaton->out(pairState.second)) {
                PairEdges pair_edges = PairEdges(t1, t2);

                if (!FindDepsByAutomaton::is_dependent_by_pair_edges(
                        dependent_var_num, dependency_vars_nums, reset_vars_nums,
                        pair_edges)) {
                    return false;
                }
            }
        }
    }

    return true;
}

/**
 * A Variable X is dependent on the set Y if for all pair-states (s1, s2), not exists
 * an assignment 𝜋 of Y such that both s1(𝜋, X=True, ...), s2(𝜋, X=False, ...) are
 * satified (And vice-versa).
 *
 * Mathematcially, [∃Y : s1(Y, X=True, Z) & s2(Y, X=False, Z')] is not satisfiable.
 * and [∃Y : s1(Y, X=False, Z) & s2(Y, X=True, Z')] is not satisfiable as well.
 * Where Z are rest of variables.
 */
bool FindDepsByAutomaton::is_dependent_by_pair_edges(int dependent_var,
                                                     vector<int>& dependency_vars,
                                                     vector<VarIndexer>& reset_vars,
                                                     const PairEdges& edges) {
    bdd z1(edges.first.cond);
    bdd z2(edges.second.cond);

    // Replace in z2 the reset_vars with their prime vars
    bddPair* pairs = bdd_newpair();
    for (auto& var : reset_vars) {
        bdd_setpair(pairs, var.var_index, var.prime_var_index);
    }
    z2 = bdd_replace(z2, pairs);

    // Case 1: s1(Y, X=True, Z) & s2(Y, X=False, Z') is unsat
    bdd cond1 = bdd_restrict(z1, bdd_ithvar(dependent_var)) &
                bdd_restrict(z2, bdd_nithvar(dependent_var));
    bool cond1_sat = cond1 != bddfalse;
    if (cond1_sat) {
        bdd_freepair(pairs);
        return false;
    }

    // Case 2: s1(Y, X=False, Z) & s2(Y, X=True, Z') is unsat
    bdd cond2 = bdd_restrict(z1, bdd_nithvar(dependent_var)) &
                bdd_restrict(z2, bdd_ithvar(dependent_var));
    bool cond2_sat = cond2 != bddfalse;
    if (cond2_sat) {
        bdd_freepair(pairs);
        return false;
    }

    // No condition is satified
    bdd_freepair(pairs);
    return true;
}

// Return a list of pair-states, where each pair-state are states which can be
// arrived in the automaton by the same prefix.
bool FindDepsByAutomaton::get_all_compatible_states(std::vector<PairState>& pairStates,
                               const spot::twa_graph_ptr& aut) {
    unsigned init_state = aut->get_init_state_number();
    std::vector<PairState> queue = {PairState(init_state, init_state)}; // Everything in the queue is compatible
    std::unordered_set<std::pair<int, int>, pair_hash> tested_pairs;

    while (!queue.empty() && !m_stop_flag.load()) {
        PairState pairState = queue.back();
        queue.pop_back();

        // Make sure the smaller state number comes first
        if (pairState.first > pairState.second) {
            std::swap(pairState.first, pairState.second);
        }

        // Check if the pair was already tested
        if (tested_pairs.find(pairState) != tested_pairs.end()) {
            continue;
        }

        tested_pairs.insert(pairState);
        pairStates.emplace_back(pairState.first, pairState.second);

        for (auto& t1 : aut->out(pairState.first)) {
            for (auto& t2 : aut->out(pairState.second)) {
                if(m_stop_flag.load()) {
                    return false;
                }

                PairState newState(t1.dst, t2.dst);

                // Make sure the smaller state number comes first
                if (newState.first > newState.second) {
                    std::swap(newState.first, newState.second);
                }
                // Check if the pair was already tested before adding it to the queue 6
                if (tested_pairs.find(newState) != tested_pairs.end()) {
                    continue;
                }

                if(t1.dst == t2.dst) {
                    queue.emplace_back(newState);
                } else if(are_edges_shares_assignment(t1, t2)) {
                    queue.emplace_back(newState);
                }
            }
        }
    }

    return queue.empty() && !m_stop_flag.load();
}

// TODO: this is buggy to verify if variables are pair states, get back to it later (12/05/2023)
// Are the variables used in the edges are the same?
bool are_edges_shares_variable(spot::twa_graph::edge_storage_t& e1,
                               spot::twa_graph::edge_storage_t& e2) {
    auto x = bdd_support(e1.cond);
    auto y = bdd_support(e2.cond);

    return x == y;
}

// Are the variables used in the edges are the same?
bool are_edges_shares_assignment(spot::twa_graph::edge_storage_t& e1,
                               spot::twa_graph::edge_storage_t& e2) {
    return (e1.cond & e2.cond) != bddfalse;
}
