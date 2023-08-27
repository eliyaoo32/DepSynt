#ifndef BDD_VARS_CACHER_H
#define BDD_VARS_CACHER_H

#include <map>
#include <spot/tl/parse.hh>
#include <spot/twaalgos/translate.hh>
#include <string>
#include <vector>

class BDDVarsCacher {
   private:
    int m_total_vars;
    int m_prime_var_start;  // At what index of variable, the prime var starts
    int m_prime_var_coefficient;
    std::unordered_map<std::string, int> m_variable_index;
    std::unordered_map<std::string, int> m_prime_variable_index;
    spot::twa_graph_ptr m_automaton;

   public:
    BDDVarsCacher(spot::twa_graph_ptr automaton);

    int get_variable_index(std::string& variable_name);

    int get_prime_variable_index(std::string& variable_name);
};

struct NBABDDSummary {
    int total_bdds_size_repeated = 0;       // The same BDD is counted as many times as it is used
    int total_bdds_size_not_repeated = 0;   // If the same BDD is used more than once, it is counted only once
    int total_bdds = 0;         // Total number of unique BDDs used
    int min_bdd_size = 0;       // The smallest BDD size used
    int max_bdd_size = 0;       // The smallest BDD size used
    int avg_bdd_size = 0;       // The smallest BDD size used
};

void extract_nba_bdd_summary(NBABDDSummary& summary, spot::twa_graph_ptr& nba);

/* Due to bug in BuDDY where restricting variable is not working if this is the only
 * variable left, this workaround is used. */
bool can_restrict_variable(bdd& bd, int variable, bool restriction_value);

#endif