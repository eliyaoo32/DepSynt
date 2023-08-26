#include "bdd_utils.h"

#include <spot/tl/parse.hh>
#include <spot/twaalgos/translate.hh>

BDDVarsCacher::BDDVarsCacher(spot::twa_graph_ptr automaton)
    : m_automaton(automaton), m_prime_var_coefficient(0) {
    m_total_vars = automaton->ap().size();

    // Register prime variables
    m_prime_var_start =
        automaton->get_dict()->register_anonymous_variables(m_total_vars, automaton);
}

int BDDVarsCacher::get_variable_index(std::string& variable_name) {
    if (m_variable_index.find(variable_name) == m_variable_index.end()) {
        m_variable_index[variable_name] = m_automaton->register_ap(variable_name);
    }

    return m_variable_index[variable_name];
}

int BDDVarsCacher::get_prime_variable_index(std::string& variable_name) {
    if (m_prime_variable_index.find(variable_name) == m_prime_variable_index.end()) {
        m_prime_variable_index[variable_name] =
            m_prime_var_start + m_prime_var_coefficient;

        m_prime_var_coefficient++;
    }

    return m_prime_variable_index[variable_name];
}


bool can_restrict_variable(bdd& bd, int variable, bool restriction_value) {
    bdd var_bdd = restriction_value ? bdd_ithvar(variable) : bdd_nithvar(variable);

    return bdd_and(bd, var_bdd) != bddfalse;
}

void extract_nba_bdd_summary(NBABDDSummary& summary, spot::twa_graph_ptr& nba) {
    std::unordered_map<int, int> bdd_to_size;
    std::unordered_map<int, int> bdd_to_count;

    for (int i = 0; i < nba->num_states(); i++) {
        for (auto& edge : nba->out(i)) {
            int bdd_id = edge.cond.id();
            if (bdd_to_size.find(bdd_id) == bdd_to_size.end()) {
                bdd_to_size[bdd_id] = bdd_nodecount(bdd_id);
                bdd_to_count[bdd_id] = 1;
            } else {
                bdd_to_count[bdd_id]++;
            }
        }
    }

    summary.total_bdds = static_cast<int>(bdd_to_size.size());
    summary.total_bdds_size_repeated = 0;
    summary.total_bdds_size_not_repeated = 0;
    summary.min_bdd_size = std::numeric_limits<int>::max();
    summary.max_bdd_size = std::numeric_limits<int>::min();
    summary.avg_bdd_size = 0;

    for (auto& bdd_size : bdd_to_size) {
        int bdd_id = bdd_size.first;
        int bdd_count = bdd_to_count[bdd_id];
        summary.total_bdds_size_repeated += bdd_size.second * bdd_count;
        summary.total_bdds_size_not_repeated += bdd_size.second;
        summary.min_bdd_size = std::min(summary.min_bdd_size, bdd_size.second);
        summary.max_bdd_size = std::max(summary.max_bdd_size, bdd_size.second);
    }

    summary.avg_bdd_size =
        summary.total_bdds_size_not_repeated / summary.total_bdds;
}
